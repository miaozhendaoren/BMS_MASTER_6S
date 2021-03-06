//#if !defined(BSW_TIMERS_H)          // To avoid duplicate inclusion
    #include "BSW_Timers.h"
//#endif
#include "BSW_ReadADC.h"
#include "BSW_GPIO.h"
#include "BSW_SoftUART.h"
#include "APP_Readtemperature.h"
#include "APP_SendBmsData.h"
#include "APP_ReadBmsSlave.h"
#include "APP_Settings.h"
#include "APP_AuxFunctions.h"
#include "RTOS.h"
#include <p18cxxx.h>
#include <timers.h>

//extern char SlaveReceiveTimeout;

extern unsigned char ADC_Chanels[ADC_ChanalNo];
extern unsigned char ADC_ChanelCnt;
extern unsigned char ADC_SampleCnt;
extern unsigned char ADC_OK;

volatile unsigned int timerTick;
volatile unsigned int timer100msTick;
#define STATUS_B0_ALL_ROK 6
/*-##-----------Prototypes----------------------------------------------------*/
void Interrupt_low (void);
void Interrupt_high (void);
void Timer0_int (void);
void Timer1_int (void);


//real time operating system - scheduler
void RTOS(){
	
	//1s task
	if(Timer1s){
		Timer1s=0;
		
		sendCellMinVoltage_send=1;	
		sendCellMaxBalancing_send=1;
		sendCellMaxVoltage_send=1;
		
		if (ADC_OK)
		{
            CalcTemps();
			ADC_ChanelCnt = 0;                  //Za�eni ADC sekvencao
            ADC_SampleCnt = 0;
            SelChanConvADC(ADC_Chanels[ADC_ChanelCnt]);
            ADC_OK = 0;
		}
		//if readout of all cells is ok than we toggle the LED1_TOGGLE	
		LED2_TOGGLE //lifebeat
		if(test_bit(BmsStatus0, STATUS_B0_ALL_ROK)){ 		    
			if(LED2_STATUS==1) LED1_OFF
			else LED1_ON
		}else{
			LED1_OFF
		}
		
		
	}
	if(Timer100ms){
		Timer100ms=0;
		//100ms task read slave
		sendBmsStatus_send=1;
		sendCellVoltages_send=1;
		sendCellBalancingStatus_send=1;

		SlaveReceiveTimeout++;
	
    	if(!test_bit(BmsStatus0, STATUS_B0_ALL_ROK)){
 		    LED1_TOGGLE //fast blinking
		}




	}

 	sendCellVoltages();
	sendBmsStatus();
 	sendCellBalancingStatus();
 	sendCellMaxBalancing();
 	sendCellMaxVoltage();
 	sendCellMinVoltage();

	readBmsSlaves();
	checkForCan();  //read can for settings

}

/*********************************************************/
/*                       interrupts                      */
/*********************************************************/

// Timer 0 Interrupt Service Routine (Timer 0 ISR)
void Timer0_int(void) //##/////////////////////////////////////////////////////
{
	INTCONbits.TMR0IF=0;	// Clear Timer 0 overflow interrupt flag
	//WriteTimer0(65000);
	
	WriteTimer0(Timer0Value); 	
	
	interruptCAllback(); //SW uart

	
	
}// Timer0_int() END ///////////////////////////////////////////////////////////

// Timer 1 Interrupt Service Routine (Timer 1 ISR)
void Timer1_int(void) //##/////////////////////////////////////////////////////
{

	PIR1bits.TMR1IF = 0;	// Clear Timer 1 overflow interrupt flag	
	WriteTimer1(40536);

	
    timerTick++;
	
    if(timerTick>=1){ //100ms 
	    timerTick=0;
   		timer100msTick++;
    	Timer100ms=1;
    }

    if(timer100msTick>=10){
	    timer100msTick=0;
	    Timer1s=1;
    }
}
// Timer1_int() END ///////////////////////////////////////////////////////////

void interrupt myHiIsr(void)
{
    if(INTCONbits.TMR0IF)                   //check for TMR0 overflow interrupt flag
    {
        INTCONbits.TMR0IF = 0;              //clear interrupt flag
    	Timer0_int();
    }
}
// Interrupt_high() END ///////////////////////////////////////////////////////

void interrupt low_priority myLoIsr(void)
{
    if(PIR1bits.TMR1IF)                     //check for TMR1 overflow interrupt flag
    {
        PIR1bits.TMR1IF = 0;                //clear interrupt flag
        Timer1_int();
    }
	if(PIR1bits.ADIF)                       //chek for ADC INT interrupt flag
    {
        PIR1bits.ADIF = 0;                  //clear interrupt flag
        ADC_Data();
    }
}
// Interrupt_low() END ////////////////////////////////////////////////////////

