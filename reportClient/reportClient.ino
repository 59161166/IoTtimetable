//************************************************************
// this is a simple example that uses the painlessMesh library to
// setup a node that logs to a central logging node
// The logServer example shows how to configure the central logging nodes
//************************************************************
#include "painlessMesh.h"
 
#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555
 
Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
uint64_t chipid;  

#include <TFT_eSPI.h> // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
unsigned long drawTime = 0;
int pressCount = 0;
int oldCount = 0;
int oldReportSelect = 0;
int x[] = {5, 5, 250, 250};
int y[] = {50, 180, 50, 180};
int wid = 235;
int hei[] = {125,130,125,130};
int datum[] = {TR_DATUM,TR_DATUM,TL_DATUM,TL_DATUM};
String choice[] = {"MICROPHONE","LIGHT","COMPUTER","NETWORK"};
int txtX[] = {230, 230, 270, 270};
int txtY[] = {80, 220, 80, 220};
int currentPage = 0;
boolean isRegister = false;
size_t logServerId = 0;
boolean isSelectedReport = false;
String sub1[]={"","","","",""};
String sub2[]={"","","","",""};
boolean isGetSchedule=false;
Task myLoggingTask(10000, TASK_FOREVER, []() {
        DynamicJsonDocument jsonBuffer(1024);
        JsonObject msg = jsonBuffer.to<JsonObject>();
    msg["topic"] = "register";
    msg["chipId"] = ESP.getEfuseMac();
    msg["nodeId"] = mesh.getNodeId();
 
    String str;
    serializeJson(msg, str);
    if (logServerId == 0)
        mesh.sendBroadcast(str);
    else
        mesh.sendSingle(logServerId, str);
    serializeJson(msg, Serial);
    Serial.printf("\n");
});
 
 
Task getSchedule(10000, TASK_FOREVER, []() {
DynamicJsonDocument jsonBuffer(512);
        JsonObject msg = jsonBuffer.to<JsonObject>();
        msg["topic"] = "getSchedule";
        msg["chipId"] = ESP.getEfuseMac();
        msg["nodeId"] = mesh.getNodeId();
        String str;
        serializeJson(msg, str);
        if (logServerId == 0)
            mesh.sendBroadcast(str);
        else
            mesh.sendSingle(logServerId, str);
});
 
 
void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  pinMode(22,INPUT);
  pinMode(34,INPUT);
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );
 
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);
 
  // Add the task to the your scheduler
  userScheduler.addTask(myLoggingTask);
  myLoggingTask.enable();
}
 
void loop() {
  // it will run the user scheduler as well
  mesh.update();
  int reportSelect = map(analogRead(34),0,4095,0,3);
  int buttonPress = digitalRead(22);
  int reportPress = digitalRead(23);
  Serial.println(reportPress);
  if(buttonPress == 1){
    pressCount++;
  }
   if(oldCount!=pressCount){
    tft.fillScreen(TFT_LIGHTGREY);
      if(pressCount%2 == 0){
          timeTablePage();
          currentPage = 0;
          isSelectedReport = false;
      } 
      else if(pressCount%2 == 1){
          reportPage();
          currentPage = 1;
      }
      oldCount = pressCount;
   }
   if(oldReportSelect != reportSelect && currentPage == 1){
    tft.fillScreen(TFT_LIGHTGREY);
      reportPage();
      changeReportSelect(reportSelect);
      oldReportSelect = reportSelect;
      isSelectedReport = true;
   }
   if(reportPress==1 && currentPage == 1 && isRegister == true && isSelectedReport == true){
       sendReport(reportSelect);
       tft.fillScreen(TFT_LIGHTGREY);
       header("Problem is Reported");
       delay(7000);
       tft.fillScreen(TFT_LIGHTGREY);
       timeTablePage();
       currentPage = 0;
   }
  delay(1000);
}

void header(const char *string)
{
  int padding = tft.textWidth("XXXXXXXXXXXXXXXXXXXXXXXXXXX", 4);
  tft.setTextPadding(padding);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, TFT_ORANGE);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(string, 240, 10, 2);
}

