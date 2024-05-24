
#define MINIAUDIO_IMPLEMENTATION
// NOTE: Should be compiled as Objective-C++ in iOS
// TODO: Find a way to automatically compile as Objective-C++ in iOS
#include "stb_vorbis.c"
#include "AudioWrapper.h"
#include <stdexcept>
#include <SDL2/SDL.h>

#include <stdio.h>

// static ma_result ma_decoding_backend_init__libvorbis(
//     void *pUserData, ma_read_proc onRead, ma_seek_proc onSeek,
//     ma_tell_proc onTell, void *pReadSeekTellUserData,
//     const ma_decoding_backend_config *pConfig,
//     const ma_allocation_callbacks *pAllocationCallbacks,
//     ma_data_source **ppBackend) {
//   ma_result result;
//   ma_libvorbis *pVorbis;

//   (void)pUserData;

//   pVorbis = (ma_libvorbis *)ma_malloc(sizeof(*pVorbis),
//   pAllocationCallbacks); if (pVorbis == NULL) {
//     return MA_OUT_OF_MEMORY;
//   }

//   result = ma_libvorbis_init(onRead, onSeek, onTell, pReadSeekTellUserData,
//                              pConfig, pAllocationCallbacks, pVorbis);
//   if (result != MA_SUCCESS) {
//     ma_free(pVorbis, pAllocationCallbacks);
//     return result;
//   }

//   *ppBackend = pVorbis;

//   return MA_SUCCESS;
// }

// static ma_result ma_decoding_backend_init_file__libvorbis(
//     void *pUserData, const char *pFilePath,
//     const ma_decoding_backend_config *pConfig,
//     const ma_allocation_callbacks *pAllocationCallbacks,
//     ma_data_source **ppBackend) {
//   ma_result result;
//   ma_libvorbis *pVorbis;

//   (void)pUserData;

//   pVorbis = (ma_libvorbis *)ma_malloc(sizeof(*pVorbis),
//   pAllocationCallbacks); if (pVorbis == NULL) {
//     return MA_OUT_OF_MEMORY;
//   }

//   result =
//       ma_libvorbis_init_file(pFilePath, pConfig, pAllocationCallbacks,
//       pVorbis);
//   if (result != MA_SUCCESS) {
//     ma_free(pVorbis, pAllocationCallbacks);
//     return result;
//   }

//   *ppBackend = pVorbis;

//   return MA_SUCCESS;
// }

// static void ma_decoding_backend_uninit__libvorbis(
//     void *pUserData, ma_data_source *pBackend,
//     const ma_allocation_callbacks *pAllocationCallbacks) {
//   ma_libvorbis *pVorbis = (ma_libvorbis *)pBackend;

//   (void)pUserData;

//   ma_libvorbis_uninit(pVorbis, pAllocationCallbacks);
//   ma_free(pVorbis, pAllocationCallbacks);
// }

// static ma_result ma_decoding_backend_get_channel_map__libvorbis(
//     void *pUserData, ma_data_source *pBackend, ma_channel *pChannelMap,
//     size_t channelMapCap) {
//   ma_libvorbis *pVorbis = (ma_libvorbis *)pBackend;

//   (void)pUserData;

//   return ma_libvorbis_get_data_format(pVorbis, NULL, NULL, NULL, pChannelMap,
//                                       channelMapCap);
// }

// static ma_decoding_backend_vtable g_ma_decoding_backend_vtable_libvorbis = {
//     ma_decoding_backend_init__libvorbis,
//     ma_decoding_backend_init_file__libvorbis, NULL, /* onInitFileW() */
//     NULL,                                           /* onInitMemory() */
//     ma_decoding_backend_uninit__libvorbis};

// static ma_result ma_decoding_backend_init__libopus(
//     void *pUserData, ma_read_proc onRead, ma_seek_proc onSeek,
//     ma_tell_proc onTell, void *pReadSeekTellUserData,
//     const ma_decoding_backend_config *pConfig,
//     const ma_allocation_callbacks *pAllocationCallbacks,
//     ma_data_source **ppBackend) {
//   ma_result result;
//   ma_libopus *pOpus;

//   (void)pUserData;

//   pOpus = (ma_libopus *)ma_malloc(sizeof(*pOpus), pAllocationCallbacks);
//   if (pOpus == NULL) {
//     return MA_OUT_OF_MEMORY;
//   }

