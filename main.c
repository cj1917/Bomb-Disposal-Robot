/*
 12/02/20
 
 ECM Final Project main file
 
 This project is to develop the software and hardware for an autonomous
 bomb disposal robot.
 
 The solution we use involves using one sensor to determine if the bomb is 
 straight head or not. If straight ahead, move towards the bomb, otherwise
 sweep for the bomb by turning left. Once RFID is scanned, return home by
 'undoing' each previous move.
 
 Because the sweep always turns left, we would rather miss to the right of the
 beacon so it can adjust faster. For this reason, the robot is biased to
 slightly curve right when it is moving forwards
 
 We use advanced C programming concepts such as structures, pointers,
 static local variables and type casting where necessary in order to improve, 
 reliability, memory usage, speed and human readability
 
 Charles Jones & Matteo Ghiringhelli
 */
/*****************************************************************************/
// 'Boilerplate' includes and settings
#include <xc.h>
#include <pic18f4331.h>
#include <stdio.h>
#pragma config OSC = IRCIO, MCLRE=OFF, LVP=OFF

// Include headers for project-specific source files
#include "LCDIO.h"
#include "dc_motor.h"
#include "RFID.h"
#include "signal_processing.h"
#include "subroutines.h"
/*****************************************************************************/
//Global variables defined here
// These variables must be global because they need to be visible to the ISR

/* this defines the subroutine that the robot will operate on:
   0 represents the initial sweep to look for bomb
   1 represents moving towards bomb
   2 represents returning to starting position
   3 represents finished (robot has found bomb and returned)*/
volatile char robot_mode = 0;

// stores characters read from RFID (10 data bits and 2 checksum)
volatile char RFIDbuf[12];

// This flag is set to 1 once the RFID has been completely read
volatile char RFID_flag = 0;

// This structure stores the movements made by the robot and related variables
struct Movement_storage travel_moves;
/*****************************************************************************/
// setup function, initialise registers here
void setup(void)
{   
    // Board-specific setup
    OSCCON = 0x72; // set clock to 8 MHz
    while(!OSCCONbits.IOFS); // wait for clock to stabilise
    INTCONbits.GIEH = 1; //Global high priority interrupt enable
    INTCONbits.GIEL = 1;// enable low priority interrupts
    RCONbits.IPEN=1; // enable interrupt prioritisation
    
    // Initialise pins to enable LCD, RFID, motors, etc.
    init_LCD();
    init_RFID();
    init_sensor();
    initPWM(199);
    
    // motor direction pins
    TRISBbits.RB0 = 0; 
    TRISBbits.RB2 = 0;
    
    TRISDbits.RD2 = 1; // button attached to D2, used for reset and UI 
    
    // Timer overflow every 32 ms
    T0CON = 0b11000111; // enable timer 0, 256 prescaler, 8 bit
   
    // generate interrupt on timer overflow
    INTCONbits.TMR0IE=1; // enable TMR0 overflow interrupt
    INTCON2bits.TMR0IP=0; // TMR0  Low priority
}
/*****************************************************************************/
// High priority interrupt service routine for RFID reading
void __interrupt(high_priority) InterruptHandlerHigh (void)
{
    // Trigger interrupt when a character is read from the RFID
    // this can only occur when the robot is in mode 0 or 1
    if((PIR1bits.RCIF) && ((robot_mode == 1) || robot_mode==0))
    {
        //read RFID data into buffer, once all the data has been read set flag=1
        RFID_flag = processRFID(RFIDbuf, RCREG);
    }
    // this runs if you try to scan an RFID when the robot is not searching
    else
    {
        // simply reads and discards RCREG to reset interrupt flag
        char throwaway = RCREG;
    }
}
/*****************************************************************************/
// Low priority ISR for timing movements
void __interrupt(low_priority) InterruptHandlerLow(void)
{
    // Triggers on timer interrupt when robot is moving forwards or searching
    if((INTCONbits.TMR0IF) && ((robot_mode == 1) || (robot_mode == 0)))
    {
        // increment the time of the current movement
        travel_moves.time_taken[travel_moves.move_number] += 1; 
        INTCONbits.TMR0IF = 0; // reset interrupt flag
    }
    // Triggers on timer interrupt when robot is returning home
    else if((INTCONbits.TMR0IF) && robot_mode == 2)  
    {   // decrement the time of the current movement
        travel_moves.time_taken[travel_moves.move_number] -= 1; 
        INTCONbits.TMR0IF = 0; // reset interrupt flag
    }
    // Triggers on timer interrupt when robot is in any other state
    else
    {
        INTCONbits.TMR0IF = 0; // Simply reset interrupt flag
    }
}
/*****************************************************************************/
// main function
void main(void)
{
  // first we call the setup function to initialise all the necessary registers
  setup();
  
  //now, we declare the structures that need to be visible to the main function
  struct DC_motor motorL, motorR; //declare 2 motor structures
  initMotorValues(&motorL, &motorR); // initialise values in each structure
  
  // these define how fast the robot moves in each operation
  const int SEARCHING_SPEED = 50;
  const int MOVING_SPEED = 95;
  
  stabiliseAverage(); // wait for moving average to stabilise
  waitForInput(); // wait until user presses button to start
  
  // loop, this runs forever
  while(1)
  {
      // Subroutine for sweeping to search for bomb
      if(robot_mode == 0)
      {
          robot_mode = scanForBeacon(&motorL, &motorR, SEARCHING_SPEED,
                                    &travel_moves, &RFID_flag);
      }
      
      // Subroutine to move towards bomb
      else if(robot_mode == 1)
      {
          robot_mode = moveToBeacon(&motorL, &motorR, MOVING_SPEED,
                                    &travel_moves, &RFID_flag);
      }
    
      // Subroutine to return to starting position
      else if(robot_mode == 2)
      {
          robot_mode = returnHome(&motorL, &motorR, MOVING_SPEED, 
                                    SEARCHING_SPEED, &travel_moves);
      }
      
      // Subroutine for once bomb has been found and robot has returned
      else if(robot_mode == 3)
      {
          robot_mode = stopAndDisplay(&motorL, &motorR, MOVING_SPEED,RFIDbuf);
      }
      
      // Program should never reach here, so print error message if it does
      else 
      {
          LCDString("Critical Error");
      }
  }
}

/*
 Note: we use if ... else if ... else if ... else... to improve the
 program flow and ensure that it always executes in the correct order when
 robot_mode is set properly. This helps avoid bugs that might occur if the robot
 unexpectedly enters the wrong subroutine.
 */