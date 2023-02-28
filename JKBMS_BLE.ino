#include <NimBLEDevice.h>
#include <WiFi.h>
#include "ThingSpeak.h"

const char* ssid = "DssVss247";   // your network SSID (name) 
const char* password = "35019997092441399430";   // your network password
#define WIFI_TIMEOUT_MS 20000 // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 30000 // Wait 30 seconds after a failed connection attempt

WiFiClient  client;

unsigned long myChannelNumberBMS = 4;
const char * myWriteAPIKeyBMS = "xxxx";
unsigned long myChannelNumberBatt = 3;
const char * myWriteAPIKeyBatt = "xxxx";


// Timer variables
unsigned long lastTimeThing = 0;
unsigned long timerDelayThing = 15000;
bool dataReal = false;


const char* Geraetename = "JK_B2A20S20P";  
static BLEUUID serviceUUID("ffe0"); // The remote service we wish to connect to.
static BLEUUID    charUUID("ffe1"); //ffe1 // The characteristic of the remote service we are interested in.
byte getdeviceInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x97, 0x00, 0xdf, 0x52, 0x88, 0x67, 0x9d, 0x0a, 0x09, 0x6b, 0x9a, 0xf6, 0x70, 0x9a, 0x17, 0xfd}; // Device Infos
byte getInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10}; 
//byte getInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x96, 0x00, 0x79, 0x62, 0x96, 0xed, 0xe3, 0xd0, 0x82, 0xa1, 0x9b, 0x5b, 0x3c, 0x9c, 0x4b, 0x5d}; 
//byte balancer_an[20] = {0xaa, 0x55, 0x90, 0xeb, 0x1f, 0x04, 0x01, 0x00, 0x00, 0x00, 0xd3, 0x27, 0x86, 0x3b, 0xe2, 0x12, 0x4d, 0xb0, 0xb6, 0x00};

unsigned long sendingtime = 0;
unsigned long bleScantime = 0;
unsigned long newdatalasttime = 0;
unsigned long ble_connection_time = 0;
unsigned long lastPrintTime = 0;

byte receivedBytes_main[320];
int frame = 0;
bool received_start = false;
bool received_start_frame = false;
bool received_complete = false;
bool new_data = false;
byte BLE_Scan_counter = 0;

//BMS Werte
#define MAX_PV_POWER 2500
#define MAX_BATT_VOLT 60
#define MIN_BATT_VOLT 40
#define MAX_CHARGE_CURRENT MAX_PV_POWER/MIN_BATT_VOLT
float cellVoltage[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float Average_Cell_Voltage = 0;
float Delta_Cell_Voltage = 0;
float Current_Balancer = 0;
float Battery_Voltage = 0;
float Battery_Power = 0;
float Charge_Current = 0;
float Battery_T1 = 0;
float Battery_T2 = 0;

float MOS_Temp = 0;
int Percent_Remain = 0;
float Percent_Remain_calc = 0;
float Capacity_Remain = 0;
float Nominal_Capacity = 0;
String Cycle_Count = "";
float Capacity_Cycle = 0;
uint32_t Uptime;
uint8_t sec, mi, hr, days;
float Balance_Curr = 0;
String charge = "off";
String discharge = "off";

bool debug_flg_full_log = true;
bool debug_flg = true;


void scanEndedCB(NimBLEScanResults results);

static NimBLERemoteCharacteristic* pRemoteCharacteristic;
static NimBLEAdvertisedDevice* myDevice;
static NimBLEClient* pClient;
static NimBLEScan* pScan;

static bool doConnect = false;
static bool ble_connected = false;
static uint32_t scanTime = 0; /** 0 = scan forever */

ThingSpeakClass ThingSpeak1;
ThingSpeakClass ThingSpeak2;

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.println("Connected");
        /** After connection we should change the parameters if we don't need fast response times.
         *  These settings are 150ms interval, 0 latency, 450ms timout.
         *  Timeout should be a multiple of the interval, minimum is 100ms.
         *  I find a multiple of 3-5 * the interval works best for quick response/reconnect.
         *  Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
         */
        pClient->updateConnParams(120,120,0,60);
    };

    void onDisconnect(NimBLEClient* pClient) {
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" Disconnected - Starting scan");
        NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
    };

    /** Called when the peripheral requests a change to the connection parameters.
     *  Return true to accept and apply them or false to reject and keep
     *  the currently used parameters. Default will return true.
     */
    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
        if(params->itvl_min < 24) { /** 1.25ms units */
            return false;
        } else if(params->itvl_max > 40) { /** 1.25ms units */
            return false;
        } else if(params->latency > 2) { /** Number of intervals allowed to skip */
            return false;
        } else if(params->supervision_timeout > 100) { /** 10ms units */
            return false;
        }

        return true;
    };

    /********************* Security handled here **********************
    ****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest(){
        Serial.println("Client Passkey Request");
        /** return the passkey to send to the server */
        return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key){
        Serial.print("The passkey YES/NO number: ");
        Serial.println(pass_key);
    /** Return false if passkeys don't match. */
        return true;
    };

    /** Pairing process complete, we can check the results in ble_gap_conn_desc */
    void onAuthenticationComplete(ble_gap_conn_desc* desc){
        if(!desc->sec_state.encrypted) {
            Serial.println("Encrypt connection failed - disconnecting");
            /** Find the client with the connection handle provided in desc */
            NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
            return;
        }
    };
};


