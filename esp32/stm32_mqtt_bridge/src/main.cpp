#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

const char* kWifiSsid = "nie";
const char* kWifiPassword = "12345678";

const char* kMqttHost = "192.168.233.107";
const uint16_t kMqttPort = 1884;
const char* kTopicPrefix = "iotgw/dev/";

constexpr int kStm32RxPin = 16;  // ESP32 RX <- STM32 TX
constexpr int kStm32TxPin = 17;  // ESP32 TX -> STM32 RX
constexpr uint32_t kStm32Baud = 115200;

HardwareSerial Stm32Serial(1);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String stm32Line;
uint32_t lastHeartbeatMs = 0;

String JsonStringValue(const String& json, const char* key) {
    String needle = String("\"") + key + "\"";
    int pos = json.indexOf(needle);
    if (pos < 0) return "";

    int colon = json.indexOf(':', pos + needle.length());
    if (colon < 0) return "";

    int firstQuote = json.indexOf('"', colon + 1);
    if (firstQuote < 0) return "";

    int secondQuote = json.indexOf('"', firstQuote + 1);
    if (secondQuote < 0) return "";

    return json.substring(firstQuote + 1, secondQuote);
}

bool JsonNumberValue(const String& json, const char* key, double& out) {
    String needle = String("\"") + key + "\"";
    int pos = json.indexOf(needle);
    if (pos < 0) return false;

    int colon = json.indexOf(':', pos + needle.length());
    if (colon < 0) return false;

    int start = colon + 1;
    while (start < (int)json.length() && (json[start] == ' ' || json[start] == '\t')) {
        start++;
    }

    int end = start;
    if (end < (int)json.length() && json[end] == '-') {
        end++;
    }

    while (end < (int)json.length() &&
           ((json[end] >= '0' && json[end] <= '9') || json[end] == '.')) {
        end++;
    }

    if (end <= start) return false;
    out = json.substring(start, end).toDouble();
    return true;
}

bool JsonBoolLikeValue(const String& json, const char* key, double& out) {
    if (JsonNumberValue(json, key, out)) {
        return true;
    }

    String value = JsonStringValue(json, key);
    value.toLowerCase();
    if (value == "on" || value == "true" || value == "open" || value == "start") {
        out = 1;
        return true;
    }
    if (value == "off" || value == "false" || value == "close" || value == "stop") {
        out = 0;
        return true;
    }
    return false;
}

void PublishSensor(const char* sensorId, double value, const String& driver, int decimals) {
    String topic = String(kTopicPrefix) + "telemetry/" + sensorId;
    String payload = String("{\"device_id\":\"") + sensorId +
                     "\",\"type\":\"sensor\",\"source\":\"stm32\",\"driver\":\"" + driver +
                     "\",\"data\":{\"value\":" + String(value, decimals) +
                     "},\"ts\":" + String(millis()) + "}";

    if (mqtt.publish(topic.c_str(), payload.c_str())) {
        Serial.print("[MQTT] PUB ");
        Serial.print(topic);
        Serial.print(" ");
        Serial.println(payload);
    } else {
        Serial.print("[MQTT] publish failed: ");
        Serial.println(topic);
    }
}

void HandleStm32Json(String line) {
    line.trim();
    if (!line.startsWith("{")) {
        Serial.print("[STM32] not json: ");
        Serial.println(line);
        return;
    }

    String driver = JsonStringValue(line, "driver");
    Serial.print("[STM32] RX ");
    Serial.println(line);

    double value = 0.0;
    bool published = false;

    if (JsonNumberValue(line, "temp", value)) {
        PublishSensor("temp", value, "stm32", 1);
        published = true;
    }
    if (JsonNumberValue(line, "humi", value)) {
        PublishSensor("humi", value, "stm32", 1);
        published = true;
    }
    if (JsonNumberValue(line, "light", value)) {
        PublishSensor("light", value, "stm32", 0);
        published = true;
    }
    if (JsonNumberValue(line, "ir", value)) {
        PublishSensor("ir", value, "stm32", 0);
        published = true;
    }
    if (published) return;

    if (driver == "aht30" || driver == "dht11") {
        double tempInt = 0.0;
        double tempDeci = 0.0;
        double humiInt = 0.0;
        double humiDeci = 0.0;

        if (JsonNumberValue(line, "temp_int", tempInt) &&
            JsonNumberValue(line, "temp_deci", tempDeci)) {
            double sign = tempInt < 0 ? -1.0 : 1.0;
            PublishSensor("temp", tempInt + sign * tempDeci / 10.0, driver, 1);
        }

        if (JsonNumberValue(line, "humi_int", humiInt) &&
            JsonNumberValue(line, "humi_deci", humiDeci)) {
            PublishSensor("humi", humiInt + humiDeci / 10.0, driver, 1);
        }
        return;
    }

    if (driver == "lightsensor") {
        double light = 0.0;
        if (JsonNumberValue(line, "light", light)) {
            PublishSensor("light", light, driver, 0);
        }
        return;
    }

    if (driver == "infrared") {
        double ir = 0.0;
        if (JsonNumberValue(line, "ir", ir)) {
            PublishSensor("ir", ir, driver, 0);
        }
        return;
    }

    String deviceId = JsonStringValue(line, "device_id");
    if (deviceId.length() > 0 && JsonNumberValue(line, "value", value)) {
        PublishSensor(deviceId.c_str(), value, "generic", 1);
    } else {
        Serial.print("[STM32] unsupported json: ");
        Serial.println(line);
    }
}

String LastTopicSegment(const String& topic) {
    int slash = topic.lastIndexOf('/');
    if (slash < 0 || slash + 1 >= (int)topic.length()) return topic;
    return topic.substring(slash + 1);
}

