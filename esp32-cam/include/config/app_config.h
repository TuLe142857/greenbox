#ifndef APP_CONFIG_CLASS_H
#define APP_CONFIG_CLASS_H

#include<Arduino.h>
#include<Preferences.h>

class AppConfig{
private:
    static const String HTML_TEMPLATE;
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
    Serial.printf("Write config wifi %s %s\n", wifi_ssid, wifi_password);
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE);
    pref.putString(AppConfig::KEY_WIFI_SSID, wifi_ssid);
    pref.putString(AppConfig::KEY_WIFI_PASSWORD, wifi_password);
    pref.end();
}

void AppConfig::writeConfigServer(const String &server_url){
    Serial.printf("Write server wifi %s\n", server_url);
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE);
    pref.putString(AppConfig::KEY_SERVER_URL, server_url);
    pref.end();
}

void AppConfig::clearConfig(bool clear_wifi_config, bool clear_server_config){
    Preferences pref;
    pref.begin(AppConfig::NAMESPACE);
    if(clear_server_config){
        pref.remove(AppConfig::KEY_WIFI_SSID);
        pref.remove(AppConfig::KEY_WIFI_PASSWORD);
    }
    if(clear_server_config){
        pref.remove(AppConfig::KEY_SERVER_URL);
    }
    pref.end();
}

// void AppConfig::runConfig(const String &wifi_ap_ssid, const String &wifi_ap_password, std::function<void()> dosomething){
//     bool running = true;
//     Preferences pref;
//     pref.begin(AppConfig::NAMESPACE);
//     String default_server_url;
//     if (pref.isKey(AppConfig::KEY_SERVER_URL))
//         default_server_url = pref.getString(AppConfig::KEY_SERVER_URL);
//     else
//         default_server_url = "";
//     pref.end();
    
//     /* ----------------------------------------------
//                     WIFI AP
//     -------------------------------------------------*/
//     IPAddress apIp(192, 168, 4, 1);
//     WiFi.mode(WIFI_AP_STA);
//     if(wifi_ap_password.length() == 0){
//         WiFi.softAP(wifi_ap_ssid);
//     }else{
//         WiFi.softAP(wifi_ap_ssid, wifi_ap_password);
//     }
//     delay(100);
//     WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));



//     /* ----------------------------------------------
//                     WEB SERVER
//     -------------------------------------------------*/
//     WebServer server(80);

//     server.on("/", [&server](){
//         server.send(200, "text/html", AppConfig::HTML_TEMPLATE);
//     });

//     server.onNotFound([&server](){
//         server.send(200, "text/html", AppConfig::HTML_TEMPLATE);
//     });

//     server.on("/default-config", [&server, &default_server_url](){    
//         server.send(200, "text/plain", default_server_url);
//     });

//     server.on("/scan-wifi", [&server](){
//         JsonDocument doc;
//         JsonArray arr = doc.to<JsonArray>();

//         static String encryptionName[WIFI_AUTH_MAX] = {"OPEN", "WPA", "WPA2", "WPA/WPA2", "EAP", "EAP", "WPA3", "WPA2/WPA3", "WAPI", "WPA3_ENT_192"};
//         int network_count = WiFi.scanNetworks();
//         for(int i =0; i < network_count; i++){
//             JsonObject item = arr.add<JsonObject>();
//             item["ssid"] = WiFi.SSID(i).c_str();
//             item["rssid"] = WiFi.RSSI(i);
//             item["encryption"] = encryptionName[WiFi.encryptionType(i)];
//         }

//         String response;
//         serializeJson(doc, response);
//         server.send(200, "application/json", response);
//     });


//     /*
//     request params key: 
//         + wifi_ssid:String, not empty, required
//         + wifi_password:String, optional
//         + server_url: String, not empty, is required when there's no default config stored in nvram(default_server_url == "")
//     */
//     server.on("/submit", HTTP_POST, [&server, &running, &default_server_url](){
//         if (!server.hasArg("wifi_ssid")){
//             server.send(400, "text/plain", "Missing parameters");
//             return;
//         }
            
//         if (default_server_url.length() == 0 && !server.hasArg("server_url")){
//             server.send(400, "text/plain", "Missing parameters");
//             return;
//         }

//         if (server.arg("wifi_ssid").length() == 0){
//             server.send(400, "text/plain", "Wifi ssid can not be empty");
//             return;
//         }
            

//         if (server.hasArg("server_url") && server.arg("server_url").length() == 0){
//             server.send(400, "text/plain", "Server url can not be empty");
//             return;
//         }
            

//         AppConfig::writeConfigWiFi(server.arg("wifi_ssid"), server.arg("wifi_password"));
//         if(server.hasArg("server_url")) {
//             AppConfig::writeConfigServer(server.arg("server_url"));
//         }

//         server.send(200, "text/plain", "ok");
//         delay(500);
//         running=false;
//     });

//     server.begin();

    

//     /* ----------------------------------------------
//                     DNS SERVER
//     -------------------------------------------------*/
//     DNSServer dnsServer;
//     dnsServer.start(53, "*", apIp);


    
//     /* ----------------------------------------------
//                     LOOP
//     -------------------------------------------------*/
//     while(running){
//         dnsServer.processNextRequest();
//         server.handleClient();
//         delay(1);
//     }
//     server.stop();
//     dnsServer.stop();
//     WiFi.mode(WIFI_OFF);
// }
#endif