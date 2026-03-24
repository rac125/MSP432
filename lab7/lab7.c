//******************************************************************************
// Main Comment Header:
// Jacob Bachle and Rafael Cano
// Current Semester: Spring 2026
// Lab Section: 1
// Date Created: 3/3/26
// Description:
// Bumper switches control left and right wheel motion using PWM.
// LEDs display status per certification sheet.
//******************************************************************************

// DriverLib Includes
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// Standard Includes
#include <stdint.h>
#include <stdbool.h>

// Function Prototypes
void toggleAllLEDSONTest(void);
void toggleAllLEDSOFFTest(void);

void toggleRSLKLEDTest(void);

void bumperSwitchesHandler(void);

void configure432IO(void);
void configureRobotIO(void);
//void readBumperSwitches(void);
void serviceLeftMotorLED(void);
void serviceRightMotorLED(void);
void serviceLeftMotor(void);
void serviceRightMotor(void);
void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel);

// Functions added in lab 7
void LEDStatesControl(void);
void serviceLEDs(void);
void readLineSensors(void);
void processSensorData(void);


// Global Defines
#define PERIOD 2000 //Determined in lab 4 to be PERIOD in which motor would appear continuous
#define DUTY 50
#define CLOCKDIVIDER TIMER_A_CLOCKSOURCE_DIVIDER_48
#define LEFTCHANNEL  TIMER_A_CAPTURECOMPARE_REGISTER_4
#define RIGHTCHANNEL TIMER_A_CAPTURECOMPARE_REGISTER_3

// Typedefs to store states
typedef enum {motorOFF,motorForward,motorReverse,motorTest} motorStates;
typedef enum {bumperON, bumperOFF} bumperStates;
typedef enum {RED, YELLOW, GREEN, CYAN, BLUE, WHITE} LED2state;
typedef enum {on, off} LED1state;
typedef enum {PowerUp, StandBy, Tracking, Lost} mode;

//Current states
motorStates leftMotorState = motorOFF;
motorStates rightMotorState = motorOFF;
bumperStates BMP0 = bumperOFF;
bumperStates BMPn = bumperOFF;
LED1state LED1State = off;
LED2state LED2State;
mode currentMode = PowerUp;

Timer_A_PWMConfig timerPWMConfig;

// Timer Base
const uint32_t TIMER = TIMER_A0_BASE;

// Global vars
volatile uint8_t sensorValues = 0;
volatile uint8_t leftCount = 0;
volatile uint8_t rightCount = 0;
volatile uint8_t totalCount = 0;

//******************************************************************************
// Name of Function: main
// Description: Initializes hardware and continuously polls bumper switches
// to control wheel direction and LED indicators.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
int main(void) {
    MAP_WDT_A_holdTimer();

    configure432IO();
    configureRobotIO();

    // Configure PWM for both wheels
    //configPWMTimer(PERIOD, CLOCKDIVIDER, DUTY, LEFTCHANNEL);
    //configPWMTimer(PERIOD, CLOCKDIVIDER, DUTY, RIGHTCHANNEL);

    //serviceLeftMotor();
    //serviceRightMotor();

    toggleRSLKLEDTest();

    Timer_A_startCounter(TIMER, TIMER_A_UP_MODE);

    __enable_interrupts();

    while(1) {
        readLineSensors();
        processSensorData();
        LEDStatesControl();
        serviceLEDs();
        __delay_cycles(1500000); //Delay 500 ms
    }
}

//******************************************************************************
// Name of Function: configure432IO
// Description: Configures LaunchPad LED1 and RGB LED2 as outputs
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void configure432IO(void) {
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
}

