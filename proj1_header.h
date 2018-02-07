/////////////////////////////////////////////
/**
*
* ECE 544 Embedded System Design with FPGAs
* @author : Roy Kravitz
* @modified : Amit Solapurkar
* Date: 17 Jan 2018
* Project 1: Color Wheel Implementation
* Project description: 
* @note : The helper functions, display
* related functions and the interrupt
* handlers are re-used from Getting-
* started project
*
*/
/////////////////////////////////////////////

///////////////////////////////////////
/*************Libraries**************/
//////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "platform.h"
#include "xparameters.h"
#include "xstatus.h"
#include "microblaze_sleep.h"
#include "nexys4IO.h"
#include "pmodOLEDrgb.h"
#include "pmodENC.h"
#include "xgpio.h"
#include "xintc.h"
#include "xtmrctr.h"
//#include "rgb_hsv.h"

/////////////////////////////////////////
/*********Constant Definitions*********/
/////////////////////////////////////////


// AXI timer parameters
#define AXI_TIMER_DEVICE_ID		XPAR_AXI_TIMER_0_DEVICE_ID
#define AXI_TIMER_BASEADDR		XPAR_AXI_TIMER_0_BASEADDR
#define AXI_TIMER_HIGHADDR		XPAR_AXI_TIMER_0_HIGHADDR
#define TmrCtrNumber			0


// Definitions for peripheral NEXYS4IO
#define NX4IO_DEVICE_ID		XPAR_NEXYS4IO_0_DEVICE_ID
#define NX4IO_BASEADDR		XPAR_NEXYS4IO_0_S00_AXI_BASEADDR
#define NX4IO_HIGHADDR		XPAR_NEXYS4IO_0_S00_AXI_HIGHADDR

// Definitions for peripheral PMODOLEDRGB
#define RGBDSPLY_DEVICE_ID		XPAR_PMODOLEDRGB_0_DEVICE_ID
#define RGBDSPLY_GPIO_BASEADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_GPIO_BASEADDR
#define RGBDSPLY_GPIO_HIGHADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_GPIO_HIGHADD
#define RGBDSPLY_SPI_BASEADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_SPI_BASEADDR
#define RGBDSPLY_SPI_HIGHADDR	XPAR_PMODOLEDRGB_0_AXI_LITE_SPI_HIGHADDR

// Definitions for peripheral PMODENC
#define PMODENC_DEVICE_ID		XPAR_PMODENC_0_DEVICE_ID
#define PMODENC_BASEADDR		XPAR_PMODENC_0_S00_AXI_BASEADDR
#define PMODENC_HIGHADDR		XPAR_PMODENC_0_S00_AXI_HIGHADDR

// Interrupt Controller parameters
#define INTC_DEVICE_ID			XPAR_INTC_0_DEVICE_ID
#define FIT_INTERRUPT_ID		XPAR_MICROBLAZE_0_AXI_INTC_FIT_TIMER_0_INTERRUPT_INTR


// Clock frequency
#define CPU_CLOCK_FREQ_HZ	XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define AXI_CLOCK_FREQ_HZ	XPAR_CPU_M_AXI_DP_FREQ_HZ

// Timer Declaration
#define PWM_TIMER_DEVICE_ID		XPAR_TMRCTR_0_DEVICE_ID


// Fixed Interval timer - 100 MHz input clock, 40KHz output clock
// FIT_COUNT_1MSEC = FIT_CLOCK_FREQ_HZ * .001
#define FIT_IN_CLOCK_FREQ_HZ	CPU_CLOCK_FREQ_HZ
#define FIT_CLOCK_FREQ_HZ		40000
#define FIT_COUNT				(FIT_IN_CLOCK_FREQ_HZ / FIT_CLOCK_FREQ_HZ)
#define FIT_COUNT_1MSEC			40

// GPIO parameters
#define GPIO_0_DEVICE_ID			XPAR_AXI_GPIO_0_DEVICE_ID
#define GPIO_0_INPUT_0_CHANNEL		1
#define GPIO_0_OUTPUT_0_CHANNEL		2