/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {

  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    Serial.print("Advertised Device found: ");
    Serial.println(advertisedDevice->toString().c_str());
    //if(advertisedDevice->isAdvertisingService(NimBLEUUID("DEAD")))
    if(advertisedDevice->isAdvertisingService(serviceUUID))
    {
      Serial.println("Found Our Service");
      if (advertisedDevice->getName() == Geraetename)
      {
        Serial.println("Device Name Correct");                
        /** stop scan before connecting */
        NimBLEDevice::getScan()->stop();
        Serial.println("Scan Stopped");                
        //pScan->stop();
        /** Save the device reference in a global for the client to use*/
        //advDevice = advertisedDevice;
        myDevice = advertisedDevice;
        /** Ready to connect now */
        doConnect = true;
      }
    }
  };
};

static void notifyCB(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

  if(debug_flg_full_log) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    for (int i = 0; i < length; i++)  {
      Serial.print(pData[i],HEX);
      Serial.print(", ");
    }
    Serial.println("");
  }
  if(pData[0] == 0x55 && pData[1] == 0xAA && pData[2] == 0xEB && pData[3] == 0x90 && pData[4] == 0x02) {
    Serial.println("Daten anerkannt !");
    received_start = true;
    received_start_frame = true;
    received_complete = false;
    frame = 0;
    for (int i = 0; i < length; i++)  {
      receivedBytes_main[frame] = pData[i];
      frame++;
    }
  }

  if(received_start && !received_start_frame && !received_complete) {
//     Serial.println("Daten erweitert !");
    for (int i = 0; i < length; i++)  {
      receivedBytes_main[frame] = pData[i];
      frame++;
    }
  
    if(frame == 300) {
      Serial.println("New Data for Analyse Complete...");
      received_complete = true;
      received_start = false;
      new_data = true;
      BLE_Scan_counter = 0;    
    }
    if((frame > 300)) {
      
      Serial.println("Fehlerhafte Daten !!");     
      frame = 0;
      received_start = false;
      new_data = false;    
    }
    
  }
  received_start_frame = false;
}

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results){
    Serial.println("Scan Ended");
}


/** Create a single global instance of the callback class to be used by all clients */
static ClientCallbacks clientCB;


// /** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  //pClient = NimBLEDevice::getClientByPeerAddress(myDevice->getAddress());
  pClient->setClientCallbacks(&clientCB, false);
  delay(500);
  pClient->connect(myDevice, false);
  delay(500);
  Serial.println(" - Connected to server");  

  NimBLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if(pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if(pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCB);
    Serial.println("Notify the characteristic");
  }

  pRemoteCharacteristic->writeValue(getdeviceInfo, 20);
  sendingtime = millis();
  Serial.println("Sending device Info");

  ble_connected = true;
  doConnect = false;
  Serial.println("We are now connected to the BLE Server.");

  return true;
}


void setup (){
  Serial.begin(115200);
  Serial.println("Starting NimBLE Client");
  /** Initialize NimBLE, no device name spcified as we are not advertising */
  NimBLEDevice::init("");


  WiFi.mode(WIFI_STA);   

  //OTA
  // ArduinoOTA.setPort(3232);
  // ArduinoOTA.setHostname(OTA_Hostname);
  // ArduinoOTA.setPassword(OTA_Passwort);
  // // Password can be set with it's md5 value as well
  // // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");  
  // ota_call();
  // ArduinoOTA.begin();    

  ThingSpeak1.begin(client);  // Initialize ThingSpeak
  ThingSpeak2.begin(client);  // Initialize ThingSpeak

  /** Optional: set the transmit power, default is 3db */
#ifdef ESP_PLATFORM
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
#else
  NimBLEDevice::setPower(9); /** +9db */
#endif

  pClient = NimBLEDevice::createClient();
  Serial.println(" - Created client");

  /** create new scan */
  pScan = NimBLEDevice::getScan();

  /** create a callback that gets called when advertisers are found */
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

  /** Set scan interval (how often) and window (how long) in milliseconds */
  pScan->setInterval(45);
  pScan->setWindow(15);

  /** Active scan will gather scan response data from advertisers
    *  but will use more energy from both devices
    */
  pScan->setActiveScan(true);
  /** Start scanning for advertisers for the scan time specified (in seconds) 0 = forever
    *  Optional callback for when scanning stops.
    */
  pScan->start(scanTime, scanEndedCB);


}