//******************************************************************************
// Name of Function: configureRobotIO
// Description: Configures bumper switches, motor pins, and PWM outputs
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void configureRobotIO(void) {
    // Configure IR LED control pins as outputs
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN3);  // Even LEDs
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3); // Start OFF

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P9, GPIO_PIN2);  // Odd LEDs
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P9, GPIO_PIN2); // Start OFF

    // Bumper switches config
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN7 | GPIO_PIN6 | GPIO_PIN5 | GPIO_PIN3 | GPIO_PIN2 | GPIO_PIN0);

    // Direction pins
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN5 | GPIO_PIN4 | GPIO_PIN2 | GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5 | GPIO_PIN4 | GPIO_PIN2 | GPIO_PIN0); //Default low

    // Sleep/Enable pins
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6); //Default low

    // PWM outputs
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);

    // RSLK LED config
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7); //Default off

    //enable the port I/O interrupt for the switch
    GPIO_enableInterrupt(GPIO_PORT_P4,  GPIO_PIN7 | GPIO_PIN6 | GPIO_PIN5 | GPIO_PIN3 | GPIO_PIN2 | GPIO_PIN0);
    GPIO_interruptEdgeSelect(GPIO_PORT_P4,  GPIO_PIN7 | GPIO_PIN6 | GPIO_PIN5 | GPIO_PIN3 | GPIO_PIN2 | GPIO_PIN0, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_clearInterruptFlag(GPIO_PORT_P4,  GPIO_PIN7 | GPIO_PIN6 | GPIO_PIN5 | GPIO_PIN3 | GPIO_PIN2 | GPIO_PIN0);
    MAP_GPIO_registerInterrupt(GPIO_PORT_P4, bumperSwitchesHandler);

}

/*
//******************************************************************************
// Name of Function: readBumperSwitches
// Description: Reads bumper switches and updates wheel states and LEDs
// according to certification sheet.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void readBumperSwitches(void) {
    // Default state (no bumpers pressed)
    leftMotorState  = motorOFF;
    rightMotorState = motorOFF;

    // BMP5  (P4.7) -> Left Forward
    if(BMP5 == bumperON) {
        leftMotorState = motorForward;
    }

    // BMP4 (P4.6) -> Left Reverse
    else if(BMP4 == bumperON) {
        leftMotorState = motorReverse;
    }

    // BMP3 (P4.5) -> Right Forward
    else if(BMP3 == bumperON) {
        rightMotorState = motorForward;
    }

    // BMP2 (P4.3) -> BOTH Forward
    else if(BMP2 == bumperON) {
        leftMotorState  = motorForward;
        rightMotorState = motorForward;
    }

    // BMP1 (P4.2) -> BOTH Reverse
    else if(BMP1 == bumperON) {
        leftMotorState  = motorReverse;
        rightMotorState = motorReverse;
    }

    // BMP0 (P4.0) -> Right Reverse
    else if(BMP0 == bumperON) {
        rightMotorState = motorReverse;
    }

    // Update motor states
    serviceLeftMotor();
    serviceRightMotor();

    // Update LEDs based on new states
    serviceLeftMotorLED();
    serviceRightMotorLED();
}
*/

//******************************************************************************
// Name of Function: serviceRightMotor
// Description: sets right motor configuration bits based on current right motor
// state.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void serviceRightMotor(void) {
    switch(rightMotorState) {
        case motorOFF:
            // Disable motor
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN6);

            // Direction pins low
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0 | GPIO_PIN2);
            break;

        case motorForward:
            // IN1 = 1, IN2 = 0
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);

            // set direction
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5);

            // Enable motor
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
            break;

        case motorReverse:
            // IN1 = 0, IN2 = 1
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);

            // set direction
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN5);

            // Enable motor
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
            break;

        case motorTest:
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0 | GPIO_PIN2);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
            break;
    }
}

