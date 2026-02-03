#pragma once

#include "esphome/core/component.h"

#include <cstddef>
#include <cstdint>

class AudioOutput;
class AudioGenerator;
class AudioFileSource;
class AudioFileSourceBuffer;

namespace esphome {
namespace espaudio {

class ESPAudio : public Component {
 public:
  enum GeneratorType { WAV, MP3, UNK };

  void setup() override;
  void loop() override;

  void stop();
  bool play_file(const char *filename, GeneratorType type);
  bool play_stream(const char *url, GeneratorType type);
  bool play_data(const uint8_t *data, size_t size, GeneratorType type);
  void play(const char *filename);
  bool is_playing() const;

 protected:
  ::AudioOutput *out_{};
  ::AudioGenerator *gen_{};
  ::AudioFileSource *src_{};
  ::AudioFileSourceBuffer *buf_{};
  bool fs_initialized_{};

  ::AudioFileSource *open_file_(const char *filename);
  ::AudioFileSource *open_stream_(const char *url);
  ::AudioFileSource *open_data_(const uint8_t *data, size_t size);
  bool play_(::AudioFileSource *src, GeneratorType type);
  GeneratorType get_file_type_(const char *filename) const;
  bool is_stream_(const char *filename) const;
};

}  // namespace espaudio
}  // namespace esphome
