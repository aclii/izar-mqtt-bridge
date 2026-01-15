#include "web_config_server.h"
#include <DNSServer.h>
#include <WiFi.h>
#include <Update.h>
#include "wifi_manager.h"
#include "web_logger.h"

WebConfigServer webConfigServer(&configManager);

namespace {
DNSServer dnsServer;
}

namespace {
const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IZAR MQTT Configuration</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 640px;
            margin: 0 auto;
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 24px;
            text-align: center;
        }
        .header h1 { font-size: 22px; margin-bottom: 6px; }
        .header p { opacity: 0.9; font-size: 13px; }
        .content { padding: 24px; }
        .section {
            margin-bottom: 22px;
            padding-bottom: 18px;
            border-bottom: 1px solid #e0e0e0;
        }
        .section:last-child { border-bottom: none; }
        .section h2 {
            font-size: 16px;
            color: #667eea;
            margin-bottom: 12px;
        }
        .form-group { margin-bottom: 12px; }
        label {
            display: block;
            font-size: 13px;
            font-weight: 600;
            color: #333;
            margin-bottom: 6px;
        }
        input {
            width: 100%;
            padding: 10px;
            border: 2px solid #e0e0e0;
            border-radius: 5px;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        input:focus { outline: none; border-color: #667eea; }
        input[type="number"] { width: 140px; }
        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 26px;
            border-radius: 5px;
            font-size: 15px;
            font-weight: 600;
            cursor: pointer;
            width: 100%;
            margin-top: 8px;
        }
        .btn-danger { background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); }
        .status {
            padding: 12px;
            border-radius: 5px;
            margin-top: 16px;
            display: none;
        }
        .status.success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .status.error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>IZAR MQTT Bridge Configuration</h1>
            <p>WiFi, MQTT, and IZAR settings</p>
        </div>
        <div class="content">
            <form id="configForm">
                <div class="section">
                    <h2>WiFi Settings</h2>
                    <div class="form-group">
                        <label for="wifiSSID">WiFi SSID</label>
                        <input type="text" id="wifiSSID" name="wifiSSID" required>
                    </div>
                    <div class="form-group">
                        <label for="wifiPassword">WiFi Password</label>
                        <input type="password" id="wifiPassword" name="wifiPassword">
                    </div>
                    <div class="form-group">
                        <label for="macAddress">MAC Address</label>
                        <input type="text" id="macAddress" name="macAddress" readonly>
                    </div>
                    <div class="form-group">
                        <label for="hostname">Hostname</label>
                        <input type="text" id="hostname" name="hostname" required>
                    </div>
                </div>

                <div class="section">
                    <h2>MQTT Settings</h2>
                    <div class="form-group">
                        <label for="mqttBroker">MQTT Broker (IP or Hostname)</label>
                        <input type="text" id="mqttBroker" name="mqttBroker" placeholder="homeassistant.local" required>
                    </div>
                    <div class="form-group">
                        <label for="mqttPort">MQTT Port</label>
                        <input type="number" id="mqttPort" name="mqttPort" min="1" max="65535" required>
                    </div>
                    <div class="form-group">
                        <label for="mqttUsername">MQTT Username (optional)</label>
                        <input type="text" id="mqttUsername" name="mqttUsername">
                    </div>
                    <div class="form-group">
                        <label for="mqttPassword">MQTT Password (optional)</label>
                        <input type="password" id="mqttPassword" name="mqttPassword">
                    </div>
                    <div class="form-group">
                        <label for="mqttClientId">MQTT Client ID</label>
                        <input type="text" id="mqttClientId" name="mqttClientId" required>
                    </div>
                    <div class="form-group">
                        <label for="mqttBaseTopic">Base MQTT Topic</label>
                        <input type="text" id="mqttBaseTopic" name="mqttBaseTopic" required>
                    </div>
                </div>

                <div class="section">
                    <h2>IZAR Settings</h2>
                    <div class="form-group">
                        <label for="serialNumber">Serial Number</label>
                        <input type="text" id="serialNumber" name="serialNumber" placeholder="Leave empty for discovery mode">
                    </div>
                </div>

                <button type="submit" class="btn">💾 Save Configuration</button>
                <button type="button" class="btn btn-danger" onclick="resetConfig()">🔄 Reset to Defaults</button>
                <div id="status" class="status"></div>
            </form>

            <div class="section" style="margin-top: 20px; text-align: center;">
                <a href="/update" style="color: #667eea; text-decoration: none; font-weight: 600;">
                    🚀 Firmware Update
                </a>
                <span style="margin: 0 10px; color: #ccc;">|</span>
                <a href="/logs" style="color: #667eea; text-decoration: none; font-weight: 600;">
                    📋 View Logs
                </a>
            </div>
        </div>
    </div>

    <script>
        window.addEventListener('DOMContentLoaded', () => {
            fetch('/api/config')
                .then(response => response.json())
                .then(data => {
                    Object.keys(data).forEach(key => {
                        const element = document.getElementById(key);
                        if (element) {
                            element.value = data[key];
                        }
                    });
                })
                .catch(err => console.error('Error loading config:', err));
        });

        document.getElementById('configForm').addEventListener('submit', async (e) => {
            e.preventDefault();

            const formData = new FormData(e.target);
            const config = Object.fromEntries(formData.entries());
            config.mqttPort = parseInt(config.mqttPort);

            try {
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                });

                const result = await response.json();
                showStatus(result.success ? 'success' : 'error', result.message);

                if (result.success) {
                    setTimeout(() => {
                        showStatus('success', 'Rebooting device...');
                        setTimeout(() => location.reload(), 3000);
                    }, 1000);
                }
            } catch (err) {
                showStatus('error', 'Failed to save configuration: ' + err.message);
            }
        });

        function showStatus(type, message) {
            const status = document.getElementById('status');
            status.className = 'status ' + type;
            status.textContent = message;
            status.style.display = 'block';
        }

        async function resetConfig() {
            if (!confirm('Reset all settings to defaults? This will restart the device.')) {
                return;
            }
            try {
                const response = await fetch('/api/reset', { method: 'POST' });
                const result = await response.json();
                showStatus('success', result.message + ' Rebooting...');
                setTimeout(() => location.reload(), 3000);
            } catch (err) {
                showStatus('error', 'Failed to reset: ' + err.message);
            }
        }
    </script>