//******************************************************************************
// Name of Function: serviceLeftMotor
// Description: sets left motor configuration bits based on current left motor
// state.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void serviceLeftMotor(void) {
    switch(leftMotorState) {
        case motorOFF:
            // Disable motor
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7);

            // Direction pins low
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4 | GPIO_PIN5);
            break;

        case motorForward:
            // IN1 = 1, IN2 = 0
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);

            // set direction
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4);

            // Enable motor
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
            break;

        case motorReverse:
            // IN1 = 0, IN2 = 1
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);

            // set direction
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN4);

            // Enable motor
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
            break;

        case motorTest:
            // Both direction high (optional test mode)
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN4 | GPIO_PIN5);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
            break;
    }
}

//******************************************************************************
// Name of Function: serviceLeftMotorLED
// Description: Sets RSLK LEDs high or low based on current motor states.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void serviceLeftMotorLED(void) {
    switch(leftMotorState) {
    case motorOFF:
        // RSLK LED Front and Rear OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0); // Front Left OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN6); // Rear Left OFF
        break;
    case motorForward:
        // RSLK LED Front ON Rear OFF
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0); // Front Left ON
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN6);  // Rear Left OFF
        break;
    case motorReverse:
        // RSLK LED Rear ON Front OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0);  // Front Left OFF
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN6); // Rear Left ON
        break;
    case motorTest:
        //RSLK LED Front and Rear ON
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0);
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN6);
        break;
    }
}

//******************************************************************************
// Name of Function: serviceRightMotorLED
// Description: Sets RSLK LEDs high or low based on current motor states.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void serviceRightMotorLED(void) {
    switch(rightMotorState) {
    case motorOFF:
        // RSLK LED Front and Rear OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN5); // Front Left OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN7); // Rear Left OFF
        break;
    case motorForward:
        // RSLK LED Front ON Rear OFF
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN5); // Front Left ON
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN7);  // Rear Left OFF
        break;
    case motorReverse:
        // RSLK LED Rear ON Front OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN5);  // Front Left OFF
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN7); // Rear Left ON
        break;
    case motorTest:
        //RSLK LED Front and Rear ON
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN5);
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN7);
        break;
    }
}

//******************************************************************************
// Name of Function: toggleAllLEDSONTest
// Description: Turns all LEDS on. Used as a helper function in test mode.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void toggleAllLEDSONTest(void) {
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
}

//******************************************************************************
// Name of Function: toggleAllLEDSOFFTest
// Description: Turns all LEDS off. Used as a helper function in test mode.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void toggleAllLEDSOFFTest(void) {
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
}

//******************************************************************************
// Name of Function: toggleRSLKLEDTest
// Description: Turns all LEDS of, after a 3 second delay, turns all LEDs off.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void toggleRSLKLEDTest(void) {
    toggleAllLEDSONTest();
    __delay_cycles(9000000); //aprox. 3 seconds
    toggleAllLEDSOFFTest();
}

//******************************************************************************
// Name of Function: bumperSwitchesHandler
// Description: Updates bumper states.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void bumperSwitchesHandler(void) {
    uint16_t status;
    _delay_cycles(30000);
    status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P4);
    switch(status){
        case GPIO_PIN6: //BPM5
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
            break;
        case GPIO_PIN5: //BMP4
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN5);
            break;
        case GPIO_PIN4: //BPM3
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN4);
            break;
        case GPIO_PIN3: //BMP2
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN3);
            break;
        case GPIO_PIN2: //BMP1
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN2);
            break;
        case GPIO_PIN0: // BMP0
            if (BMP0 == bumperOFF) {
                BMP0 = bumperON;
            }
            else {
                BMP0 = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN0);
            break;
        //End of Case
    }

    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4, status);

    //readBumperSwitches();
}

void processSensorData(void) {
    leftCount = (sensorValues & BIT0) + (sensorValues & BIT1) + (sensorValues & BIT2) + (sensorValues & BIT3);
    rightCount = (sensorValues & BIT4) + (sensorValues & BIT5) + (sensorValues & BIT6) + (sensorValues & BIT7);
}

