#include <string.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include <PubSubClient.h>

#define BUTTON_PIN D1

#define DEBUG_SERIAL true

#if DEBUG_SERIAL
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

ESP8266WebServer server(80);
Preferences prefs;
WiFiClient espClient;
PubSubClient client(espClient);

bool APMode = false;

String ssid = "";
String password = "";
String mqttBrokerUrl = "";
int mqttBrokerPort = 1883;
String state_topic = "";
String command_topic = "";
String mqtt_username = "";
String mqtt_password = "";

unsigned long statusLEDUntil = millis();
unsigned long lastUpdate = millis();

bool output = false;

void setup() {
    if (DEBUG_SERIAL) Serial.begin(19200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    prefs.begin("mqtt-switch");

    loadSettings();

    delay(200);

    if (digitalRead(BUTTON_PIN) == LOW) {
        DEBUG_PRINTLN("Button pressed - starting Access Point");
        startAccessPoint();
        APMode = true;
    } else {
        DEBUG_PRINTLN("Button not pressed - connecting to WiFi");
        connectToWiFi();

        client.setServer(mqttBrokerUrl.c_str(), mqttBrokerPort);
        client.setCallback(callback);

        APMode = false;
    }

    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
    
    randomSeed(micros());
}

void loop() {
    digitalWrite(LED_BUILTIN, millis() < statusLEDUntil ? LOW : HIGH);

    if (APMode) {
        if (statusLEDUntil < millis() - 1000UL) {
            statusLEDUntil = millis() + 1000UL;
        }
    } else {
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN("Disconnected, trying to reconnect...");

            waitForWifi();

            DEBUG_PRINTLN();
            DEBUG_PRINT("Connected, IP address: ");
            DEBUG_PRINTLN(WiFi.localIP());
        }

        if (!client.connected()) {
            reconnectMQTT();
        }

        if (client.connected()) {
            client.loop();
        }
    }

    server.handleClient();
}

void waitForWifi() {
    while (WiFi.status() != WL_CONNECTED) {
        delay(125);
        digitalWrite(LED_BUILTIN, LOW);
        delay(125);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(125);
        digitalWrite(LED_BUILTIN, LOW);
        delay(125);
        digitalWrite(LED_BUILTIN, HIGH);
        DEBUG_PRINT(".");
    }
}

// Function to save settings to EEPROM
void saveSettings(String ssid, String password, String mqttBrokerUrl, int mqttBrokerPort, String state_topic, String command_topic, String mqtt_username, String mqtt_password) {
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.putString("mqtt_broker_url", mqttBrokerUrl);
    prefs.putInt("mqtt_broker_port", mqttBrokerPort);
    prefs.putString("state_topic", state_topic);
    prefs.putString("command_topic", command_topic);
    prefs.putString("mqtt_username", mqtt_username);
    prefs.putString("mqtt_password", mqtt_password);
}

// Function to load settings from EEPROM
void loadSettings() {
    ssid = prefs.getString("ssid");
    password = prefs.getString("password");
    mqttBrokerUrl = prefs.getString("mqtt_broker_url");
    mqttBrokerPort = prefs.getInt("mqtt_broker_port");
    state_topic = prefs.getString("state_topic");
    command_topic = prefs.getString("command_topic");
    mqtt_username = prefs.getString("mqtt_username");
    mqtt_password = prefs.getString("mqtt_password");
}

// Function to handle root web page
void handleRoot() {
    auto htmlEscape = [](const String &str) -> String {
        String escaped = str;
        escaped.replace("&", "&amp;");
        escaped.replace("<", "&lt;");
        escaped.replace(">", "&gt;");
        escaped.replace("\"", "&quot;");
        escaped.replace("'", "&#39;");
        return escaped;
    };

    String page = "<html><body>"
        "<h1>WiFi Configuration</h1>"
        "<form action='/save' method='POST'>"
        "SSID: <input type='text' name='ssid' value='" + htmlEscape(ssid) + "'><br>"
        "Password: <input type='password' name='password' value='" + htmlEscape(password) + "'><br>"
        "MQTT Broker Url: <input type='text' name='url' value='" + htmlEscape(mqttBrokerUrl) + "'><br>"
        "MQTT Broker Port: <input type='number' name='port' value='" + htmlEscape(String(mqttBrokerPort)) + "'><br>"
        "State topic: <input type='text' name='state_topic' value='" + htmlEscape(state_topic) + "'><br>"
        "Command topic: <input type='text' name='command_topic' value='" + htmlEscape(command_topic) + "'><br>"
        "MQTT Username: <input type='text' name='mqtt_username' value='" + htmlEscape(mqtt_username) + "'><br>"
        "MQTT Password: <input type='password' name='mqtt_password' value='" + htmlEscape(mqtt_password) + "'><br>"
        "<input type='submit' value='Save'>"
        "</form></body></html>";
    server.send(200, "text/html", page);
}

// Function to handle saving the WiFi credentials
void handleSave() {
    ssid = server.arg("ssid");
    password = server.arg("password");
    mqttBrokerUrl = server.arg("url");
    mqttBrokerPort = server.arg("port").toInt();
    state_topic = server.arg("state_topic");
    command_topic = server.arg("command_topic");
    mqtt_username = server.arg("mqtt_username");
    mqtt_password = server.arg("mqtt_password");

    saveSettings(ssid, password, mqttBrokerUrl, mqttBrokerPort, state_topic, command_topic, mqtt_username, mqtt_password);

    String page = "<html><body><h1>Settings Saved!</h1></body></html>";
    server.send(200, "text/html", page);
}

// Setup access point mode
void startAccessPoint() {
    String clientId = "MQTT-Switch-";
    clientId += String(random(0xffff), HEX);
    DEBUG_PRINTF("Opening AP as '%s'\n", clientId.c_str());
    WiFi.softAP(clientId.c_str());
    IPAddress myIP = WiFi.softAPIP();
    DEBUG_PRINTLN("AP IP address: ");
    DEBUG_PRINTLN(myIP);
}

// Setup station mode
void connectToWiFi() {
    DEBUG_PRINTF("Connecting to: %s\n", ssid.c_str());
    String clientId = "MQTT-Switch-";
    clientId += String(random(0xffff), HEX);
    WiFi.hostname(clientId.c_str());

    WiFi.begin(ssid.c_str(), password.c_str());

    waitForWifi();

    DEBUG_PRINTLN("WiFi connected");
    DEBUG_PRINT("IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
}

void reconnectMQTT() {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        client.publish(state_topic.c_str(), output ? "1" : "0");
        // ... and resubscribe
        client.subscribe(command_topic.c_str());
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
    }
}

void callback(char* topic, byte* payload, unsigned int len) {
    if (len <= 0) {
        return;
    }

    if (strcmp(topic, command_topic.c_str()) == 0) {
        if (payload[0] == '1') {
            output = true;
            statusLEDUntil = 4294967295;
            DEBUG_PRINTLN("Recieved: ON");
            client.publish(state_topic.c_str(), "1");
        } else if (payload[0] == '0') {
            output = false;
            statusLEDUntil = 0;
            DEBUG_PRINTLN("Recieved: OFF");
            client.publish(state_topic.c_str(), "0");
        }
    }
}
