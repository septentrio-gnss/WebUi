#include "web/WebServerManager.h"
#include "AppGlobals.h"
#include "utils/BootProfiler.h"

static const char* wifiStatusToText(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
        case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
        case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
        case WL_CONNECTED: return "WL_CONNECTED";
        case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
        case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
        case WL_DISCONNECTED: return "WL_DISCONNECTED";
        default: return "UNKNOWN";
    }
}

static void printStaDiagnostics() {
    wl_status_t status = WiFi.status();

    Serial.printf("[STA] Status: %d (%s) | RSSI: %d dBm | STA IP: %s\n",
                  status,
                  wifiStatusToText(status),
                  WiFi.RSSI(),
                  WiFi.localIP().toString().c_str());
}

static void scanForConfiguredSsid() {
    Serial.printf("[STA] Scanning for configured SSID '%s'...\n", WIFI_SSID.c_str());

    int networkCount = WiFi.scanNetworks(false, true);

    if (networkCount <= 0) {
        Serial.printf("[STA] Scan result: no networks found or scan failed (%d).\n", networkCount);
        WiFi.scanDelete();
        return;
    }

    bool found = false;

    for (int i = 0; i < networkCount; i++) {
        if (WiFi.SSID(i) == WIFI_SSID) {
            found = true;
            Serial.printf("[STA] Target SSID found | channel=%d | RSSI=%d dBm | encryption=%d\n",
                          WiFi.channel(i),
                          WiFi.RSSI(i),
                          WiFi.encryptionType(i));
        }
    }

    if (!found) {
        Serial.println("[STA] Target SSID NOT found. Check hotspot is ON, 2.4 GHz compatible, and SSID spelling.");
    }

    WiFi.scanDelete();
}

void initFileSystem() {
    if (!SPIFFS.begin(true)) {
        Serial.println("CRITICAL: SPIFFS mount failed! Halting.");
        while (1) delay(1000);
    }

    Serial.println("SPIFFS for web files OK.");
}

void initWiFiAndMdns() {
    WiFi.persistent(false);

    // Clean any previous STA/AP state from the WiFi driver
    WiFi.disconnect(true, true);
    delay(300);

    // Force pure Access Point mode only
    WiFi.mode(WIFI_AP);
    delay(300);

    WiFi.setHostname("dualy-esp32");

    IPAddress local_AP_IP(192, 168, 3, 1);
    IPAddress gateway(192, 168, 3, 1);
    IPAddress subnet(255, 255, 255, 0);

    Serial.println("\nStarting PURE AP mode 'DUALY_CONFIG'...");

    if (!WiFi.softAPConfig(local_AP_IP, gateway, subnet)) {
        Serial.println("Error: AP configuration failed!");
    }

    bool apStarted = WiFi.softAP("DUALY_CONFIG", nullptr, 6, false, 4);

    delay(300);

    Serial.printf("AP started: %s\n", apStarted ? "YES" : "NO");

    Serial.print("AP SSID: ");
    Serial.println(WiFi.softAPSSID());

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    Serial.print("AP channel: ");
    Serial.println(WiFi.channel());

    Serial.println("PURE AP mode active. STA is disabled for AP recovery test.");

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

        if (SPIFFS.exists("/index.html.gz")) {
            static bool firstIndexServedLogged = false;

            if (!firstIndexServedLogged) {
                BOOT_LOG("Serving first compressed WebUI index.html.gz");
                firstIndexServedLogged = true;
            }

            AsyncWebServerResponse *response =
                request->beginResponse(SPIFFS, "/index.html.gz", "text/html");

            response->addHeader("Content-Encoding", "gzip");
            response->addHeader("Cache-Control", "no-cache");
            request->send(response);
            return;
        }

        if (SPIFFS.exists("/index.html")) {
            static bool firstIndexServedLogged = false;

            if (!firstIndexServedLogged) {
                BOOT_LOG("Serving first WebUI index.html");
                firstIndexServedLogged = true;
            }

            AsyncWebServerResponse *response =
                request->beginResponse(SPIFFS, "/index.html", "text/html");

            response->addHeader("Cache-Control", "no-cache");
            request->send(response);
            return;
        }

        Serial.println("[HTTP ERROR] /index.html.gz and /index.html not found in SPIFFS");
        request->send(404, "text/plain", "index.html not found.");
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        String path = request->url();

        if (path.endsWith("/")) {
            path += "index.html";
        }

        String pathGz = path + ".gz";
        String contentType = "text/plain";

        if (path.endsWith(".html")) contentType = "text/html";
        else if (path.endsWith(".css")) contentType = "text/css";
        else if (path.endsWith(".js")) contentType = "application/javascript";
        else if (path.endsWith(".png")) contentType = "image/png";
        else if (path.endsWith(".svg")) contentType = "image/svg+xml";

        if (SPIFFS.exists(pathGz)) {
            AsyncWebServerResponse *response =
                request->beginResponse(SPIFFS, pathGz, contentType);

            response->addHeader("Content-Encoding", "gzip");
            response->addHeader("Cache-Control", "public, max-age=3600");
            request->send(response);
            return;
        }

        if (SPIFFS.exists(path)) {
            AsyncWebServerResponse *response =
                request->beginResponse(SPIFFS, path, contentType);

            if (path.endsWith(".css") || path.endsWith(".js") ||
                path.endsWith(".svg") || path.endsWith(".png")) {
                response->addHeader("Cache-Control", "public, max-age=3600");
            } else {
                response->addHeader("Cache-Control", "no-cache");
            }

            request->send(response);
            return;
        }

        Serial.printf("Not Found: %s\n", path.c_str());
        request->send(404, "text/plain", "Not Found");
    });

    server.begin();
}

