#ifndef WEB_LOGGER_H
#define WEB_LOGGER_H

#include <Arduino.h>

constexpr size_t WEB_LOG_ENTRY_MSG_SIZE = 1024;

struct WebLogEntry {
    uint32_t id;
    uint32_t t;
    uint8_t level;
    char msg[WEB_LOG_ENTRY_MSG_SIZE];
};

class WebLogger {
  public:
    void logf(uint8_t level, const char* tag, const char* format, ...);
    String getLogsJSON(uint32_t sinceId);
    void clear();

  private:
    static constexpr size_t kMaxEntries = 200;
    WebLogEntry entries[kMaxEntries]{};
    size_t head = 0;
    size_t count = 0;
    uint32_t nextId = 0;

    void appendEscaped(String& out, const char* text);
};

extern WebLogger webLogger;

#endif // WEB_LOGGER_H
