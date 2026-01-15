#include "web_logger.h"
#include <stdarg.h>

WebLogger webLogger;

void WebLogger::logf(uint8_t level, const char* tag, const char* format, ...) {
    WebLogEntry& entry = entries[head];
    entry.id = ++nextId;
    entry.t = millis();
    entry.level = level;
    int prefixLen = snprintf(entry.msg, sizeof(entry.msg), "[%s] ", tag);
    if (prefixLen < 0) {
        entry.msg[0] = '\0';
        prefixLen = 0;
    }

    if (static_cast<size_t>(prefixLen) < sizeof(entry.msg)) {
        va_list args;
        va_start(args, format);
        vsnprintf(entry.msg + prefixLen, sizeof(entry.msg) - static_cast<size_t>(prefixLen), format, args);
        va_end(args);
    }

    head = (head + 1) % kMaxEntries;
    if (count < kMaxEntries) {
        count++;
    }
}

String WebLogger::getLogsJSON(uint32_t sinceId) {
    String json = "[";
    bool first = true;

    if (count == 0) {
        json += "]";
        return json;
    }

    size_t start = (head + kMaxEntries - count) % kMaxEntries;
    for (size_t i = 0; i < count; i++) {
        const WebLogEntry& entry = entries[(start + i) % kMaxEntries];
        if (entry.id <= sinceId) {
            continue;
        }

        if (!first) {
            json += ",";
        }
        first = false;

        json += "{\"i\":";
        json += entry.id;
        json += ",\"t\":";
        json += entry.t;
        json += ",\"l\":";
        json += entry.level;
        json += ",\"m\":\"";
        appendEscaped(json, entry.msg);
        json += "\"}";
    }

    json += "]";
    return json;
}

void WebLogger::clear() {
    head = 0;
    count = 0;
    nextId = 0;
}

void WebLogger::appendEscaped(String& out, const char* text) {
    for (const char* p = text; *p; ++p) {
        char c = *p;
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<uint8_t>(c) < 0x20) {
                // skip control chars
            } else {
                out += c;
            }
            break;
        }
    }
}
