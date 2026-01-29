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

  memset(pOutput, 0, frameCount * sizeof(ma_int16) * outputChannels);

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

    for (ma_uint32 frame = 0; frame < framesToRead; ++frame) {
      for (int channel = 0; channel < soundData->channels; ++channel) {
        int outputChannel = channel % outputChannels;
        ma_int32 sample =
            ((ma_int16 *)pOutput)[frame * outputChannels + outputChannel] +
            static_cast<ma_int32>(
                gain *
                soundData->resampledData[(soundData->currentFrame + frame) *
                                             soundData->channels +
                                         channel]);

        // Clamp
        if (sample > 32767)
          sample = 32767;
        if (sample < -32768)
          sample = -32768;

        ((ma_int16 *)pOutput)[frame * outputChannels + outputChannel] =
            (ma_int16)sample;
      }
    }

    soundData->currentFrame += framesToRead;
    if (framesToRead < frameCount) {
      soundData->playing = false;
    }
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
}

AudioWrapper::~AudioWrapper() {
  unloadSounds();
  // Backend destroyed via unique_ptr
}

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