void reportPage(){
  header("Report Problems");
  tft.fillRect(5,50,235,125,TFT_GREEN);
  tft.fillRect(5,180,235,130,TFT_GREEN);
  tft.fillRect(250,50,235,125,TFT_GREEN);
  tft.fillRect(250,180,235,130,TFT_GREEN);

  createReportText(TR_DATUM,"MICROPHONE",230,80);
  createReportText(TR_DATUM,"LIGHT",230,220);
  createReportText(TL_DATUM,"COMPUTER",270,80);
  createReportText(TL_DATUM,"NETWORK",270,220);
}

void createReportText(int datum,String text,int x,int y){
  tft.setTextSize(0);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.setTextDatum(datum);
  tft.drawString(text, x, y, 4);
}

void timeTablePage(){
  header("Time Table");
  tft.fillRect(5,50,465,130,TFT_GREEN);
  tft.fillRect(5,185,465,130,TFT_BLUE);
  tft.setTextSize(0);
  createTimetableText(TFT_GREEN,TC_DATUM,"NOW STUDYING", 250, 60,2);
  createTimetableText(TFT_BLUE,TC_DATUM,"NEXT SUBJECT", 250, 190,2);
  if(isGetSchedule==true){
    createTimetableText(TFT_GREEN,TC_DATUM,sub1[0]+" ("+sub1[1]+")", 250, 90,2);
    createTimetableText(TFT_GREEN,TC_DATUM,sub1[2], 250, 110,2);
    createTimetableText(TFT_GREEN,TC_DATUM,sub1[3]+".00 - "+sub1[4]+".00", 250, 140,2);
    createTimetableText(TFT_BLUE,TC_DATUM,sub2[0]+" ("+sub2[1]+")", 250, 220,2);
    createTimetableText(TFT_BLUE,TC_DATUM,sub2[2], 250, 240,2);
    createTimetableText(TFT_BLUE,TC_DATUM,sub2[3]+".00 - "+sub2[4]+".00", 250, 260,2);
  }
}

void createTimetableText(int color,int datum,String text,int x,int y,int sizeT){
  tft.setTextSize(0);
  tft.setTextColor(TFT_BLACK, color);
  tft.setTextDatum(datum);
  tft.drawString(text, x, y, sizeT);
}

void changeReportSelect(int c){
  tft.fillRect(x[c],y[c],wid,hei[c],TFT_RED);
  tft.setTextSize(0);
  tft.setTextColor(TFT_BLACK, TFT_RED);
  tft.setTextDatum(datum[c]);
  tft.drawString(choice[c], txtX[c], txtY[c], 4);
  Serial.println(txtX[c]+" "+txtY[c]);
}

void sendReport(int reportType){
        DynamicJsonDocument jsonBuffer(512);
        JsonObject msg = jsonBuffer.to<JsonObject>();
        msg["topic"] = "sendReport";
        msg["value"] = choice[reportType];
        msg["chipId"] = ESP.getEfuseMac();
        msg["nodeId"] = mesh.getNodeId();
 
    String str;
    serializeJson(msg, str);
    if (logServerId == 0)
        mesh.sendBroadcast(str);
    else
        mesh.sendSingle(logServerId, str);
}

 
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("logClient: Received from %u msg=%s\n", from, msg.c_str());
  DynamicJsonDocument jsonBuffer(1024 + msg.length());
  DeserializationError error = deserializeJson(jsonBuffer, msg);
  if (error) {
    Serial.printf("DeserializationError\n");
    return;
  }
  JsonObject root = jsonBuffer.as<JsonObject>();
  //Serial.println(root.as<String>());
  if(root.containsKey("status")&&isRegister == false) {
    myLoggingTask.disable();
    userScheduler.addTask(getSchedule);
    getSchedule.enable();  
    isRegister = true;
  }
  if(root.containsKey("data")&&isRegister == true) {
          sub1[0]=root["data"][0]["course_code"].as<String>();
          sub1[1]=root["data"][0]["group"].as<String>();
          sub1[2]=root["data"][0]["title"].as<String>();
          sub1[3]=root["data"][0]["start_time"].as<String>();
          sub1[4]=root["data"][0]["end_time"].as<String>();

          sub2[0]=root["data"][1]["course_code"].as<String>();
          sub2[1]=root["data"][1]["group"].as<String>();
          sub2[2]=root["data"][1]["title"].as<String>();
          sub2[3]=root["data"][1]["start_time"].as<String>();
          sub2[4]=root["data"][1]["end_time"].as<String>();
          isGetSchedule=true;
  }
}