#define GPIO_1_DEVICE_ID		XPAR_AXI_GPIO_1_DEVICE_ID
#define GPIO_1_INPUT_0_CHANNEL		1
#define GPIO_1_INPUT_1_CHANNEL		2
#define GPIO_2_DEVICE_ID		XPAR_AXI_GPIO_2_DEVICE_ID
#define GPIO_2_INPUT_0_CHANNEL		1
#define GPIO_2_INPUT_1_CHANNEL		2
#define GPIO_3_DEVICE_ID		XPAR_AXI_GPIO_3_DEVICE_ID
#define GPIO_3_INPUT_0_CHANNEL		1
#define GPIO_3_INPUT_1_CHANNEL		2



////////////////////////////////////////
/********Variable Definitions**********/
////////////////////////////////////////

// Microblaze peripheral instances
uint64_t 	timestamp = 0L;

PmodOLEDrgb	pmodOLEDrgb_inst;
PmodENC 	pmodENC_inst;

XGpio		GPIOInst0;					// GPIO instance
// GPIO instances 1,2 and 3 are used for PWM detection in hardware 
XGpio		GPIOInst1;					
XGpio		GPIOInst2;					
XGpio		GPIOInst3;					
XIntc 		IntrptCtlrInst;				// Interrupt Controller instance
XTmrCtr		AXITimerInst;				// PWM timer instance

volatile u32			gpio_in=0;			// GPIO input port
volatile u32			gpio1_in0=0;				// GPIO1 ch0 input port
volatile u32			gpio1_in1=0;				// GPIO1 ch1 input port
volatile u32			gpio2_in0=0;				// GPIO2 ch0 input port
volatile u32			gpio2_in1=0;				// GPIO2 ch1 input port
volatile u32			gpio3_in0=0;				// GPIO3 ch0 input port
volatile u32			gpio3_in1=0;				// GPIO3 ch1 input port

//hsv variables

uint16_t temp_store;

//for reducing flickering display
int count = 0;
int disp = 0;

// Value of color counts from software

volatile u32			sw_high_red = 0;
volatile u32  			sw_low_red = 0;
volatile u32			sw_high_blue = 0;
volatile u32  			sw_low_blue = 0;
volatile u32			sw_high_green = 0;
volatile u32  			sw_low_green = 0;

//temporary counts
volatile u32            temp_red_high = 0;
volatile u32            temp_red_low = 0;
volatile u32            temp_green_high = 0;
volatile u32            temp_green_low = 0;
volatile u32            temp_blue_high = 0;
volatile u32            temp_blue_low = 0;

//high,low count signals
volatile u32            cnt_red_high=0;
volatile u32            cnt_red_low=0;
volatile u32            cnt_green_high=0;
volatile u32            cnt_green_low=0;
volatile u32            cnt_blue_high=0;
volatile u32            cnt_blue_low=0;

uint16_t ledvalue;

