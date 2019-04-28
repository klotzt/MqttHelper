
#include <ESP8266WiFi.h>
#include <DNSServer.h>        //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h> //Local WebServer used to serve the configuration portal
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <AsyncMqttClient.h>
/**
* internal helper struct for eeprom storage
* of parameters
*/
typedef struct
{
    char user[40];
    char pw[40];
    char server[40];
    char fingerprint[60];
} MqttHelperConfig;
/**
 * MqttHelper class
 * 
 * Instanciates the necessary elements to do a secure wifi and cloud connection
 * 
 * Configuration fingerpint can be created using
 * openssl s_client -connect node.klotzt.org:8883 < /dev/null 2>/dev/null | openssl x509 -fingerprint -noout -in /dev/stdin
 */
class MqttHelper
{
public:
    MqttHelper(const char* name="ESPMQTT");
    ~MqttHelper();
    void setup();
    void loop();
    
    AsyncMqttClient m_mqttClient;
private:
    static void onResetPage();
    static void onRootPage();
    static void saveConfigCallback();

    static MqttHelper* m_instance;
    std::string m_name;
    WiFiManager m_wifiManager;
    ESP8266HTTPUpdateServer m_httpUpdater;
    ESP8266WebServer m_server;
    MqttHelperConfig m_config;
    bool m_shouldSaveConfig;
};