#include <107-Arduino-MCP2515.h>
// #include <mcp2515.h>
// #include "mbed.h"
// #include "Arduino_CAN.h"
#include <LiquidCrystal.h>
#include <SPI.h>
#include <RPC.h>
#include <Arduino_USBHostMbed5.h>
#include <DigitalOut.h>
#include <FATFileSystem.h>
#include <SPI.h>
#include <WiFi.h>

// speed reading stuffs
const int rs = 6, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const byte sensorPin = 13;  // ir sensor read pin
const byte fuelPin = 5; // fuel sensor read pin

byte IRstate = 1;
byte IRValue = 1;
byte fuelValue = 1;
byte fuelState = 1;

byte IRcount = 0;  // number of tick marks detected by ir sensor last time sensor data was gathered
byte fuelCount = 0; // number of fuel activations

const byte lapbutton = 4; // button input
bool lap_button_position = true;

int lap = 0;
// float lapExact = 1.0;

// float lapDistance = 1.60;

const byte burnSpeed = 5; //Speed at which to start a burn (arbitrary numbers put in for testing - needs fine-tuning)

const byte coastSpeed = 25; //Speed at which to start coasting

const byte numSense = 16; //Number of tape lines on wheel cover





const unsigned long microSecsToHours = 3600000000;  // conversion factor between microseconds and hours.

float wheelRPM = 0.0;  // wheel RPM based on the number of tick marks counted during the last sensor reading

float diameter = 1.5;     //diameter of wheel in feet

float circumference = 4.83333333333;      //circumference in feet

double vehicleSpeed = 0.0;     //current speed in mph

double distance = 0.0;  // distance traveled during program runtime

double averageSpeed = 0.0;


unsigned long start = 0.0;  // time since the program started, or since reaching a certain speed, when starting to read sensor data 

unsigned long currTime = 0.0;  // time since the program started, or since reaching a certain speed
unsigned long IRcounter = 0.0; // number of tick marks detected by IR sensor during program runtime
unsigned long fuelCounter = 0.0; // number of fuel activations detected during program runtime

unsigned long testTime1 = 0;
unsigned long testTime2 = 0;

byte fuelInjector = 5;  // fuel injector read pin

byte burnLED = 3;  // burn LED pin
byte coastLED = 2; // coast LED pin

unsigned long vehicleStartTime = micros();  // time since program start at which vehicle passed a certain speed
boolean notStarted = true;



unsigned long ProcessTime = 0; // variable for how long it takes to process read values
unsigned long ProcessStart = micros(); // time when processing started
unsigned long ProcessEnd = micros(); // time when processing ended

unsigned long LastReset = millis();// last time LCD reset in miliseconds since program started 
const unsigned long ResetDelay = 30000; // time in miliseconds between restarting the LCD
unsigned long readDelay = 5000; // time in milliseconds to wait to log speed data
unsigned long lastRead = currTime - readDelay; // last time speed data was logged


bool speedUpdateReady = false; // if a speed update is ready on the m4 core


// usb logging stuff

USBHostMSD msd;
mbed::FATFileSystem usb("SMV_STUFF");

// mbed::DigitalOut pin5(PC_6, 0);
mbed::DigitalOut otg(PB_8, 1);

std::string Runs = "Run"; // the first 3 charachters of the appropriate file name
int runNumber = 0; // the run currently active given files in usb drive
int runNumberTemp = 0;
std::string fileNameRunSubstring;
std::string fileNameSubstring;
std::string fileNameString;
char fileNameRuns[4];

String filePath = "/SMV_STUFF/Runs/errorlog.txt"; // temporary path for potential error handling
unsigned long usbConnectionDelay = 1000; // ms to wait between attemtps to connect to usb drive
unsigned long lastConnectionAttempt = 0; // ms of last connection attempt
unsigned long usbConnectionTimeout = 10000; // ms to wait before stopping attempts to connect to usb drive
bool usbConnectionTimedout = false; // if usb connection has timed out
bool usbConnected = false; // if usb connection is present



// wifi server stuff