typedef struct RGB_str{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb;

typedef struct HSV_str{
	uint8_t h;
	uint8_t s;
	uint8_t v;
} hsv;

hsv data_HSV = { 0, 255 , 255 };
rgb data_RGB;


//////////////////////////////////////////
/******Macro Inline Function Def*********/
///////////////////////////////////////////
 
#define MIN(a, b)  ( ((a) <= (b)) ? (a) : (b) )
#define MAX(a, b)  ( ((a) >= (b)) ? (a) : (b) )

/////////////////////////////////////////
/*********Function Prototypes***********/
/////////////////////////////////////////


void PMDIO_itoa(int32_t value, char *string, int32_t radix);
void PMDIO_puthex(PmodOLEDrgb* InstancePtr, uint32_t num);
void PMDIO_putnum(PmodOLEDrgb* InstancePtr, int32_t num, int32_t radix);

int	 do_init(void);							// initialize system
int AXI_Timer_initialize(void);


void update_display(hsv data_HSV);
//void update_display(int hue, int saturation, int value);
void update_7seg(uint8_t duty_red, uint8_t duty_blue, uint8_t duty_green);
void add_delay_msec(unsigned int msec);

int pwm_hw();
void get_HSV_val(void);

void FIT_Handler(void);										// fixed interval timer interrupt handler

void led_test(void);          // initial testing of LEDs 
	
void pwm_sw();
int cmp_HSV( hsv prev,hsv next);
//////////////////////////////////////////////////
/***************Helper Functions****************/
/**
* @param None
* @return None
*
* This function is executed after startup and after
* resetting. It initializes the peripherals and 
* registers the interrupt handlers
*/
////////////////////////////////////////////////////

int	 do_init(void)
{
	uint32_t status;				// status from Xilinx Lib calls

	// initialize the Nexys4 driver and (some of)the devices
	status = (uint32_t) NX4IO_initialize(NX4IO_BASEADDR);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// set all of the display digits to blanks and turn off
	// the decimal points using the "raw" set functions.
	// These registers are formatted according to the spec
	// and should remain unchanged when written to Nexys4IO...
	// something else to check w/ the debugger when we bring the
	// drivers up for the first time
	NX4IO_SSEG_setSSEG_DATA(SSEGHI, 0x0058E30E);
	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x00144116);

	OLEDrgb_begin(&pmodOLEDrgb_inst, RGBDSPLY_GPIO_BASEADDR, RGBDSPLY_SPI_BASEADDR);

	// initialize the pmodENC and hardware
	status = pmodENC_initialize(&pmodENC_inst, PMODENC_BASEADDR);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// initialize the GPIO instances
	status = XGpio_Initialize(&GPIOInst0, GPIO_0_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		xil_printf("\n GPIO 0 init failure");
		return XST_FAILURE;
	}

	status = XGpio_Initialize(&GPIOInst1, GPIO_1_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		xil_printf("\n GPIO 1 init failure");
		return XST_FAILURE;
	}

	status = XGpio_Initialize(&GPIOInst2, GPIO_2_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		xil_printf("\n GPIO 2 init failure");
		return XST_FAILURE;
	}

	status = XGpio_Initialize(&GPIOInst3, GPIO_3_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		xil_printf("\n GPIO 3 init failure");
		return XST_FAILURE;
	}
	// GPIO0 channel 1 is an 8-bit input port.
	// GPIO0 channel 2 is an 8-bit output port.
	XGpio_SetDataDirection(&GPIOInst0, GPIO_0_INPUT_0_CHANNEL, 0xFF);
	XGpio_SetDataDirection(&GPIOInst0, GPIO_0_OUTPUT_0_CHANNEL, 0x00);

	// GPIO1 channel 1 is an 32-bit input port.
	// GPIO1 channel 2 is an 32-bit input port.
	XGpio_SetDataDirection(&GPIOInst1, GPIO_1_INPUT_0_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIOInst1, GPIO_1_INPUT_1_CHANNEL, 0xFFFFFFFF);

	// GPIO2 channel 1 is an 32-bit input port.
	// GPIO2 channel 2 is an 32-bit input port.
	XGpio_SetDataDirection(&GPIOInst2, GPIO_2_INPUT_0_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIOInst2, GPIO_2_INPUT_1_CHANNEL, 0xFFFFFFFF);

	// GPIO3 channel 1 is an 32-bit input port.
	// GPIO3 channel 2 is an 32-bit input port.
	XGpio_SetDataDirection(&GPIOInst3, GPIO_3_INPUT_0_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIOInst3, GPIO_3_INPUT_1_CHANNEL, 0xFFFFFFFF);

	status = AXI_Timer_initialize();
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// initialize the interrupt controller
	status = XIntc_Initialize(&IntrptCtlrInst, INTC_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
	   return XST_FAILURE;
	}
	
	// connect the fixed interval timer (FIT) handler to the interrupt
	status = XIntc_Connect(&IntrptCtlrInst, FIT_INTERRUPT_ID,
						   (XInterruptHandler)FIT_Handler,
						   (void *)0);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;

	}

	// start the interrupt controller such that interrupts are enabled for
	// all devices that cause interrupts.
	status = XIntc_Start(&IntrptCtlrInst, XIN_REAL_MODE);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// enable the FIT interrupt
	XIntc_Enable(&IntrptCtlrInst, FIT_INTERRUPT_ID);
	return XST_SUCCESS;
}


