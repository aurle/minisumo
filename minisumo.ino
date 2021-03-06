/*******************************************************************************************
Example code to test the hardware from the Maggie Walker SumoBot kit 
Step 1) Make sure you have your board set to Teensy LC under tools->board
Step 2) Find "Define the ports" and change them to match the configuration of your robot.
Step 3) Find "Test Modes" and select one of the tests by removing the // in front of it
Step 4) Connect the robot to the computer with a micro USB cable
Step 5) Click the round circle with the -> in it at the top of the Arduino editor
Step 6) Press the white button on the robot if instructed to upload the program.
Step 7) Disconnect the robot from the USB cable
Step 8) Turn on the robot and see if the test works
Step 9) Repeat for other tests as needed
Step 10) Create your own new program, feel free to use anything from here you want.
********************************************************************************************/

#include <avr/io.h>

//#define DEBUG //uncomment this if you want to use serial monitor to view debug information

//Test Modes
//Select one of the following
//#define SIGNALS_TEST      //display signal values using the serial monitor
//#define DRIVE_TEST        //test motors by driving forward and backward repeatedly
//#define ROOMBA_TEST       //test Proximity by driving and then turning when objects are detected
//#define FIELD_TEST        //test surface circuit is working by staying in a field defined by color
//#define ROOMBA_FIELD_TEST //The roomba and field tests combined
//#define CUPS_TEST         //test everything by pushing objects out of a playing field.

//Control the wheel direction
const int kForward = 0; //Indicate motion should be in a forward direction
const int kReverse = 1; //Indicate motion should be in a reverse direction
const int kRight   = 0; //Indicate the robot should travel to the right
const int kLeft    = 1; //Indicate the robot should travel to the left

//These values were determined experimentally by reading the analog values with the
//robot on different surfaces and may be different for each robot and may even change
//if the sensor alignment changes. The SIGNALS_TEST can be used to determine the values
//if you need to use them in your code.
const int kWhiteTile     =  60;
const int kWhiteTileHigh = 100; //equals about 60
const int kGreenTileLow  = 300; //equals ? 375 320
const int kGreenTileHigh = 400;
const int kBlackTileLow  = 600; //equals ? 632 611
const int kBlackTileHigh = 650;
const int kPlayFieldLow  = 500;

//Other constants
const int kFieldRange   = 150; //Range used to determine if we are still in the field
const int kBorderDetect =  80; //High End of the value read when the Border is detected
const int kAttackSpeed  =  50; //Speed the robot moves when it attacks
const int kHalfSpeed    =  50;
const int kMaxSpeed     = 100;
const int kHuntSpeed    =  30; //Speed the robot moves when it is searching for an opponent
const int kStop         =   0; //Speed of zero will stop the robot

const int kmaxPWM = 254; //maximum value of the PWM signal used to control motor speed

#if defined(ROOMBA_TEST) || defined(ROOMBA_FIELD_TEST)
const int kProximityLevel = 500; //level used to detect an object before turning
#elif defined(CUPS_TEST)
const int kProximityLevel = 200; //level used to detect an object before attacking
#endif

//Define the ports
const int kLeftWheelDir      =  2; //DO Set left wheel direction
const int kLeftWheelSpeed    =  3; //PWM Pin for left motor speed
const int kRightWheelSpeed   =  4; //PWM Pin for right motor speed
const int kRightWheelDir     =  5; //DO Set right wheel direction
const int kSurfacePower       =  7; //Enable power to the surface detector LEDS
const int kArduinoLed        = LED_BUILTIN; //13 DO Arduino LED
const int kSurfaceRear        = 14; //AI rear surface detector
const int kSurfaceFrontLeft   = 15; //AI front center surface detector
const int kSurfaceFrontCenter = 16; //AI front right surface detector
const int kSurfaceFrontRight  = 17; //AI front left surface detector
const int kProximityDetect       = 18; //AI Input from the Infra Red detector
const int kSwitch1           = 23; //DI Not used in demo code
const int kSwitch2           = 24; //DI Not used in demo code

