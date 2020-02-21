/*
 * 12/02/20
 * 
 * ECM Final Project main file
 * 
 * This project is to develop the software and hardware for an autonomous
 * bomb disposal robot.
 
 */

/*****************************************************************************/
// Include statements and boilerplate code
#include <xc.h>
#include <pic18f4331.h>
#include <stdio.h>
#pragma config OSC = IRCIO, MCLRE=OFF, LVP=OFF

#include "LCDIO.h"
#include "dc_motor.h"
#include "RFID.h"

/*****************************************************************************/
//Global variables defined here

// this defines the subroutine that the robot will operate on:
// 0 represents searching for bomb
// 1 represents returning to starting position
// 2 represents finished (robot has found bomb and returned)
volatile char robot_mode = 0;

// stores the characters read from RFID (10 data bits and 2 checksum)
volatile char RFIDbuf[12];

/*****************************************************************************/
// setup function, initialise registers here
void setup(void)
{   
    // set the clock to 8 MHz and wait for it to stabilise
    OSCCON = 0x72; 
    while(!OSCCONbits.IOFS);
    INTCONbits.GIEH = 1; //Global high priority interrupt enable
    INTCONbits.GIEL = 1;// enable low priority interrupts
    RCONbits.IPEN=1; // enable interrupt prioritisation
    
    LCD_Init();
    init_RFID();
    
    TRISCbits.RC3 = 1;
}
/*****************************************************************************/
// High priority interrupt service routine
void __interrupt(high_priority) InterruptHandlerHigh (void)
{
    // Trigger interrupt when a character is read from the RFID
    // this can only occur when the robot is in searching mode
    if((PIR1bits.RCIF) && (robot_mode == 0))
    {
        // process the RFID data, once all the data has been read, it is
        // displayed and RFID_flag is set to 1
        char RFID_flag = processRFID(RFIDbuf, RCREG);
        
        // once the RFID has been read, check against checksum and set 
        // robot to return home
        if(RFID_flag == 1)
        {
            check_RFID(RFIDbuf);
            robot_mode = 1;
        }
    }
    // this runs if you try to scan an RFID when the robot is not searching
    else
    {
        // simply reads and discards RCREG to reset interrupt flag
        char throwaway = RCREG;
    }
}
/*****************************************************************************/
// Low priority interrupt service routine
void __interrupt(low_priority) InterruptHandlerLow (void)
{
    
}
/*****************************************************************************/
// main function
void main(void)
{
  // first we call the setup function
  setup();
  
  
  // loop, this runs forever
  while(1){
      
      // Subroutine to search for bomb
      while(robot_mode == 0)
      {
          
      }
      
      // Subroutine to return to starting position
      while(robot_mode == 1)
      {
          // for debugging only, remove later
          robot_mode = 2;
      }
      
      // Subroutine for once bomb has been found and robot has returned
      while(robot_mode == 2)
      {
          while(PORTCbits.RC3 == 1)
          {
              ClearLCD();
              LCD_String("RESETTING ROBOT");
              __delay_ms(100);
              robot_mode = 0;
          }
      }
  }  
}