/****************************************************************************
*
* AXI timer initializes it to generate out a 4Khz signal, Which is given to the Nexys4IO module as clock input.
* DO NOT MODIFY
*
*****************************************************************************/
int AXI_Timer_initialize(void){

	uint32_t status;				// status from Xilinx Lib calls
	u32		ctlsts;		// control/status register or mask

	status = XTmrCtr_Initialize(&AXITimerInst,AXI_TIMER_DEVICE_ID);
		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	status = XTmrCtr_SelfTest(&AXITimerInst, TmrCtrNumber);
		if (status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	ctlsts = XTC_CSR_AUTO_RELOAD_MASK | XTC_CSR_EXT_GENERATE_MASK | XTC_CSR_LOAD_MASK |XTC_CSR_DOWN_COUNT_MASK ;
	XTmrCtr_SetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber,ctlsts);

	//Set the value that is loaded into the timer counter and cause it to be loaded into the timer counter
	XTmrCtr_SetLoadReg(AXI_TIMER_BASEADDR, TmrCtrNumber, 24998);
	XTmrCtr_LoadTimerCounterReg(AXI_TIMER_BASEADDR, TmrCtrNumber);
	ctlsts = XTmrCtr_GetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber);
	ctlsts &= (~XTC_CSR_LOAD_MASK);
	XTmrCtr_SetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber, ctlsts);

	ctlsts = XTmrCtr_GetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber);
	ctlsts |= XTC_CSR_ENABLE_TMR_MASK;
	XTmrCtr_SetControlStatusReg(AXI_TIMER_BASEADDR, TmrCtrNumber, ctlsts);

	XTmrCtr_Enable(AXI_TIMER_BASEADDR, TmrCtrNumber);
	return XST_SUCCESS;

}

///////////////////////////////////////////////
/**
*
*/
///////////////////////////////////////////////

void add_delay_msec(unsigned int msec)
{

	if ( msec == 0 ) {
		return;
	}

	timestamp = timestamp + msec;
	usleep(msec * 1000);

}

/////////////////////////////////////////////////
/**
* @param None
* @return None
*
* This function tests the LEDs
*/
/////////////////////////////////////////////////

void led_test(void)
{
	
	// xil_printf("\nLED test");
	NX4IO_setLEDs(0x00005555);
	add_delay_msec(1000);
	
	ledvalue = 0x0001;
	do
	{
		NX4IO_setLEDs(ledvalue);
		add_delay_msec(500);
		ledvalue = ledvalue << 1;
	}while (ledvalue != 0);

	/*
	xil_printf("\nRGB LED Test");
	
	//NX4IO_RGBLED_setChn1En(RGB1, true, true, true);
	NX4IO_RGBLED_setDutyCycle(RGB1, 0, 0, 16);
	add_delay_msec(2000);
	
	NX4IO_RGBLED_setDutyCycle(RGB1, true, false, false);
	add_delay_msec(2000);
*/
}

///////////////////////////////////////////////////
/***********Interrupt Handlers**************/
/**
* @param None
* @return None
* 
* Fixed interval timer interrupt handler
* This function reads the GPIO port which reads back
* the hardware generated PWM wave for the RGB LEDs.
* This function detects the PWM wave in software
*/
////////////////////////////////////////////////////

void FIT_Handler(void)
{
	// Read the GPIO port to read back the generated PWM signal for RGB led's
	gpio_in = XGpio_DiscreteRead(&GPIOInst0, GPIO_0_INPUT_0_CHANNEL);

		// The following program counts the high counts (logic 1)
		// and low counts (logic 0) for each color which helps to 
		// determine the Duty Cycle 		

		// Detect counts for Green color
	 	if((gpio_in & 0x0001) == 0x0001)
	 		{

	 			if(cnt_green_high == 1 && cnt_green_low == 1)
	 			{
	 				sw_high_green= temp_green_high;
	 				sw_low_green= temp_green_low;

	 				temp_green_high = 1;
	 				temp_green_low = 0;

	 				cnt_green_high = 0;
	 				cnt_green_low = 0;

	 			}

	 			// Increment high count
	 			else
	 			{
	 				cnt_green_high = 1;
	 				temp_green_high= temp_green_high + 1;
	 			}
	 		}

	 		else
	 		{
	 			// Increment low count
	 			if(cnt_green_high == 1)
	 			{
	 				cnt_green_low = 1;
	 				temp_green_low = temp_green_low + 1;
	 			}

	 			else
				{
	 				cnt_green_low = 0;
				}
	 		}

	 	// Detect counts for Blue color

	 	if((gpio_in & 0x0002) == 0x0002)
	 			{
	 				if(cnt_blue_high == 1 && cnt_blue_low == 1)
	 				{
	 					sw_high_blue= temp_blue_high;
	 					sw_low_blue= temp_blue_low;

	 					temp_blue_high = 1;
	 					temp_blue_low = 0;

	 					cnt_blue_high = 0;
	 					cnt_blue_low = 0;

	 				}

	 				// Increment high count
	 				else
	 				{
	 					cnt_blue_high = 1;
	 					temp_blue_high = temp_blue_high + 1;
	 				}
	 			}

	 			else
	 			{
	 				// Increment low count value
	 				if(cnt_blue_high == 1)
	 				{
	 					cnt_blue_low = 1;
	 					temp_blue_low = temp_blue_low + 1;
	 				}

	 				else
					{
	 					cnt_blue_low = 0;
					}
	 			}
				
		// Detect counts for Red color

	 	if ((gpio_in & 0x0004) == 0x0004)
	 	{
	 		// Check high and low count signals

	 		if(cnt_red_high == 1 && cnt_red_low == 1)
	 		{
	 			sw_high_red= temp_red_high;
	 			sw_low_red= temp_red_low;

	 			temp_red_high = 1;
	 			temp_red_low = 0;

	 			cnt_red_high = 0;
	 			cnt_red_low = 0;

	 	    }

	 		// Increment high count
	 		else
	 		{
	 			cnt_red_high = 1;
	 			temp_red_high= temp_red_high + 1;
	 		}
	 	}

	 	else
	 	{
	 		// Increment low count
	 		if(cnt_red_high == 1)
	 		{
	 			cnt_red_low = 1;
	 			temp_red_low = temp_red_low + 1;
	 		}

	 		else
			{
	 			cnt_red_low = 0;
			}
	 	}

	 	disp = 0;

	 	if(count>1)
	 		{
				disp = 1;
				count = 0;
	 		}

	 	count++;

}

