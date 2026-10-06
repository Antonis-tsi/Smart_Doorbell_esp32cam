//Αντώνης Τσίγγερης 
//Smart DoorBell ESP32_CAM
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

//  σύνδεση στο Wi-Fi δίκτυο
const char* ssid = "*******";
const char* password = "*******";

// Πληροφορίες για το Telegram Bot (Chat ID και Token)
String chatId = "*******";
String BOTtoken = "*******";

// Μεταβλητές για εσωτερική χρήση
bool sendPhoto = false;             // Αν οριστεί σε true, αποστέλλεται φωτογραφία
bool flashEnabled = true;          // Αν ενεργοποιηθεί, το φλας θα ανάψει κατά τη λήψη φωτογραφίας
int ringtone = 0;                  // Επιλογή μελωδίας για τον ήχο κουδουνιού

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

// Ορισμός των GPIO ακίδων
#define BUTTON 13
#define FLASH_LED 4
#define BUZZER 12

// Ορισμός των GPIO που χρησιμοποιεί η κάμερα AI-THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Μεταβλητές για συγχρονισμό με Telegram
const unsigned long BOT_MTBS = 1000;  // Καθυστέρηση για ανάκτηση νέων μηνυμάτων
unsigned long bot_lasttime;

void handleNewMessages(int numNewMessages);   // Συνάρτηση διαχείρισης μηνυμάτων
String sendPhotoTelegram();                   // Συνάρτηση αποστολής φωτογραφίας

// Συνάρτηση αναπαραγωγής μελωδίας μέσω του buzzer
void playRingtone() {
  int melody1[] = {659, 659, 0, 659, 0, 523, 659, 784};
  int duration1[] = {150, 150, 150, 150, 150, 150, 150, 450};

  int melody2[] = {880, 880, 880, 0, 880, 0, 880, 880};
  int duration2[] = {300, 300, 300, 150, 300, 150, 300, 450};

  if (ringtone == 0) {
    for (int i = 0; i < sizeof(melody1)/sizeof(int); i++) {
      if (melody1[i]) tone(BUZZER, melody1[i]);
      delay(duration1[i]);
      noTone(BUZZER);
      delay(50);
    }
  } else {
    for (int i = 0; i < sizeof(melody2)/sizeof(int); i++) {
      if (melody2[i]) tone(BUZZER, melody2[i]);
      delay(duration2[i]);
      noTone(BUZZER);
      delay(50);
    }
  }
}

// Συνάρτηση αποστολής φωτογραφίας στο Telegram
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t * fb = NULL;

  // Αν το φλας είναι ενεργό, ενεργοποίησέ το πριν τη λήψη
  if (flashEnabled) {
    digitalWrite(FLASH_LED, HIGH);
    delay(150);
  }

  fb = esp_camera_fb_get();  // Λήψη φωτογραφίας

  if (flashEnabled) {
    delay(150);
    digitalWrite(FLASH_LED, LOW);
  }

  if(!fb) {
    Serial.println("Camera capture failed");
    bot.sendMessage(chatId, " Αποτυχία λήψης φωτογραφίας. Κάνω επανεκκίνηση...", "");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }

  Serial.println("Connect to " + String(myDomain));

  // Δημιουργία σύνδεσης με Telegram
  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    // Δημιουργία HTTP multipart μηνύματος με τη φωτογραφία
    String head = "--IotCircuitHub\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chatId + "\r\n--IotCircuitHub\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--IotCircuitHub--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;

    // Αποστολή HTTP αιτήματος POST στο Telegram
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=IotCircuitHub");
    clientTCP.println();
    clientTCP.print(head);

    // Αποστολή δεδομένων της εικόνας σε κομμάτια
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      } else {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }

    clientTCP.print(tail);
    esp_camera_fb_return(fb);  // Επιστροφή του frame buffer

    // Περιμένει για απάντηση από το Telegram
    int waitTime = 5000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + waitTime) > millis()) {
      Serial.print(".");
      delay(100);
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (c == '\n') {
          if (getAll.length() == 0) state = true;
          getAll = "";
        } else if (c != '\r') {
          getAll += String(c);
        }
        if (state == true) {
          getBody += String(c);
        }
        startTimer = millis();
      }
      if (getBody.length() > 0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  } else {
    getBody = "Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
    bot.sendMessage(chatId, " Αποτυχία σύνδεσης στο Telegram. Επανεκκίνηση...", "");
    delay(1000);
    ESP.restart();
  }

  return getBody;
}