int g_led_state = LOW; //Initialize the state of the heartbeat LED to a known value

int g_surface_left_start=0; //Used to store the analog signal detected at power up
int g_surface_center_start=0; //Used to store the analog signal detected at power up
int g_surface_right_start=0; //Used to store the analog signal detected at power up


void setup() {
  // put your setup code here, to run once:
#if defined(DEBUG) || defined(SIGNALS_TEST)
  Serial.begin(9600);
  while (! Serial); //Wait for the serial monitor to connect
#endif

  pinMode(kLeftWheelDir, OUTPUT);     //DO
  pinMode(kLeftWheelSpeed, OUTPUT);   //PWM Output
  pinMode(kRightWheelDir, OUTPUT);    //DO
  pinMode(kRightWheelSpeed, OUTPUT);  //PWM Output
  pinMode(kArduinoLed, OUTPUT);       //DO
  pinMode(kSurfacePower, OUTPUT);      //DO
  pinMode(kSurfaceFrontLeft, INPUT);   //AI
  pinMode(kSurfaceFrontCenter, INPUT); //AI
  pinMode(kSurfaceFrontRight, INPUT);  //AI
  pinMode(kSurfaceRear, INPUT);        //AI
  pinMode(kProximityDetect, INPUT);       //AI

  digitalWrite(kSurfacePower,1); //Turn on the power to the LEDs on the surface detection board
  delay(100); //Wait for the LEDS to get power, probably not needed

  //Read the signal level of the surface when the robot is turned out
  g_surface_left_start   = analogRead(kSurfaceFrontLeft);
  g_surface_center_start = analogRead(kSurfaceFrontCenter);
  g_surface_right_start  = analogRead(kSurfaceFrontRight);
}

//if the opponents robot is detected, push it out
//When the Proximity detects something in the playing field, drive towards it
//with everything you have.
//TODO: Try to keep the target in the center of the field of vision.
void attackMode()
{
  drive(kForward,kAttackSpeed);
}

//wheelSpeed - calculate PWM value needed to turn the wheel
//Inputs:
//  direction - desired direction for the wheel to turn kForward or kReverse
//  speed - desired speed for the wheel to turn 0-100%
//Outputs:
//  returns The PWM signal value needed to turn the wheel as requested
int wheelSpeed(int direction, int speed) {
  //kForward max speed = 254
  //kReverse max speed = 0
  int signal_pwm=0;
  if (direction==kForward)
    signal_pwm = ((kmaxPWM * speed) / 100);
  else if (direction==kReverse)
    signal_pwm = (kmaxPWM - ((kmaxPWM * speed) / 100));

  return signal_pwm;
}

//drive - move the robot in the direction specified at the speed specified
//Inputs:
//  direction - The direction to travel kForward or kReverse
//  speed - The speed to move at 0 - 100%
void drive(int direction, int speed) {
#ifdef DEBUG
  Serial.print("\ndrive\n");
#endif
  digitalWrite(kLeftWheelDir, direction);
  digitalWrite(kRightWheelDir, direction);
  analogWrite(kLeftWheelSpeed, wheelSpeed(direction,speed));
  analogWrite(kRightWheelSpeed, wheelSpeed(direction,speed));  
}

//huntMode - Function to use when searching for opponents.
//You can do anything you want here
void huntMode(int direction, int speed) {
#ifdef DEBUG
  Serial.print("\nhunt\n");
#endif
  digitalWrite(kLeftWheelDir, kForward);
  digitalWrite(kRightWheelDir, kForward);
  if(direction==kLeft)
  {
    analogWrite(kLeftWheelSpeed, wheelSpeed(direction,speed-25));
    analogWrite(kRightWheelSpeed, wheelSpeed(direction,speed));  
  }
  else
  {
    analogWrite(kLeftWheelSpeed, wheelSpeed(direction,speed));
    analogWrite(kRightWheelSpeed, wheelSpeed(direction,speed-25));    
  }
}

