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

#include "proj1_header.h"
//#include "rgb_hsv.h"


//////////////////////////////////////////
/************Main Program****************/
//////////////////////////////////////////

int main(void)
{
	init_platform();
	
	uint32_t sts;
	
	sts = do_init();
	if(XST_SUCCESS != sts)
	{
		OLEDrgb_SetCursor(&pmodOLEDrgb_inst,0,1);
		OLEDrgb_PutString(&pmodOLEDrgb_inst,"ERROR HAS OCCURED");
		OLEDrgb_SetCursor(&pmodOLEDrgb_inst,0,3);
		OLEDrgb_PutString(&pmodOLEDrgb_inst,"INIT FAILED... THE PROGRAM WILL EXIT");
		OLEDrgb_SetCursor(&pmodOLEDrgb_inst,0,6);
		OLEDrgb_PutString(&pmodOLEDrgb_inst,"SORRY FOR THE INCONVENIENCE");
		exit(1);
	}
	
	microblaze_enable_interrupts();
	
	xil_printf("ECE 544 Color Wheel Implementation");
	xil_printf("\nBy: Kalyani Chawak");
	
	led_test();
	
	// turn the lights out

	NX410_SSEG_setAllDigits(SSEGHI, CC_BLANK, CC_BLANK, CC_BLANK, CC_BLANK, DP_NONE);
	NX410_SSEG_setAllDigits(SSEGLO, CC_BLANK, CC_BLANK, CC_BLANK, CC_BLANK, DP_NONE);

	NX4IO_RGBLED_setDutyCycle(RGB1, 0, 0, 0);
	NX4IO_RGBLED_setChnlEn(RGB1, false, false, false);
	
	get_HSV_val();

	OLEDrgb_Clear(&pmodOLEDrgb_inst);
	OLEDrgb_end(&pmodOLEDrgb_inst);
	
	// cleanup and exit
    cleanup_platform();
    exit(0);

}









