//Source Code
/*******************************************************************************
Module:
main function used as part of elevatorSummative.c

 Author - Shubham Aggarwal
 Date - June 11th 2012

 Explain Operation of Program here:
	This program is designed to simulate an elevator. There are two inputs 
        (up and down) and three floors. A stepper motor rotates to pull up a
        paper elevator, depending on the input buttons. The interrupt is
        designed to simulate a fire alarm within the elevator.
	
 Hardware Notes:
        There are three indicator LEDs which indicate the floor level, as well
        as a red LED which indicates the fire alarm. A seven segment display
        outputs the floor number. Two pushbutton switches are used for up and
        down buttons and another one is used for the fire alarm. A piezoelectric
        buzzer is used during the fire alarm sequence and to indicate that the
        floor has arrived.

*******************************************************************************/

/*******************************************************************************
        Include Files
 ******************************************************************************/
#include "p24fj32ga002.h"

/*******************************************************************************
        Symbolic Constants used by main()
*******************************************************************************/

/*******************************************************************************
         Local Function Prototypes
*******************************************************************************/
void initializePorts (void);

void initializeTimer (void);
void delay (float milli);

void handleInputs(void);

void segmentDisplay (int a, int b, int c, int d, int e, int f, int g);
void updateIndicators(void);

void elevatorUp (int cycles);
void elevatorDown (int cycles);

void initializeInterrupt1 (void);
void __attribute__((interrupt,no_auto_psv)) _INT1Interrupt (void);

void buzzer (int length);
void fireAlarm (void);

/*******************************************************************************
        Configuration Bit Macros
*******************************************************************************/
_CONFIG2 (FNOSC_FRC)
_CONFIG1 (JTAGEN_OFF & FWDTEN_OFF & ICS_PGx2)

/*******************************************************************************
        Constants
*******************************************************************************/
#define MOTOR_DELAY                 30 //The delay between each motor step
#define ONE_FLOOR_TICKS             144 //The number of steps between each floor

//Main Inputs
#define UP_BUTTON                   _RA4
#define DOWN_BUTTON                 _RB5

//Indicator LEDs
#define FIRST_FLOOR_LED             _LATB15
#define SECOND_FLOOR_LED            _LATB14
#define THIRD_FLOOR_LED             _LATB13
#define FIRE_ALARM_LED              _LATB12

#define BUZZER                      _LATB10

//Seven Segment Display
#define SEG_A                       _LATB7
#define SEG_B                       _LATB6
#define SEG_C                       _LATB4
#define SEG_D                       _LATB3
#define SEG_E                       _LATA2
#define SEG_F                       _LATB8
#define SEG_G                       _LATB9

//Stepper motor
#define BLACK                       _LATB1
#define YELLOW                      _LATA1
#define BROWN                       _LATB0
#define ORANGE                      _LATA0

/*******************************************************************************
        Global Variable Declarations
*******************************************************************************/
int currentFloorLevel = 1; /* Used to keep track of the floor that the elevator
                           is on */
int motorPosition = 0; /* Incremented when elevator is going up and decremented
                        * when elevator is going down. Acts like an encoder,
                        * recording position of the motor at all times. */
                                            
int motorSetpoint = ONE_FLOOR_TICKS/4; /*Used for inputting the number of cycles
                                        *that the motor must complete into the
                                        *elevatorUp and elevatorDown functions*/

float buzzerDelay = 0; /* Used to vary the square wave pulse sent to the buzzer
                        * so thatdifferent tones can be produced */

int buzzerCounter = 0; /*Controls the amount of times the buzzer sequence
                        * is repeated */

int counter; //Miscellaneous variable used to control 'for loops'

/*******************************************************************************
        main() function
*******************************************************************************/

int main (void)
{
    //Initilize and configure the PIC
    initializeTimer();
    initializePorts();
    initializeInterrupt1();

    while (1)
    {
        handleInputs();
        updateIndicators();
    }

} //End elevatorSummative.c

