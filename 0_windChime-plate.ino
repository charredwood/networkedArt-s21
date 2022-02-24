#include <Servo.h>
#include "WiFiNINA.h"
#include "ArduinoHttpClient.h"

// ssid information
char ssid[] = "network";
char password[] = "passwd";

// server we want to connect to
const char serverAddress[] = "server";
int port = 8080;

// create a client
WiFiClient tcpSocket;

// create a websocket instance
WebSocketClient webSocket = WebSocketClient(tcpSocket, serverAddress, port);

Servo myservo; // create servo object to control a servo
int nPos = 100; // variable to store the servo position
int servoPin = 12;
int vibPin = 2;
int servoNum = 0;
int hapticNum = 0;
const long interval = 3000;
unsigned long prevTime = 0;

void setup() {
  Serial.begin(9600);

  //connect to the network
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid); // print the network name (SSID)
    WiFi.begin(ssid, password); // try to connect
    delay(2000);
  }

  // When you're connected, print out the device's network status:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  //connect to the socket
  connectToWsServer();
  
  pinMode(servoPin, OUTPUT);
  pinMode(vibPin, OUTPUT);
  myservo.attach(12); // attaches the servo on pin 9 to the servo object
}

void loop() {
  int vib;
  unsigned long currentTime = millis();

    if (webSocket.connected()) {
      int msgLength = webSocket.parseMessage(); // look for incoming messages
      if (msgLength > 0) { // if something is there, read and print it
        String message = webSocket.readString();
        String validChunk = message.substring(24);
        int messageLength = message.length();
//          Serial.println(validChunk);
        if (validChunk.startsWith("r")) {
          int breakPoint = validChunk.indexOf(":") + 1;
          String servoVal = validChunk.substring(breakPoint, messageLength - 1);
          servoNum = servoVal.toInt();
          Serial.print("servoVal ");
          Serial.println(servoNum);
        } else if (validChunk.startsWith("p")) {
          int breakPoint = validChunk.indexOf(":") + 1;
          String hapticVal = validChunk.substring(breakPoint, messageLength - 1);
          hapticNum = hapticVal.toInt();
          Serial.print("hapticVal");
          Serial.println(hapticNum);
        }
      }
    }

  nPos = map(servoNum, -20, 10, 75, 125);
  // check the mapped value
//  Serial.println(nPos);
  myservo.write(nPos);

  if (currentTime - prevTime >= interval) {
    digitalWrite(vibPin, LOW);
  }

  // motor part
  if (hapticNum < -17 || hapticNum > 17) {
    prevTime = currentTime;
    digitalWrite(vibPin, HIGH);
    Serial.println("vibrating!");
  }
}

// connect to the server
void connectToWsServer() {
  Serial.println("attempting to connect");
  boolean error = webSocket.begin();
  if (error) {
    Serial.println("failed to connect");
  } else {
    Serial.println("connected");
    sendJsonMessage("", 0); // on connect, send an empty message
  }
}

// Wrap message as JSON and send it away
void sendJsonMessage(String key, int val) {
  webSocket.beginMessage(TYPE_TEXT); //message type:text
  webSocket.print("{\"clientName\":\"receiver\"");
  if (key != "") { // if there is no key, just send name
    webSocket.print(",\""); //comma,opening quotation mark
    webSocket.print(key); //key
    webSocket.print("\":"); //closing quotation mark, colon
    webSocket.print(val); //value
  }
  webSocket.print("}");
  webSocket.endMessage();
}
