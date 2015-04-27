//*****************************************************************************
//
// Read proximity data from PING sensor
//
//		Author:			Dustin Horvath
//		Organization:	KU/EECS/EECS 388
//		Date:			2014�04�08
//
//		Purpose: Read in values from the PING Ultrasonic Proximity sensor
//
//		Notes: This program makes use of code provided by Dr. Gary Minden 
//				in the Stellaris driver library 
//
//*****************************************************************************
//


#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "Drivers/rit128x96x4.h"
#include "drivers/uartstdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"



//*****************************************************************************
//
// Task initialization
void ProxySensor( void *pvParameters ) {

	// Enable UART0, to be used as a serial console.
	//
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	//
	// Initialize the UART standard I/O.
	//
	UARTStdioInit( 0 );
	UARTprintf( "Task_Button on LM3S1968 starting\n" );


	//*****************************************************************************
	//
	// Constants and Variables
	#define NbrEdgeTimeSamples 100
	#define MaxNbrEdgeTimeSamples 10
	long int NegEdgeTimeSamples[NbrEdgeTimeSamples];
	long int NegEdgeTimeSampleIdx;

	// Enable GPIO Port D
	SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOD );
	//
	//Configure PortD<0> as input with weak-pull-up.
	//PortD<0> will be used to read the sensor data.
	//
	GPIOPinTypeGPIOInput( GPIO_PORTD_BASE, GPIO_PIN_0 );
	GPIOPadConfigSet( GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU );
	
	//
	// Configure PortD<1> as open drain output with 2 mA drive.
	// PortD<1> will be used to initiate a measurement.
	// Initialize PortD<1> to a 0 (constant 0x00).
	// For the PING sensor, input 0 is idle, while signal input 1 is used to signal the start of measuring.
	//
	GPIOPinTypeGPIOOutput( GPIO_PORTD_BASE, GPIO_PIN_1 );
	GPIOPadConfigSet( GPIO_PORTD_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_OD );
	GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 );			// Write 0 to pin, pulling the signal low.
	
	//
	// Configure PortD<2> as standard output to supply power to the sensor.
	// Initialize PortD<2> to 1 (constant 0x04)
	//
	GPIOPinTypeGPIOOutput( GPIO_PORTD_BASE, GPIO_PIN_2 );
	GPIOPadConfigSet( GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD );
	GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_2, 0x04 );

	long int Delay_1mS = ( configTICK_RATE_HZ ) / 1000;
	long int Delay_5mS = ( 5 * configTICK_RATE_HZ ) / 1000;
	long int Delay_10mS = ( 10 * configTICK_RATE_HZ ) / 1000;
	
	//
	//Configure Timer_0_A to count down continuously before resetting.
	//Timer_0_A will be enabled after a start pulse is generated and
	// disabled at the end of collecting data.
	//Adapted from TI Stellaris Timer Example
	//
	SysCtlPeripheralEnable( SYSCTL_PERIPH_TIMER0 );
	TimerConfigure( TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC );
	TimerLoadSet( TIMER0_BASE, TIMER_A, 50000 );
	TimerPrescaleSet( TIMER0_BASE, TIMER_A, 9 );
	
	long int signal_send_termination;
	long int signal_receive_start;
	long int signal_receive_end;
	
	while ( 1 ) {
	
	// Test blocks adapted from Dr. Minden's. These can be used to determine whether initialization has been completed
	//  successfully, and to check if the hardware is functioning by design. One of the problems he ran into involved
	//  the timer not having up-shot functionality, which has been changed here to periodic. Small changes have been made.
	
	/*
		// PORT D TEST BLOCK
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 );		// Set signal to 0
		vTaskDelay(  1 * Delay_1mS );
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x02 );		// Set signal to 1
		vTaskDelay(  10 * Delay_1mS );
	*/

		// PORT D CONFIRMED WORKING USING OSCILLOSCOPE

	/*
		// TIMER TEST BLOCK
		TimerLoadSet( TIMER0_BASE, TIMER_A, 50000 );				// Load initial Timer value. This has been changed to start high and count down.
		TimerEnable( TIMER0_BASE, TIMER_A );						// Enable (Start) Timer
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x02 );			// Send a HIGH signal for 5ms. PING sensor requires high input signal to start.
		TimerWrite1 = TimerValueGet( TIMER0_BASE, TIMER_A )			// Capture current time
		PortD_0_A = GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_0 );
		SysCtlDelay( 16667 * 5 );									// SysCtlDelay holds control of cpu during wait, rather than releasing to OS.
																	// 5ms is used as the signal time for the PING sensor.
		TimerEndStart = TimerValueGet( TIMER0_BASE, TIMER_A );		// Capture time at start signal end
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 );			// Set input signal back to LOW.
		SysCtlDelay( 2 );											// Delay 6 cycles
		PortD_0_B = GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_0 );
		//UARTprintf( "PortD_0_A,_B: %d, %d\n", PortD_0_A, PortD_0_B );
	*/

		
	// This code block has been moderately adapted from Dr. Minden's temperature sensor code
	/*
		//
		// Now the code to capture the data. We'll capture the high-to-low
		// transition time of the data signal. A short interval indicates
		// a zero (0) and a long interval indicates a one (1).
		//
		// We wait for a LOW-TO-HIGH transition; record the time;
		// wait for a HIGH-TO-LOW transition. We read the data
		// on PortD<0>
		//
		// We will capture MaxNbrEdgeTimeSamples.
		// Capture time at start of reading data
		
		// The input AND OUTPUT signals for the PING sensor are "idle" at 0 and signalling at 1. The length of
		//  the 1 pulse returned on pin 0 is indicative of the range sensed by the proximity sensor.
		TimerWait0 = TimerValueGet( TIMER0_BASE, TIMER_A );
		for ( NegEdgeTimeSampleIdx = 0; NegEdgeTimeSampleIdx < MaxNbrEdgeTimeSamples; NegEdgeTimeSampleIdx++ ) {
			//
			// Wait for LOW-TO-HIGH transition
			//
			// Capture time at start of reading data
			TimerWait1 = TimerValueGet( TIMER0_BASE, TIMER_A );
			PortD_0_C = GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_0 );
			while ( GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_0 ) == 0 ) {
				// Capture time at last read of data
				TimeWait2 = TimerValueGet( TIMER0_BASE, TIMER_A );
			}
			//
			// Record transition time.
			//
			NegEdgeTimeSamples[NegEdgeTimeSampleIdx] = TimerValueGet( TIMER0_BASE, TIMER_A );
			//
			// Wait for HIGH-TO-LOW transition.
			// This marks the end of the signal transmission
			while ( GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_0 ) == 1 ) {
			}
		}
		
		
		// SEND all of the recorded measurements over UART to the host. This only runs once after they've all finished.
		if(NegEdgeTimeSampleIdx >= MaxNbrEdgeTimeSamples){
			for( int i = 0; i < MaxNbrEdgeTimeSamples; i++){
				UARTprintf( "Recorded times: %d, %d\n", i, NegEdgeTimeSamples[i] );
			}
		}
	*/
	

		TimerEnable( TIMER0_BASE, TIMER_A );								// Starts the timer counting down.
		SysCtlDelay( 5 );													// It takes 5 cycles for the timer to start after enabling
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x02 );					// Begins 1 signal output.
		SysCtlDelay( 50000 * 5  );											// Waits 5ms, the length of typical PING sensor signal.
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 );					// After wait, pulls signal back down to zero.
		signal_send_termination = TimerValueGet( TIMER0_BASE, TIMER_A );	// Records time that the transmit signal ended.
		SysCtlDelay( 2 );													// Wait here for the 0 value to be written to pin 1 (output).

		// Waits here for the return signal from the sensor. Sensor replies with 1's.
		while ( GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_0 ) == 0 ) {
		}
		// Records time that low-high RX occurred. This is when the RX signal starts.
		signal_receive_start = TimerValueGet( TIMER0_BASE, TIMER_A );
		

		// Waits here for RX signal to end. Signal drops back to 0.
		while ( GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_0 ) == 1 ) {
		}
		// Records time that high-low RX occurred. This is when the RX signal ends.
		signal_receive_end = TimerValueGet( TIMER0_BASE, TIMER_A ) - signal_receive_start;

		//TimerDisable( TIMER0_BASE, TIMER_A );						// Shuts the timer off so that it can be started at its load value.
		// Send the values over Uart to the host. The timer isn't running now so sending time shouldn't be a problem.
		UARTprintf( "Interim, response signal : %d, %d\n", 
			signal_receive_start - signal_send_termination, 
			signal_receive_end - signal_receive_start );
			
		vTaskDelay( 1000 );
	

	}

}