/////////////////////////////////////////////////////
/**
*
*/
/////////////////////////////////////////////////////

int pwm_hw()
{
	//xil_printf("\n PWM hardware detection");

	gpio1_in0 = XGpio_DiscreteRead(&GPIOInst1, GPIO_1_INPUT_0_CHANNEL); //red high time
	gpio1_in1 = XGpio_DiscreteRead(&GPIOInst1, GPIO_1_INPUT_1_CHANNEL); //red low time
	gpio2_in0 = XGpio_DiscreteRead(&GPIOInst2, GPIO_2_INPUT_0_CHANNEL); //green high time
	gpio2_in1 = XGpio_DiscreteRead(&GPIOInst2, GPIO_2_INPUT_1_CHANNEL); //green low time
	gpio3_in0 = XGpio_DiscreteRead(&GPIOInst3, GPIO_3_INPUT_0_CHANNEL); //blue high time
	gpio3_in1 = XGpio_DiscreteRead(&GPIOInst3, GPIO_3_INPUT_1_CHANNEL); //blue low time


	//xil_printf("gpio_1_in0: %d, \ngpio_1_in1: %d\n", gpio1_in0, gpio1_in1);

	//xil_printf("gpio_2_in0: %d, \ngpio_2_in1: %d\n", gpio2_in0, gpio2_in1);

	//xil_printf("gpio_3_in0: %d, \ngpio_3_in1: %d\n", gpio3_in0, gpio3_in1);


	// Calculating the duty cycle
	uint32_t hw_duty_red = gpio1_in0*100*8.25/(gpio1_in0+gpio1_in1);
	uint32_t hw_duty_green = gpio2_in0*100*4.1/(gpio2_in0+gpio2_in1);
	uint32_t hw_duty_blue = gpio3_in0*100*8.25/(gpio3_in0+gpio3_in1);

	if(disp == 1)
	{
		//xil_printf("\nDuty cycles for red green blue are %d,%d,%d",hw_duty_red,hw_duty_green,hw_duty_blue);
		update_7seg(hw_duty_red,hw_duty_green,hw_duty_blue);
		//add_delay_msec(1);
	}

	return 0;
}

//////////////////////////////////////////////////////////////
/****************Display Related Functions*******************/
/**
*
*
*/
//////////////////////////////////////////////////////////////

