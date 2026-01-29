#define MINIAUDIO_IMPLEMENTATION
#include "../targets.h"
#include "AudioWrapper.h"
#include <stdexcept>
#include <SDL2/SDL.h>
#include "decoder.h"
#include <sndfile.h>
#include <stdio.h>
#include <mutex>
#include <portaudio.h>

// Define IAudioBackend interface here
struct AudioWrapper::IAudioBackend {
  virtual ~IAudioBackend() = default;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool isStarted() const = 0;
  virtual int getSampleRate() const = 0;
};

// Biquad Implementation
void Biquad::processStereo(float *buffer, size_t frameCount) {
  for (size_t i = 0; i < frameCount; ++i) {
    // Left Channel
    float inL = buffer[i * 2];
    float outL = inL * b0 + z1;
    z1 = inL * b1 + z2 - a1 * outL;
    z2 = inL * b2 - a2 * outL;
    buffer[i * 2] = outL;

    // Right Channel
    float inR = buffer[i * 2 + 1];
    float outR = inR * b0 + z1_r;
    z1_r = inR * b1 + z2_r - a1 * outR;
    z2_r = inR * b2 - a2 * outR;
    buffer[i * 2 + 1] = outR;
  }
}

void Biquad::setLowShelf(float fs, float f0, float gainDb, float Q) {
  float A = std::pow(10.0f, gainDb / 40.0f);
  float w0 = 2.0f * 3.14159265f * f0 / fs;
  float alpha = std::sin(w0) / (2.0f * Q);
  float cosw0 = std::cos(w0);

  float b0_tmp =
      A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alpha);
  float b1_tmp = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0);
  float b2_tmp =
      A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alpha);
  float a0_tmp = (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alpha;
  float a1_tmp = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0);
  float a2_tmp = (A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alpha;

  b0 = b0_tmp / a0_tmp;
  b1 = b1_tmp / a0_tmp;
  b2 = b2_tmp / a0_tmp;
  a1 = a1_tmp / a0_tmp;
  a2 = a2_tmp / a0_tmp;
}

void Biquad::setHighShelf(float fs, float f0, float gainDb, float Q) {
  float A = std::pow(10.0f, gainDb / 40.0f);
  float w0 = 2.0f * 3.14159265f * f0 / fs;
  float alpha = std::sin(w0) / (2.0f * Q);
  float cosw0 = std::cos(w0);

  float b0_tmp =
      A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alpha);
  float b1_tmp = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
  float b2_tmp =
      A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alpha);
  float a0_tmp = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alpha;
  float a1_tmp = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
  float a2_tmp = (A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alpha;

  b0 = b0_tmp / a0_tmp;
  b1 = b1_tmp / a0_tmp;
  b2 = b2_tmp / a0_tmp;
  a1 = a1_tmp / a0_tmp;
  a2 = a2_tmp / a0_tmp;
}

namespace {
// Mixing logic extracted to be backend-agnostic
void mixAudio(void *pOutput, ma_uint32 frameCount, int outputChannels,
              UserData *userData) {
  if (userData == nullptr)
    return;

  std::lock_guard<std::mutex> lock(*userData->mutex);
  if (!userData->stopwatch->isRunning())
    return;

  auto *soundDataList = userData->soundDataList;
  if (soundDataList == nullptr || soundDataList->empty())
    return;

  // Resize mix buffer if necessary
  size_t requiredSamples = frameCount * outputChannels;
  if (userData->mixBuffer->size() < requiredSamples) {
    userData->mixBuffer->resize(requiredSamples);
  }

  // Clear mix buffer
  std::fill(userData->mixBuffer->begin(),
            userData->mixBuffer->begin() + requiredSamples, 0.0f);
  float *mixBuffer = userData->mixBuffer->data();

  float gain = 0.9f;

  for (auto &soundData : *soundDataList) {
    if (!soundData->playing)
      continue;

    ma_uint32 framesToRead = frameCount;
    ma_uint32 framesAvailable =
        soundData->resampledFrameCount - soundData->currentFrame;
    if (framesToRead > framesAvailable) {
      framesToRead = framesAvailable;
    }

    const short *src = soundData->resampledData.data();
    size_t currentFrame = soundData->currentFrame;
    int channels = soundData->channels;

    for (ma_uint32 frame = 0; frame < framesToRead; ++frame) {
      for (int channel = 0; channel < channels; ++channel) {
        // Map input channel to output channel (simple modulo)
        int outputChannel = channel % outputChannels;

        // Convert int16 to float (-1.0 to 1.0 range approx)
        float sample =
            src[(currentFrame + frame) * channels + channel] / 32768.0f;

        mixBuffer[frame * outputChannels + outputChannel] += sample * gain;
      }
    }

    soundData->currentFrame += framesToRead;
    if (framesToRead < frameCount) {
      soundData->playing = false;
    }
  }

  // Apply Effects
  if (userData->bassFilter) {
    userData->bassFilter->processStereo(mixBuffer, frameCount);
  }
  if (userData->trebleFilter) {
    userData->trebleFilter->processStereo(mixBuffer, frameCount);
  }

  // Convert back to int16
  ma_int16 *outPtr = (ma_int16 *)pOutput;
  for (size_t i = 0; i < requiredSamples; ++i) {
    float sample = mixBuffer[i];

    // Hard clip
    if (sample > 1.0f)
      sample = 1.0f;
    if (sample < -1.0f)
      sample = -1.0f;

    outPtr[i] = (ma_int16)(sample * 32767.0f);
  }
}
} // namespace