void delayedStaTask(void *parameter) {
    BOOT_LOG("STA background task started");

    // Give user enough time to connect to AP and open WebUI before STA changes WiFi behavior.
    vTaskDelay(pdMS_TO_TICKS(8000));

    if (!WIFI_STA_ENABLED || WIFI_SSID.length() == 0) {
        Serial.println("[STA] Not started: WiFi client disabled or SSID missing.");
        vTaskDelete(NULL);
        return;
    }

    BOOT_LOG("Starting STA WiFi connection in background");

    WiFi.persistent(false);

    // IMPORTANT:
    // ESP32-S3 crashes if WiFi sleep is disabled while BLE is active.
    // Keep modem sleep enabled when WiFi + BLE coexist.
    WiFi.setSleep(true);

    WiFi.setAutoReconnect(true);

    Serial.printf("[STA] Before AP_STA | AP SSID: %s | AP IP: %s | AP clients: %d | channel: %d\n",
              WiFi.softAPSSID().c_str(),
              WiFi.softAPIP().toString().c_str(),
              WiFi.softAPgetStationNum(),
              WiFi.channel());

    // Keep the WebUI AP alive, then enable STA only after the WebUI is already available.
    WiFi.mode(WIFI_AP_STA);
    vTaskDelay(pdMS_TO_TICKS(200));

    Serial.printf("[STA] AP still available | SSID: %s | IP: %s | channel: %d\n",
                  WiFi.softAPSSID().c_str(),
                  WiFi.softAPIP().toString().c_str(),
                  WiFi.channel());

    //scanForConfiguredSsid();

    if (NTRIP_AUTO_CONNECT &&
        NTRIP_HOST.length() > 0 &&
        NTRIP_MOUNT.length() > 0) {
        ntripUserRequestConnect = true;
        Serial.println("[NTRIP] Auto-connect armed. NTRIP runtime will wait for STA if needed.");
    } else {
        Serial.println("[NTRIP] Auto-connect not armed: missing NTRIP config.");
    }

    WiFi.disconnect(false, false);
    vTaskDelay(pdMS_TO_TICKS(200));

    Serial.printf("[STA] Connecting to SSID '%s'...\n", WIFI_SSID.c_str());
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());

    unsigned long startAttempt = millis();
    unsigned long lastDiagnostic = 0;

    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");

        if (millis() - lastDiagnostic > 3000) {
            Serial.println();
            printStaDiagnostics();
            lastDiagnostic = millis();
        }
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        BOOT_LOG("STA WiFi connected in background");
        Serial.print("[STA] Connected. STA IP: ");
        Serial.println(WiFi.localIP());
        Serial.printf("[STA] Connected channel: %d | RSSI: %d dBm\n", WiFi.channel(), WiFi.RSSI());

        if (ntripUserRequestConnect) {
            Serial.println("[NTRIP] Auto-connect request already active after STA connection.");
        } else {
            Serial.println("[NTRIP] Auto-connect skipped: missing NTRIP config.");
        }
    } else {
        Serial.printf("[STA] Connection failed. Final status: %d (%s)\n",
                      WiFi.status(),
                      wifiStatusToText(WiFi.status()));
        Serial.println("[STA] AP remains available. WebUI stays reachable for configuration/debug.");
    }

    vTaskDelete(NULL);
}