void LEDStatesControl(void) {
    totalCount = leftCount + rightCount;

    if (totalCount == 0) {
        LED2State = WHITE;
    }
    else if ((leftCount > 0) & (rightCount == 0)) {
        LED2State = RED;
    }
    else if ((rightCount > 0) & (leftCount == 0)) {
        LED2State = BLUE;
    }
    else if (leftCount > rightCount) {
        LED2State = YELLOW;
    }
    else if (rightCount > leftCount) {
        LED2State = CYAN;
    }
    else {
        LED2State = GREEN;
    }

    /*
    // LED1 mode control
    switch(currentMode) {
        case PowerUp:
            // Toggle LED1 once
            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
            __delay_cycles(1500000); //Delay 500 ms
            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);

            // Toggle LED2 white once
            GPIO_toggleOutputOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
            __delay_cycles(1500000); //Delay 500 ms
            GPIO_toggleOutputOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
            break;
        case StandBy:
            if (totalCount > 0) {
                //Toggle LED1 at 1 Hz
                GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
                __delay_cycles(1500000); //Delay 500 ms
            }
            else {
                LED1State = off;
            }
            break;
        case Tracking:
            LED1State = on;
            break;
        case Lost:
            LED1State = off;
            break;
    }
    */
}

void serviceLEDs(void) {
    toggleAllLEDSOFFTest(); //Sets all LED pins to low
    switch(LED1State) {
    case on:
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        break;
    case off:
        //Turn off LED1, already off
        break;
    }
    switch(LED2State) {
    case WHITE:
        //Turn on white
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
        break;
    case RED:
        //Turn on red
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
        break;
    case BLUE:
        //Turn on blue
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
        break;
    case YELLOW:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1);
        break;
    case CYAN:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN2);
        break;
    case GREEN:
        //turn on green
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
        break;
    }
}

//******************************************************************************
// Name of Function: readLineSensors
// Description: Configures Timer_A PWM channel for selected wheel
// Input Parameters: clockPeriod, clockDivider, duty cycle %, channel
// Return: none
// Author: Rafael Cano
//******************************************************************************
void readLineSensors(void) {
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P9, GPIO_PIN2);

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    __delay_cycles(30);
    MAP_GPIO_setAsInputPin(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    __delay_cycles(4500);
    sensorValues = MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    //sensorValues = P7->IN;
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P9, GPIO_PIN2);

}

//******************************************************************************
// Name of Function: configPWMTimer
// Description: Configures Timer_A PWM channel for selected wheel
// Input Parameters: clockPeriod, clockDivider, duty cycle %, channel
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel) {
    uint16_t dutyCycle = duty * clockPeriod / 100;
    timerPWMConfig.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timerPWMConfig.clockSourceDivider = clockDivider;
    timerPWMConfig.timerPeriod = clockPeriod;
    timerPWMConfig.compareOutputMode =
    TIMER_A_OUTPUTMODE_TOGGLE_SET;
    timerPWMConfig.compareRegister = channel;
    timerPWMConfig.dutyCycle = dutyCycle;

    MAP_Timer_A_generatePWM(TIMER, &timerPWMConfig);
    MAP_Timer_A_stopTimer(TIMER);
}
void powerUpLEDSON(void) {
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0, GPIO_PIN1, GPIO_PIN2);
    
}

//******************************************************************************
// Name of Function: toggleAllLEDSOFFTest
// Description: Turns all LEDS off. Used as a helper function in test mode.
// Input Parameters: void
// Return: none
// Author: Rafael Cano
//******************************************************************************
void powerUpLEDSOFF(void) {
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0, GPIO_PIN1, GPIO_PIN2);
}

//******************************************************************************
// Name of Function: toggleRSLKLEDTest
// Description: Turns all LEDS of, after a 3 second delay, turns all LEDs off.
// Input Parameters: void
// Return: none
// Author: Rafael Cano
//******************************************************************************
void toggleRSLKLEDTest(void) {
    powerUpLEDSON();
    __delay_cycles(9000000); //aprox. 3 seconds
    powerUpLEDSOFF();
}
