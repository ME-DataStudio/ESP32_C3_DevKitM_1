#include <WiFi.h>
#include <time.h>
#include "ESPAsyncWebServer.h"
#include <Wire.h>
#include <SPIFFS.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//PIN and PWMchannel for alarmsound, PIN for button
// Output pin
const int TONE_OUTPUT_PIN = 19;
const int BUTTON_INPUT_PIN = 3;
// The ESP32 has 16 channels which can generate 16 independent waveforms. We will use PWM channel 0 here
const int TONE_PWM_CHANNEL = 0; 

const char* ssid = "YOURSSID";
const char* password = "YOURPASSWORD";

const char* PARAM_TIJD = "alarmtijd";
const char* PARAM_AANUIT = "alarmAanUit";
String alarmAAN = "off";
int alarmuur = 7;
int alarmmin = 0;
bool alarmtoon = false;
int lastState = HIGH; // the previous state from the input pin
int currentState;     // the current reading from the input pin

int GMTOffset = 7200;  //Replace with your GMT Offset in seconds
int daylightOffset = 0;  // Replace with your daylight savings offset in seconds
int dim = 0; //dim the display;

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire, -1);
AsyncWebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)

//not found handler
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = SPIFFS.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(const char * path, const char * message){
String strmsg;
  
  Serial.printf("Writing file: %s\r\n", path);
  File file = SPIFFS.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
  if(path=="/alarmtijd.txt"){
    strmsg = message;
    
    alarmuur = strmsg.substring(0,2).toInt();
    alarmmin = strmsg.substring(3).toInt();
 
    Serial.println(alarmuur);
    Serial.println(alarmmin);
  }
  else if (path=="/alarmaanuit.txt"){
    alarmAAN = message;
    Serial.println(alarmAAN);
  }
}

void IRAM_ATTR buttonPressed(){
  const char * message = "off";
  writeFile("/alarmaanuit.txt",message);
}

// Replaces placeholder with Alarm time value
String processor(const String& var){
  Serial.println(var);
  String tijdstip = readFile("/alarmtijd.txt");
  String alarmgezet = "<input type=\"checkbox\" id=\"alarmAanUit\" name=\"alarmAanUit\" ";
  if(readFile("/alarmaanuit.txt")=="on"){
    alarmgezet += " checked>";
  }
  else {
    alarmgezet += " >";
  }
  
  if(var == "ALARM"){
    String alarm = ""; 
    alarm += "<form action=\"/get\"  target=\"hidden-form\">";
    alarm += "<label for=\"alarmtijd\">Kies een tijd:</label>";
    alarm += "<input type=\"time\" id=\"alarmtijd\" name=\"alarmtijd\" value=\"" + tijdstip+ "\">";
    alarm += "</br>";
    alarm += "<label class=\"toggle\">";
    alarm += alarmgezet;
    alarm += "<span class=\"slider\"></span>";
    alarm += "<span class=\"labels\" data-on=\"ON\" data-off=\"OFF\"></span>";
    alarm += "</label>";
    alarm += "</br>";
    alarm += "<button class=\"button \"type=\"submit\">Stuur</button>";
    alarm += "</form>";
    alarm += "<iframe style=\"display:none\" name=\"hidden-form\"></iframe>";
    return alarm;
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  Wire.begin(0,1);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.setTextColor(WHITE);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected to Wi-Fi!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());  //Show ESP32 IP on serial
  
  // setup time
  configTime(GMTOffset, daylightOffset, "0.in.pool.ntp.org","time.nist.gov");
  
  // dim display
  display.dim(true);
  display.ssd1306_command(0xD9);
  display.ssd1306_command(0);  //max 34, min 0

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    int paramsNr = request->params();
    Serial.println(paramsNr);
    for(int i=0;i<paramsNr;i++){
      AsyncWebParameter* p = request->getParam(i);
 
      Serial.print("Param name: ");
      Serial.println(p->name());
 
      Serial.print("Param value: ");
      Serial.println(p->value());
 
      Serial.println("------");
      if(p->name()==PARAM_TIJD) {
        inputMessage = p->value();
        if(inputMessage.length() > 0){
          writeFile("/alarmtijd.txt",inputMessage.c_str());
        } 
      }
      else if(p->name()==PARAM_AANUIT) {
        inputMessage = p->value();
        writeFile("/alarmaanuit.txt",inputMessage.c_str());
      }
      else {
        inputMessage = "No message sent";
      }
    }
    if(paramsNr==1){
      inputMessage = "off";
      writeFile("/alarmaanuit.txt",inputMessage.c_str());
    }

    request->send(200, "text/text", inputMessage);
  });
  
  server.onNotFound(notFound);

  server.begin();

  ledcAttachPin(TONE_OUTPUT_PIN, TONE_PWM_CHANNEL);
  pinMode(BUTTON_INPUT_PIN,INPUT_PULLUP);
  attachInterrupt(BUTTON_INPUT_PIN,buttonPressed,RISING);
  
}

void loop() {
  time_t rawtime = time(nullptr);
  struct tm* timeinfo = localtime(&rawtime);
  
  Serial.println(currentState);
  //Serial.print("Time: ");
  //Serial.print(timeinfo->tm_hour);
  //Serial.print(":");
  //Serial.print(timeinfo->tm_min);
  //Serial.print(":");
  //Serial.println(timeinfo->tm_sec);
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0,10);
  display.print(timeinfo->tm_hour);
  display.print(":");
  if( timeinfo->tm_min <10)
    display.print("0");
  display.print(timeinfo->tm_min);
  display.setTextSize(2);
  display.setCursor(90,15);
  display.print(":");
  if( timeinfo->tm_sec <10)
    display.print("0");
  display.print(timeinfo->tm_sec);
  display.display();
  delay(1000);
  if(alarmAAN=="on"){
    Serial.println("A A N");
    if(timeinfo->tm_hour == alarmuur){
      Serial.println("ALARM UUR");
      if(timeinfo->tm_min == alarmmin){
        Serial.println("ALARM MIN");
        alarmtoon=true;   
      }
    }
  } else {
    alarmtoon=false;
  }
  if(alarmtoon){
    // 1000Hz tone for 1 second
    ledcWriteTone(TONE_PWM_CHANNEL, 1000);
    delay(1000);    
    ledcWrite(TONE_PWM_CHANNEL, 0); // Stop tone      
  }
}
