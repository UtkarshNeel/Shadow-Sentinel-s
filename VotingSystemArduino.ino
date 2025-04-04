#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <SPI.h>
#include <MFRC522.h>

// Hardware pins
#define SS_PIN D2  // RFID SDA pin
#define RST_PIN D1 // RFID reset pin

// WiFi credentials
const char* WIFI_SSID = "Neeraj";
const char* WIFI_PASSWORD = "12345678";

// Firebase configuration
#define FIREBASE_HOST "shadow-sentinels-51e88-default-rtdb.firebaseio.com" // No https://
#define FIREBASE_AUTH "AIzaSyA5zvBpiNtIrK4w85de_XKSeUCz9IrwTK4"

// Firebase objects
FirebaseData fbData;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json; // For structured data

// RFID reader
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port
  
  // Initialize SPI and RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("\nConnecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
    while (true); // Halt if no connection
  }

  // Initialize Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("System ready. Scan RFID cards...");
}

void loop() {
  // Check for new cards
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Read card UID
  String cardUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardUID += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print("Card UID: ");
  Serial.println(cardUID);

  // Create JSON object
  json.clear();
  json.add("uid", cardUID);
  json.add("timestamp", millis()); // Replace with NTP time if needed
  json.add("status", "unverified");
  json.add("reader_id", "ESP8266_01"); // Optional identifier

  // Push to Firebase
  if (Firebase.pushJSON(fbData, "/RFID_Logs", json)) {
    Serial.println("Data sent to Firebase!");
    Serial.print("Path: ");
    Serial.println(fbData.dataPath()); // Shows where data was stored
  } else {
    Serial.println("Firebase Error:");
    Serial.println(fbData.errorReason());
  }

  delay(1000); // Prevent duplicate scans
  mfrc522.PICC_HaltA(); // Stop reading
}