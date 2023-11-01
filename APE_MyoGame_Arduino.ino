#include <WiFiS3.h>
#include <WiFiUdp.h>
#include "arduino_secrets.h"

// WIFI SETUP
char ssid[] = SECRET_SSID;                // Network SSID stored in arduino_secrets.h
char pass[] = SECRET_PASS;                // Network password stored in arduino_secrets.h
char server_greeting[] = SERVER_GREETING; // Server handshake message in arduino_secrets.h
char client_greeting[] = CLIENT_GREETING; // Client handshake message in arduino_secrets.h

int status = WL_IDLE_STATUS;              // Current Wi-Fi connection status

bool is_connected = false;                // True, if server has been identified
unsigned int local_port = 2390;           // Local port that we listen on
IPAddress server_ip;                      // IP of Unity server 
unsigned int server_port;                 // Port of Unity server
char packet_buf[256];                     // Buffer for incoming packets
WiFiUDP udp;                              // UDP handle

// HARDWARE SETUP
const int E1_PIN = A0;                    // E1 signal
const int E2_PIN = A1;                    // E2 signal
const int LED_PIN = 2;                    // Status LED
const int BUTTON_PIN = 3;                 // Button example, not necessary but available on the stick
int button_state = 0;                     // Stores the current button state

void setup() {
  // Initialize serial communication
  Serial.begin(9600); 

  // Configure hardware
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  analogReadResolution(14);

  // Check if Wi-Fi module is operational
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Wi-Fi module not operational!");

    // Eternal loop of shame
    while (true) {
      digitalWrite(LED_PIN, HIGH);
      delay(1500);
      digitalWrite(LED_PIN, LOW);
      delay(1500);
    }
  }

  // Check Wi-Fi firmware
  String firmware_version = WiFi.firmwareVersion();

  if (firmware_version < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Wi-Fi firmware not up to date!");
  }

  // Connect to network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to: ");
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass);
    delay(10000); // Wait for connection before attempting again
  }

  // Connection established
  Serial.println("Successfully connected to: ");
  printCurrentNetwork();
  printWifiData();

  // Start listening on local port
  udp.begin(local_port);
  Serial.println("Listening...");
}

void loop() {
  // Receive data
  udpReceive();

  if (is_connected) {  
    // Read button
    button_state = digitalRead(BUTTON_PIN);
    if (button_state == LOW) {
      Serial.println("Button is pressed.");
    }

    // Measure inputs
    int e1_value = analogRead(E1_PIN);
    int e2_value = analogRead(E2_PIN);
    // Bullshit input manipulation to get a useful axis from our analogue stick
    e1_value -= 8192 + 65;

    if (e1_value < 0) {
      e1_value = 0;
    }

    e1_value *= 2;

    e2_value -= 8192;

    if (e2_value < 0) {
      e2_value = 0;
    }

    e2_value *= 2;

    // End of bullshit input manipulation

    String measurements = "M;";
    measurements = measurements + String(e1_value) + ";" + String(e2_value);
    udpSend(measurements);
    delay(1); 
  }
}

// Sends UDP packets to the Unity server
void udpSend(String message) {
  int len = message.length() + 1;
  char send_buf[len];
  message.toCharArray(send_buf, len);
  
  Serial.println("Sending message: " + message);

  udp.beginPacket(server_ip, server_port);
  udp.write(send_buf);
  udp.endPacket();
}

// Receives incoming UDP packets
void udpReceive() {
  // Determine packet size
  int packet_size = udp.parsePacket();

  // If data is available -> read it
  if (packet_size) {
    int len = udp.read(packet_buf, 255);

    if (len > 0) {
      packet_buf[len] = 0;
    }

    String message = packet_buf;
    message.trim();

    // Display message
    Serial.print("\n");
    Serial.print("Received packet of size: ");
    Serial.println(packet_size);
    Serial.print("From ");
    Serial.print(udp.remoteIP());
    Serial.print(" : ");
    Serial.println(udp.remotePort());
    Serial.println("packet_buf: ");
    Serial.print(">");
    Serial.print(packet_buf);
    Serial.println("<");
    Serial.println("message: ");
    Serial.print(">");
    Serial.print(message);
    Serial.println("<");

    // If we are not connected to the server, look for handshake
    if (!is_connected) {
      if (strcmp(message.c_str(), server_greeting) == 0) {
        Serial.println("Potential server found...");
        server_ip = udp.remoteIP();
        server_port = udp.remotePort();
        udpSend(client_greeting);
      }

      if (strcmp(message.c_str(), "ACK") == 0) {
        Serial.println("Server confirmed!");
        is_connected = true;
      }
    } else {
      // Connection established, ignore all garbage that is not sent from the server
      if (udp.remoteIP() == server_ip && udp.remotePort() == server_port) {
        // Process commands
        
      }
    }
  }
}

  // Print Arduino network info
void printWifiData() {
  // Print IP
  IPAddress ip = WiFi.localIP();
  Serial.print("IP: ");
  Serial.println(ip);

  // Print MAC
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  printMacAddress(mac);
}

// Print current network info
void printCurrentNetwork() {
    // Print SSID
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // Print MAC of router
    byte bssid[6];
    WiFi.BSSID(bssid);
    Serial.print("BSSID: ");
    printMacAddress(bssid);

    // Print signal strength
    long rssi = WiFi.RSSI();
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(rssi);
    Serial.println(" dBm");

    // Print encryption type
    byte encryption = WiFi.encryptionType();
    Serial.print("Encryption: ");
    Serial.print(encryption, HEX);
    Serial.println();
}

// Pretty print MAC address
void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }

    Serial.print(mac[i], HEX);
    
    if (i > 0) {
      Serial.print(":");
    }
  }

  Serial.println();
}