void update_7seg(uint8_t red,uint8_t green, uint8_t blue)
{

	//xil_printf("Displaying the duty cycle on 7-segment display");
	
	// clear all the decimal points
	NX4IO_SSEG_setDecPt(SSEGLO, DIGIT7, false);
	NX4IO_SSEG_setDecPt(SSEGLO, DIGIT6, false);
	NX4IO_SSEG_setDecPt(SSEGLO, DIGIT5, false);
	NX4IO_SSEG_setDecPt(SSEGLO, DIGIT4, false);
	NX4IO_SSEG_setDecPt(SSEGHI, DIGIT3, false);
	NX4IO_SSEG_setDecPt(SSEGHI, DIGIT2, false);
	NX4IO_SSEG_setDecPt(SSEGHI, DIGIT1, false);
	NX4IO_SSEG_setDecPt(SSEGHI, DIGIT0, false);

	// Setting the digits to display duty cycle for red color
	NX4IO_SSEG_setDigit(SSEGHI, DIGIT7,(int) (red/10));
    NX4IO_SSEG_setDigit(SSEGHI, DIGIT6,(int) (red%10));
	
	NX4IO_SSEG_setDigit(SSEGHI, DIGIT5, CC_BLANK);
	
	// Setting the digits to display duty cycle for green color
	NX4IO_SSEG_setDigit(SSEGHI, DIGIT4,(int) (green/10));
	NX4IO_SSEG_setDigit(SSEGLO, DIGIT3,(int) (green%10));

	NX4IO_SSEG_setDigit(SSEGLO, DIGIT2, CC_BLANK);
	
	// Setting the digits to display duty cycle for blue color
	NX4IO_SSEG_setDigit(SSEGLO, DIGIT1,(int) (blue/10));
    NX4IO_SSEG_setDigit(SSEGLO, DIGIT0,(int) (blue%10));

	return;

}

/////////////////////////////////////////////////////////
/**
*
*/
/////////////////////////////////////////////////////////

void update_display(hsv data_HSV)

{
	//xil_printf("Displaying data on OLED display");
	
	OLEDrgb_Clear(&pmodOLEDrgb_inst);

	OLEDrgb_DrawRectangle(&pmodOLEDrgb_inst, 60, 10, 80, 40, OLEDrgb_BuildHSV(data_HSV.h,data_HSV.s,data_HSV.v), true, OLEDrgb_BuildHSV(data_HSV.h,data_HSV.s,data_HSV.v));
	
	OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 0, 1);
	OLEDrgb_PutString(&pmodOLEDrgb_inst, "H: ");
	PMDIO_putnum(&pmodOLEDrgb_inst, data_HSV.h, 10);

	OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 0, 3);
	OLEDrgb_PutString(&pmodOLEDrgb_inst, "S: ");
	PMDIO_putnum(&pmodOLEDrgb_inst, data_HSV.s, 10);

	OLEDrgb_SetCursor(&pmodOLEDrgb_inst, 0, 5);
	OLEDrgb_PutString(&pmodOLEDrgb_inst, "V: ");
	PMDIO_putnum(&pmodOLEDrgb_inst, data_HSV.v, 10);

}


/////////////////////////////////////////////////
/**
*
*/
/////////////////////////////////////////////////

void PMDIO_itoa(int32_t value, char *string, int32_t radix)
{
	char tmp[33];
	char *tp = tmp;
	int32_t i;
	uint32_t v;
	int32_t  sign;
	char *sp;

	if (radix > 36 || radix <= 1)
	{
		return;
	}

	sign = ((10 == radix) && (value < 0));
	if (sign)
	{
		v = -value;
	}
	else
	{
		v = (uint32_t) value;
	}

  	while (v || tp == tmp)
  	{
		i = v % radix;
		v = v / radix;
		if (i < 10)
		{
			*tp++ = i+'0';
		}
		else
		{
			*tp++ = i + 'a' - 10;
		}
	}
	sp = string;

	if (sign)
		*sp++ = '-';

	while (tp > tmp)
		*sp++ = *--tp;
	*sp = 0;

  	return;
}

/////////////////////////////////////////////////
/**
*
*/
/////////////////////////////////////////////////

void PMDIO_puthex(PmodOLEDrgb* InstancePtr, uint32_t num)
{
  char  buf[9];
  int32_t   cnt;
  char  *ptr;
  int32_t  digit;
  
  ptr = buf;
  for (cnt = 7; cnt >= 0; cnt--) {
    digit = (num >> (cnt * 4)) & 0xF;
    
    if (digit <= 9)
	{
      *ptr++ = (char) ('0' + digit);
	}
    else
	{
      *ptr++ = (char) ('a' - 10 + digit);
	}
  }

  *ptr = (char) 0;
  OLEDrgb_PutString(InstancePtr,buf);
  
  return;
}

