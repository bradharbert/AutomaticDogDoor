//
// Automated Doggie Door
// Brad Harbert - harbert.brad@gmail.com
// www.harbert.io
// 10/01/2018
// Version 4.0
//


// PIN Variables
int pirInside = D2;                        // Digital pin D0 for PIR SENSOR (INSIDE)
int pirOutside = D3;                       // Digital pin D2 for PIR SENSOR (OUTSIDE)
int relayUp = A0;                          // Analog pin A0 for RELAY (UP)
int relayDown = A2;                        // Analog pin A2 for RELAY (DOWN)
int buttonUp = D4;                         // Digital pin D4 for UP BUTTON
int buttonDown = D6;                       // Digital pin D6 for DOWN BUTTON
int buttonMode = D7;                       // Digital pin D7 for MODE BUTTON
int ledAuto = A4;                          // Analog pin A4 for Automatic Mode LED
int ledManual = A5;                        // Analog pin A5 for Manual Mode LED

//State Variables
int statePirInside = LOW;                   // Initial State of PIR SENSOR (INSIDE)
int statePirOutside = LOW;                  // Initial State of PIR SENSOR (OUTSIDE)
int stateButtonMode = LOW;                  // Initial State of Automatic/Manual Button
int stateButtonUp = LOW;                    // Initial State of UP BUTTON
int stateButtonDown = LOW;                  // Initial State of DOWN BUTTON

char valuePirInside;
char valuePirOutside;

String modeState = "manual";                // Current Mode - Begin in Manual Mode (manual || automatic)
String machineState = "";                   // Particle Variable to reflect current state of Machine
int initialLedMode = LOW;                   // Setting an inital mode to turn on the LED at the start only

// Timing Variables
int actuatorTime = 12000;                   // Set Time (milliseconds) for Actuator to Complete Up or Down Stroke (12 seconds)
int holdOpenTime = 7000;                    // Set Time (milliseconds) to leave door Open at Top (7 seconds)
int stateReportTime = 1000;                 // Set Time (milliseconds) to send out a debug message

// State Machine Variables
int stateTime = 10;                         // Time between successive calls to a state function (10 milliseconds)
int elapsedStateTime = 0;                   // Number of milliseconds spent in the current state
int stateReportTimer = 0;                   // Number of milliseconds spent in the current state

// NOTE: We will start in the START LOWERING DOOR state.  This will end us up in the DOOR CLOSED state once
// it closes. If it is already closed, we will be driving it closed for 12 seconds, so the state machine
// will take 12 seconds before it is in the DOOR CLOSED state.
int currentState = 4;                     // This is the current state of the state machine

// STATE MACHINE:
//
// A State Machine will be implemented to control the operation of the door.  
// This will eliminate the need for 'delay' statements and will allow the door to 
// reverse part way through its opening or closing operations
//
// The states of the state machine are as follows (the number is the identifier of the state)
//
// 0 - DOOR CLOSED
// 1 - START RAISING DOOR
// 2 - DOOR RAISING
// 3 - DOOR OPEN
// 4 - START LOWERING DOOR
// 5 - DOOR LOWERING
//
// STATE DETAILS:
//
// 0 - DOOR CLOSED
// This will be the initial state of the state machine when the system starts.  This
// state will look for the UP BUTTON to be pressed
//
// 1 - START RAISING DOOR
// This state starts the door raising, then goes on to the RAISING DOOR state
//
// 2 - DOOR RAISING
// When in this state, the door will be raising.  This state will also look for a 
// DOWN BUTTON so it can branch to the START LOWERING DOOR state
//
// 3 - DOOR OPEN
// This state holds the door open for 7 seconds (holdOpenTime).  It also checks for 
// an early press of the down button in case the user doesn not want to wait
// 
// 4 - START LOWERING DOOR
// This state starts the door lowering, then goes on to the LOWERING DOOR state
//
// 5 - DOOR LOWERING
// When in this state, the door will be lowering.  This state will look for
// an UP BUTTON, as well as the proximity sensors.  If any of those is detected, 
// it will go to the RAISING DOOR state again.
//
 
void setup() {
  
  pinMode(pirInside, INPUT_PULLDOWN);                        // Declare PIR SENSOR (INSIDE) as input
  pinMode(pirOutside, INPUT_PULLDOWN);                       // Declare PIR SENSOR (OUTSIDE) as input
  
  pinMode(ledAuto, OUTPUT);                         // Declare Automatic Mode LED as output
  pinMode(ledManual, OUTPUT);                       // Declare Manual Mode LED as output
  
  pinMode(relayUp, OUTPUT);                         // Declare RELAY (UP) as output
  pinMode(relayDown, OUTPUT);                       // Declare RELAY (DOWN) as output

  pinMode(buttonMode, INPUT);                       // Declare MODE BUTTON as Input
  pinMode(buttonUp, INPUT);                         // Declare UP BUTTON as Input
  pinMode(buttonDown, INPUT);                       // Declare DOWN BUTTON as Input
  
  Particle.variable("machineState", machineState);  // Particle Variable for Current State of Machine
  Particle.variable("modeState", modeState);        // Particle Variable for Current Mode of Machine
  
  //Particle.variable("valuePirInside", valuePirInside);
  //Particle.variable("valuePirOutside", valuePirOutside);
  
  Particle.function("CurrentMode", fxCurrentMode);  // Particle Function to Change Mode from Internet
  Particle.function("OpenClose", fxOpenClose);      // Particle Function to Open or Close the door

}

