#include "espaudio.h"

#include <cstring>

#include <Arduino.h>

#include "esphome/core/log.h"

#if defined(USE_ESP32) && !defined(USE_ESP32_VARIANT_ESP32C3)
#include "AudioOutputI2S.h"
#define ESPAUDIO_OUTPUT() new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC)
#else
#include "AudioOutputI2SNoDAC.h"
#define ESPAUDIO_OUTPUT() new AudioOutputI2SNoDAC()
#endif

#if defined(SDCARD)
#define ESPAUDIO_SD_HEADER <SD.h>
#include ESPAUDIO_SD_HEADER
#include "AudioFileSourceSD.h"
#define ESPAUDIO_FS SD
#define ESPAUDIO_FS_STR "SD"
using ESPAudioFileSource = AudioFileSourceSD;
#else
#include "LittleFS.h"
#include "AudioFileSourceLittleFS.h"
#define ESPAUDIO_FS LittleFS
#define ESPAUDIO_FS_STR "LittleFS"
using ESPAudioFileSource = AudioFileSourceLittleFS;
#endif

#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioFileSourcePROGMEM.h"

namespace esphome {
namespace espaudio {

static const char *const TAG = "espaudio";
static const uint8_t I2SO_DATA_PIN = 2;

void ESPAudio::setup() { this->fs_initialized_ = ESPAUDIO_FS.begin(); }

void ESPAudio::loop() {
  if (this->gen_ && this->gen_->isRunning()) {
    if (!this->gen_->loop()) {
      this->stop();
    }
  }
}

void ESPAudio::stop() {
  if (this->gen_ != nullptr) {
    this->gen_->stop();  // also stops output (out) and file (src)
    delete this->gen_;
    this->gen_ = nullptr;
  }
  if (this->out_ != nullptr) {
    delete this->out_;
    this->out_ = nullptr;
  }
  if (this->buf_ != nullptr) {
    delete this->buf_;
    this->buf_ = nullptr;
  }
  if (this->src_ != nullptr) {
    delete this->src_;
    this->src_ = nullptr;
  }

#if defined(USE_ESP8266)
  pinMode(I2SO_DATA_PIN, OUTPUT);
#endif

  ESP_LOGD(TAG, "Stopped playing");
}

bool ESPAudio::play_file(const char *filename, GeneratorType type) {
  this->stop();
  return this->play_(this->open_file_(filename), type);
}

bool ESPAudio::play_stream(const char *url, GeneratorType type) {
  this->stop();
  return this->play_(this->open_stream_(url), type);
}

bool ESPAudio::play_data(const uint8_t *data, size_t size, GeneratorType type) {
  this->stop();
  return this->play_(this->open_data_(data, size), type);
}

void ESPAudio::play(const char *filename) {
  auto type = this->get_file_type_(filename);

  bool play_result = false;
  if (type == MP3 || type == WAV) {
    play_result = this->is_stream_(filename) ? this->play_stream(filename, type) : this->play_file(filename, type);
  } else {
    ESP_LOGD(TAG, "Unknown audio file type %s", filename);
  }

  if (play_result) {
    ESP_LOGD(TAG, "Started playing audio file %s", filename);
  } else {
    ESP_LOGD(TAG, "Failed to play audio file %s", filename);
    this->stop();
  }
}

bool ESPAudio::is_playing() const { return this->gen_ && this->gen_->isRunning(); }

AudioFileSource *ESPAudio::open_file_(const char *filename) {
  if (!this->fs_initialized_) {
    // try mount again
    this->fs_initialized_ = ESPAUDIO_FS.begin();
    if (!this->fs_initialized_) {
      ESP_LOGD(TAG, ESPAUDIO_FS_STR " is not initialized");
      return nullptr;
    }
  }
  this->src_ = new ESPAudioFileSource();
  if (!this->src_->open(filename)) {
    ESP_LOGD(TAG, "Can't open file %s", filename);
    delete this->src_;
    this->src_ = nullptr;
    return nullptr;
  }
  return this->src_;
}

AudioFileSource *ESPAudio::open_stream_(const char *url) {
  this->src_ = new AudioFileSourceHTTPStream();
  if (!this->src_->open(url)) {
    ESP_LOGD(TAG, "Can't open http stream %s", url);
    delete this->src_;
    this->src_ = nullptr;
    return nullptr;
  }
  this->buf_ = new AudioFileSourceBuffer(this->src_, 2048);
  return this->buf_;
}

AudioFileSource *ESPAudio::open_data_(const uint8_t *data, size_t size) {
  this->src_ = new AudioFileSourcePROGMEM(data, size);
  if (!this->src_->isOpen()) {
    ESP_LOGD(TAG, "Can't read data array");
    delete this->src_;
    this->src_ = nullptr;
    return nullptr;
  }
  return this->src_;
}

bool ESPAudio::play_(AudioFileSource *src, GeneratorType type) {
  if (!src) {
    return false;
  }
  if (type == WAV) {
    this->gen_ = new AudioGeneratorWAV();
  } else if (type == MP3) {
    this->gen_ = new AudioGeneratorMP3();
  } else {
    return false;
  }
  this->out_ = ESPAUDIO_OUTPUT();
  return this->gen_->begin(src, this->out_);
}

ESPAudio::GeneratorType ESPAudio::get_file_type_(const char *filename) const {
  const char *dot = strrchr(filename, '.');
  if (!dot) {
    return UNK;
  }
  dot++;
  if ((dot[0] == 'w' || dot[0] == 'W') && (dot[1] == 'a' || dot[1] == 'A') && (dot[2] == 'v' || dot[2] == 'V') &&
      (dot[3] == 0)) {
    return WAV;
  }
  if ((dot[0] == 'm' || dot[0] == 'M') && (dot[1] == 'p' || dot[1] == 'P') && (dot[2] == '3') &&
      (dot[3] == 0)) {
    return MP3;
  }
  return UNK;
}

bool ESPAudio::is_stream_(const char *filename) const {
  const char *http = strstr(filename, "http://");
  const char *https = strstr(filename, "https://");
  return (http && (http - filename) == 0) || (https && (https - filename) == 0);
}

}  // namespace espaudio
}  // namespace esphome