// Miniaudio Backend Implementation
class MiniaudioBackend : public AudioWrapper::IAudioBackend {
public:
  MiniaudioBackend(UserData *userData) {
    ma_device_config deviceConfig =
        ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_s16;
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate = 0; // Use native sample rate
    deviceConfig.dataCallback = dataCallback;
    deviceConfig.pUserData = userData;

    if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
      throw std::runtime_error(
          "Failed to initialize miniaudio playback device.");
    }
    SDL_Log("[Miniaudio] Initialized with sample rate: %d", device.sampleRate);
  }

  ~MiniaudioBackend() override { ma_device_uninit(&device); }

  void start() override {
    if (!ma_device_is_started(&device)) {
      ma_device_start(&device);
      SDL_Log("[Miniaudio] Started playback device.");
    }
  }

  void stop() override {
    if (ma_device_is_started(&device)) {
      ma_device_stop(&device);
      SDL_Log("[Miniaudio] Stopped playback device.");
    }
  }

  bool isStarted() const override { return ma_device_is_started(&device); }

  int getSampleRate() const override { return device.sampleRate; }

private:
  ma_device device;

  static void dataCallback(ma_device *pDevice, void *pOutput,
                           const void *pInput, ma_uint32 frameCount) {
    auto *userData = (UserData *)pDevice->pUserData;
    // Miniaudio output matches the logic expected by mixAudio (int16 buffer)
    mixAudio(pOutput, frameCount, pDevice->playback.channels, userData);
  }
};

// PortAudio Backend Implementation
class PortAudioBackend : public AudioWrapper::IAudioBackend {
public:
  PortAudioBackend(UserData *userData)
      : userData(userData), stream(nullptr), sampleRate(44100) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
      SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "[PortAudio] init error: %s",
                   Pa_GetErrorText(err));
      throw std::runtime_error("Failed to initialize PortAudio");
    }

    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice(); // Default

// Try to find ASIO device on Windows
#ifdef TARGET_OS_WINDOWS
    int numDevices = Pa_GetDeviceCount();
    for (int i = 0; i < numDevices; ++i) {
      const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
      const PaHostApiInfo *hostApi = Pa_GetHostApiInfo(info->hostApi);
      if (hostApi && hostApi->type == paASIO) {
        outputParameters.device = i;
        SDL_Log("Found ASIO device: %s", info->name);
        break;
      }
    }