void loop(){
    
// Begin by Reading Values from Sensors
  statePirInside = digitalRead(pirInside);              // Read value from PIR SENSOR (INSIDE)
  statePirOutside = digitalRead(pirOutside);            // Read value from PIR SENSOR (OUTSIDE)
  stateButtonUp = digitalRead(buttonUp);                // Read value from UP BUTTON
  stateButtonDown = digitalRead(buttonDown);            // Read value from DOWN BUTTON
  stateButtonMode = digitalRead(buttonMode);            // Read value from MODE BUTTON

// Turn on the Manual LED at Power On Only
  if (initialLedMode == LOW) {
    digitalWrite(ledManual, HIGH);
    digitalWrite(ledAuto, LOW);
    initialLedMode = HIGH;
  }
  
//   Check for AUTOMATIC || MANUAL Button Press
  if (stateButtonMode == HIGH) {
    delay(250);
    if (modeState == "manual") {
     modeState = "automatic";
     Particle.publish("modeState", "Automatic");
     digitalWrite(ledAuto, HIGH);
     digitalWrite(ledManual, LOW); 
    } else {
      modeState = "manual";
      Particle.publish("modeState", "Manual");
      digitalWrite(ledAuto, LOW);
      digitalWrite(ledManual, HIGH);
    }
  }
  

// Execute the current state 
  switch(currentState) {
    case 0:                                             // 0 - DOOR CLOSED
      stateDoorClosed();
      break;
    case 1:                                             // 1 - START RAISING DOOR
      stateStartRaisingDoor();
      break;
    case 2:                                             // 2 - DOOR RAISING
      stateDoorRaising();
      break;
    case 3:                                             // 3 - DOOR OPEN
      stateDoorOpen();;
      break;
    case 4:                                             // 4 - START LOWERING DOOR
      stateStartLoweringDoor();
      break;
    case 5:                                             // 5 - DOOR LOWERING
      stateDoorLowering();
      break;
    default:                                            // Invalid state, lower the door to get us back to DOOR CLOSED
      branchToState(4);
      break;
  }
  
  delay(stateTime);                                     // Delay for the state delay time

  elapsedStateTime += stateTime;                        // Increment the time in the state
  stateReportTimer += stateTime;

  if(stateReportTimer > stateReportTime) {              // See if it is time to report the state

    stateReportTimer = 0;                               // Reset timer
  }


}

////////////////////////////////////////////////////////////
//////////////////////// STATES ////////////////////////////
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////
//////////////// 0 - DOOR CLOSED ///////////////////
////////////////////////////////////////////////////
void stateDoorClosed(void)
{
    
    if (machineState != "Door Closed") {                                // Publish Event only if it is different from previous State
      
        Particle.publish("machineState", "Door Closed");
        machineState = "Door Closed";
        
    }
  
  // MANUAL MODE /////////////////////////

  if (modeState == "manual") {

    if (stateButtonUp == HIGH) {                                        // If the UP button is pressed, start the door raising
      branchToState(1);
    }
  }
  // AUTOMATIC MODE //////////////////////
  else if (modeState == "automatic") {

    if ((statePirInside == HIGH) || (statePirOutside == HIGH)) {       // If either PIR is high, start the open sequence
      branchToState(1);
    }
  }
}

////////////////////////////////////////////////////
//////////// 1 - START RAISING DOOR ////////////////
////////////////////////////////////////////////////
void stateStartRaisingDoor(void)  {
    
    //Particle.publish("valuePirInside", String(statePirInside));                     // DEBUGGING to determine PIR Values
    //Particle.publish("valuePirOutside", String(statePirOutside));
    
    if (machineState != "Start Door Raising") {                             // Publish Event only if it is different from previous State
      
        // Particle.publish("machineState", "Start Door Raising");
        machineState = "Start Door Raising";
        
    }
  
  digitalWrite(relayDown, LOW);                                             // Start the door raising
  digitalWrite(relayUp, HIGH);

  branchToState(2);                                                         // Go to the 2 - DOOR RAISING state
}