char ssid[] = "SMV_UMASS";        // your network SSID (name)
char pass[] = "TELEM_ACCESS";    // your network password (use for WPA, or use as key for WEP)
///int keyIndex = 0;                 // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;

int connection_number = 0;

WiFiServer server(80);

bool wifiConnected = false; // if properly connected to wifi
unsigned long wifiCheckDelay = 100; // ms to wait between checking wifi connection
unsigned long wifiCheckTime = 0; // time of last wifi connection check
unsigned long wifiStartupDelay = 100; // ms to wait for wifi connection to start up
unsigned long wifiStartupTime = 0; // time of wifi connection start

unsigned long WiFiTimeout = 300; // milliseconds to wait for client to start sending data
// no effect:
const int maxClients = 20; // arbitrary number specifying the maximum number of clients
WiFiClient connectedClients[maxClients];




// testing vars
unsigned long tempTimeStamp1 = 0;
unsigned long tempTimeStamp2 = 0;
unsigned long loopNuber = 0;


void setup() {
  // initialize Dual core
  if (RPC.cpu_id() == CM7_CPUID) {
    blink(LEDB, 100); //blink blue LED (M7 core)
  } else {
    blink(LEDG, 100); //blink green LED (M4 core)
  }
  // RPC.begin();
  RPC.bind("updateReady", speedUpdate);

  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  Serial1.begin(115200);
  // pinMode(sensorPin, INPUT);
  pinMode(burnLED, OUTPUT);
  pinMode(coastLED, OUTPUT);
  analogWrite(A3, 0);
  lcd.begin(16, 2);
  Serial.println("Serial initialized");

  pinMode(3, OUTPUT);


  
  pinMode(LEDB, OUTPUT);
  pinMode(LEDG, OUTPUT);
  // pinMode(LEDR, OUTPUT);


  // initialize usb
  msd.connect();



  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    digitalWrite(LEDR, LOW);
    while (true);
  }


}

// the loop routine runs over and over again forever:

void loop() {
  // loop time tracker
  loopNuber++;
  if ((millis()-tempTimeStamp1)>=100) {
    logPrint("looped! (spam lol)\nloop #" + String(loopNuber) + "\nmillis: " + String(millis()) + "\nloop millis:" + String(millis()-tempTimeStamp1));
  }
  tempTimeStamp1 = millis();
  // for identifying the loops that take a long time to run



  String buffer = "";
  while (RPC.available()) {
    buffer += (char)RPC.read();  // Fill the buffer with characters
  }
  if (buffer.length() > 0) {
    logPrint("Message from core 2:\n"+ buffer);
  }



  // usb setup (constant to allow connection/disconnection of USB device)
  if(!msd.connected() && !usbConnectionTimedout && ((millis()-lastConnectionAttempt)>=usbConnectionDelay)){
    // Serial.println("Attempting to connect to usb device");
    msd.connect();
    if(usbConnected){
      usb.unmount();
    }
    usbConnected = false; // maybe if I set this here it'll be able to handle the usb drive getting unplugged
  } else if (msd.connected() && !usbConnected){  
    unsigned long usbTimeStart = millis();
    // Serial.println("usb connected");
    USB_Setup();
    usbConnected = true;
    logPrint("USB setup took "+String(millis()-usbTimeStart) + "ms to complete");
    // Serial.println("testing usb logging time");
    usbTimeStart = millis();
    logPrint("Testing usb logging time, starting at "+String(usbTimeStart)+"ms");
    logPrint("test took " + String(millis()-usbTimeStart) + "ms to complete");
  }

  
  
  // wifi check
  if (!wifiConnected && (status != WL_CONNECTED)) {
    // Serial.print("Attempting to connect to SSID: ");
    logPrint("Attempting to connect to SSID: ");
    // Serial.println(ssid);
    logPrint(String(ssid));
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    if (status == WL_CONNECTED){
      // Serial.println("Sucess!\nWiFi connection starting");
      logPrint("Sucess!\n\rWiFi connection starting");
      digitalWrite(86,HIGH);
      wifiStartupTime = micros();
    } else {
      // Serial.println("Attempt failed");
      logPrint("Attempt failed");
    }
    wifiConnected = false;
  } else if ((status == WL_CONNECTED) && ((micros()-wifiStartupTime)>=wifiStartupDelay) && !wifiConnected){
    
    server.begin();
    // you're connected now, so print out the status:
    // Serial.println("wifi connected");
    logPrint("wifi connected");
    print_wifi_status();
    wifiConnected = true;


  }

  if (status != WiFi.status()){
    status = WiFi.status();
    // Serial.println("Wifi Status: ");
    // Serial.println(status);
    logPrint("Wifi status changed, new status: " + String(status));
    if (status == 5){
      logPrint("so we've disconnected!");
      digitalWrite(86,LOW);
      wifiConnected = false;
    }
  }


  
  // vehicleSpeed = RPC.call("getSpeed").as<double>();
  // Serial.println("speed: " + String(vehicleSpeed));

  if (speedUpdateReady){
    // Serial.println("speed update noticed!");
    vehicleSpeed = RPC.call("getSpeed").as<double>();
    averageSpeed = RPC.call("getAverageSpeed").as<double>();
    wheelRPM = RPC.call("getWheelRPM").as<double>();
    
    distance = RPC.call("getDistance").as<double>();
    speedUpdateReady = false;
    if ((millis()-lastRead) >= readDelay){
      // Serial.println(millis());
      // Serial.println(lastRead);
      lastRead = millis();

      logPrint("Speed update recieved, current speed: " + String(vehicleSpeed, 5)+ "mph; average speed: " + String(averageSpeed,5) + "mph; wheel RPM: " + String(wheelRPM,2) + "rpm; distance traveled:" + String(distance,10)+"miles");
    }
    // logPrint("Speed update recieved, current speed: " + String(vehicleSpeed)+ "mph; average speed: " + String(averageSpeed) + "mph; wheel RPM: " + String(wheelRPM) + "rpm");

    //LCD Display outputs
    lcd.setCursor(0, 0);
    lcd.print(vehicleSpeed);
    lcd.print(F("MPH "));
    lcd.print(distance);
    lcd.print(F("miles"));
    lcd.setCursor(0, 1);
    lcd.print(F("AV:"));
    lcd.print(averageSpeed);
    // lcd.print(F("MPH"));
    lcd.print(F("MPH Lap"));
    lcd.print(lap);
    
  }


  // restart the LCD display regularly
  if (LastReset - millis() > ResetDelay) {
    LastReset = millis();
    lcd.begin(16, 2);

    //LCD Display outputs
    lcd.setCursor(0, 0);
    lcd.print(vehicleSpeed);
    lcd.print(F("MPH "));
    lcd.print(distance);
    lcd.print(F("miles"));
    lcd.setCursor(0, 1);
    lcd.print(F("AV:"));
    lcd.print(averageSpeed);
    // lcd.print(F("MPH"));
    lcd.print(F("MPH Lap"));
    lcd.print(lap);
  }

  
  // attempt to respond to any incoming update requests
  if(wifiConnected){
    wifi_running();
  }

}

bool speedUpdate(){
  speedUpdateReady = true;
}



void blink(int led, int delaySeconds) {
  for (int i; i < 10; i++) {
    digitalWrite(led, LOW);
    delay(delaySeconds);
    digitalWrite(led, HIGH);
    delay(delaySeconds);
  }
  if(!RPC.begin()){
    Serial.println("Core 2 start failed");
  } else {
    Serial.println("Core 2 started");
  }
}