void SendControlToStm32(const String& actuator, const String& payload) {
    String control;

    if (payload.indexOf("\"type\"") >= 0 && payload.indexOf("\"control\"") >= 0) {
        control = payload;
    } else {
        double value = 0.0;
        double on = 0.0;
        double br = 0.0;
        double sp = 0.0;
        double dir = 0.0;

        bool hasValue = JsonBoolLikeValue(payload, "value", value);
        bool hasOn = JsonBoolLikeValue(payload, "on", on);
        bool hasBr = JsonNumberValue(payload, "br", br);
        bool hasSp = JsonNumberValue(payload, "sp", sp);
        bool hasDir = JsonNumberValue(payload, "dir", dir);
        double cmdValue = 0.0;
        bool hasCmd = JsonBoolLikeValue(payload, "cmd", cmdValue);

        String compact = payload;
        compact.trim();
        if (!hasValue && !hasOn && (compact == "0" || compact == "1")) {
            value = compact.toInt();
            hasValue = true;
        }

        if (!hasOn && hasValue) {
            on = value;
            hasOn = true;
        }
        if (!hasOn && hasCmd) {
            on = cmdValue;
            hasOn = true;
        }

        if (actuator == "led") {
            control = "{\"type\":\"control\",\"payload\":{";
            bool wrote = false;
            if (hasOn) {
                control += String("\"led_on\":") + (int)on;
                wrote = true;
            }
            if (hasBr) {
                if (wrote) control += ",";
                control += String("\"led_br\":") + (int)br;
            }
            control += "}}";
        } else if (actuator == "motor") {
            control = "{\"type\":\"control\",\"payload\":{";
            bool wrote = false;
            if (hasOn) {
                control += String("\"motor_on\":") + (int)on;
                wrote = true;
            }
            if (hasSp) {
                if (wrote) control += ",";
                control += String("\"motor_sp\":") + (int)sp;
                wrote = true;
            }
            if (hasDir) {
                if (wrote) control += ",";
                control += String("\"motor_dir\":") + (int)dir;
            }
            control += "}}";
        } else if (actuator == "buzzer") {
            control = String("{\"type\":\"control\",\"payload\":{\"buzzer\":") +
                      (int)(hasOn ? on : value) + "}}";
        } else {
            control = payload;
        }
    }

    Stm32Serial.print(control);
    Stm32Serial.print("\r\n");

    Serial.print("[STM32] TX ");
    Serial.println(control);
}

void OnMqttMessage(char* topic, byte* payload, unsigned int length) {
    String msg;
    msg.reserve(length + 1);
    for (unsigned int i = 0; i < length; ++i) {
        msg += (char)payload[i];
    }

    String topicStr(topic);
    Serial.print("[MQTT] SUB ");
    Serial.print(topicStr);
    Serial.print(" ");
    Serial.println(msg);

    SendControlToStm32(LastTopicSegment(topicStr), msg);
}

void ConnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.print("[WiFi] Connecting to ");
    Serial.println(kWifiSsid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(kWifiSsid, kWifiPassword);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
}

void ConnectMqtt() {
    while (!mqtt.connected()) {
        String clientId = "esp32-stm32-bridge-" + String((uint32_t)ESP.getEfuseMac(), HEX);

        Serial.print("[MQTT] Connecting to ");
        Serial.print(kMqttHost);
        Serial.print(":");
        Serial.println(kMqttPort);

        if (mqtt.connect(clientId.c_str())) {
            String cmdTopic = String(kTopicPrefix) + "cmd/#";
            mqtt.subscribe(cmdTopic.c_str());
            Serial.print("[MQTT] subscribed ");
            Serial.println(cmdTopic);
        } else {
            Serial.print("[MQTT] failed rc=");
            Serial.println(mqtt.state());
            delay(2000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(1500);
    Serial.println();
    Serial.println("[BOOT] ESP32 STM32 MQTT bridge");
    Serial.println("[BOOT] Serial ready");
    Stm32Serial.begin(kStm32Baud, SERIAL_8N1, kStm32RxPin, kStm32TxPin);
    Serial.println("[BOOT] STM32 UART ready");

    // 延时 10 秒，让你有时间打开串口监视器
    Serial.println("[BOOT] ESP32 starting in 10 seconds...");
    Serial.println("[BOOT] ESP32 starting in 10 seconds...");
    for (int i = 10; i > 0; --i) {
        Serial.print("[BOOT] wait ");
        Serial.println(i);
        delay(1000);
    }
    Serial.println("[BOOT] Starting now.");

    ConnectWiFi();
    mqtt.setServer(kMqttHost, kMqttPort);
    mqtt.setCallback(OnMqttMessage);
}

void loop() {
    ConnectWiFi();
    ConnectMqtt();
    mqtt.loop();

    uint32_t now = millis();
    if (now - lastHeartbeatMs >= 5000) {
        lastHeartbeatMs = now;
        Serial.print("[RUN] wifi=");
        Serial.print(WiFi.status() == WL_CONNECTED ? "ok" : "down");
        Serial.print(" ip=");
        Serial.print(WiFi.localIP());
        Serial.print(" mqtt=");
        Serial.println(mqtt.connected() ? "ok" : "down");
    }

    while (Stm32Serial.available()) {
        char ch = (char)Stm32Serial.read();
        if (ch == '\n') {
            HandleStm32Json(stm32Line);
            stm32Line = "";
        } else {
            stm32Line += ch;
            if (stm32Line.length() > 512) {
                Serial.println("[STM32] line too long, dropped");
                stm32Line = "";
            }
        }
    }
}
