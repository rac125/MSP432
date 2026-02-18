//******************************************************************************
// Main Comment Header:
// Jacob Bachle and Rafael Cano
// Current Semester: Spring 2026
// Lab Section: 1
// Date Created: 2/17/26
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

void configure432IO(void);
void configureRobotIO(void);
void readBumperSwitches(void);
void serivceLeftMotorLED(void);
void serviceRightMotorLED(void);
void serviceLeftMotor(void);
void serviceRightMotor(void);
void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel);

// Global Defines
#define PERIOD 2000 //Determined in lab 4 to be PERIOD in which motor would appear continuous
#define DUTY 50
#define CLOCKDIVIDER TIMER_A_CLOCKSOURCE_DIVIDER_48
#define LEFTCHANNEL  TIMER_A_CAPTURECOMPARE_REGISTER_4
#define RIGHTCHANNEL TIMER_A_CAPTURECOMPARE_REGISTER_3

// Typedefs to store states
typedef enum {motorOFF,motorForward,motorReverse,motorTest} motorStates;

//Default states
motorStates leftMotorState, rightMotorState;

Timer_A_PWMConfig timerPWMConfig;

// Timer Base
const uint32_t TIMER = TIMER_A0_BASE;

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
    configPWMTimer(PERIOD, CLOCKDIVIDER, DUTY, LEFTCHANNEL);
    configPWMTimer(PERIOD, CLOCKDIVIDER, DUTY, RIGHTCHANNEL);

    Timer_A_startCounter(TIMER, TIMER_A_UP_MODE);

    while(1) {
        //Update States
        readBumperSwitches();

        //toggleRSLKLEDTest();
        //Delay for debounce
        __delay_cycles(1500000); //Software delay for simplicity, could configure timer with seperate base
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


}

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
    if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN7) == GPIO_INPUT_PIN_LOW) {
        leftMotorState = motorForward;
    }

    // BMP4 (P4.6) -> Left Reverse
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN6) == GPIO_INPUT_PIN_LOW) {
        leftMotorState = motorReverse;
    }

    // BMP3 (P4.5) -> Right Forward
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN5) == GPIO_INPUT_PIN_LOW) {
        rightMotorState = motorForward;
    }

    // BMP2 (P4.3) -> BOTH Forward
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN3) == GPIO_INPUT_PIN_LOW) {
        leftMotorState  = motorForward;
        rightMotorState = motorForward;
    }

    // BMP1 (P4.2) -> BOTH Reverse
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2) == GPIO_INPUT_PIN_LOW) {
        leftMotorState  = motorReverse;
        rightMotorState = motorReverse;
    }

    // BMP0 (P4.0) -> Right Reverse
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0) == GPIO_INPUT_PIN_LOW) {
        rightMotorState = motorReverse;
    }

    // Update motor states
    serviceLeftMotor();
    serviceRightMotor();

    // Update LEDs based on new states
    serivceLeftMotorLED();
    serviceRightMotorLED();
}

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

void serivceLeftMotorLED(void) {
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

void toggleAllLEDSONTest(void) {
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
}

void toggleAllLEDSOFFTest(void) {
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
}

void toggleRSLKLEDTest(void) {
    toggleAllLEDSONTest();
    __delay_cycles(9000000);
    toggleAllLEDSOFFTest();
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