int logPrint(String to_log){
  unsigned long printTimer = millis();
  if(Serial){
    Serial.print("Attempting to print: \"");
    Serial.print(to_log);
    Serial.print("\" to ");
    Serial.println(filePath);
    
  }
  // Serial.print("Attempting to print: \"");
  // Serial.print(to_log);
  // Serial.print("\" to ");
  // Serial.println(filePath);
  if (!usbConnected || !msd.connected()){
    if(Serial){
      Serial.println("But no USB drive is connected!");
      Serial.println("\nTook "+ String(millis()-printTimer)+ "ms to fail to log to usb");
    }
    return 1;
  }
  
  digitalWrite(88,LOW);
  
  mbed::fs_file_t file;
  struct dirent *ent;
  int dirIndex = 0;
  int res = 0;
  int err = 0;
  FILE *f = fopen(filePath.c_str(), "a+");
  // file has been opened, writing is below

  err = fprintf(f, "\n%ums :", millis());
  if (err < 0) {
    Serial.println("Open" + filePath);
    Serial.println("Fail :(");
    error("error: %s (%d)\n", strerror(errno), -errno);
    digitalWrite(86,LOW);
  }


  const int buffer_length = 256;
  char buf[buffer_length];
  int characters_printed = 0;
  String current_substring = "";
  
  while (characters_printed < to_log.length()) {
    current_substring = to_log.substring(characters_printed, characters_printed + min(to_log.length()-characters_printed,buffer_length));
    fflush(stdout);
    current_substring.toCharArray(buf, buffer_length);
    err = fprintf(f, "%s", buf);
    if (err < 0) {
      Serial.println("Fail :(");
      error("error: %s (%d)\n", strerror(errno), -errno);
      digitalWrite(86,LOW);
    }
    characters_printed+=buffer_length;
  }

  // end of writing to file
  err = fclose(f);
  if (err < 0) {
    Serial.println("File closing");
    fflush(stdout);
    Serial.print("fclose error:");
    Serial.print(strerror(errno));
    Serial.print(" (");
    Serial.print(-errno);
    Serial.print(")");
    digitalWrite(86,LOW);
  }
  
  digitalWrite(88,HIGH);
  if (Serial){
    Serial.println("\nTook "+ String(millis()-printTimer)+ "ms to log to usb");
  }
  return 0;
}

void USB_Setup(){
  
  // Serial.println("Mounting USB device...");
  int err =  usb.mount(&msd);
  if (err) {
    Serial.print("Error mounting USB device ");
    Serial.println(err);
    digitalWrite(86,LOW);
    while (1);
  }
  // Serial.print("read done ");

  char buf[256];
  // Display the root directory
  // Serial.print("Opening the runs directory... ");
  DIR* d = opendir("/SMV_STUFF/Runs/");
  // Serial.println(!d ? "Fail :(" : "Done");
  if (!d) {
      snprintf(buf, sizeof(buf), "error: %s (%d)\r\n", strerror(errno), -errno);
      Serial.print(buf);
      digitalWrite(86,LOW);
  }
  // Serial.println("done.");

  // Serial.println("runs directory:");
  unsigned int count { 0 };
  while (true) {
      struct dirent* e = readdir(d);
      if (!e) {
          break;
      }
      count++;
      snprintf(buf, sizeof(buf), "    %s", e->d_name);
      // Serial.print(buf);
      fileNameString = e->d_name;
      fileNameSubstring = fileNameString.substr(0,3);
      if(fileNameSubstring.compare(Runs) == 0){
        // Serial.println(" is a run!");
        fileNameRunSubstring = fileNameString.substr(3,4);
        fileNameString.copy(fileNameRuns,4,4);
        runNumberTemp = 1000*(fileNameRuns[0]-'0')+100*(fileNameRuns[1]-'0')+10*(fileNameRuns[2]-'0')+(fileNameRuns[3]-'0');
        }
        if(runNumberTemp>runNumber){
          runNumber = runNumberTemp;
        }
      else {
        // Serial.println(" is not a run!");
      }
  }
  // Serial.print(count);
  // Serial.println(" files found!");
  // Serial.print("last run: ");
  // Serial.println(runNumber);



  snprintf(buf, sizeof(buf), "Closing the runs directory... ");
  // Serial.print(buf);
  fflush(stdout);
  err = closedir(d);
  snprintf(buf, sizeof(buf), "%s\r\n", (err < 0 ? "Fail :(" : "OK"));
  // Serial.print(buf);
  if (err < 0) {
      snprintf(buf, sizeof(buf), "error: %s (%d)\r\n", strerror(errno), -errno);
      digitalWrite(86,LOW);
      Serial.print(buf);
  }

  mbed::fs_file_t file;
  struct dirent *ent;
  int dirIndex = 0;
  int res = 0;
  filePath = "/SMV_STUFF/Runs/Run_";
  runNumber ++;
  int runNumber_len = String(runNumber).length();
  for(int i = 0; i < 4-runNumber_len; i++){
    filePath += '0';
  }
  filePath += String(runNumber) + ".txt";

  // Serial.println("Open" + filePath);
  FILE *f = fopen(filePath.c_str(), "w+");
  fflush(stdout);
  err = fprintf(f, "Starting run #%d | usb connected %ums past startup\n", runNumber, millis());
  if (err < 0) {
    digitalWrite(86,LOW);
    Serial.println("Fail :(");
    error("error: %s (%d)\n", strerror(errno), -errno);
  }

  // Serial.println("File closing");
  fflush(stdout);
  err = fclose(f);
  if (err < 0) {
    digitalWrite(86,LOW);
    Serial.print("fclose error:");
    Serial.print(strerror(errno));
    Serial.print(" (");
    Serial.print(-errno);
    Serial.print(")");
  } else {
    // Serial.println("File closed");
  }
}


