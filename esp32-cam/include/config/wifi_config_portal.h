#ifndef WIFI_CONFIG_PORTAL_H
#define WIFI_CONFIG_PORTAL_H

#include<Arduino.h>
#include<Preferences.h>
#include<WiFi.h>
#include<WebServer.h>
#include<DNSServer.h>
#include<ArduinoJson.h>
#include<functional>
#include<config/app_config.h>

/**
 * @brief Host a Web Server and DNS Server for Wi-Fi configuration.
 */
class WiFiConfigPortal{
private:
    static const String HTML_TEMPLATE;
    WebServer webServer;
    DNSServer dnsServer;
    bool running;
    bool need_config_server_url;
    String default_server_url;
public:
    WiFiConfigPortal();

    /**
     * @brief Inititalize WifiConfig WebServer && DNS Server
     * @param ap_ssid  Wifi Access Point SSID
     * @param ap_password WiFi Access Point PASSWORD. Default: ""
     */
    void init(const String ap_ssid, const String ap_password="");

    /**
     * @param dosomething function to handle something while run, to prevent blocking
     */
    void run(std::function<void()>dosomething=[](){});
};

WiFiConfigPortal::WiFiConfigPortal():webServer(80), running(false){};

void WiFiConfigPortal::init(const String ap_ssid, const String ap_password){
    
    this->need_config_server_url = !AppConfig::readConfigServer(this->default_server_url);


    /*------------------------------------------
                    WIFI AP
     -------------------------------------------*/
    WiFi.mode(WIFI_AP_STA);
    IPAddress apIp(192, 168, 4, 1);
    if(ap_password.length() == 0 || ap_password == nullptr){
        WiFi.softAP(ap_ssid);
    }
    else{
        WiFi.softAP(ap_ssid, ap_password);
    }
    delay(100);
    WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));


    /*------------------------------------------
                    WEB SERVER
     -------------------------------------------*/
    this->webServer.on("/", [this](){
        this->webServer.send(200, "text/html", WiFiConfigPortal::HTML_TEMPLATE);
    });

    this->webServer.onNotFound([this](){
        this->webServer.send(200, "text/html", WiFiConfigPortal::HTML_TEMPLATE);
    });

    this->webServer.on("/default-config", [this](){
        this->webServer.send(200, "text/plain", this->default_server_url);
    });

    this->webServer.on("/scan-wifi", [this](){
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        static String encryptionName[WIFI_AUTH_MAX] = {"OPEN", "WPA", "WPA2", "WPA/WPA2", "EAP", "EAP", "WPA3", "WPA2/WPA3", "WAPI", "WPA3_ENT_192"};
        int network_count = WiFi.scanNetworks();
        for(int i =0; i < network_count; i++){
            JsonObject item = arr.add<JsonObject>();
            item["ssid"] = WiFi.SSID(i).c_str();
            item["rssi"] = WiFi.RSSI(i);
            item["encryption"] = WiFi.encryptionType(i) < WIFI_AUTH_MAX 
                ? encryptionName[WiFi.encryptionType(i)]
                : "UNKNOWN";
        }
        String response;
        serializeJson(doc, response);
        this->webServer.send(200, "application/json", response);
    });

    this->webServer.on("/submit", HTTP_POST, [this](){
        // check request param
        if(! (this->webServer.hasArg("wifi_ssid"))) {
            this->webServer.send(400, "text/plain", "Missing parameter 'wifi_ssid'");
            return;
        }
        if(this->webServer.arg("wifi_ssid").length() == 0){
            this->webServer.send(400, "text/plain", "wifi_ssid can not be empty");
            return;
        }

        if(this->need_config_server_url && !this->webServer.hasArg("server_url")){
            this->webServer.send(400, "text/plain", "Missing parameter 'server_url'");
            return;
        }
        if(this->need_config_server_url && this->webServer.arg("server_url").length()==0){
            this->webServer.send(400, "text/plain", "server_url can not be empty");
            return;
        }

        // write config
        AppConfig::writeConfigWiFi(this->webServer.arg("wifi_ssid"), this->webServer.arg("wifi_password"));
        if(this->webServer.hasArg("server_url")){
            AppConfig::writeConfigServer(this->webServer.arg("server_url"));
        }

        this->webServer.send(200, "text/plain", "OK");
        delay(500);
        this->running = false;
    });

    this->webServer.begin();


    /*------------------------------------------
                    DNS SERVER
     -------------------------------------------*/
    this->dnsServer.start(53, "*", apIp);
}

