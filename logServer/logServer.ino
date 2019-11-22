//************************************************************
// this is a simple example that uses the painlessMesh library to 
// setup a single node (this node) as a logging node
// The logClient example shows how to configure the other nodes
// to log to this server
//************************************************************
#include <QueueArray.h>
#include <HTTPClient.h>
#include "painlessMesh.h"
#include <asyncHTTPrequest.h>

asyncHTTPrequest request;
QueueArray <String> queueUri;
QueueArray <int> queueNodeId;

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555
size_t logServerId = 0;
Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
// Prototype
void receivedCallback( uint32_t from, String &msg );


// Send my ID every 10 seconds to inform others
Task logServerTask(10000, TASK_FOREVER, []() {
#if ARDUINOJSON_VERSION_MAJOR==6
        DynamicJsonDocument jsonBuffer(1024);
        JsonObject msg = jsonBuffer.to<JsonObject>();
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& msg = jsonBuffer.createObject();
#endif
    msg["topic"] = "logServer";
    msg["nodeId"] = mesh.getNodeId();

    String str;
    serializeJson(msg, str);
    mesh.sendBroadcast(str);

    // log to serial

    serializeJson(msg, Serial);
    Serial.printf("\n");
});

Task qTask(1000, TASK_FOREVER, []() {
   if(queueUri.count()>0){
    String uri = queueUri.peek();
    Serial.println(uri);
    if(request.readyState() == 0 || request.readyState() == 4){
      Serial.println("jimmyyyyyyyyy");
        request.open("GET", uri.c_str());
        request.send();
    }

  }
});

#define   STATION_SSID     "nongHum"
#define   STATION_PASSWORD "qwerty123456"
#define HOSTNAME "HTTP_Bridge"

String schedule = "";
void setup() {
  Serial.begin(115200);
        
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE | DEBUG ); // all types on
  //mesh.setDebugMsgTypes( ERROR | CONNECTION | SYNC | S_TIME );  // set before init() so that you can see startup messages
  mesh.setDebugMsgTypes( ERROR | CONNECTION | S_TIME );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(HOSTNAME);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  mesh.onNewConnection([](size_t nodeId) {
    Serial.printf("New Connection %u\n", nodeId);
  });

  mesh.onDroppedConnection([](size_t nodeId) {
    Serial.printf("Dropped Connection %u\n", nodeId);
  });

  // Add the task to the your scheduler
  userScheduler.addTask(logServerTask);
  userScheduler.addTask(qTask);
  logServerTask.enable();
   qTask.enable();
  request.onReadyStateChange(requestCB);
  queueUri.setPrinter (Serial);
  queueNodeId.setPrinter (Serial);
}

void loop() {
  // it will run the user scheduler as well
  mesh.update(); 
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("logServer: Received from %u msg=%s\n", from, msg.c_str());
  DynamicJsonDocument jsonBuffer(msg.length()+512);
  DeserializationError error = deserializeJson(jsonBuffer, msg);
  if (error) {
    Serial.printf("DeserializationError\n");
    return;
  }
  JsonObject root = jsonBuffer.as<JsonObject>();
  //Serial.println(root);
  if (root.containsKey("topic")) {
      if (String("register").equals(root["topic"].as<String>())) {
            queueUri.push("http://electric-report.herokuapp.com/api/node/register?id="+root["chipId"].as<String>());
            queueNodeId.enqueue(root["nodeId"]);
      }
      else if (String("getSchedule").equals(root["topic"].as<String>())) {
            queueUri.push("http://electric-report.herokuapp.com/api/node/schedule?id="+root["chipId"].as<String>());
            queueNodeId.enqueue(root["nodeId"]);
      }
      else if (String("sendReport").equals(root["topic"].as<String>())) {
            queueUri.push("http://electric-report.herokuapp.com/api/node/room-report?id="+root["chipId"].as<String>()+"&message="+root["value"].as<String>());
            queueNodeId.enqueue(root["nodeId"]);
      }
      else if (String("sendElectric").equals(root["topic"].as<String>())) {
            queueUri.push("http://electric-report.herokuapp.com/api/node/report?id="+root["chipId"].as<String>()+"&volt="+root["volt"].as<String>()+"&amp="+root["amp"].as<String>()+"&watt="+root["watt"].as<String>());
            queueNodeId.enqueue(root["nodeId"]);
      }
      Serial.printf("Handled from %u msg=%s\n", from, msg.c_str());
  }
}
bool runstate = true;

void sendRequest(String uri){
  
    if(request.readyState() == 0 || request.readyState() == 4){
      Serial.println("jimmxxxx");
      Serial.println(uri.c_str());
        request.open("GET", uri.c_str());
        request.send();
    }
}

void requestCB(void* optParm, asyncHTTPrequest* request, int readyState){
    if(readyState== 0 || readyState == 4){
        int nodeId  = queueNodeId.peek();
        String res = request->responseText();
        Serial.println(res);
        mesh.sendSingle(nodeId, res);
        Serial.println(queueUri.pop());
        Serial.println(queueNodeId.pop());
        Serial.println();
        request->setDebug(false);
        runstate = true;
    }
}