</body>
</html>
)rawliteral";
}

WebConfigServer::WebConfigServer(ConfigManager* cm) : server(80), configManager(cm) {}

void WebConfigServer::begin() {
    setupRoutes();
    server.begin();
    startDnsIfNeeded();
    LOG_INFO("Web", "Web configuration server started on port 80");
}

void WebConfigServer::handle() {
    startDnsIfNeeded();
    if (dnsStarted) {
        dnsServer.processNextRequest();
    }
}

void WebConfigServer::stop() {
    server.end();
    if (dnsStarted) {
        dnsServer.stop();
        dnsStarted = false;
    }
}

void WebConfigServer::setupRoutes() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(200, "text/html", CONFIG_HTML); });

    server.onNotFound([](AsyncWebServerRequest* request) {
        if (wifiManager.isApModeActive()) {
            request->redirect("/");
            return;
        }
        request->send(404, "text/plain", "Not Found");
    });

    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        const Config& config = configManager->getConfig();

        StaticJsonDocument<768> doc;
        doc["wifiSSID"] = config.wifiSSID;
        doc["wifiPassword"] = "";
        doc["macAddress"] = WiFi.macAddress();
        doc["hostname"] = config.hostname;
        doc["mqttBroker"] = config.mqttBroker;
        doc["mqttPort"] = config.mqttPort;
        doc["mqttUsername"] = config.mqttUsername;
        doc["mqttPassword"] = "";
        doc["mqttClientId"] = config.mqttClientId;
        doc["mqttBaseTopic"] = config.mqttBaseTopic;
        doc["serialNumber"] = config.serialNumber;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on(
        "/api/config", HTTP_POST, [](AsyncWebServerRequest* request) {}, nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                request->_tempObject = new String();
                static_cast<String*>(request->_tempObject)->reserve(total);
            }

            String* body = static_cast<String*>(request->_tempObject);
            body->concat(reinterpret_cast<char*>(data), len);

            if (index + len != total) {
                return;
            }

            StaticJsonDocument<768> doc;
            DeserializationError error = deserializeJson(doc, *body);
            delete body;
            request->_tempObject = nullptr;

            if (error) {
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
                return;
            }

            Config& config = configManager->getConfig();

            if (doc.containsKey("wifiSSID")) {
                String ssid = String(doc["wifiSSID"] | "");
                ssid.trim();
                strlcpy(config.wifiSSID, ssid.c_str(), sizeof(config.wifiSSID));
            }
            if (doc.containsKey("wifiPassword")) {
                const char* wifiPassword = doc["wifiPassword"] | "";
                if (strlen(wifiPassword) > 0) {
                    strlcpy(config.wifiPassword, wifiPassword, sizeof(config.wifiPassword));
                }
            }
            if (doc.containsKey("hostname")) {
                String hostname = String(doc["hostname"] | "");
                hostname.trim();
                strlcpy(config.hostname, hostname.c_str(), sizeof(config.hostname));
            }

            if (doc.containsKey("mqttBroker"))
                strlcpy(config.mqttBroker, doc["mqttBroker"] | "", sizeof(config.mqttBroker));
            if (doc.containsKey("mqttPort"))
                config.mqttPort = doc["mqttPort"] | MQTT_PORT_DEFAULT;
            if (doc.containsKey("mqttUsername"))
                strlcpy(config.mqttUsername, doc["mqttUsername"] | "", sizeof(config.mqttUsername));
            if (doc.containsKey("mqttPassword")) {
                const char* mqttPassword = doc["mqttPassword"] | "";
                if (strlen(mqttPassword) > 0) {
                    strlcpy(config.mqttPassword, mqttPassword, sizeof(config.mqttPassword));
                }
            }
            if (doc.containsKey("mqttClientId"))
                strlcpy(config.mqttClientId, doc["mqttClientId"] | "", sizeof(config.mqttClientId));
            if (doc.containsKey("mqttBaseTopic"))
                strlcpy(config.mqttBaseTopic, doc["mqttBaseTopic"] | "", sizeof(config.mqttBaseTopic));

            if (doc.containsKey("serialNumber")) {
                String serial = String(doc["serialNumber"] | "");
                serial.trim();
                strlcpy(config.serialNumber, serial.c_str(), sizeof(config.serialNumber));
            }

            configManager->save();

            request->send(200, "application/json", "{\"success\":true,\"message\":\"Configuration saved\"}");
            delay(1000);
            ESP.restart();
        });

    server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        configManager->reset();
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Configuration reset\"}");
        delay(1000);
        ESP.restart();
    });

    server.on("/logs", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Device Logs</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Courier New', monospace; background: #1e1e1e; color: #d4d4d4; padding: 10px; }
        .header { background: #252526; padding: 12px; border-radius: 6px; margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; gap: 10px; flex-wrap: wrap; }
        .header h1 { font-size: 18px; color: #4ec9b0; }
        .controls { display: flex; gap: 8px; align-items: center; }
        .btn { padding: 6px 12px; border: 1px solid #3c3c3c; background: #2d2d30; color: #d4d4d4; border-radius: 3px; cursor: pointer; font-size: 12px; }
        .btn:hover { background: #3e3e42; border-color: #007acc; }
        .log-container { background: #252526; border: 1px solid #3c3c3c; border-radius: 5px; height: calc(100vh - 110px); overflow-y: auto; padding: 10px; font-size: 12px; line-height: 1.5; }
        .log-entry { padding: 3px 0; border-bottom: 1px solid #2d2d30; }
        .log-entry:last-child { border-bottom: none; }
        .log-time { color: #858585; margin-right: 8px; }
        .log-level { display: inline-block; padding: 2px 6px; border-radius: 3px; font-weight: bold; margin-right: 8px; min-width: 56px; text-align: center; font-size: 10px; }
        .level-1 { background: #f14c4c; color: white; }
        .level-2 { background: #cca700; color: white; }
        .level-3 { background: #007acc; color: white; }
        .level-4 { background: #4ec9b0; color: white; }
        .back-link { color: #4ec9b0; text-decoration: none; font-size: 12px; }
        .back-link:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="header">
        <div>
            <h1>📋 Device Logs</h1>
            <a href="/" class="back-link">← Back to Config</a>
        </div>
        <div class="controls">
            <button class="btn" id="autoScrollBtn">Auto-scroll: ON</button>
            <button class="btn" id="clearBtn">Clear</button>
        </div>
    </div>
    <div class="log-container" id="logContainer"></div>

    <script>
        let autoScroll = true;
        let lastId = 0;
        const logContainer = document.getElementById('logContainer');
        const autoScrollBtn = document.getElementById('autoScrollBtn');
        const clearBtn = document.getElementById('clearBtn');

        autoScrollBtn.addEventListener('click', () => {
            autoScroll = !autoScroll;
            autoScrollBtn.textContent = 'Auto-scroll: ' + (autoScroll ? 'ON' : 'OFF');
            if (autoScroll) logContainer.scrollTop = logContainer.scrollHeight;
        });

        clearBtn.addEventListener('click', async () => {
            await fetch('/api/logs/clear', { method: 'POST' });
            logContainer.innerHTML = '';
            lastId = 0;
        });

        async function fetchLogs() {
            try {
                const response = await fetch('/api/logs?since_id=' + lastId);
                const logs = await response.json();
                if (logs.length) {
                    logs.forEach(addLogEntry);
                    lastId = Math.max(lastId, logs[logs.length - 1].i);
                    if (autoScroll) logContainer.scrollTop = logContainer.scrollHeight;
                }
            } catch (err) {
                // ignore
            }
        }

        function addLogEntry(log) {
            const levelNames = { 1: 'ERROR', 2: 'WARN', 3: 'INFO', 4: 'DEBUG' };
            const level = levelNames[log.l] || 'LOG';
            const entry = document.createElement('div');
            entry.className = 'log-entry';
            entry.innerHTML = '<span class="log-time">' + formatTime(log.t) + '</span>' +
                              '<span class="log-level level-' + log.l + '">' + level + '</span>' +
                              '<span>' + escapeHtml(log.m) + '</span>';
            logContainer.appendChild(entry);
        }

        function formatTime(ms) {
            const seconds = Math.floor(ms / 1000);
            const minutes = Math.floor(seconds / 60);
            const hours = Math.floor(minutes / 60);
            const h = hours.toString().padStart(2, '0');
            const m = (minutes % 60).toString().padStart(2, '0');
            const s = (seconds % 60).toString().padStart(2, '0');
            const millis = (ms % 1000).toString().padStart(3, '0');
            return h + ':' + m + ':' + s + '.' + millis;
        }

        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }

        setInterval(fetchLogs, 1000);
        fetchLogs();
    </script>
</body>
</html>
        )rawliteral");
    });

    server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest* request) {
        uint32_t sinceId = 0;
        if (request->hasParam("since_id")) {
            sinceId = request->getParam("since_id")->value().toInt();
        }
        String json = webLogger.getLogsJSON(sinceId);
        request->send(200, "application/json", json);
    });

    server.on("/api/logs/clear", HTTP_POST, [](AsyncWebServerRequest* request) {
        webLogger.clear();
        request->send(200, "application/json", "{\"success\":true}");
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Firmware Update</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .container {
            max-width: 520px;
            width: 100%;
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 24px;
            text-align: center;
        }
        .content { padding: 24px; }
        .upload-area {
            border: 3px dashed #667eea;
            border-radius: 10px;
            padding: 32px 16px;
            text-align: center;
            cursor: pointer;
            margin-bottom: 16px;
        }
        input[type="file"] { display: none; }
        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 5px;
            font-size: 15px;
            font-weight: 600;
            cursor: pointer;
            width: 100%;
        }
        .btn:disabled { background: #ccc; cursor: not-allowed; }
        .progress { margin-top: 16px; display: none; }
        .progress-bar { height: 18px; background: #e0e0e0; border-radius: 9px; overflow: hidden; }
        .progress-fill { height: 100%; width: 0%; background: #667eea; color: white; font-size: 12px; text-align: center; }
        .status { margin-top: 12px; display: none; }
        .back-link { display: block; text-align: center; margin-top: 16px; color: #667eea; text-decoration: none; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚀 Firmware Update</h1>
            <p>Upload a new firmware binary</p>
        </div>
        <div class="content">
            <div class="upload-area" id="uploadArea">
                <div>Click or drop firmware.bin here</div>
                <input type="file" id="fileInput" accept=".bin">
            </div>
            <button class="btn" id="uploadBtn" disabled>Upload Firmware</button>
            <div class="progress" id="progress">
                <div class="progress-bar">
                    <div class="progress-fill" id="progressFill">0%</div>
                </div>
            </div>
            <div class="status" id="status"></div>
            <a href="/" class="back-link">← Back to Configuration</a>
        </div>
    </div>
    <script>
        const uploadArea = document.getElementById('uploadArea');
        const fileInput = document.getElementById('fileInput');
        const uploadBtn = document.getElementById('uploadBtn');
        const progress = document.getElementById('progress');
        const progressFill = document.getElementById('progressFill');
        const status = document.getElementById('status');
        let selectedFile = null;

        uploadArea.addEventListener('click', () => fileInput.click());
        uploadArea.addEventListener('dragover', (e) => { e.preventDefault(); });
        uploadArea.addEventListener('drop', (e) => {
            e.preventDefault();
            if (e.dataTransfer.files.length) handleFile(e.dataTransfer.files[0]);
        });
        fileInput.addEventListener('change', (e) => {
            if (e.target.files.length) handleFile(e.target.files[0]);
        });

        function handleFile(file) {
            if (!file.name.endsWith('.bin')) {
                showStatus('Please select a .bin file');
                return;
            }
            selectedFile = file;
            uploadBtn.disabled = false;
        }

        uploadBtn.addEventListener('click', () => {
            if (!selectedFile) return;
            uploadBtn.disabled = true;
            progress.style.display = 'block';

            const formData = new FormData();
            formData.append('firmware', selectedFile);

            const xhr = new XMLHttpRequest();
            xhr.upload.addEventListener('progress', (e) => {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    progressFill.style.width = percent + '%';
                    progressFill.textContent = percent + '%';
                }
            });
            xhr.addEventListener('load', () => {
                if (xhr.status === 200) {
                    showStatus('Upload successful. Rebooting...');
                } else {
                    showStatus('Upload failed: ' + xhr.responseText);
                    uploadBtn.disabled = false;
                }
            });
            xhr.addEventListener('error', () => {
                showStatus('Upload failed. Please try again.');
                uploadBtn.disabled = false;
            });
            xhr.open('POST', '/update');
            xhr.send(formData);
        });

        function showStatus(message) {
            status.textContent = message;
            status.style.display = 'block';
        }
    </script>
</body>
</html>
        )rawliteral");
    });

    server.on(
        "/update", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            bool ok = !Update.hasError();
            AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", ok ? "OK" : "FAIL");
            response->addHeader("Connection", "close");
            request->send(response);
            if (ok) {
                delay(1000);
                ESP.restart();
            }
        },
        [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
            if (!index) {
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }
            if (!Update.hasError()) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
            }
            if (final) {
                if (!Update.end(true)) {
                    Update.printError(Serial);
                }
            }
        });
}

void WebConfigServer::startDnsIfNeeded() {
    if (dnsStarted) {
        return;
    }

    if (!wifiManager.isApModeActive()) {
        return;
    }

    dnsServer.start(53, "*", WiFi.softAPIP());
    dnsStarted = true;
    LOG_INFO("Web", "DNS captive portal started");
}