void loop (){
  //ArduinoOTA.handle();
  if (doConnect == true) {
    if (!connectToServer()) {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      delay(500);
      ble_connected = false;
      doConnect = false;
      dataReal = false;
    }
  }
  if (ble_connected) {
    if(received_complete) {
      if(new_data) {
        Datenanalyse();
        
        newdatalasttime = millis();
      }
    }
  }
  if (dataReal == true) {
    if ((millis() - lastTimeThing) > timerDelayThing) {
  
      // Connect or reconnect to WiFi
      if(WiFi.status() != WL_CONNECTED){
        Serial.print("Attempting to connect");
        while(WiFi.status() != WL_CONNECTED){
          WiFi.begin(ssid, password); 
          delay(5000);   
          Serial.println("Restarting Wifi") ;
        } 
        Serial.println("\nConnected.");
      }

      // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
      // pieces of information in a channel.  Here, we write to field 1.
      ThingSpeak1.setField(1, Battery_Voltage);
      ThingSpeak1.setField(2, Percent_Remain_calc);
      ThingSpeak1.setField(3, Battery_Power);
      ThingSpeak1.setField(4, Charge_Current);
      ThingSpeak1.setStatus("OK");


      
      int x = ThingSpeak1.writeFields(myChannelNumberBMS, myWriteAPIKeyBMS);
      if(x == 200){
        Serial.println("Channel update successful.");
      }
      else{
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }
      ThingSpeak2.setField(1, cellVoltage[0]);
      ThingSpeak2.setField(2, cellVoltage[1]);
      ThingSpeak2.setField(3, cellVoltage[2]);
      ThingSpeak2.setField(4, cellVoltage[3]);
      ThingSpeak2.setField(5, cellVoltage[4]);
      ThingSpeak2.setField(6, cellVoltage[5]);
      ThingSpeak2.setField(7, cellVoltage[6]);
      ThingSpeak2.setField(8, cellVoltage[7]);

      x = ThingSpeak2.writeFields(myChannelNumberBatt, myWriteAPIKeyBatt);      
      dataReal = false;
      if(x == 200){
        Serial.println("Channel update successful.");
      }
      else{
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }
      lastTimeThing = millis();
    }
  }
  // BLE Get Device Data Trigger ...
  if(((millis() - sendingtime) > 500) && sendingtime != 0) { // millis() >  sendingtime + sendingtimer aktive_sending && 
    sendingtime = 0;
    Serial.println("gesendet!");
    //aktive_sending = false;
    pRemoteCharacteristic->writeValue(getInfo, 20);
  }

  // BLE connection is there, but no new data since 60 secs.
  if(!doConnect && ble_connected && (millis() >= (newdatalasttime + 60000))){//} && newdatalasttime != 0){
    ble_connected = false;
    delay(200);
    Serial.println("BLE-Disconnect/terminated");
    newdatalasttime = millis();
    pClient->disconnect();
  }

  //BLE not connected
  if((!ble_connected && !doConnect && (millis() - bleScantime) > 10000)) {

    Serial.println("BLE -> Reconnecting " + String(BLE_Scan_counter));
    bleScantime = millis();
    pScan->start(5, false);
    BLE_Scan_counter++;
  }  

  if(BLE_Scan_counter > 20) {
    delay(200);
    Serial.println("BLE isn´t receiving new Data form BMS... and no BLE reconnection possible, Reboot ESP...");
    ESP.restart();
  }
  if (millis() - 1000 > lastPrintTime) 
  {
    Serial.println("ble_connected = " + String(ble_connected));
    Serial.println("doConnect = " + String(doConnect));
    Serial.println("BLE_Scan_counter = " + String(BLE_Scan_counter));
    Serial.println("millis() = " + String(millis()));
    Serial.println("newdatalasttime + 60000 = " + String(newdatalasttime + 60000));
    Serial.println("bleScantime + 10000 = " + String(bleScantime + 10000));
    lastPrintTime = millis();
  }
}