//turn - pivots the robot left or right
//Inputs:
//  direction - the direction the robot should turn, kLeft or kRight
//  speed - The speed of the robot wheels when turning 0-100%
void turn(int direction, int speed) {
#ifdef DEBUG
  Serial.print("\nturn\n");
#endif
  digitalWrite(kLeftWheelDir, direction);
  digitalWrite(kRightWheelDir, 1 - direction);
  analogWrite(kLeftWheelSpeed, wheelSpeed(direction,speed));
  analogWrite(kRightWheelSpeed, wheelSpeed(1 - direction,speed));
}

//stopRobot - stop the robot from moving
void stopRobot(){
#ifdef DEBUG
  Serial.print("\nstopRobot\n");
#endif
  drive(kForward,kStop);
}

#ifdef CUPS_TEST
//onField - Detects if the robot is on the Field
//Inputs:
//  left - Last signal read from the front left surface detect sensor
//  center - Last signal read from the front center surface detect sensor
//  right - Last signal read from the front right surface detect sensor
//Outputs:
// Returns true if all of the surface detect sensors detect the Field
// Returns false if any of the surface detect sensors do not detect the Field
boolean onField (int left, int center, int right)
{
  #ifdef DEBUG
  Serial.print("\nleft_start ");
  Serial.print(g_surface_center_start);
  Serial.print("center start ");
  Serial.print(g_surface_center_start);
  Serial.print("right start ");
  Serial.print(g_surface_right_start);
  #endif
  if(left>kPlayFieldLow && center>kPlayFieldLow && right>kPlayFieldLow)
    return 1;

   return 0;
}
#else
boolean onField (int left, int center, int right)
{
  #ifdef DEBUG
  Serial.print("\nleft_start ");
  Serial.print(g_surface_left_start);
  Serial.print(" left_now ");
  Serial.print(left);
  Serial.print(" center start ");
  Serial.print(g_surface_center_start);
  Serial.print(" center_now ");
  Serial.print(center);
  Serial.print(" right start ");
  Serial.print(g_surface_right_start);
  Serial.print(" right_now ");
  Serial.print(right);

  #endif

  if(   (   (left >= (g_surface_left_start - kFieldRange)) 
         && (left <= (g_surface_left_start + kFieldRange)))
     && (    (center >= (g_surface_center_start - kFieldRange)) 
          && (center <= (g_surface_center_start + kFieldRange)))
     && (    (right >= (g_surface_right_start - kFieldRange)) 
          && (right <= (g_surface_right_start + kFieldRange))))
  {
    #ifdef DEBUG
    Serial.print("\nonField returning 1\n");
    #endif

    return 1;
  }

   return 0;
}
#endif

//onBorder - Detects if any of the surface detect sensors detect the border
//Inputs:
//  left - Last signal read from the front left surface detect sensor
//  center - Last signal read from the front center surface detect sensor
//  right - Last signal read from the front right surface detect sensor
//Outputs:
// Returns true if any of the surface detect sensors detect the surface
// Returns false if none of the surface detect sensor detect the surface
boolean onBorder(int left, int center, int right)
{
  if(   (left>0 && left < kBorderDetect)
     || (center>0 && center < kBorderDetect)
     || (right>0 && right < kBorderDetect))
     return 1;

   return 0;
}