//////////////////////////////////////////////
/**
*
*/
/////////////////////////////////////////////

void PMDIO_putnum(PmodOLEDrgb* InstancePtr, int32_t num, int32_t radix)
{
  char  buf[16];

  PMDIO_itoa(num, buf, radix);
  OLEDrgb_PutString(InstancePtr,buf);

  return;
}

/////////////////////////////////////////////
/**
*
*/
//////////////////////////////////////////////

void get_HSV_val(void)
{
	int  rot_inc;
	int RotaryCnt;

	bool rot_no_neg;

	// test the rotary encoder functions
	rot_inc = 1;
	rot_no_neg = false;

	// Initialize and clear the rotary encoder
	pmodENC_init(&pmodENC_inst, rot_inc, rot_no_neg);
	pmodENC_clear_count(&pmodENC_inst);

	while(1)
	{

        hsv temp = data_HSV;
		// read the new value from the rotary encoder
		pmodENC_read_count(&pmodENC_inst, &RotaryCnt);

		// exit the loop if button is pressed 
		if ( pmodENC_is_button_pressed(&pmodENC_inst) )
		{
			xil_printf("\n Exiting while loop get_HSV_val");
			
			// Ending message
			OLEDrgb_Clear(&pmodOLEDrgb_inst);
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst,0,1);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"That's all Folks!!");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst,0,3);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"Thank you for your time");
			OLEDrgb_SetCursor(&pmodOLEDrgb_inst,0,6);
			OLEDrgb_PutString(&pmodOLEDrgb_inst,"Have a great day ahead");
				break;
		}

		// Assigning the hue value 
		data_HSV.h = RotaryCnt;

		// Buttons are used to adjust S and V values
		if (NX4IO_isPressed(BTNU))
					data_HSV.v++;
				
		else if (NX4IO_isPressed(BTND))
					data_HSV.v--;

		else if (NX4IO_isPressed(BTNR))
					data_HSV.s++;

		else if (NX4IO_isPressed(BTNL))
					data_HSV.s--;


		// Extract RGB values from HSV

		temp_store = OLEDrgb_BuildHSV(data_HSV.h,data_HSV.s,data_HSV.v);
		data_RGB.r = OLEDrgb_ExtractRFromRGB(temp_store);
		data_RGB.g = OLEDrgb_ExtractGFromRGB(temp_store);
		data_RGB.b = OLEDrgb_ExtractBFromRGB(temp_store);

		// Check switch status for sw/hw PWM detection 

		ledvalue = NX4IO_getSwitches();


		if(ledvalue == 1)
		{
			NX4IO_setLEDs(ledvalue);
			pwm_hw();
		}
		else 
		{
			ledvalue = 0;
			NX4IO_setLEDs(ledvalue);
			//xil_printf("\nSW PWM detection");
			pwm_sw();
		}

		// Display HSV duty cycle on OLED display

		if (!cmp_HSV(temp,data_HSV))
		{
			update_display(data_HSV);
			NX4IO_RGBLED_setChnlEn(RGB1, true, true, true);
			NX4IO_RGBLED_setDutyCycle(RGB1, data_RGB.r, data_RGB.g, data_RGB.b);
			NX4IO_RGBLED_setChnlEn(RGB2, true, true, true);
			NX4IO_RGBLED_setDutyCycle(RGB2, data_RGB.r, data_RGB.g, data_RGB.b);

		}

	}  //exit the loop

	NX4IO_SSEG_putU32Hex(0xFFFFFFFF);
	add_delay_msec(5000);

}

void pwm_sw()
{

	// Calculating duty cycle for different colors

	uint16_t duty_red = sw_high_red*100*8.25/(sw_low_red + sw_high_red );
	uint16_t duty_blue = sw_high_blue*100*8.25/(sw_low_blue + sw_high_blue );
	uint16_t duty_green = sw_high_green*100*4.1/(sw_low_green + sw_high_green );

	if (disp == 1)
	{

		// Displaying the values on 7segment display
		//xil_printf("\n*******SW******* Duty cycles for red green blue are %d,%d,%d",duty_red,duty_green,duty_blue);
		update_7seg(duty_red,duty_green,duty_blue);

	}
}

int cmp_HSV( hsv prev,hsv next)
{
	if(prev.h == next.h && prev.s == next.s && prev.v == next.v)
		return 1;
	else
		return 0;
}