//   result = ma_libopus_init(onRead, onSeek, onTell, pReadSeekTellUserData,
//                            pConfig, pAllocationCallbacks, pOpus);
//   if (result != MA_SUCCESS) {
//     ma_free(pOpus, pAllocationCallbacks);
//     return result;
//   }

//   *ppBackend = pOpus;

//   return MA_SUCCESS;
// }

// static ma_result ma_decoding_backend_init_file__libopus(
//     void *pUserData, const char *pFilePath,
//     const ma_decoding_backend_config *pConfig,
//     const ma_allocation_callbacks *pAllocationCallbacks,
//     ma_data_source **ppBackend) {
//   ma_result result;
//   ma_libopus *pOpus;

//   (void)pUserData;

//   pOpus = (ma_libopus *)ma_malloc(sizeof(*pOpus), pAllocationCallbacks);
//   if (pOpus == NULL) {
//     return MA_OUT_OF_MEMORY;
//   }

//   result =
//       ma_libopus_init_file(pFilePath, pConfig, pAllocationCallbacks, pOpus);
//   if (result != MA_SUCCESS) {
//     ma_free(pOpus, pAllocationCallbacks);
//     return result;
//   }

//   *ppBackend = pOpus;

//   return MA_SUCCESS;
// }

// static void ma_decoding_backend_uninit__libopus(
//     void *pUserData, ma_data_source *pBackend,
//     const ma_allocation_callbacks *pAllocationCallbacks) {
//   ma_libopus *pOpus = (ma_libopus *)pBackend;

//   (void)pUserData;

//   ma_libopus_uninit(pOpus, pAllocationCallbacks);
//   ma_free(pOpus, pAllocationCallbacks);
// }

// static ma_result ma_decoding_backend_get_channel_map__libopus(
//     void *pUserData, ma_data_source *pBackend, ma_channel *pChannelMap,
//     size_t channelMapCap) {
//   ma_libopus *pOpus = (ma_libopus *)pBackend;

//   (void)pUserData;

//   return ma_libopus_get_data_format(pOpus, NULL, NULL, NULL, pChannelMap,
//                                     channelMapCap);
// }

// static ma_decoding_backend_vtable g_ma_decoding_backend_vtable_libopus = {
//     ma_decoding_backend_init__libopus,
//     ma_decoding_backend_init_file__libopus, NULL, /* onInitFileW() */ NULL,
//     /* onInitMemory() */ ma_decoding_backend_uninit__libopus};
AudioWrapper::AudioWrapper() {
  // pCustomBackendVTables[0] = &g_ma_decoding_backend_vtable_libvorbis;
  // pCustomBackendVTables[1] = &g_ma_decoding_backend_vtable_libopus;
  // resourceManagerConfig.ppCustomDecodingBackendVTables =
  // pCustomBackendVTables; resourceManagerConfig.customDecodingBackendCount =
  //     sizeof(pCustomBackendVTables) / sizeof(pCustomBackendVTables[0]);
  // resourceManagerConfig.pCustomDecodingBackendUserData = nullptr;
  engineConfig = ma_engine_config_init();
  auto result = ma_engine_init(&engineConfig, &engine);
  if (result != MA_SUCCESS) {
    throw std::runtime_error("Failed to initialize audio engine.");
  }
  result = ma_sound_group_init(&engine, 0, nullptr, &soundGroup);
}

AudioWrapper::~AudioWrapper() {
  unloadSounds();

  ma_engine_uninit(&engine);
}

bool AudioWrapper::loadSound(const char *path) {
  auto result = ma_sound_init_from_file(&engine, path, MA_SOUND_FLAG_DECODE,
                                        nullptr, nullptr, &sounds[path]);

  return result == MA_SUCCESS;
}

void AudioWrapper::preloadSounds(const std::vector<std::string> &paths) {
  for (const auto &path : paths) {
    loadSound(path.c_str());
  }
}

bool AudioWrapper::playSound(const char *path) {
  bool result = true;
  if (sounds.find(path) == sounds.end()) {
    result = loadSound(path);
  }
  ma_engine_play_sound(&engine, path, &soundGroup);
  return result;
}

void AudioWrapper::stopSounds() {
  ma_sound_group_stop(&soundGroup);
  ma_sound_group_start(&soundGroup);
}

void AudioWrapper::unloadSound(const char *path) {
  ma_sound sound = sounds[path];
  ma_sound_uninit(&sound);
  sounds.erase(path);
}

void AudioWrapper::unloadSounds() {
  for (auto &sound : sounds) {
    ma_sound_uninit(&sound.second);
  }
  sounds.clear();
}