#endif

    if (outputParameters.device == paNoDevice) {
      throw std::runtime_error("No default output device.");
    }

    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(outputParameters.device);
    sampleRate = (int)deviceInfo->defaultSampleRate;

    outputParameters.channelCount = 2; // Stereo
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&stream,
                        nullptr, // No input
                        &outputParameters, (double)sampleRate,
                        paFramesPerBufferUnspecified,
                        paClipOff, // We clamp manually
                        paCallback, this);

    if (err != paNoError) {
      SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "[PortAudio] OpenStream error: %s",
                   Pa_GetErrorText(err));
      throw std::runtime_error("Failed to open audio stream");
    }
    SDL_Log("[PortAudio] Output device: %s", deviceInfo->name);
    SDL_Log("[PortAudio] Initialized with sample rate: %d", sampleRate);
  }

  ~PortAudioBackend() override {
    if (stream) {
      Pa_CloseStream(stream);
    }
    Pa_Terminate();
  }

  void start() override {
    if (stream && Pa_IsStreamStopped(stream)) {
      Pa_StartStream(stream);
      SDL_Log("[PortAudio] Started playback stream.");
    }
  }

  void stop() override {
    if (stream && !Pa_IsStreamStopped(stream)) {
      Pa_StopStream(stream);
      SDL_Log("[PortAudio] Stopped playback stream.");
    }
  }

  bool isStarted() const override {
    return stream && !Pa_IsStreamStopped(stream);
  }

  int getSampleRate() const override { return sampleRate; }

private:
  UserData *userData;
  PaStream *stream;
  int sampleRate;

  static int paCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags, void *userData) {
    auto *backend = (PortAudioBackend *)userData;
    // PortAudio requesting paInt16, so outputBuffer is int16*
    mixAudio(outputBuffer, (ma_uint32)framesPerBuffer, 2, backend->userData);
    return paContinue;
  }
};

// AudioWrapper Implementation

AudioWrapper::AudioWrapper(Stopwatch *stopwatch) : stopwatch(stopwatch) {
  userData.mutex = &soundDataListMutex;
  userData.soundDataList = &soundDataList;
  userData.stopwatch = stopwatch;
  userData.mixBuffer = &mixBuffer;
  userData.bassFilter = &bassFilter;
  userData.trebleFilter = &trebleFilter;

#if TARGET_OS_DESKTOP
  // Default to PortAudio on Desktop
  try {
    backend = std::make_unique<PortAudioBackend>(&userData);
    SDL_Log("Initialized PortAudio backend.");
  } catch (const std::exception &e) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO,
                 "Failed to initialize PortAudio backend: %s. Falling back to "
                 "Miniaudio.",
                 e.what());
    backend = std::make_unique<MiniaudioBackend>(&userData);
  }
#else
  // Default to Miniaudio on other platforms
  backend = std::make_unique<MiniaudioBackend>(&userData);
  SDL_Log("Initialized Miniaudio backend.");
#endif

  startDevice();

  // setBassBoost(0.0f);
  // setTrebleBoost(20.0f);
}

AudioWrapper::~AudioWrapper() {
  unloadSounds();
  // Backend destroyed via unique_ptr
}

int AudioWrapper::IAudioBackend::getSampleRate() const {
  return 44100;
} // Default if virtual fails

bool AudioWrapper::loadSound(const path_t &path,
                             std::atomic<bool> &isCancelled) {
  std::vector<short> pcmData;
  SF_INFO sfInfo;
  auto soundData = std::make_shared<SoundData>();
  bool result = decodeAudioToPCM(path, pcmData, sfInfo, isCancelled);
  if (!result) {
    SDL_Log("Failed to decode audio file %ls", path.c_str());
    return false;
  }
  if (isCancelled)
    return false;

  soundData->currentFrame = 0;
  soundData->channels = sfInfo.channels;
  soundData->originalSampleRate = sfInfo.samplerate;
  soundData->playing = false;

  int targetSampleRate = backend ? backend->getSampleRate() : 44100;
  if (targetSampleRate == 0)
    targetSampleRate = 44100;
  SDL_Log("Target sample rate: %d, File sample rate: %d", targetSampleRate,
          sfInfo.samplerate);

  if (targetSampleRate == sfInfo.samplerate) {
    // Optimization: Skip resampling
    soundData->isResampled = false;
    soundData->resampledData = std::move(pcmData);
    soundData->resampledFrameCount =
        soundData->resampledData.size() / soundData->channels;
    SDL_Log("Loaded sound without resampling (Rate: %d)", targetSampleRate);
  } else {
    // Initialize the resampler
    SDL_Log("Resampling audio data from %d Hz to %d Hz", sfInfo.samplerate,
            targetSampleRate);
    soundData->isResampled = true;
    ma_resampler_config resamplerConfig = ma_resampler_config_init(
        ma_format_s16, sfInfo.channels, sfInfo.samplerate, targetSampleRate,
        ma_resample_algorithm_linear);
    if (ma_resampler_init(&resamplerConfig, nullptr, &soundData->resampler) !=
        MA_SUCCESS) {
      SDL_Log("Failed to initialize resampler.");
      return false;
    }
    if (isCancelled)
      return false;

    // Resample the audio data to target rate

    ma_uint64 resampledFrameCount =
        (ma_uint64)((double)pcmData.size() / sfInfo.channels *
                    targetSampleRate / sfInfo.samplerate);
    soundData->resampledData.resize(resampledFrameCount * sfInfo.channels);
    ma_uint64 size = (ma_uint64)pcmData.size();
    if (isCancelled)
      return false;
    ma_resampler_process_pcm_frames(&soundData->resampler, pcmData.data(),
                                    &size, soundData->resampledData.data(),
                                    &resampledFrameCount);
    if (isCancelled)
      return false;
    soundData->resampledFrameCount = resampledFrameCount;
  }

  {
    std::lock_guard<std::mutex> lock(soundDataListMutex);
    soundDataIndexMap[path] = soundDataList.size();
    soundDataList.push_back(soundData);
  }
  return true;
}