void wifi_running(){
  // listen for incoming clients
    
  WiFiClient client = server.available();
  String client_msg = "";
  tempTimeStamp2 = millis();
  if (client) {
    digitalWrite(87, LOW);
    client_msg = "Recieved from ";
    client_msg += client.remoteIP().toString();
    client_msg += " on port ";
    client_msg += client.remotePort();
    client_msg += " at ";
    client_msg += WiFi.RSSI();
    client_msg += "dBm:\n";
    // Serial.println("new client");
    // an HTTP request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (millis()-tempTimeStamp2>=WiFiTimeout){
        client.stop();
        logPrint("client connection ended after waiting "+ String(millis()-tempTimeStamp2) + "ms for data ");
        break; // timeout after set time
      }
      if (client.available()) {
        tempTimeStamp2 = millis();
        char c = client.read();
        client_msg += (c);
        // Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // Serial.print(client_msg);
          // logPrint(client_msg);
          // client_msg = "";
          // send a standard HTTP response header
          // client.println("HTTP/1.1 200 OK");
          // client.println("Content-Type: text/html");
          // client.println("Connection: close");  // the connection will be closed after completion of the response
          // client.println("Refresh: 5.0");  // refresh the page automatically every 5 sec
          // client.println();
          // client.println("<!DOCTYPE HTML>");
          // client.println("<html><head></head>");
          // client.println("<html><head><style>body {inline-size: inherit;block-size: inherit;padding: 0;margin: 0;}div { font-size: 50vmin;display: flex;align-items: center;justify-content: center;flex: 0 0 100vb;block-size: 100vb;}</style></head><body><div>");
          // output the value of each analog input pin
          
          // long rssi = WiFi.RSSI();
          // client.print("signal strength (RSSI):");
          // client.print(rssi);
          // client.println(" dBm");

          // client.print("#");
          // client.println(connection_number);
          // Serial.print("Connection #");
          // Serial.println(connection_number);
          // connection_number++;

          // client.println("<br />");
          // for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
          //   int sensorReading = analogRead(analogChannel);
          //   client.print("analog input ");
          //   client.print(analogChannel);
          //   client.print(" is ");
          //   client.print(sensorReading);
          //   client.println("<br />");
          // }
          // client.println("</html>");
          // client.println("</div></body></html>");
          /*
          <div id="demo">
          <h1>The XMLHttpRequest Object</h1>
          <button type="button" onclick="loadDoc()">Change Content</button>
          </div>
          
          <div id=\"demo\"><h1>The XMLHttpRequest Object</h1><button type=\"button\" onclick=\"loadDoc()\">Change Content</button></div>

          <script>
          function loadDoc() {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
              if (this.readyState == 4 && this.status == 200) {
                document.getElementById("demo").innerHTML =
                this.responseText;
              }
            };
            xhttp.open("GET", "ajax_info.txt", true);
            xhttp.send();
          }
          </script>

          <script>function loadDoc() {\nvar xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
              if (this.readyState == 4 && this.status == 200) {
                document.getElementById("demo").innerHTML =
                this.responseText;
              }
            };
            xhttp.open("GET", "ajax_info.txt", true);
            xhttp.send();
          }
          </script>


          */
          // client.println("<body>");
          // client.println("<div id=\"demo\"><h1>The XMLHttpRequest Object</h1><button type=\"button\" onclick=\"loadDoc()\">Change Content</button></div>");
                    
          // client.println("<html><body><div id=\"demo\"><h1>The XMLHttpRequest Object</h1><button type=\"button\" onclick=\"loadDoc()\">Change Content</button></div><script>function loadDoc() {  var xhttp = new XMLHttpRequest();  xhttp.onreadystatechange = function() {   if (this.readyState == 4 && this.status == 200) {      ");
          // client.println("document.getElementById(\"demo\").innerHTML =      this.responseText;    }  };  xhttp.open(\"GET\", \"ajax_info.txt\", true);  xhttp.send();}</script></body></html>");
          // Serial.println("</div><script>function loadDoc() {  var xhttp = new XMLHttpRequest();  xhttp.onreadystatechange = function() {   if (this.readyState == 4 && this.status == 200) {      ");
          // Serial.println("document.getElementById(\"demo\").innerHTML =      this.responseText;    }  };  xhttp.open(\"GET\", \"ajax_info.txt\", true);  xhttp.send();}</script></body></html>");
          // client.println("</body></html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // analyze recieved message.
    if(currentLineIsBlank){
      tempTimeStamp2 = millis()-tempTimeStamp2;
      // Serial.print(client_msg);
      parse_client_response(client_msg, client);
      // logPrint("client took " + String(tempTimeStamp2) + "ms to transmit request");
      logPrint(client_msg);
      
      String buffer = "";
      while (RPC.available()) {
        buffer += (char)RPC.read();  // Fill the buffer with characters
      }
      if (buffer.length() > 0) {
        Serial.print(buffer);
      }
      // vehicleSpeed = RPC.call("getSpeed").as<double>();
      // averageSpeed = RPC.call("getAverageSpeed").as<double>();
      // wheelRPM = RPC.call("getWheelRPM").as<double>();
      
      // distance = RPC.call("getDistance").as<double>();
      // logPrint("Speed update recieved, current speed: " + String(vehicleSpeed)+ "mph; average speed: " + String(averageSpeed) + "mph; wheel RPM: " + String(wheelRPM) + "rpm; distance traveled:" + String(distance)+"miles");
      tempTimeStamp2 = millis();
      client_msg = "";
                
    }
    // give the web browser time to receive the data
    // delay(1);

    // // close the connection:
    // client.stop();
    // Serial.println("client disconnected");
    // Serial.println("client communication took " + String(millis()-tempTimeStamp2) + "ms of arduino time");
    // logPrint("client communication took " + String(millis()-tempTimeStamp2) + "ms of arduino time");
    
    digitalWrite(87, HIGH);
  }
}

