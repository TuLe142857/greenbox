#ifndef APP_CONFIG_CLASS_H
#define APP_CONFIG_CLASS_H

#include<Arduino.h>
#include<Preferences.h>

/**
 * @brief Manages application configuration settings. Stores settings (WiFi credentials, Server URL) is in the ESP32's non-volatile storage (NVS) using the Preferences library.
 */
class AppConfig{
private:
    static const  char* NAMESPACE;
    static const  char* KEY_WIFI_SSID;
    static const  char* KEY_WIFI_PASSWORD;
    static const  char* KEY_SERVER_URL;
public:
    static bool readConfig(String &wifi_ssid, String &wifi_password, String &server_url);
    static bool readConfigWiFi(String &wifi_ssid, String &wifi_password);
    static bool readConfigServer(String &server_url);

    static void writeConfig(const String &wifi_ssid, const String &wifi_password, const String &server_url);
    static void writeConfigWiFi(const String &wifi_ssid, const String &wifi_password);
    static void writeConfigServer(const String &server_url);
    
    static void clearConfig(bool clear_wifi_config=true, bool clear_server_config=false);
};

const  char* AppConfig::NAMESPACE = "greenbox";
const  char* AppConfig::KEY_WIFI_SSID = "wifi_ssid";
const  char* AppConfig::KEY_WIFI_PASSWORD = "wifi_password";
const  char* AppConfig::KEY_SERVER_URL = "server_url";

bool AppConfig::readConfig(String &wifi_ssid, String &wifi_password, String &server_url){
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE, true);
    bool check = pref.isKey(AppConfig::KEY_WIFI_SSID) && (pref.getString(AppConfig::KEY_WIFI_SSID).length() != 0) &&
                 pref.isKey(AppConfig::KEY_WIFI_PASSWORD) && 
                 pref.isKey(AppConfig::KEY_SERVER_URL) && (pref.getString(AppConfig::KEY_SERVER_URL).length() != 0);
    if (!check){
        pref.end();
        return false;
    }
    wifi_ssid = pref.getString(AppConfig::KEY_WIFI_SSID);
    wifi_password = pref.getString(AppConfig::KEY_WIFI_PASSWORD);
    server_url = pref.getString(AppConfig::KEY_SERVER_URL);
    pref.end();
    return true;
}

bool AppConfig::readConfigWiFi(String &wifi_ssid, String &wifi_password){
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE, true);
    bool check = pref.isKey(AppConfig::KEY_WIFI_SSID) && (pref.getString(AppConfig::KEY_WIFI_SSID).length() != 0) &&
                 pref.isKey(AppConfig::KEY_WIFI_PASSWORD);
    if (!check){
        pref.end();
        return false;
    }
    wifi_ssid = pref.getString(AppConfig::KEY_WIFI_SSID);
    wifi_password = pref.getString(AppConfig::KEY_WIFI_PASSWORD);
    pref.end();
    return true;
}

bool AppConfig::readConfigServer(String &server_url){
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE, true);
    bool check = pref.isKey(AppConfig::KEY_SERVER_URL) && (pref.getString(AppConfig::KEY_SERVER_URL).length() != 0);
    if (!check){
        pref.end();
        return false;
    }
    server_url = pref.getString(AppConfig::KEY_SERVER_URL);
    pref.end();
    return true;
}

void AppConfig::writeConfig(const String &wifi_ssid, const String &wifi_password, const String &server_url){
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE);
    pref.putString(AppConfig::KEY_WIFI_SSID, wifi_ssid);
    pref.putString(AppConfig::KEY_WIFI_PASSWORD, wifi_password);
    pref.putString(AppConfig::KEY_SERVER_URL, server_url);
    pref.end();
}

void AppConfig::writeConfigWiFi(const String &wifi_ssid, const String &wifi_password=""){
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE);
    pref.putString(AppConfig::KEY_WIFI_SSID, wifi_ssid);
    pref.putString(AppConfig::KEY_WIFI_PASSWORD, wifi_password);
    pref.end();
}

void AppConfig::writeConfigServer(const String &server_url){
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE);
    pref.putString(AppConfig::KEY_SERVER_URL, server_url);
    pref.end();
}

void AppConfig::clearConfig(bool clear_wifi_config, bool clear_server_config){
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE);
    if(clear_wifi_config){
        pref.remove(AppConfig::KEY_WIFI_SSID);
        pref.remove(AppConfig::KEY_WIFI_PASSWORD);
    }
    if(clear_server_config){
        pref.remove(AppConfig::KEY_SERVER_URL);
    }
    pref.end();
}
#endif