/*******************************************************************************
 * Function:    initializePorts
 *
 * PreCondition: none
 * Input:   none
 * Output:  none
 * Side Effects: none
 *
 * Overview:    Convert all analog ports to digital and teach them to be inputs
 *              or outputs, depending on their intended use.
 *
 * Note:
 * ****************************************************************************/
void initializePorts (void)
{
    AD1PCFG = 0b11111111111; //Convert all analog ports to digital I/O
    TRISA = 0b10000; //Teach all A ports except RA4 to be outputs
    TRISB = 0b0000000000100100; /* Teach all B ports except RB2 and RB5 to be
                                 * outputs */
}

/*******************************************************************************
 * Function:    initializeTimer
 *
 * PreCondition: none
 * Input:   none
 * Output:  none

 * Overview:    This is intended to initialize the microcontroller so that
 *              we can generate delay
 * Note:
 * ****************************************************************************/
void initializeTimer (void)
{
    T2CON = 0;
    T3CON = 0;

    TMR3 = 0;
    TMR2 = 0;

    IFS0bits.T3IF = 0;
    T2CONbits.T32 = 1;
    T2CONbits.TON = 1;
}

/*******************************************************************************
 * Function:    delay
 *
 * PreCondition: none
 * Input:   Number of milliseconds to delay
 * Output:  none
 
 * Overview:    Generate a delay in milliseconds, up to ~35minutes. Used for 
 *              many purposes, including for the stepper motor, buzzer and
 *              flashing LEDs.
 * Note:
 * ****************************************************************************/
void delay (float milli)
{
    unsigned long PeriodRegisterValue;

    PeriodRegisterValue = (float) milli * 4000ul;

    TMR2 = 0;
    TMR3 = 0;

    PR3 = (unsigned int) (PeriodRegisterValue >> 16);
    PR2 = (unsigned int) (PeriodRegisterValue & 0x0000FFFF);

    while (!_T3IF == 1);
        _T3IF = 0;
}

/*******************************************************************************
 * Function:    handleInputs
 *
 * PreCondition: none
 * Input:   none
 * Output:  none
 * Side Effects: none
 *
 * Overview: This function checks whether or not the up or down buttons are
 *           pressed and moves the stepper motor accordingly. It is coded in
 *           such a way that if both buttons are pressed at the same time, or
 *           if the elevator is already at the lowest or highest level, it will
 *           not move the stepper motor or increment the requestedFloorVariable.
 *           It waits for 1 second before moving the elevator to simulate the 
 *           time it takes for passengers to get on or off.
 *
 * Note:
 * ****************************************************************************/
void handleInputs(void)
{
    //If the Up button is pressed and the Down button is not pressed
        if (UP_BUTTON == 0 && DOWN_BUTTON == 1)
        {
            //If the elevator is currently at either floor 1 or 2
            if (currentFloorLevel < 3)
            {
                currentFloorLevel++; //Then increment the floor level
                delay (1000);
                elevatorUp(motorSetpoint);

                delay (400);
                buzzer(700); //Buzzer signals that the floor has arrived
            }
        }
        //If the Up button is not pressed and the Down button is pressed
            else if (UP_BUTTON == 1 && DOWN_BUTTON == 0)
            {
                //If the elevator is currently at either floor 2 or 3
                    if (currentFloorLevel > 1)
                    {
                        currentFloorLevel--; //Then decrement the floor level
                        delay (1000);
                        elevatorDown(motorSetpoint);

                        delay (400);
                        buzzer(700); //Buzzer signals that the floor has arrived
                    }
            }
}

/*******************************************************************************
 * Function: segmentDisplay
 *
 * Precondition: Floor must be reached
 * Input: Whether a particular segment should be on or off
 * Output: none
 * Side Effects: none
 *
 * Overview: It acts like a register for each segment of the display.
 *           Able to turn on or off any segments.
 *
 * Note: 0 is to turn on, 1 is turn off, because segments require a negative
 *       signal to turn on.
 ******************************************************************************/
void segmentDisplay (int a, int b, int c, int d, int e, int f, int g)
{
    SEG_A = a;
    SEG_B = b;
    SEG_C = c;
    SEG_D = d;
    SEG_E = e;
    SEG_F = f;
    SEG_G = g;
}