void print_wifi_status() {
  // print the SSID of the network you're attached to:
  // Serial.print("SSID: ");
  logPrint("SSID: " + String(WiFi.SSID()));
  // Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  logPrint("IP Address: " + (ip.toString()));
  // Serial.print("IP Address: ");
  // Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  // Serial.print("signal strength (RSSI):");
  // Serial.print(rssi);
  // Serial.println(" dBm");
  logPrint("signal strength (RSSI): " + String(rssi)+ " dBm");
}

void parse_client_response(String client_msg, WiFiClient client){
  // Serial.print("client_msg:");
  // Serial.println(client_msg);
  int firstNewLine = client_msg.indexOf("\n"); 
  int secondNewLine = client_msg.indexOf("\n",firstNewLine+1);
  String split_msg = client_msg.substring(firstNewLine, secondNewLine); 
  int firstSpace = split_msg.indexOf(" ");
  int lastSpace = split_msg.lastIndexOf(" ");
  String requestedAddress = split_msg.substring(firstSpace+1,lastSpace);
  // Serial.print("first new line: ");
  // Serial.println(firstNewLine);
  // Serial.print("second new line: ");
  // Serial.println(secondNewLine);
  // Serial.print("first space: ");
  // Serial.println(firstSpace);
  // Serial.print("last space: ");
  // Serial.println(lastSpace);
  // Serial.print("split_msg: ");
  // Serial.println(split_msg);
  // Serial.print("requested address:");
  // Serial.println(requestedAddress);

  // respond to request
  unsigned long responseTimer = millis();
  if(requestedAddress == "/driver_info.json"){
    String responseString = "{\"speed\":" + String(vehicleSpeed/100,4) +"}"; 
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: keep-alive");  // the connection will be closed after completion of the response
    client.println("Content-Length: " + String(responseString.length()));
    client.println("Keep-Alive: timeout=" + String(WiFiTimeout)+", max=200");
    client.println();
    client.println(responseString);
    
  } else if (requestedAddress == "/"){
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: keep-alive");  // the connection will be closed after completion of the response
    client.println();
    client.println("<!DOCTYPE HTML>");
    client.println("<body>");

    // // simple website

    // client.println("<div id=\"demo\"><h1>The XMLHttpRequest Object</h1><button type=\"button\" onclick=\"loadDoc()\">Change Content</button></div>");

    
    // client.println("</div><script>function loadDoc() {  var xhttp = new XMLHttpRequest();  xhttp.onreadystatechange = function() {   if (this.readyState == 4 && this.status == 200) {      ");
    // client.println("document.getElementById(\"demo\").innerHTML =      this.responseText;    }  };  xhttp.open(\"GET\", \"driver_info.txt\", true);  xhttp.send();}</script></body></html>");


    // client.println("<div id=\"demo\"><h1>The XMLHttpRequest Object</h1><button type=\"button\" onclick=\"loadDoc()\">Change Content</button></div><div id=\"speed\"></div>");

    // client.println("<script>");
    // client.println("function loadDoc() {var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {");
    // client.println("document.getElementById(\"demo\").innerHTML =this.responseText;var obj = JSON.parse(this.responseText);var value = obj.speed;");
    // client.println("document.getElementById(\"speed\").innerHTML = value;}};xhttp.open(\"GET\", \"driver_info.txt\", true);xhttp.send();}");

    // client.println("var timedEvent = setInterval(function(){loadDoc();},1000);function outputUpdate(){clearInterval(timedEvent); timedEvent = setInterval(function(){loadDoc()},1000);}");
    // client.println("</script>");

    // client.println("</body></html>");

    //kahn website
    client.println("<html>");
    client.println("<head><title>CSS-only Responsive Gauge</title><meta name=\"viewport\" contents=\"width=device-width, initial-scale=1.0\"><meta charset=\"utf-8\"></head>");
    client.println("<body>");
    client.println("<div class=\"gauge\"><div class=\"gauge__body\"><div class=\"gauge__fill\"><div></div></div><div class=\"gauge__cover\"></div>");

    client.println("<style>");
    client.println(".gauge {width: 100%;max-width: 250px;font-family: \"Roboto\", sans-serif;font-size: 30vw;color: #004033;}");
    
    client.println(".gauge__body {width: 98%;height: 0;padding-bottom: 45%;background: #b4c0be;position: absolute;border-top-left-radius: 100% 200%;border-top-right-radius: 100% 200%;overflow: hidden;}");
    
    client.println(".gauge__fill {position: absolute;top: 100%;left: -2%;width: 100%;height: 110%;background: #009578;transform-origin: 51% 0%;transform: rotate(0turn);transition: transform 0.2s ease-out;}");
    
    client.println(".gauge__cover {width: 75%;height: 150%;background: #ffffff;border-radius: 50%;position: absolute;top: 25%;left: 50%;transform: translateX(-50%);");
    client.println("display: flex;align-items: center;justify-content: center;padding-bottom: 25%;box-sizing: border-box;}");
    client.println("</style>");
    
    client.println("<script>const gaugeElement = document.querySelector(\".gauge\");");
    
    client.println("function setGaugeValue(gauge, value) {if (value < 0) {value = 0;}if (value > 0.4) {value = 0.4;}");
    client.println("gauge.querySelector(\".gauge__fill\").style.transform = `rotate(${value / 0.8}turn)`;");
    client.println("gauge.querySelector(\".gauge__cover\").textContent = `${Math.round(value * 100)}`;}");
    
    client.println("setGaugeValue(gaugeElement, 0.4);");
    
    client.println("function loadDoc(){var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200){");
    client.println("var obj = JSON.parse(this.responseText);var gague = document.querySelector(\".gauge\");var value = obj.speed;setGaugeValue(gague, value);");
    client.println("}};xhttp.open('GET','/driver_info.json',true); xhttp.send();}");
    
    client.println("var timedEvent = setInterval(function(){loadDoc();},2000);function outputUpdate(){clearInterval(timedEvent); timedEvent = setInterval(function(){loadDoc()},2000);}");
    client.println("</script>");

    client.println("</body></html>");

    /*
    
    <html>
    <head><title>CSS-only Responsive Gauge</title><meta name=\"viewport\" contents=\"width=device-width, initial-scale=1.0\"><meta charset=\"utf-8\"></head>
    <body>
    <div class=\"gauge\"><div class=\"gauge__body\"><div class=\"gauge__fill\"><div></div></div><div class=\"gauge__cover\"></div>

    <style>
    .gauge {width: 100%;max-width: 250px;font-family: \"Roboto\", sans-serif;font-size: 30vw;color: #004033;}

    .gauge__body {width: 98%;height: 0;padding-bottom: 45%;background: #b4c0be;position: absolute;border-top-left-radius: 100% 200%;border-top-right-radius: 100% 200%;overflow: hidden;}

    .gauge__fill {position: absolute;top: 100%;left: -2%;width: 100%;height: 110%;background: #009578;transform-origin: 51% 0%;transform: rotate(0turn);transition: transform 0.2s ease-out;}

    .gauge__cover {width: 75%;height: 150%;background: #ffffff;border-radius: 50%;position: absolute;top: 25%;left: 50%;transform: translateX(-50%);
    display: flex;align-items: center;justify-content: center;padding-bottom: 25%;box-sizing: border-box;}
    </style>

    <script>const gaugeElement = document.querySelector(\".gauge\");

    function setGaugeValue(gauge, value) {if (value < 0) {value = 0;}if (value > 0.4) {value = 0.4;}
    gauge.querySelector(\".gauge__fill\").style.transform = `rotate(${value / 0.8}turn)`;
    gauge.querySelector(\".gauge__cover\").textContent = `${Math.round(value * 100)}`;}

    setGaugeValue(gaugeElement, 0.4);

    function loadDoc(){var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200){
    var obj = JSON.parse(this.responseText);var gague = document.querySelector(\".gauge\");var value = obj.speed;setGaugeValue(gague, value);
    }};xhttp.open('GET','/driver_info.json',true); xhttp.send();}

    var timedEvent = setInterval(function(){loadDoc();},1000);function outputUpdate(){clearInterval(timedEvent); timedEvent = setInterval(function(){loadDoc()},1000);}
    </script>
        


    // basic website:
    <div id=\"demo\"><h1>The XMLHttpRequest Object</h1><button type=\"button\" onclick=\"loadDoc()\">Change Content</button></div><div id=\"speed\"></div>
        
    <script>
    function loadDoc() {var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {
    var obj = JSON.parse(this.responseText);var value = obj.data[0].dataValue;document.getElementById(\"speed\").innerHTML = value;
    document.getElementById(\"demo\").innerHTML =this.responseText;}};xhttp.open(\"GET\", \"driver_info.txt\", true);xhttp.send();}

    var timedEvent = setInterval(function(){loadDoc();},2000);function outputUpdate(){clearInterval(timedEvent); timedEvent = setInterval(function(){loadDoc()},2000);}
    </script> 

    */

  }
  responseTimer = millis()-responseTimer;
  // Serial.println("Server response took " +String(responseTimer) + "ms");
  // logPrint("Server response took " +String(responseTimer) + "ms");

}



