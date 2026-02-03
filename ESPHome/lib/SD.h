#pragma once

#if defined(USE_ESP8266) && !defined(SDCARD)
#include <FS.h>

#ifndef FILE_READ
#define FILE_READ "r"
#endif

class SDClass {
 public:
  bool begin(uint8_t = 0) { return false; }
  fs::File open(const char *path, const char *mode = FILE_READ) { return fs::File(); }
};

static SDClass SD;

#else
#include_next <SD.h>
#endif
