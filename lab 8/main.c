//*****************************************************************************
//
//	FreeRTOS Demo
//
//		Blink the LED
//
//		Author: 		Gary J. Minden
//		Organization:	KU/EECS/EECS 388
//		Date:			2013-04-19
//		Version:		1.0
//
//		Purpose:		Example program that demonstrates:
//							(1) setting up a GPIO pin (LED)
//							(2) blinking (toggling) a LED
//
//		Notes:			Updated at KU and adapted from the TI LM3S1968 blinky
//						and other examples.
//
//*****************************************************************************
// Edited by Brad Torrence 2014-4-7

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"

#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/gpio.h"
#include "Drivers/rit128x96x4.h"

#include "FreeRTOS.h"
#include "task.h"
#include "Drivers/uartstdio.h"

#include "stdio.h"
#include "semphr.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

//*****************************************************************************
//
//	The number of clock cycles between SysTick interrupts. The SysTick interrupt
//	period is 1 mS. All events in the application occur at some fraction of
//	this clock rate.
//
//*****************************************************************************
#define SYSTICK_FREQUENCY		1000

extern volatile int long xPortSysTickCount;

//*****************************************************************************
//
//	Task to output Initial screen.
//
//*****************************************************************************

void Uart(void *pvParameters){
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

	while(1){
		//UARTprintf("%d\n",str);
		vTaskDelay(1000);
	}

}

void PrintInit(){
    //
	// 	Code to cause a wait for a "Select Button" press
    //
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	GPIOPinTypeGPIOInput(GPIO_PORTG_BASE, GPIO_PIN_7);
    GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	RIT128x96x4Init(1000000);
	RIT128x96x4StringDraw("FreeRTOS starting\n", 8, 0, 15);
	RIT128x96x4StringDraw("Press \"Select\" Button", 0, 24, 15);
	RIT128x96x4StringDraw("To Continue", 32, 32, 15);
	while(GPIOPinRead(GPIO_PORTG_BASE, GPIO_PIN_7));
	SysCtlPeripheralReset(SYSCTL_PERIPH_GPIOG);
	SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOG);
}

//*****************************************************************************
//
//	Task to blink the LED
//
//*****************************************************************************
void BlinkLED(void *pvParameters){
    //
    // Enable the GPIO Port G.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);

    //
    // Configure GPIO_G to drive the Status LED.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);
    GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	unsigned long LED_Data = 0;

	while(1){
		//
	 	// Toggle the LED.
		//
		LED_Data = GPIOPinRead(GPIO_PORTG_BASE, GPIO_PIN_2);
		LED_Data = LED_Data ^ 0x04;
		GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_2, LED_Data);

		vTaskDelay(250);
	}
}

static unsigned long TimerCount;
static char hours = 0;
static char minutes = 0;
static char seconds = 0;
static char cSeconds = 0;


//*****************************************************************************
//
//	Task to Display the systick count
//
//*****************************************************************************



xSemaphoreHandle Timer_0_A_Semaphore;



extern void Task_TimeOfDay(void *pvParameters){

	// Enable the timer hardware
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	
	// Configure the timer to run as two separate 16 bit timers, then configure timer A 
	TimerConfigure( TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
	
	// Set the prescale value. This is the factor that the timer bit counter will be divided by
	// to determine its period. Here, we divide by TEN, but the hardware wants a 9, because it starts
	// at index 0
	TimerPrescaleSet( TIMER0_BASE, TIMER_A, 9);
	
	// Set a load value. After the timer reaches zero, reset the timer to 50000*period time.
	TimerLoadSet( TIMER0_BASE, TIMER_A, 50000);

	//Enable Timer_0_A interrupt in the peripheral
	TimerIntEnable( TIMER0_BASE, TIMER_TIMA_TIMEOUT );
	//Enable Timer_0_A interrupt in NVIC
	IntEnable( INT_TIMER0A );

	// start the timer
	TimerEnable( TIMER0_BASE, TIMER_A);

	//screen
	char				TimeString[32];

	//
	//	Initialize the OLED display and write status.
	//
	RIT128x96x4Init(1000000);
	RIT128x96x4Clear();
	RIT128x96x4StringDraw("Timer_Interrupt", 8, 0, 8);

	// The integer that will hold the number of interrupt ticks
	long TimerStatus_1 = 0;

	// Declare a semaphore binary here. This will be grabbed by the ISR to increment the counter
	vSemaphoreCreateBinary(Timer_0_A_Semaphore);

	while(1){

		// Wait here until the semaphore is freed up
		xSemaphoreTake( Timer_0_A_Semaphore, portMAX_DELAY );

		// Increment cSeconds every time the semaphore is released. This should effectively keep the two counters in sync.
		// This is mostly deprecated, from Experiment 1. It's still used for the display, but the interrupt counter
		// could also be parsed into these values using a modulo.
		cSeconds++;
		if(cSeconds > 99){
			cSeconds = 0;
			seconds++;
		}
		if(seconds > 59){
			seconds = 0;
			minutes++;
		}
		if(minutes > 59){
			minutes = 0;
			hours++;
		}

		
		// Print all the time values to teh LED screen
		sprintf(TimeString, "Time: %d", hours);
		RIT128x96x4StringDraw(TimeString, 0, 16, 15);
		sprintf(TimeString, ":%d", minutes);
		RIT128x96x4StringDraw(TimeString, 48, 16, 15);
		sprintf(TimeString, ":%d", seconds);
		RIT128x96x4StringDraw(TimeString, 68, 16, 15);
		sprintf(TimeString, ":%d", cSeconds);
		RIT128x96x4StringDraw(TimeString, 88, 16, 15);

		// Wait prevents this task from 
		vTaskDelay(10);
	}
}

#pragma INTERRUPT (Timer_0_A_ISR_Handler);

// Declare the ISR handler. Returns void and is of __interrupt type
__interrupt void Timer_0_A_ISR_Handler() {
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	// increments the counter using the timer's hardware interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerCount++;


	//
	// "Give" the Timer_0_A_Semaphore
	// This releases the semaphore so that other tasks can pick it up
	xSemaphoreGiveFromISR( Timer_0_A_Semaphore, &xHigherPriorityTaskWoken );
	//
	// If xHigherPriorityTaskWoken was set to true,
	// we should yield. The actual macro used here is
	// port specific.
	//
	if ( xHigherPriorityTaskWoken ) {
		vPortYieldFromISR( );
	}


}


//*****************************************************************************
//
//	Main
//
//*****************************************************************************

int main(void) {

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);

    xTaskCreate(Uart, (signed portCHAR*) "Uart", 32, NULL, 1, NULL);

	//
	//	Create a task to blink LED
	//
	xTaskCreate(BlinkLED, (signed portCHAR*) "Blinky", 32, NULL, 1, NULL);



	//RUNS EXPERIMENT TASK
	xTaskCreate(Task_TimeOfDay, (signed portCHAR*) "Task_TimeOfDay", 512, NULL, 1, NULL);



	//
	//	Start FreeRTOS Task Scheduler
	//
	vTaskStartScheduler();

	while(1){}
}
