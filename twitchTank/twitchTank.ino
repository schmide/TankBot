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
   let twitch drive a tank, what could possibly go wrong? ( ͡° ͜ʖ ͡°)

  MY CODE:

  THE CIRCUIT:
  D5 - Motor Right - 14
  D6 - Motor Left - 12
  D7 - Servo - 13



  NOTES:
    chatroom location of #bother_tank":
    #chatrooms:32178044:f6f6337a-71f1-42f3-8405-91499640fa84

    when login:
    after NICK response with " 376 " (space before and after important to filter)
    after PASS response with " 366 "
    messages might stack and get ip ban on user

*/

//==============Pins===========================
#define MOTOR_RF 4
#define MOTOR_RB 12
#define MOTOR_LF 16
#define MOTOR_LB 14

//==============Constants======================
const String chunnel = "#chatrooms:32178044:4e7e70a1-ee13-41b2-a202-3a9cdbec4653";
const String userName = "bother_tank";
const int msOut = 400;
const int speedLimit = 125; //0-255 for pwm shenanigans

//==============Globals========================
int state = 0;
String msg = "";
String command = "";
unsigned long timer = 0;
bool cmdFlag = 1;

//==============Library stuff==================
//espWIFI
WiFiClient wiclient;
IRCClient client(host, 6667, wiclient);

//==============Setup==========================
void setup() {
  //wifi
  WiFi.begin(ssid, password);

  //OTA stuffs
  //ArduinoOTA.setHostname("TankBot");
  ArduinoOTA.begin();

  //serial
  Serial.begin(115200);

  //backcall
  client.setCallback(answerMachine);

  pinMode(MOTOR_RF, OUTPUT);
  pinMode(MOTOR_RB, OUTPUT);
  pinMode(MOTOR_LF, OUTPUT);
  pinMode(MOTOR_LB, OUTPUT);

  digitalWrite(MOTOR_RF, LOW);
  digitalWrite(MOTOR_RB, LOW);
  digitalWrite(MOTOR_LF, LOW);
  digitalWrite(MOTOR_LB, LOW);
}

//======FUNCTIONS THAT MAKE THE DO THINGS=======
typedef void (*CmdList)();
const static struct cmds {
  const char *command;
  CmdList func;
  //  const char *desc;
} ppList[] = {
  {"f", forward},
  {"b", back},
  {"l", left},
  {"r", right},
  {"a", attack},
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
  checkConnect();
}

//==============Main Functions==================
//uses global state to run IRC bot
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
      cmdTimeout();
      break;
  }
}

//==============Secondary Functions=============
void checkConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    state = 0;
    Serial.println("checkConnect: WiFi borked");
  } else if (!wiclient.connected()) {
    state = 1;
    Serial.println("checkConnect: Client borked");
  }
}

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


void cmdTimeout() {
  unsigned long currentMillis = millis();
  client.loop();
  if (currentMillis - timer > msOut) {
    cmdFlag = 1;
  }
  
}

void clientTime(int delayer){
  unsigned long currentMillis = millis();
  while(currentMillis + delayer < millis()){
    ArduinoOTA.handle();
    client.loop();
  }
}

void answerMachine(IRCMessage hollaBack) {
  if (hollaBack.nick != userName && cmdFlag == 1) {
    for (int i = 0; i < ppSize ; i++) {
      if (hollaBack.text.equalsIgnoreCase(ppList[i].command)) {
        cmdFlag = 0;
        ppList[i].func();
        timer = millis();
        break;
      }
    }
  }
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
//==============BOT FUNCTIONS================
void forward() {
  sendTwitchMessage("tendies on!");
  analogWrite(MOTOR_RF, speedLimit);
  analogWrite(MOTOR_LF, speedLimit);
  clientTime(200);
  digitalWrite(MOTOR_RF, LOW);
  digitalWrite(MOTOR_LF, LOW);
}

void back() {
  sendTwitchMessage("tendies retreat!");
  analogWrite(MOTOR_RB, speedLimit);
  analogWrite(MOTOR_LB, speedLimit);
  clientTime(200);
  digitalWrite(MOTOR_RB, LOW);
  digitalWrite(MOTOR_LB, LOW);
}

void left() {
  sendTwitchMessage("tendies left");
  analogWrite(MOTOR_RF, speedLimit);
  analogWrite(MOTOR_LB, speedLimit);
  clientTime(200);
  digitalWrite(MOTOR_RF, LOW);
  digitalWrite(MOTOR_LB, LOW);
}

void right() {
  sendTwitchMessage("tendies right!");
  analogWrite(MOTOR_RB, speedLimit);
  analogWrite(MOTOR_LF, speedLimit);
  clientTime(200);
  digitalWrite(MOTOR_RB, LOW);
  digitalWrite(MOTOR_LF, LOW);
}

void attack() {
  sendTwitchMessage("REEEEEEEEE");
}
