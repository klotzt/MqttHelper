#include "MqttHelper.h"
#include <EEPROM.h>
#include <functional>

#define MQTT_LOG(str) Serial.println(str)

MqttHelper* MqttHelper::m_instance=nullptr;

MqttHelper::MqttHelper(const char *name)
    : m_name(name),
      m_shouldSaveConfig(false)
{
    memset(&m_config, 0, sizeof(m_config));
    m_instance=this;
}

MqttHelper::~MqttHelper()
{
}

void MqttHelper::setup()
{
    Serial.begin(9600);
    EEPROM.begin(512);
    EEPROM.get(0, m_config);
    EEPROM.end();
    m_wifiManager.setDebugOutput(true);
    m_wifiManager.setBreakAfterConfig(false);
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", m_config.server, 40);
    WiFiManagerParameter custom_mqtt_user("user", "mqtt user", m_config.user, 40);
    WiFiManagerParameter custom_mqtt_pw("password", "mqtt password", m_config.pw, 40);
    WiFiManagerParameter custom_mqtt_fingerprint("fingerprint", "mqtt fingerprint", m_config.fingerprint, 60);
    m_wifiManager.addParameter(&custom_mqtt_server);
    m_wifiManager.addParameter(&custom_mqtt_fingerprint);
    m_wifiManager.addParameter(&custom_mqtt_user);
    m_wifiManager.addParameter(&custom_mqtt_pw);
    m_wifiManager.setSaveConfigCallback(&MqttHelper::saveConfigCallback);
    if (m_wifiManager.autoConnect(m_name.c_str()))
    {

        MQTT_LOG("Wifi Mananger connected...");
    }
    if (m_shouldSaveConfig)
    {
        MQTT_LOG("Saving config to eeprom now");
        EEPROM.begin(512);
        memset(&m_config, 0, sizeof(m_config));
        memcpy(m_config.server, custom_mqtt_server.getValue(), custom_mqtt_server.getValueLength());
        memcpy(m_config.user, custom_mqtt_user.getValue(), custom_mqtt_user.getValueLength());
        memcpy(m_config.pw, custom_mqtt_pw.getValue(), custom_mqtt_pw.getValueLength());
        memcpy(m_config.fingerprint, custom_mqtt_fingerprint.getValue(), custom_mqtt_fingerprint.getValueLength());

        EEPROM.put(0, m_config);
        delay(200);
        EEPROM.commit();
        EEPROM.end();
    }
    if (MDNS.begin(m_name.c_str()))
    {
        Serial.println("MDNS responder started");
    }
    m_httpUpdater.setup(&m_server);
    m_server.on("/", &MqttHelper::onRootPage);
    m_server.on("/reset", &MqttHelper::onResetPage);
    m_server.begin();

  MQTT_LOG("Starting wifi setup...");
  MQTT_LOG("Custom params: ");
  MQTT_LOG(m_config.server);
  MQTT_LOG(m_config.user);
  MQTT_LOG(m_config.pw);
  MQTT_LOG(m_config.fingerprint);
  const char *txt = m_config.fingerprint;
  unsigned int i = 0;
  uint8_t fp[20];
  int fpPos = 0;
  while ((txt[i] != 0) && (i < strlen(m_config.fingerprint)))
  {
    // assume 2 digits and a colon for now...
    char foo[3];
    foo[0] = txt[i++];
    foo[1] = txt[i++];
    foo[2] = 0;
    i++; // skip :
    uint8_t parsed=strtol(foo, NULL, 16);
    fp[fpPos++] = parsed;
  }

  MQTT_LOG("Setting up mqtt");
  
  m_mqttClient.setServer(m_config.server, 8883);
  m_mqttClient.setSecure(true);
  m_mqttClient.addServerFingerprint(fp);
  m_mqttClient.setCredentials(m_config.user, m_config.pw);


}

void MqttHelper::loop()
{
  m_server.handleClient();
  MDNS.update();

}

void MqttHelper::onRootPage()
{
    if (m_instance==nullptr) return;

    static const char *rootPage = "<!DOCTYPE html> \
                        <html>  \
                        <body> \
                            <center> \
                                %s<hr> \
                              < a href = \"update\"> Update Firmware</ a><br> \
                              < a href = \"reset\"> Reset Configuration</ a><br> \
                            </ center> \
                        </ body> \
                     </ html>";
    char txt[1024];
    snprintf (txt, 1024, rootPage, m_instance->m_name.c_str());
    m_instance->m_server.send(200, "text/html", txt);
    MQTT_LOG("Root called");
}

void MqttHelper::onResetPage()
{
    if (m_instance==nullptr) return;

    m_instance->m_wifiManager.resetSettings();
    m_instance->m_server.send(200, "text/html", "Config reset!");

    EEPROM.begin(512);
    for (int i = 0; i < 512; i++)
    {
        EEPROM.write(i, 0);
    }
    delay(200);
    EEPROM.commit();
    EEPROM.end();
    MQTT_LOG("Config reset done");
}

void MqttHelper::saveConfigCallback()
{
    if (m_instance==nullptr) return;
    MQTT_LOG("Should save config here!");
    m_instance->m_shouldSaveConfig = true;
}