void WiFiConfigPortal::run(std::function<void()>dosomething){
    this->running = true;
    while(this->running){
        this->dnsServer.processNextRequest();
        this->webServer.handleClient();
        dosomething();
        delay(1);
    }
    delay(2000);
    this->dnsServer.stop();
    this->webServer.stop();
    WiFi.mode(WIFI_OFF);
}

const String WiFiConfigPortal::HTML_TEMPLATE = R"raw_string(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GreenBox Setup</title>
    <style>
        :root {
            --primary: #4CAF50;
            --primary-dark: #388E3C;
            --bg: #f4f7f6;
            --card-bg: #ffffff;
            --text: #333;
            --text-light: #666;
            --border: #ddd;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg);
            color: var(--text);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
        }
        .container {
            background: var(--card-bg);
            padding: 30px;
            border-radius: 12px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.1);
            width: 100%;
            max-width: 400px;
        }
        h1 {
            color: var(--primary);
            text-align: center;
            margin: 0 0 5px 0;
            font-size: 28px;
        }
        .subtitle {
            text-align: center;
            color: var(--text-light);
            margin: 0 0 25px 0;
            font-size: 14px;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: 500;
            font-size: 14px;
        }
        .input-row {
            display: flex;
            gap: 10px;
        }
        input {
            width: 100%;
            padding: 10px;
            border: 1px solid var(--border);
            border-radius: 6px;
            font-size: 16px;
            box-sizing: border-box;
            outline: none;
        }
        input:focus {
            border-color: var(--primary);
        }
        /* Buttons */
        .btn-icon {
            padding: 0 12px;
            background: #eee;
            border: 1px solid var(--border);
            border-radius: 6px;
            cursor: pointer;
            font-size: 18px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .btn-submit {
            width: 100%;
            padding: 12px;
            background: var(--primary);
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            margin-top: 10px;
            transition: background 0.3s;
        }
        .btn-submit:hover {
            background: var(--primary-dark);
        }
        /* Wifi List */
        #wifi-list {
            list-style: none;
            padding: 0;
            margin: 10px 0 0 0;
            max-height: 200px;
            overflow-y: auto;
            border: 1px solid var(--border);
            border-radius: 6px;
            display: none; /* Hidden by default */
        }
        #wifi-list li {
            padding: 10px;
            border-bottom: 1px solid #eee;
            cursor: pointer;
            font-size: 14px;
            display: flex;
            justify-content: space-between;
        }
        #wifi-list li:last-child {
            border-bottom: none;
        }
        #wifi-list li:hover {
            background-color: #f0f9f0;
        }
        /* Advanced Toggle */
        .advanced-toggle {
            text-align: center;
            margin: 15px 0;
            font-size: 14px;
        }
        .advanced-toggle a {
            color: var(--primary);
            text-decoration: none;
            cursor: pointer;
        }
        /* Footer */
        .footer-note {
            margin-top: 25px;
            padding-top: 15px;
            border-top: 1px solid #eee;
            font-size: 12px;
            color: #888;
            text-align: center;
            line-height: 1.4;
        }
        /* Utilities */
        .hidden { display: none !important; }
    </style>
</head>
<body>

