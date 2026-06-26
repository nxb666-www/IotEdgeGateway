#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

static const char *WIFI_SSID = "nie";
static const char *WIFI_PASS = "12345678";
static const char *MQTT_HOST = "192.168.233.107";
static const uint16_t MQTT_PORT = 1883;
static const char *MQTT_CLIENT_ID = "stm32-f411";

static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);
static String uartLine;
static unsigned long lastReconnectMs = 0;

static bool extractNumber(const String &json, const char *key, String &out)
{
    String name = String("\"") + key + "\"";
    int p = json.indexOf(name);
    if (p < 0) return false;
    int colon = json.indexOf(':', p + name.length());
    if (colon < 0) return false;

    int start = colon + 1;
    while (start < (int)json.length() && isspace((unsigned char)json[start])) start++;

    int end = start;
    if (end < (int)json.length() && json[end] == '-') end++;
    while (end < (int)json.length() &&
           (isdigit((unsigned char)json[end]) || json[end] == '.')) {
        end++;
    }

    if (end <= start) return false;
    out = json.substring(start, end);
    return true;
}

static void publishSensor(const char *id, const char *driver, const String &value)
{
    char topic[64];
    snprintf(topic, sizeof(topic), "iotgw/dev/telemetry/%s", id);

    String payload = "{\"device_id\":\"";
    payload += id;
    payload += "\",\"type\":\"sensor\",\"source\":\"stm32\",\"driver\":\"";
    payload += driver;
    payload += "\",\"data\":{\"value\":";
    payload += value;
    payload += "},\"ts\":0}";

    mqtt.publish(topic, payload.c_str());
}

static void publishTelemetryLine(const String &line)
{
    String value;
    if (extractNumber(line, "temp", value)) {
        publishSensor("temp", "aht30", value);
    }
    if (extractNumber(line, "humi", value)) {
        publishSensor("humi", "aht30", value);
    }
    if (extractNumber(line, "light", value)) {
        publishSensor("light", "lightsensor", value);
    }
    if (extractNumber(line, "ir", value)) {
        publishSensor("ir", "infrared", value);
    }
}

static void onMqttMessage(char *topic, uint8_t *payload, unsigned int length)
{
    (void)topic;
    for (unsigned int i = 0; i < length; ++i) {
        Serial.write(payload[i]);
    }
    Serial.write('\n');
}

static void ensureWifi()
{
    if (WiFi.status() == WL_CONNECTED) return;

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(200);
    }
}

static void ensureMqtt()
{
    if (mqtt.connected()) return;
    if (millis() - lastReconnectMs < 3000) return;
    lastReconnectMs = millis();

    ensureWifi();
    if (WiFi.status() != WL_CONNECTED) return;

    if (mqtt.connect(MQTT_CLIENT_ID)) {
        mqtt.subscribe("iotgw/dev/cmd/#");
    }
}

static void handleUart()
{
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            uartLine.trim();
            if (uartLine.length() > 0 && mqtt.connected()) {
                publishTelemetryLine(uartLine);
            }
            uartLine = "";
            continue;
        }

        if (uartLine.length() < 220) {
            uartLine += c;
        } else {
            uartLine = "";
        }
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.setTimeout(10);
    WiFi.setAutoReconnect(true);
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onMqttMessage);
    mqtt.setBufferSize(512);
}

void loop()
{
    ensureMqtt();
    mqtt.loop();
    handleUart();
}