void AudioWrapper::preloadSounds(const std::vector<path_t> &paths,
                                 std::atomic<bool> &isCancelled) {
  for (const auto &path : paths) {
    loadSound(path, isCancelled);
  }
}

bool AudioWrapper::playSound(const path_t &path) {
  // TODO: support multiplexing with same sound
  std::lock_guard<std::mutex> lock(soundDataListMutex);

  if (backend && !backend->isStarted()) {
    backend->start();
  }

  if (soundDataIndexMap.find(path) == soundDataIndexMap.end()) {
    SDL_Log("Sound not found: %s", path_t_to_utf8(path).c_str());
    return false;
  }

  auto &soundData = soundDataList[soundDataIndexMap[path]];
  soundData->currentFrame = 0;
  soundData->playing = true;

  return true;
}

void AudioWrapper::startDevice() {
  if (backend) {
    backend->start();
  }
}

void AudioWrapper::stopSounds() {
  std::lock_guard<std::mutex> lock(soundDataListMutex);
  if (backend) {
    backend->stop();
  }
  for (auto &soundData : soundDataList) {
    soundData->playing = false;
  }
}

void AudioWrapper::unloadSound(const path_t &path) {
  std::lock_guard<std::mutex> lock(soundDataListMutex);
  if (soundDataIndexMap.find(path) != soundDataIndexMap.end()) {
    size_t index = soundDataIndexMap[path];
    auto &soundData = soundDataList[index];
    if (soundData->isResampled) {
      ma_resampler_uninit(&soundData->resampler, nullptr); // Cleanup resampler
    }

    soundDataList.erase(soundDataList.begin() + index);
    soundDataIndexMap.erase(path);

    // Update indices in the map
    for (auto &entry : soundDataIndexMap) {
      if (entry.second > index) {
        entry.second--;
      }
    }
  }
}

void AudioWrapper::unloadSounds() {
  stopSounds();
  {
    std::lock_guard<std::mutex> lock(soundDataListMutex);
    for (auto &soundData : soundDataList) {
      if (soundData->isResampled) {
        ma_resampler_uninit(&soundData->resampler,
                            nullptr); // Cleanup resampler
      }
    }
    soundDataList.clear();
    soundDataIndexMap.clear();
  }
}

void AudioWrapper::setBassBoost(float db) {
  std::lock_guard<std::mutex> lock(soundDataListMutex); // Protect filter coeffs

  // Get current sample rate from backend or default
  int rate = backend ? backend->getSampleRate() : 44100;
  if (rate == 0)
    rate = 44100;

  // Low Shelf at 100Hz
  bassFilter.setLowShelf((float)rate, 100.0f, db);
}

void AudioWrapper::setTrebleBoost(float db) {
  std::lock_guard<std::mutex> lock(soundDataListMutex); // Protect filter coeffs

  // Get current sample rate
  int rate = backend ? backend->getSampleRate() : 44100;
  if (rate == 0)
    rate = 44100;

  // High Shelf at 3000Hz (or user preference 8-10kHz? 3kHz is mid-high, let's
  // say 4kHz)
  trebleFilter.setHighShelf((float)rate, 4000.0f, db);
}