<div class="container">
    <h1>GreenBox</h1>
    <p class="subtitle">Thùng rác thông minh phân loại rác</p>

    <form action="/submit" method="POST">
        <div class="form-group">
            <label>Tên Wifi (SSID)</label>
            <div class="input-row">
                <input type="text" id="ssid" name="wifi_ssid" placeholder="Nhập hoặc Scan..." required>
                <button type="button" class="btn-icon" onclick="scanWifi()" title="Quét Wifi">🔍</button>
            </div>
            <ul id="wifi-list"></ul>
        </div>

        <div class="form-group">
            <label>Mật khẩu Wifi</label>
            <div class="input-row">
                <input type="password" id="password" name="wifi_password" placeholder="Mật khẩu...">
                <button type="button" class="btn-icon" onclick="togglePass()" title="Hiện/Ẩn">👁️</button>
            </div>
        </div>

        <div id="advanced-section" class="form-group hidden">
            <label>Server URL</label>
            <input type="text" id="server_url" name="server_url" placeholder="http://192.168.1.100:5000">
        </div>

        <div id="toggle-container" class="advanced-toggle hidden">
            <a onclick="showAdvanced()">⚙️ Cấu hình nâng cao (Server URL)</a>
        </div>

        <button type="submit" class="btn-submit">Lưu Cấu Hình</button>
    </form>

    <div class="footer-note">
        * Lưu ý: Nếu muốn cài đặt lại Wifi sau này, hãy nhấn giữ nút <b>RESET</b> trên thân thùng rác.
    </div>
</div>

<script>
    // 1. Logic xử lý Server URL khi load trang
    fetch('/default-config')
        .then(response => response.text())
        .then(url => {
            const serverInput = document.getElementById('server_url');
            const section = document.getElementById('advanced-section');
            const toggle = document.getElementById('toggle-container');

            if (url && url.trim().length > 0) {
                // Đã có config -> Ẩn input, hiện nút toggle
                serverInput.value = url;
                section.classList.add('hidden');
                toggle.classList.remove('hidden');
                serverInput.required = false;
            } else {
                // Chưa có config -> Hiện input, bắt buộc nhập
                section.classList.remove('hidden');
                toggle.classList.add('hidden');
                serverInput.required = true;
            }
        })
        .catch(err => {
            // Lỗi mạng hoặc server -> Hiện mặc định cho an toàn
            document.getElementById('advanced-section').classList.remove('hidden');
        });

    function showAdvanced() {
        document.getElementById('advanced-section').classList.remove('hidden');
        document.getElementById('toggle-container').classList.add('hidden');
        document.getElementById('server_url').focus();
    }

    // 2. Logic Quét Wifi
    function scanWifi() {
        const list = document.getElementById('wifi-list');
        list.style.display = 'block';
        list.innerHTML = '<li style="justify-content:center; color:#666">Đang quét... ⏳</li>';

        fetch('/scan-wifi')
            .then(response => response.json())
            .then(data => {
                list.innerHTML = '';
                if (data.length === 0) {
                    list.innerHTML = '<li style="justify-content:center">Không tìm thấy Wifi</li>';
                    return;
                }
                
                // Sắp xếp theo độ mạnh tín hiệu (RSSI)
                data.sort((a, b) => b.rssi - a.rssi);

                data.forEach(net => {
                    const li = document.createElement('li');
                    // Tính icon sóng dựa trên RSSI
                    let signal = 'weak';
                    if (net.rssi > -60) signal = 'strong';
                    else if (net.rssi > -70) signal = 'med';

                    // Khóa nếu có pass
                    const lockIcon = net.encryption === 'OPEN' ? '' : '🔒';

                    li.innerHTML = `
                        <span>${lockIcon} <b>${net.ssid}</b></span>
                        <small>${net.rssi} dBm</small>
                    `;
                    li.onclick = () => {
                        document.getElementById('ssid').value = net.ssid;
                        document.getElementById('password').focus();
                        list.style.display = 'none';
                    };
                    list.appendChild(li);
                });
            })
            .catch(() => {
                list.innerHTML = '<li style="justify-content:center; color:red">Lỗi khi quét!</li>';
            });
    }

    // 3. Logic ẩn hiện pass
    function togglePass() {
        const input = document.getElementById('password');
        input.type = input.type === 'password' ? 'text' : 'password';
    }
</script>

</body>
</html>
)raw_string";

#endif