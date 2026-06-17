#include "web/WebServerManager.h"
#include "AppGlobals.h"
#include "utils/BootProfiler.h"

void initFileSystem() {
    if (!SPIFFS.begin(true)) {
        Serial.println("CRITICAL: SPIFFS mount failed! Halting.");
        while (1) delay(1000);
    }

    Serial.println("SPIFFS for web files OK.");
}

void initWiFiAndMdns() {
    WiFi.setHostname("dualy-esp32");

    IPAddress local_AP_IP(192, 168, 3, 1);
    IPAddress gateway(192, 168, 3, 1);
    IPAddress subnet(255, 255, 255, 0);

    Serial.println("\nStarting AP 'DUALY_CONFIG'...");

    if (WIFI_STA_ENABLED && WIFI_SSID.length() > 0) {
        Serial.println("Mode: AP + STA (Station enabled)");
        WiFi.mode(WIFI_AP_STA);
    } else {
        Serial.println("Mode: AP Only (Station disabled)");
        WiFi.mode(WIFI_AP);
    }

    if (!WiFi.softAPConfig(local_AP_IP, gateway, subnet)) {
        Serial.println("Error: AP configuration failed!");
    }

    WiFi.softAP("DUALY_CONFIG");

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    if (WIFI_STA_ENABLED && WIFI_SSID.length() > 0) {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
        Serial.println("Station WiFi connection started in background.");
    } else {
        Serial.println("Station WiFi not started.");
    }

    if (MDNS.begin("dualy")) {
        Serial.println("mDNS started: http://dualy.local");
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("mDNS Error. Direct IP still available: http://192.168.3.1");
    }
}

void initHttpServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        static bool firstHttpRequestLogged = false;

        if (!firstHttpRequestLogged) {
            BOOT_LOG("First HTTP request received for /");
            firstHttpRequestLogged = true;
        }

        if (!SPIFFS.exists("/index.html")) {
            Serial.println("[HTTP ERROR] /index.html not found in SPIFFS");
            request->send(404, "text/plain", "index.html not found.");
            return;
        }

        static bool firstIndexServedLogged = false;

        if (!firstIndexServedLogged) {
            BOOT_LOG("Serving first WebUI index.html");
            firstIndexServedLogged = true;
        }

        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html", "text/html");
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        String path = request->url();
        if (path.endsWith("/")) path += "index.html";

        String pathGz = path + ".gz";
        String contentType = "text/plain";

        if (path.endsWith(".html")) contentType = "text/html";
        else if (path.endsWith(".css")) contentType = "text/css";
        else if (path.endsWith(".js")) contentType = "application/javascript";
        else if (path.endsWith(".png")) contentType = "image/png";
        else if (path.endsWith(".svg")) contentType = "image/svg+xml";

        if (SPIFFS.exists(pathGz)) {
            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, pathGz, contentType);
            response->addHeader("Content-Encoding", "gzip");
            response->addHeader("Cache-Control", "public, max-age=3600");
            request->send(response);
        } else if (SPIFFS.exists(path)) {
            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, contentType);

            if (path.endsWith(".css") || path.endsWith(".js") || path.endsWith(".svg") || path.endsWith(".png")) {
                response->addHeader("Cache-Control", "public, max-age=3600");
            } else {
                response->addHeader("Cache-Control", "no-cache");
            }

            request->send(response);
        } else {
            Serial.printf("Not Found: %s\n", path.c_str());
            request->send(404, "text/plain", "Not Found");
        }
    });

    server.begin();
}
