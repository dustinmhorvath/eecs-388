//*****************************************************************************
//
// Read proximity data from PING sensor
//
//		Author:			Dustin Horvath
//		Organization:	KU/EECS/EECS 388
//		Date:			2014–04–08
//
//		Purpose: Read in values from the PING Ultrasonic Proximity sensor
//
//		Notes: This program makes use of code provided by Dr. Gary Minden 
//				in the Stellaris driver library 
//
//		Version		0.78
//					Attempts using two pins for I/O were unsuccessful due to the design of
//					the PING Ultrasonic sensor. A rework is in progress to make functional
//					using a single pin.
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

	// Enable GPIO Port D
	SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOD );

	//
	// Configure PortD<1> as open drain output with 2 mA drive.
	// PortD<1> will be used to initiate a measurement.
	// Initialize PortD<1> to a 0 (constant 0x00).
	// For the PING sensor, input 0 is idle, while signal input 1 is used to signal the start of measuring.
	//
	GPIOPinTypeGPIOOutput( GPIO_PORTD_BASE, GPIO_PIN_1 );
	GPIOPadConfigSet( GPIO_PORTD_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD );
	GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 );			// Write 0 to pin, pulling the signal low.

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

	TimerEnable( TIMER0_BASE, TIMER_A );								// Starts the timer counting down.


	while ( 1 ) {

	// Test blocks adapted from Dr. Minden's. These can be used to determine whether initialization has been completed
	//  successfully, and to check if the hardware is functioning by design. One of the problems he ran into involved
	//  the timer not having up-shot functionality, which has been changed here to periodic. Small changes have been made.
	

	/*
		// PORT D TEST BLOCK
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 );		// Set signal to 0
		SysCtlDelay( 70 );
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x02 );		// Set signal to 1
		SysCtlDelay( 70 );
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
																	// Test using 5ms is used as the signal time for the PING sensor.
		TimerEndStart = TimerValueGet( TIMER0_BASE, TIMER_A );		// Capture time at start signal end
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 );			// Set input signal back to LOW.
		SysCtlDelay( 2 );											// Delay 6 cycles
		PortD_0_B = GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_0 );
		//UARTprintf( "PortD_0_A,_B: %d, %d\n", PortD_0_A, PortD_0_B );
	*/

		// Configure PortG[1] as OUTPUT.
		GPIOPinTypeGPIOOutput( GPIO_PORTD_BASE, GPIO_PIN_1 );
		GPIOPadConfigSet( GPIO_PORTD_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD );

		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 );
		SysCtlDelay( 100 );
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x02 );					// Begins 1 signal output.
		SysCtlDelay( 60 ) ;													// Waits ~5us, the length of typical PING sensor signal.
		GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 );					// After wait, pulls signal back down to zero.
		SysCtlDelay( 100 );

		// Configure PortG[1] as INPUT.
		GPIOPinTypeGPIOInput( GPIO_PORTD_BASE, GPIO_PIN_1 );
		GPIOPadConfigSet( GPIO_PORTD_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_OD );

		// Waits here for the return signal from the sensor. Sensor replies with 1's.
		while ( GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_1 ) == 0 ) {
			//UARTprintf( "signal value: %d,\n", GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_1 )); // FOR TESTING
		}
		TimerLoadSet( TIMER0_BASE, TIMER_A, 0xFFFF );						// Load timer to maximum
		// Records time that low-high RX occurred. This is when the RX signal starts.
		signal_receive_start = TimerValueGet( TIMER0_BASE, TIMER_A );


		// Waits here for RX signal to end. Signal drops back to 0.
		while ( GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_1 ) == 1 ) {
		}
		// Records time that high-low RX occurred. This is when the RX signal ends.
		signal_receive_end = TimerValueGet( TIMER0_BASE, TIMER_A ) - signal_receive_start;


		// Send time values over Uart.
		//UARTprintf( "Interim, response signal : %d, %d\n",
		//	signal_receive_start - signal_send_termination,
		//	signal_receive_end - signal_receive_start );

		UARTprintf( "signal value: %d,\n", GPIOPinRead( GPIO_PORTD_BASE, GPIO_PIN_1 ));


	}

}