////////////////////////////////////////////////////
//////////// 2 - DOOR RAISING //////////////////////
////////////////////////////////////////////////////
void stateDoorRaising(void) {
    
    if (machineState != "Door Raising") {                             // Publish Event only if it is different from previous State
      
        Particle.publish("machineState", "Door Raising");
        machineState = "Door Raising";
        
    }

  if (elapsedStateTime >= actuatorTime) {                                 // If we have waited long enough for the door to open

    digitalWrite(relayUp, LOW);                                         // Stop the door from raising
    branchToState(3);                                                   // Go to the 3 - DOOR OPEN State
  }
  
  // Manual Mode
  if (stateButtonDown == HIGH) {                                     // If the DOWN button is pressed, start the door lowering again
    branchToState(4);                                                   // Go to the 4 - START LOWERING DOOR State
  }
}

////////////////////////////////////////////////////
//////////// 3 - DOOR OPEN /////////////////////////
////////////////////////////////////////////////////
void stateDoorOpen(void) {
    
    if (machineState != "Door Open") {                             // Publish Event only if it is different from previous State
      
        Particle.publish("machineState", "Door Open");
        machineState = "Door Open";
        
    }

  // MANUAL MODE /////////////////////////
  if (modeState == "manual") {

    if (stateButtonDown == HIGH) {                                      // If the DOWN button is pressed...

      branchToState(4);                                               // Go to the 4 - START LOWERING DOOR State
    }
  }

  // AUTOMATIC MODE //////////////////////
  else if (modeState == "automatic") {

    if (elapsedStateTime >= holdOpenTime) {                             // Once the door has been held open long enough...

      branchToState(4);                                               // Go to the 4 - START LOWERING DOOR state
    }
  }
}

////////////////////////////////////////////////////
//////////// 4 - START LOWERING DOOR ///////////////
////////////////////////////////////////////////////
void stateStartLoweringDoor(void) {
    
    //Particle.publish("valuePirInside", String(statePirInside));                     // DEBUGGING to determine PIR Values
    //Particle.publish("valuePirOutside", String(statePirOutside));
    
    if (machineState != "Start Lowering Door") {                             // Publish Event only if it is different from previous State
      
        //Particle.publish("machineState", "Start Lowering Door");
        machineState = "Start Lowering Door";
        
    }
  
  digitalWrite(relayUp, LOW);                                               // Start the Door Closing
  digitalWrite(relayDown, HIGH);

  branchToState(5);                                                         // Go to the 5 - DOOR LOWERING State
}

////////////////////////////////////////////////////
//////////// 5 - DOOR LOWERING ////////////////////
////////////////////////////////////////////////////
void stateDoorLowering(void) {
    
    if (machineState != "Lowering Door") {                             // Publish Event only if it is different from previous State
      
        Particle.publish("machineState", "Lowering Door");
        machineState = "Lowering Door";
        
    }

  if (elapsedStateTime >= actuatorTime) {                                 // If we have waited long enough for the door to finish closing

    digitalWrite(relayUp, LOW);                                         // Stop the door from lowering
    digitalWrite(relayDown, LOW);

    branchToState(0);                                                   // Go to the 0 - DOOR CLOSED state
  }

  // MANUAL MODE /////////////////////////
  if (modeState == "manual") {

    if (stateButtonUp == HIGH) {                                        // If the UP button is pressed, start the door Opening
    
      branchToState(1);                                               // Go to the 1 - START RAISING DOOR State
    }

  }
  // AUTOMATIC MODE //////////////////////
  else if (modeState == "automatic") {

    if ((statePirInside == HIGH) || (statePirOutside == HIGH)) {        // If either PIR is high, we don't want to squish anybody, so start the open sequence
      
      branchToState(1);                                               // Go to the 1 - START RAISING DOOR State
    }
  }
}

////////////////////////////////////////////////////////////
//////////////////////// PARTICLE FUNCTIONS/////////////////
////////////////////////////////////////////////////////////

int fxCurrentMode(String command) {
        
        if (command.equalsIgnoreCase("automatic")) {
            modeState = "automatic";
            Particle.publish("modeState", "Automatic");
            digitalWrite(ledAuto, HIGH);
            digitalWrite(ledManual, LOW);
            return 1;
        }
        else if (command.equalsIgnoreCase("manual")) {
            modeState = "manual";
            Particle.publish("modeState", "Manual");
            digitalWrite(ledAuto, LOW);
            digitalWrite(ledManual, HIGH);
            return 1;
        }
        
        return -1;

}

int fxOpenClose(String command) {
    
        if (command.equalsIgnoreCase("open")) {
            branchToState(1);
            return 1;
        }
        else if (command.equalsIgnoreCase("close")) {
            branchToState(4);
            return 1;
        }
        
        return -1;

    
}

////////////////////////////////////////////////////////////
///////////////////// HELPER FUNCTIONS /////////////////////
////////////////////////////////////////////////////////////

void branchToState(int state) {
  
  currentState = state;                                   // Set the next state
  elapsedStateTime = 0;                                   // Reset the elapsed state time
}