/*******************************************************************************
 * Function:    updateIndicators
 *
 * PreCondition: Floor must be reached
 * Input:   none
 * Output:  none
 * Side Effects: none

 * Overview: Displays the appropriate floor level on the segment display and 
             turns on the appropriate indicator LED.
 *
 * Note:
 * ****************************************************************************/
void updateIndicators(void)
{
//If first floor
    if (currentFloorLevel == 1)
    {
        FIRST_FLOOR_LED = 1;
        SECOND_FLOOR_LED = 0;
        THIRD_FLOOR_LED = 0;
        segmentDisplay(1, 0, 0, 1, 1, 1, 1); //Display a '1'
    }

    //If second floor
        else if (currentFloorLevel == 2)
        {
            FIRST_FLOOR_LED = 0;
            SECOND_FLOOR_LED = 1;
            THIRD_FLOOR_LED = 0;
            segmentDisplay(0, 0, 1, 0, 0, 1, 0); //Display a '2'
        }

    //If third floor
        else if (currentFloorLevel == 3)
        {
            FIRST_FLOOR_LED = 0;
            SECOND_FLOOR_LED = 0;
            THIRD_FLOOR_LED = 1;
            segmentDisplay(0, 0, 0, 0, 1, 1, 0); //Display a '3'
        }
}

/*******************************************************************************
 * Function:    elevatorUp
 *
 * PreCondition: Up button must be pressed and floor level must be less than 3
 * Input: The number of 4-step cycles to complete
 * Output: none
 * Side Effects: none
 *
 * Overview: Turns the stepper motor for the desired number of cycles
 *
 * Note:
 * ****************************************************************************/
void elevatorUp (int cycles)
{
    for (counter = 0; counter < cycles; counter++)
    {

        BLACK = 0;
        YELLOW = 0;
        BROWN = 0;
        ORANGE = 1;
        delay(MOTOR_DELAY);
        motorPosition++; //Increment the motor position for each step travelled

        BLACK = 0;
        YELLOW = 0;
        BROWN = 1;
        ORANGE = 0;
        delay(MOTOR_DELAY);
        motorPosition++;

        BLACK = 0;
        YELLOW = 1;
        BROWN = 0;
        ORANGE = 0;
        delay(MOTOR_DELAY);
        motorPosition++;

        BLACK = 1;
        YELLOW = 0;
        BROWN = 0;
        ORANGE = 0;
        delay(MOTOR_DELAY);
        motorPosition++;
    }
}

/*******************************************************************************
 * Function:    elevatorDown
 *
 * PreCondition: Down button must be pressed and floor level must be greater
 *               than 1
 * Input:   The number of 4 step cycles to complete
 * Output:  none
 * Side Effects: none
 *
 * Overview: Each loop is 4 steps so the input is the number of cycles the motor
 *           needs to move.
 * Note:
 * ****************************************************************************/
void elevatorDown (int cycles)
{
    for (counter = 0; counter < cycles; counter++)
    {
        BLACK = 1;
        YELLOW = 0;
        BROWN = 0;
        ORANGE = 0;
        delay(MOTOR_DELAY);
        motorPosition--; //Decrement the motor position for each step travelled

        BLACK = 0;
        YELLOW = 1;
        BROWN = 0;
        ORANGE = 0;
        delay(MOTOR_DELAY);
        motorPosition--;

        BLACK = 0;
        YELLOW = 0;
        BROWN = 1;
        ORANGE = 0;
        delay(MOTOR_DELAY);
        motorPosition--;

        BLACK = 0;
        YELLOW = 0;
        BROWN = 0;
        ORANGE = 1;
        delay(MOTOR_DELAY);
        motorPosition--;
    }
}

/*******************************************************************************
 * Function:   initializeInterrupt
 *
 * PreCondition: none
 * Input:   none
 * Output:  none
 * Side Effects: none
 *
 * Overview: Configures RB2 on the PIC to be used as an interrupt
 *
 * Note:
 * ****************************************************************************/
