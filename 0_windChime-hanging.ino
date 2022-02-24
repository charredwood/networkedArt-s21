// include libraries
#include "WiFiNINA.h"
#include "ArduinoHttpClient.h"
#include "Arduino_LSM6DS3.h"
#include "MadgwickAHRS.h"

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

// initialize a Madgwick filter:
Madgwick filter;
// sensor's sample rate is fixed at 104 Hz:
const float sensorRate = 104.00;

// values for orientation:
float roll = 0.0;
float pitch = 0.0;
//float heading = 0.0;

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

  // attempt to start the IMU:
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU");
    // stop here if you can't access the IMU:
    while (true);
  }
  // start the filter to run at the sample rate:
  filter.begin(sensorRate);

  //connect to the socket
  connectToWsServer();
}

void loop() {
  // values for acceleration and rotation:
  float xAcc, yAcc, zAcc;
  float xGyro, yGyro, zGyro;

  // check if the IMU is ready to read:
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    // read accelerometer and gyrometer:
    IMU.readAcceleration(xAcc, yAcc, zAcc);
    IMU.readGyroscope(xGyro, yGyro, zGyro);

    // update the filter, which computes orientation:
    filter.updateIMU(xGyro, yGyro, zGyro, xAcc, yAcc, zAcc);

    // print the heading, pitch and roll
    roll = filter.getRoll();
    pitch = filter.getPitch();
//    heading = filter.getYaw();
    if (webSocket.connected()) {
      int msgLength = webSocket.parseMessage(); // look for incoming messages
      if (msgLength > 0) { // if something is there, read and print it
        String message = webSocket.readString();
        Serial.println(message);
      }
      // send to server
//      sendJsonMessage("heading", heading);
      sendJsonMessage("pitch", pitch);
      sendJsonMessage("roll", roll);
    }
  }
  delay(60);
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
  webSocket.print("{\"clientName\":\"sender\"");
  if (key != "") { // if there is no key, just send name
    webSocket.print(",\""); //comma,opening quotation mark
    webSocket.print(key); //key
    webSocket.print("\":"); //closing quotation mark, colon
    webSocket.print(val); //value
  }
  webSocket.print("}");
  webSocket.endMessage();
}