// Συνάρτηση διαχείρισης εισερχόμενων μηνυμάτων από Telegram
void handleNewMessages(int numNewMessages){
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++){
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != chatId){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.println(text);

    // Εντολή για λήψη φωτογραφίας
    if (text == "/photo") {
      sendPhoto = true;
      Serial.println("New photo request");
    }

    // Εντολή για αρχικό μήνυμα και οδηγίες
    if (text == "/start"){
      String welcome = "Welcome to the ESP32-CAM DoorBell.\n";
      welcome += "/start - Εμφάνιση Menu\n";
      welcome += "/photo - Λήψη φωτογραφίας\n";
      welcome += "/flash - Ενεργοποίηση/Απενεργοποίηση φλας\n";
      welcome += "/ringtone - Εναλλαγή μελωδίας\n";
      bot.sendMessage(chatId, welcome, "Markdown");
    }

    // Εναλλαγή κατάστασης φλας
    if (text == "/flash") {
      flashEnabled = !flashEnabled;
      String status = flashEnabled ? " Το φλας ΕΝΕΡΓΟΠΟΙΗΘΗΚΕ" : " Το φλας ΑΠΕΝΕΡΓΟΠΟΙΗΘΗΚΕ";
      bot.sendMessage(chatId, status, "");
    }

    // Εναλλαγή μελωδίας κουδουνιού
    if (text == "/ringtone") {
      ringtone = (ringtone + 1) % 2;
      bot.sendMessage(chatId, " Άλλαξα μελωδία!", "");
    }
  }
}

// Συνάρτηση αρχικοποίησης συσκευής ESP32-CAM
void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Απενεργοποίηση brownout detector
  Serial.begin(115200);
  delay(1000);

  // Ρύθμιση ακίδων
  pinMode(FLASH_LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Σύνδεση στο WiFi
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

  // Διαμόρφωση κάμερας
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  // Μείωση ανάλυσης για ταχύτερη αποστολή
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF);

  bot.sendMessage(chatId, "Esp32-CAM είναι ενεργό!", "");

  String welcome = "Welcome to the ESP32-CAM DoorBell.\n";
  welcome += "/start - Εμφάνιση Menu\n";
  welcome += "/photo - Λήψη φωτογραφίας\n";
  welcome += "/flash - Ενεργοποίηση/Απενεργοποίηση φλας\n";
  welcome += "/ringtone - Εναλλαγή μελωδίας\n";
  bot.sendMessage(chatId, welcome, "Markdown");
}

// Κύρια επαναλαμβανόμενη λειτουργία
void loop(){
  if (sendPhoto){
    Serial.println("Preparing photo");
    delay(100);
    sendPhotoTelegram();
    sendPhoto = false;
  }

  // Έλεγχος για πάτημα κουμπιού
  if (digitalRead(BUTTON) == LOW) {
    Serial.println(" Button pressed - capturing photo");
    playRingtone();                   // Παίζει ο ήχος
    sendPhotoTelegram();             // Στέλνει φωτογραφία
    bot.sendMessage(chatId,"Κάποιος είναι στην πόρτα σου!");
    sendPhoto = false;
    delay(500);
  }

  // Έλεγχος για νέα μηνύματα από Telegram
  if (millis() - bot_lasttime > BOT_MTBS){
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages){
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}