void initializeInterrupt1 (void)//Initializing Interrupt 1
{
    /* Initialize Input Interrupt Switch (AN4/RP2/RB2) for use as a fire alarm
     * switch */

_INT1R = 2; //Assign INT1 input function to RP2 (RB2)
	
_INT1EP = 1; /* Interrupt INT1 on negative edge (when button is pressed,
              * it returns a '0') */
	
_INT1IF = 0; //Clear INT1 flag
	
_INT1IE = 1; //Enable INT1 interrupt

}//end initializeInterrupt1

/*******************************************************************************
 * Function:    interrupt
 *
 * PreCondition: Interrupt button (Fire alarm) must be pressed
 * Input:   none
 * Output:  none
 * Side Effects:
 *
 * Overview: Flashes the Fire alarm LED and the letter 'F' three times. Then the
 *	     elevator is moved down to the ground floor. PIC is then reset.
 *
 * Note:
 * ****************************************************************************/
void __attribute__((interrupt,no_auto_psv)) _INT1Interrupt (void) //ISR
{
    //Clear INT1 flag after an interrupt has been initiated
    //which resets the entire process
    _INT1IF = 0;

    //Seven segment display is cleared and the indicator LEDs are turned off
        FIRST_FLOOR_LED = 0;
        SECOND_FLOOR_LED = 0;
        THIRD_FLOOR_LED = 0;
        segmentDisplay(1, 1, 1, 1, 1, 1, 1);

    delay (300);
    
   /* Flashes the letter 'F' on the segment display and the fire alarm indicator
    * LED in sync three times. */
       for (counter = 0; counter < 3; counter++)
       {
            FIRE_ALARM_LED  = 0;
            segmentDisplay(1, 1, 1, 1, 1, 1, 1);
            delay (500);

            FIRE_ALARM_LED = 1;
            segmentDisplay(0, 1, 1, 1, 0, 0, 0);
            delay (500);
        }

    fireAlarm(); //Buzzer is sounded

    motorSetpoint = motorPosition/4; /* The number of steps down to the ground
                                      * floor is the current motor position
				      * It is divided by four to get the number
                                      * of cycles */
    
    elevatorDown(motorSetpoint); //Elevator is sent to ground floor

    asm("RESET"); /* Resets the PIC so that it doesn't go back to what it was
                   * doing before after interrupt is over */
}//end _INT1Interrupt

/*******************************************************************************
 * Function:    buzzer
 *
 * PreCondition: Floor must be reached or fire alarm button must be pressed for 
 *		 buzzer to activate
 * Input:   End condition of for loop, controls how long buzzer runs
 * Output:  none
 * Side Effects: none
 *
 * Overview: Pulses a signal to the piezo buzzer to turn it on
 *
 * Note: Length inputted is just a rough indication of the time that the buzzer
 *       will run, not directly a time unit.
 * ****************************************************************************/
void buzzer (int length)
{
    for (counter = 0; counter < length; counter++)
    {
        BUZZER = 1;
        delay (0.3);

        BUZZER = 0;
        delay (0.3);
    }
}

/*******************************************************************************
 * Function:    fireAlarm
 *
 * PreCondition: fire alarm button must be pressed for buzzer to activate
 * Input:   none
 * Output:  none
 * Side Effects: none
 *
 * Overview: Pulses a signal to the piezo buzzer to turn it on. The delay starts
 *           out at 0.6 and is decremented so that the pitch increases steadily.
 *           This sequence is repeated four times.
 *
 * Note: Decrementing the delay increases pitch because the time for each pulse
 *       is decreased so the piezoelectric material vibrates faster.
 *       Vice-versa is also true. 
 * ****************************************************************************/
void fireAlarm (void)
{
    //Distinct fire alarm sound is repreated 4 times
        for (buzzerCounter = 0; buzzerCounter < 4; buzzerCounter++)
        {
            buzzerDelay = 0.6; //Sound starts out low pitched

            for (counter = 0; counter < 800; counter++)
            {
                BUZZER = 1;
                delay (buzzerDelay);

                BUZZER = 0;
                delay (buzzerDelay);

                buzzerDelay -= 0.0005;
            }
        }
}





