#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>

/* servo variables 
 *  
 */
Servo myservo;  // create servo object to control a servo
int posVal = 0;    // variable to store the servo position
int stopVal = 0;   // varibale to store the servo end position 
int servoPin = 15; // Servo motor pin

/* stepper variables
 *  
 */
// Connect the port of the stepper motor driver
int outPorts[] = {33, 27, 26, 25};


/* parallel core variables 
 *  
 */
static int taskCore = 0;  // the core that we want our other motor's code to run on 
bool motorON = false; // other motor is off initially 

/* this function is run by core 0 
 *  inside of the function I call runMotor which contains an infinite loop 
 */
void coreTask( void * pvParameters ){
      runMotor();
}

/* variables we need to get the wifi set up
 *  
 */  
String address= "http://134.122.113.13/ykl2112/running";
const char *ssid_Router     = "you-v-uh"; //Enter the router name
const char *password_Router = "12345678"; //Enter the router password

void setup() {
  Serial.begin(9600);
  delay(1000);



/* connecting to wifi 
 *  
 */
  WiFi.begin(ssid_Router, password_Router);
  Serial.println(String("Connecting to ")+ssid_Router);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup End");



/* setting up the servo 
 *  
 */
  myservo.setPeriodHertz(50);           // standard 50 hz servo
  myservo.attach(servoPin, 500, 2500);  // attaches the servo on servoPin to the servo object
  
  // set pins to output
  for (int i = 0; i < 4; i++) {
    pinMode(outPorts[i], OUTPUT);
  }


/* running one motor on a different core 
 *  
 */
  Serial.print("Starting to create task on core ");
  Serial.println(taskCore);
  xTaskCreatePinnedToCore(
                    coreTask,   /* Function to implement the task */
                    "coreTask", /* Name of the task */
                    10000,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    0,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    taskCore);  /* Core where the task should run */
 
  Serial.println("Task created...");
}



/* functions is called to run on core 0. 
 *  
 */
void runMotor(){
  while(true){
    Serial.println(motorON);
    if(motorON == true){
      Serial.println("inside of runMotor");
      for (posVal = 0; posVal <= 180; posVal += 1) { // goes from 0 degrees to 180 degrees
        // in steps of 1 degree
        myservo.write(posVal);       // tell servo to go to position in variable 'pos'
        delay(15);                   // waits 15ms for the servo to reach the position
      }
      for (posVal = 180; posVal >= 0; posVal -= 1) { // goes from 180 degrees to 0 degrees
        myservo.write(posVal);       // tell servo to go to position in variable 'pos'
        delay(15);                   // waits 15ms for the servo to reach the position
      }
  }
  }
}


/* main loop that connects us to wifi and contains a flag: motorON 
 *  that will start our other motor running on a different core
 *  
 */
 
void loop() {
  if((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(address);
 
    int httpCode = http.GET(); // start connection and send HTTP header
    Serial.print("httpCode: ");
    Serial.println(httpCode);
    if (httpCode == HTTP_CODE_OK) { 
        String response = http.getString();
        if (response.equals("false")) {
            // Do not run sculpture, perhaps sleep for a couple seconds
            motorON = false;
            Serial.println("sculpture should not run");
        }
        else if(response.equals("true")) {
            // Run sculpture
              motorON = true;
              Serial.println("Starting main loop...");
             //  in here run the code for stepper motor 
            // Rotate a full turn
            moveSteps(true, 32 * 64, 3);
            delay(500);
            // Rotate a full turn towards another direction
            moveSteps(false, 32 * 64, 3);
            delay(500);
        }
        Serial.println("Response was: " + response);
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    delay(500); // sleep for half of a second
  }
}


/* The functions below are used to run the step motor 
 *  
 *  
 *  
 *  
 *  
 */
 
//Suggestion: the motor turns precisely when the ms range is between 3 and 20
void moveSteps(bool dir, int steps, byte ms) {
  for (unsigned long i = 0; i < steps; i++) {
    moveOneStep(dir); // Rotate a step
    delay(constrain(ms,3,20));        // Control the speed
  }
}

void moveOneStep(bool dir) {
  // Define a variable, use four low bit to indicate the state of port
  static byte out = 0x01;
  // Decide the shift direction according to the rotation direction
  if (dir) {  // ring shift left
    out != 0x08 ? out = out << 1 : out = 0x01;
  }
  else {      // ring shift right
    out != 0x01 ? out = out >> 1 : out = 0x08;
  }
  // Output singal to each port
  for (int i = 0; i < 4; i++) {
    digitalWrite(outPorts[i], (out & (0x01 << i)) ? HIGH : LOW);
  }
}

void moveAround(bool dir, int turns, byte ms){
  for(int i=0;i<turns;i++)
    moveSteps(dir,32*64,ms);
}
void moveAngle(bool dir, int angle, byte ms){
  moveSteps(dir,(angle*32*64/360),ms);
}
