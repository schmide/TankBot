#include "config.h" //config file
#include "ESP8266WiFi.h" //lowercase i's thanks
#include <ArduinoOTA.h>
#include <IRCClient.h>

/*
  moved to config.h:

  static struct namingtion[ const char *werds, uint32_t val ]
  werds = html color name "inQuotes"
  val = hexidecimal value of RGB

  const char* ssid     = "MySSID";
  const char* password = "routerPw";
  const String OAuth ="oauth:numbers";
  const char* host = "irc.chat.twitch.tv";

*/

/*
  THE GOAL:
   let twitch drive a tank, what could possibly go wrong?

  MY CODE:
    A couple basic components. 
    1. ota updates
    2. connection manager runs the sequence of logins then the callback loop
    3. command runner that uses an array and no delays
    4. callback to command interpreter and array enterer
    5. connection checker that bumps the manager state back.

    the new stuff is that the interpreter gets a command and punches that
    into the array, then increments its punch in location to the next slot

    as the code completes "command" performances it will decriment the entry 
    point and shift over the array, the default "0" is a stop command.

    some custom timing stuff will be added so that users can put non-default
    times on commands.

    performances (movements) functions should be run multiple times, but yeild
    might make timing irregular. timing-based animators are needed.

  THE CIRCUIT:
    two SOT223-6 H bridges, 4 pins. 
    right motor forward - 4
    right motor backward - 12
    left motor forward - 16
    left motor backward - 14

  NOTES:
    chatroom location of #bother_tank":
    #chatrooms:32178044:f6f6337a-71f1-42f3-8405-91499640fa84
*/

//==============Pins===========================
#define MOTOR_RF 4
#define MOTOR_RB 12
#define MOTOR_LF 16
#define MOTOR_LB 14

//==============Constants======================
const String chunnel = "#chatrooms:32178044:4e7e70a1-ee13-41b2-a202-3a9cdbec4653";
const String userName = "bother_tank";
const int ramper = 10; //0 to 255 I guess
const int cmdTime = 400; //ms [replace later]
const int governator = 255;

//==============Globals========================
int state = 0;
uint8_t cmdInsert = 1;
uint8_t cmdArray[50];
int treadRightSpeed = 0;
int treadLeftSpeed = 0;
unsigned long cmdStartTime = 0;

//==============Library stuff==================
//espWIFI
WiFiClient wiclient;
IRCClient client(host, 6667, wiclient);

//==============Setup==========================
void setup() {
  //set pins and states first?
  pinMode(MOTOR_RF, OUTPUT);
  pinMode(MOTOR_RB, OUTPUT);
  pinMode(MOTOR_LF, OUTPUT);
  pinMode(MOTOR_LB, OUTPUT);

  digitalWrite(MOTOR_RF, LOW);
  digitalWrite(MOTOR_RB, LOW);
  digitalWrite(MOTOR_LF, LOW);
  digitalWrite(MOTOR_LB, LOW); 
  
  //wifi
  WiFi.begin(ssid, password);
  yield(); //maybe?

  //OTA stuffs
  ArduinoOTA.setHostname("TankBot");
  ArduinoOTA.begin();

  //serial
  Serial.begin(115200);

  //wipe your array you dirty-...
  memset(cmdArray, 0, sizeof(cmdArray));

  //go to this function when client gets *any* message
  client.setCallback(answerMachine);
}

//======weird spot the command structure goes=======
typedef void (*CmdList)();
const static struct cmds {
  const char *command;
  CmdList func;
  //  const char *desc;
} ppList[] = {
  {"s", halt},
  {"f", forward},
  {"b", back},
  {"l", left},
  {"r", right},
  {"a", attack},
  {"stop", halt},
  {"forward", forward},
  {"back", back},
  {"left", left},
  {"right", right},
  {"attack", attack}
};

const int ppSize = sizeof(ppList) / sizeof(cmds);

//==============Loooooop========================
void loop() {
  ArduinoOTA.handle();
  connectMachine();
  doCommands();
  pinOutputter();
  checkConnect();
}

//==============Main Functions==================
void connectMachine() {
  switch (state) {
    case 0:
      wifiConnect();
      break;

    case 1:
      hostConnect();
      break;

    case 2:
      ircConnect();
      break;

    case 3:
      looper();
      break;
  }
}

void checkConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    state = 0;
    Serial.println("checkConnect: WiFi borked");
  } else if (!wiclient.connected()) {
    state = 1;
    Serial.println("checkConnect: Client borked");
  }
}

//==============COMMAND FUNCTIONS===================
void answerMachine(IRCMessage hollaBack) {  
  if (hollaBack.nick != userName) { //it's not us  
    for (int i = 0; i < ppSize ; i++) { //it's a command
      if (hollaBack.text.equalsIgnoreCase(ppList[i].command)) {
        
        //deal with the array if it needs to shift
        if( cmdInsert == sizeof(cmdArray)-1 ){
          shiftArrayInsert(); 
        }
        
        //put command in array position
        cmdArray[cmdInsert] = i;
        
        if( cmdInsert != sizeof(cmdArray)-1 ){
          cmdInsert++;
        }
        
        break;

        //get the numerical command timer
        //..needs another array column row whatev
        //..v short stop default timer could be used
      }
    }
  }
}

