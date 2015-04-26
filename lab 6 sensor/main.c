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
#include "Drivers/uartstdio.h"


#include "FreeRTOS.h"
#include "task.h"
#include "driverlib/adc.h"
#include "stdio.h"
#include "queue.h"

//*****************************************************************************
//
//	The number of clock cycles between SysTick interrupts. The SysTick interrupt
//	period is 1 mS. All events in the application occur at some fraction of
//	this clock rate.
//
//*****************************************************************************
#define SYSTICK_FREQUENCY		configTICK_RATE_HZ

extern volatile int long xPortSysTickCount;




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

	char str[] = "tick";

	while(1){
		//UARTprintf("%d\n",str);
		vTaskDelay(1000);
	}

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

		vTaskDelay(750);
	}
}


extern void ProxySensor( void *pvParameters );


int main(void) {
    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);

	//xTaskCreate(BlinkLED, (signed portCHAR*) "Blinky", 256, NULL, 1, NULL);
	
	// UART code will be held, but not initialized as a task. Its functions
	// will be included in the final task that needs them. This will help
	// prevent unnecessary task switching.
	//xTaskCreate(Uart, (signed portCHAR*) "Uart", 256, NULL, 1, NULL);
	
	
	// initialize the proxysensor task
	xTaskCreate( ProxySensor, ( signed portCHAR * ) "ProxySensor", 512, NULL, 1, NULL );


	//
	//	Start FreeRTOS Task Scheduler
	//
	vTaskStartScheduler();

	while(1){}
}