//These are the various test modes selected by a #define at the top of this file
#ifdef SIGNALS_TEST
//The signals test will display the signal values of the sensors in terminal mode.
//You must select a port using tools->port and Serial Monitor under tools->Serial Monitor
//in the Arduino IDE if you are running this test. Use this test to verify the surface 
//sensors, the Proximity sensor, and the irout(surfacepower) are defined correctly and working.
void signalsTest() {
  Serial.print("\nFront Left ");
  Serial.print(analogRead(kSurfaceFrontLeft));
  Serial.print(" Front Center ");
  Serial.print(analogRead(kSurfaceFrontCenter));
  Serial.print(" Front Right ");
  Serial.print(analogRead(kSurfaceFrontRight));
  Serial.print(" Proximity");
  Serial.print(analogRead(kProximityDetect));
  delay(500);
}
#elif defined(DRIVE_TEST)
//drive forward and reverse to test both wheel motors are working properly. This verifies
//the two motor direction outputs and the PWM signals to the motors are working correctly.
void driveTest() {
  g_led_state = ~g_led_state; //change the led each cycle
  digitalWrite(kArduinoLed,g_led_state);

  drive(kForward,kHalfSpeed);
  delay(3000);
  stopRobot();
  delay(1000);
  drive(kReverse,kHalfSpeed);
  delay(3000);
  stopRobot();
  delay(1000);
}
#elif defined(ROOMBA_TEST)
//Drives forward until an object is detected by the front sensor 
//then changes direction and drives forward again. This verifies the value set
//for kProximityLevel can detect the difference between when an object is or isn't present.
void roombaTest() {
  g_led_state = ~g_led_state; //change the led each cycle
  digitalWrite(kArduinoLed,g_led_state);

  drive(kForward,kHalfSpeed); //drive forward
  while (analogRead(kProximityDetect) < kProximityLevel); //wait until an object is detected
  drive(kReverse,kHalfSpeed); //back up a little
  delay(150);
  turn(kLeft,kHalfSpeed); //change direction
  delay(150);  
}
#elif defined(FIELD_TEST)
//Stays within an area defined by the surface it was sitting on when it was turned on
//A good way to run this test is to place the robot on the floor in the hallway on a section
//of tile surrounded by a different color tile and it should stay on the tiles it was on 
//when you turned it on. This verifies the surface sensors are working and can detect different
//signal levels from different surfaces. You can adjust how sensitive it is to changes by 
//modifying the kFieldRange constant.
void fieldTest() {
  int surface_left_signal = 0;
  int surface_center_signal = 0;
  int surface_right_signal = 0;

  g_led_state = ~g_led_state; //change the led each cycle
  digitalWrite(kArduinoLed,g_led_state);

  drive(kForward,kHalfSpeed);
  
  do {
    surface_left_signal = analogRead(kSurfaceFrontLeft);
    surface_center_signal  = analogRead(kSurfaceFrontCenter);
    surface_right_signal = analogRead(kSurfaceFrontRight);  
  } while( ( onField(surface_left_signal,surface_center_signal,surface_right_signal)));

  //change direction
  drive(kReverse,kHalfSpeed);
  delay(250);
  turn(kRight,kMaxSpeed);
  delay(100);
}
#elif defined(ROOMBA_FIELD_TEST)
//Roomba Field Test - Combines Roomba Test and Field Test
//Robot will turn when it detects an object, but will also stay in an area
//defined by the surface it was sitting on when it was turned on.
void roombaFieldTest() {
  int proximity_signal=0;
  int surface_left_signal=0;
  int surface_center_signal = 0;
  int surface_right_signal=0;
  
  g_led_state = ~g_led_state; //change the led each cycle
  digitalWrite(kArduinoLed,g_led_state);

  drive(kForward,kHalfSpeed);
  
  do {
    proximity_signal = analogRead(kProximityDetect);
    surface_left_signal = analogRead(kSurfaceFrontLeft);
    surface_center_signal = analogRead(kSurfaceFrontCenter);
    surface_right_signal = analogRead(kSurfaceFrontRight);  
  } while(    proximity_signal < kProximityLevel 
           && onField(surface_left_signal, surface_center_signal, surface_right_signal));
                      
  drive(kReverse,50);
  delay(150);
  if (   surface_left_signal <= g_surface_left_start - kFieldRange 
      || surface_left_signal >= g_surface_left_start + kFieldRange )
  {
    #ifdef DEBUG
    Serial.print("left ");
    #endif
    turn(kRight,kHalfSpeed);
    delay(150);
  }
  else if (   surface_center_signal <= g_surface_center_start - kFieldRange 
           || surface_center_signal >= g_surface_center_start + kFieldRange )
  {
    #ifdef DEBUG
    Serial.print("center ");
    #endif

    drive(kReverse,kHalfSpeed);
    delay(150);
    turn(kRight,kHalfSpeed);
    delay(150);
  }
  else if (   surface_right_signal <= g_surface_right_start - kFieldRange 
           || surface_right_signal >= g_surface_right_start + kFieldRange )
  {
    #ifdef DEBUG
    Serial.print("right ");
    #endif

    turn(kLeft,kHalfSpeed);
    delay(150);
  }
  else if (proximity_signal >= kProximityLevel)
  {
    #ifdef DEBUG
    Serial.print("Proximity ");
    #endif
    
    drive(kReverse,kHalfSpeed);
    delay(200);
    turn(kLeft,kHalfSpeed);
    delay(150);
  }
}
#elif defined(CUPS_TEST)
//Stay in a playing field and push out anything detected.
void cupsTest() {
  int proximity_signal=0;
  int surface_left_signal=0;
  int surface_center_signal=0;
  int surface_right_signal=0;
  static int first_run=0;

  //This will only happen the first time. Allows for different
  //behavior when the match is first started.
  if(first_run==0)
  {
    drive(kForward,kHalfSpeed);
    first_run=1;
  }
  
  g_led_state = ~g_led_state; //change the led each cycle
  digitalWrite(kArduinoLed,g_led_state);

  //Wait for an object to be detected or for the robot to detect the surface
  //TODO: Use interrupts instead of polling
  do {
    proximity_signal = analogRead(kProximityDetect);
    surface_left_signal = analogRead(kSurfaceFrontLeft);
    surface_center_signal = analogRead(kSurfaceFrontCenter);
    surface_right_signal = analogRead(kSurfaceFrontRight);  
  } while( (   proximity_signal < kProximityLevel 
           && onField(surface_left_signal,surface_center_signal,surface_right_signal)));

  stopRobot(); //stop immediately
  
  //Figure out what happened
  //Are both front side sensors outside the playfield?
  if ( (surface_left_signal < kBorderDetect) && (surface_right_signal < kBorderDetect) )
  {
    drive(kReverse,kHalfSpeed);
    delay(250);
    turn(kRight,kMaxSpeed);
    delay(100);
    drive(kForward,kHalfSpeed);
  }
  //Did the robot leave the play field on the left side?
  else if (surface_left_signal < kBorderDetect)
  {
#ifdef DEBUG
    Serial.print("\nplayfield left exit\n");
#endif

//    drive(kReverse,50);
//    delay(150);
    turn(kRight,kMaxSpeed);
    delay(100);
    huntMode(kRight,kHuntSpeed);
  }
  //Did the robot leave the play field on the right side?
  else if (surface_right_signal < kBorderDetect)
  {
#ifdef DEBUG
    Serial.print("\nplayfield right exit\n");
#endif

//    drive(kReverse,50);
//    delay(150);
    turn(kLeft,kMaxSpeed);
    delay(100);
    huntMode(kLeft,kHuntSpeed);
  }
  //Was an object detected
  else if (proximity_signal > kProximityLevel)
  {
#ifdef DEBUG
    Serial.print("\nplayfield Proximity exit\n");
#endif
    if (onField(surface_left_signal, surface_center_signal, surface_right_signal))
    {
      attackMode();
    }
  }
}
#endif //mode


void loop() {
  // put your main code here, to run repeatedly:
  /*
  TODO: Use a switch, or a remote, to determine when to start the match
  while(digitalRead(kSwitch1)); //Wait for the switch to be released to start the battle
  */
#ifdef DEBUG
  Serial.print("\nFront Left ");
  Serial.print(analogRead(kSurfaceFrontLeft));
  Serial.print(" Front Center ");
  Serial.print(analogRead(kSurfaceFrontCenter));
  Serial.print(" Front Right ");
  Serial.print(analogRead(kSurfaceFrontRight));
#endif

#ifdef SIGNALS_TEST
  signalsTest();
#elif defined(DRIVE_TEST)
  driveTest();
#elif defined(ROOMBA_TEST)
  roombaTest();
#elif defined(FIELD_TEST)
  fieldTest();
#elif defined(ROOMBA_FIELD_TEST)
  roombaFieldTest();
#elif defined(CUPS_TEST)
  cupsTest();
#endif
}