void doCommands(){
  unsigned long timeywimey = millis();
  if(cmdStartTime + cmdTime < timeywimey){
    shiftArray();
    cmdStartTime = millis();
  }
  int cmd = cmdArray[0];
  ppList[cmd].func();
}

void pinOutputter(){
  treadRightSpeed = constrain(treadRightSpeed, -governator, governator);
  treadLeftSpeed = constrain(treadLeftSpeed, -governator, governator);
  
  if(treadRightSpeed > 0){
    analogWrite(MOTOR_RB, 0);
    analogWrite(MOTOR_RF, abs(treadRightSpeed));
  }else if(treadRightSpeed < 0){
    analogWrite(MOTOR_RF, 0);
    analogWrite(MOTOR_RB, abs(treadRightSpeed));
  }else{
    analogWrite(MOTOR_RF, 0);
    analogWrite(MOTOR_RB, 0);
  }
  
  if(treadLeftSpeed > 0){
    analogWrite(MOTOR_LB, 0);
    analogWrite(MOTOR_LF, abs(treadLeftSpeed));
  }else if(treadLeftSpeed < 0){
    analogWrite(MOTOR_LF, 0);
    analogWrite(MOTOR_LB, abs(treadLeftSpeed));
  }else{
    analogWrite(MOTOR_LF, 0);
    analogWrite(MOTOR_LB, 0);
  }
}

//==============Secondary Functions=============
void wifiConnect() {
  if (WiFi.status() == WL_CONNECTED) {
    state = 1;
    Serial.println("wifiConnect: WiFi is connected, brah");
  } else {
    blinkenLight(200);
    Serial.println("wifiConnect: couldn't WiFi today");
  }
  digitalWrite(LED_BUILTIN, LOW);
}

void hostConnect() {
  if (wiclient.connect(host, 6667)) {
    state = 2;
    Serial.println("hostConnect: host is connected, brosideon");
  } else {
    //you fail
    Serial.println("hostConnect: failed to connect you ass");
    blinkenLight(1000);
  }
}

void ircConnect() {
  blinkenLight(500);
  Serial.println("ircConnect: getting on irc");
  wiclient.print("PASS ");
  wiclient.print(OAuth);
  wiclient.println("\r");

  blinkenLight(250);
  Serial.println("ircConnect: warming tendies");
  wiclient.print("NICK ");
  wiclient.print(userName);
  wiclient.println("\r");

  blinkenLight(250);
  Serial.println("ircConnect: joining channel");
  wiclient.print("JOIN ");
  wiclient.print(chunnel);
  wiclient.println("\r");

  //the pound sign is called an octothorpe.
  sendTwitchMessage("tendies warmed ready to rock.");
  blinkenLight(250);
  state = 3;
}

void looper(){
  client.loop();
  yield();
}

//==============HELPER FUNCTIONS=============
void sendTwitchMessage(String message) {
  client.sendMessage(chunnel, message);
}

void blinkenLight(int miller_time) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(miller_time);
  digitalWrite(LED_BUILTIN, LOW);
  delay(miller_time);
}

//move over the command array at the completion of a function
void shiftArray(){
  for(int i = 1 ; i <= sizeof(cmdArray)-2 ; i++){
    cmdArray[i] = cmdArray[i+1];
  }
  cmdArray[ sizeof(cmdArray)-1 ] = 0;
  if(cmdInsert > 1){
    cmdInsert--;
  }
}

//move over the command array to make room for a fat ass command
void shiftArrayInsert(){
  for(int i = 1 ; i <= sizeof(cmdArray)-2 ; i++){
    cmdArray[i] = cmdArray[i+1];
  }
  cmdArray[ sizeof(cmdArray)-1 ] = 0;
}

//==============BOT FUNCTIONS================
//these need to set an initial animation time for serial print
//then some kind of ramp-up function based on time
//stop is insta-stop

void halt(){
  if(treadRightSpeed > 0){
    treadRightSpeed -= ramper;
    treadRightSpeed = constrain(treadRightSpeed, 0, governator);
  }else if(treadRightSpeed < 0){
    treadRightSpeed += ramper;
    treadRightSpeed = constrain(treadRightSpeed, -governator, 0);
  }
  
  if(treadLeftSpeed > 0){
    treadLeftSpeed -= ramper;
    treadLeftSpeed = constrain(treadLeftSpeed, 0, governator);
  }else if(treadLeftSpeed < 0){
    treadLeftSpeed += ramper;
    treadLeftSpeed = constrain(treadLeftSpeed, -governator, 0);
  }
}

void forward() {
  //sendTwitchMessage("tendies on!");
  treadRightSpeed += ramper;
  treadLeftSpeed += ramper;
}

void back() {
  //sendTwitchMessage("tendies retreat!");
  treadRightSpeed -= ramper;
  treadLeftSpeed -= ramper;
}

void left() {
  //sendTwitchMessage("tendies left");
  treadRightSpeed -= ramper;
  treadLeftSpeed += ramper;
  
}

void right() {
  //sendTwitchMessage("tendies right!");
  treadRightSpeed += ramper;
  treadLeftSpeed -= ramper;

}

void attack(){
  sendTwitchMessage("REEEEEEEEE");
  
}

