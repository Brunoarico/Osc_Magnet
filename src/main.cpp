#include <Arduino.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <OSCMessage.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>

#define ARRAY_SIZE(array) ((sizeof(array)) / (sizeof(array[0])))
#define MAGNETS 8
#define NAME "Eletroima"
#define PORT 5005
#define RESOLUTION 10

#define PWM_FREQ 5000
#define PWM_RES 8

const char* ssid = "";
const char* pass = "";


byte rele[] = {13, 14, 15, 16, 17, 18, 19, 21};

unsigned long previousMillis = 0;
unsigned long interval = 30000;

int states[] = {0, 0, 0, 0, 0, 0, 0, 0};


bool received = false;

WiFiUDP Udp;
OSCErrorCode error;

IPAddress local_IP(192, 168, 15, 253);
IPAddress gateway(192, 168, 15, 1);
IPAddress subnet(255, 255, 0, 0);

void initializeBridges() {
    Serial.println(ARRAY_SIZE(rele));

    for (int i = 0; i < ARRAY_SIZE(rele); i++) {
        ledcSetup(i, PWM_FREQ, PWM_RES);
        pinMode(rele[i], OUTPUT);
        ledcAttachPin(rele[i], i);
        states[i] = 0;
    }
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
    if (!WiFi.config(local_IP, gateway, subnet)) {
        Serial.println("STA Failed to configure");
    }
    WiFi.begin(ssid, pass);
    // Udp.begin(PORT);
}

void initWiFi() {
    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(NAME);
    if (!WiFi.config(local_IP, gateway, subnet)) {
        Serial.println("STA Failed to configure");
    }
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }
    Udp.begin(PORT);
}

void processMagnet(OSCMessage& msg) {
    int power = 0;
    int index = 0;
    if (msg.isInt(0))
        power = msg.getInt(0);
    else if (msg.isFloat(0))
        power = msg.getFloat(0);
    else if (msg.isString(0)) {
        char pwr[8];
        msg.getString(1, pwr);
        char pos[8];
        msg.getString(0, pos);
        sscanf(pwr, "%d", &power);
        sscanf(pos, "%d", &index);
    }
    if (index >= 0 && index < ARRAY_SIZE(rele)) {
        if (power >= 0 && power <= 255) {
            ledcWrite(index, power);
            Serial.println("Configured Magnet " + String(rele[index]) + ": Power: " + power);
        } else
            Serial.println("Power out of range");

    } else
        Serial.println("Magnet position out of range");
}

void setPower() {
    for (int i = 0; i < ARRAY_SIZE(rele); i++) {
        Serial.println(String(rele[i]) + " " + (states[i]));
        ledcWrite(i, states[i]);
    }
    received = false;
}

void receiveMsg() {
    OSCMessage msg;
    int size = Udp.parsePacket();
    if (size > 0) {
        while (size--) msg.fill(Udp.read());
        if (!msg.hasError()) {
            msg.dispatch("/ima", processMagnet);
        } else {
            error = msg.getError();
            Serial.print("error: ");
            Serial.println(error);
        }
    }
}


/* Esta função inicializa o wifi, caso o esp não consiga conexão com nenhum roteador ele ira subir um hotspot para configuração 
automaticamente. O nome do wifi correspondente ao esp sera "eletroima" e a senha "imaimaima". */

/* Caso o esp consiga conexão com algum roteador ele ira se conectar automaticamente e o endereço de ip sera escrito na serial*/
/* Uma vez no conectado a um wifi, caso o roteador permita o protocolo mDNS o esp pode ser invocado pelo nome eletroima.local */
void initAutoWifi() {
    WiFiManager wm;

    bool res;
    wm.setSTAStaticIPConfig(IPAddress(192, 168, 15, 253), IPAddress(192, 168, 15, 1), IPAddress(255, 255, 255, 0));  // set static ip,gw,sn
    wm.setShowStaticFields(true);                                                                                  // force show static ip fields
    res = wm.autoConnect(NAME, "imaimaima");                                                                // password protected ap

    if (!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } else {
        // if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
        if (!MDNS.begin("eletroima")) {
            Serial.println("Error starting mDNS");
            return;
        }
         Udp.begin(PORT);
    }
}

void setup() {
    Serial.begin(115200); // baudrate da conexão serial
    initializeBridges();
    initAutoWifi(); // Inicializa o wifi 
}

void loop() {
    ArduinoOTA.handle();
    receiveMsg();
}