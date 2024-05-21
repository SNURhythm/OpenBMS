#pragma once
#include <bgfx/bgfx.h>
#include <stdexcept>
#include <string>
#include <SDL2/SDL.h>
#include <map>
namespace rendering {
// singleton
class ShaderManager {
private:
  std::map<std::string, bgfx::ProgramHandle> programMap;

  ShaderManager() {} // Private constructor
  bgfx::ShaderHandle loadShader(const char *FILENAME) {
    const char *shaderPath = "???";
    switch (bgfx::getRendererType()) {
    case bgfx::RendererType::Noop:
    case bgfx::RendererType::Direct3D11:
    case bgfx::RendererType::Direct3D12:
      shaderPath = "./shaders/dx11/";
      break;
    case bgfx::RendererType::Gnm:
      shaderPath = "./shaders/pssl/";
      break;
    case bgfx::RendererType::Metal:
      shaderPath = "./shaders/metal/";
      break;
    case bgfx::RendererType::OpenGL:
      shaderPath = "./shaders/glsl/";
      break;
    case bgfx::RendererType::OpenGLES:
      shaderPath = "./shaders/essl/";
      break;
    case bgfx::RendererType::Vulkan:
      shaderPath = "./shaders/spirv/";
      break;
    default:
      throw std::runtime_error("Unknown renderer type");
    }

    size_t shaderLen = strlen(shaderPath);
    size_t fileLen = strlen(FILENAME);
    char *filePath = (char *)malloc(shaderLen + fileLen);
    memcpy(filePath, shaderPath, shaderLen);
    memcpy(&filePath[shaderLen], FILENAME, fileLen);
    SDL_Log("Loading %s", filePath);

    FILE *file = fopen(filePath, "rb");
    if (!file) {
      perror("file open error");
      throw std::runtime_error("no such file");
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    const bgfx::Memory *mem = bgfx::alloc(fileSize + 1);
    fread(mem->data, 1, fileSize, file);
    mem->data[mem->size - 1] = '\0';
    fclose(file);

    return bgfx::createShader(mem);
  }

public:
  static ShaderManager &getInstance() {
    static ShaderManager instance;
    return instance;
  }

  ShaderManager(const ShaderManager &) = delete;
  void operator=(const ShaderManager &) = delete;

  bgfx::ProgramHandle getProgram(const std::string &name) {
    if (programMap.find(name) == programMap.end()) {
      bgfx::ShaderHandle vsh = loadShader(("vs_" + name + ".bin").c_str());
      bgfx::ShaderHandle fsh = loadShader(("fs_" + name + ".bin").c_str());
      programMap[name] = bgfx::createProgram(vsh, fsh, true);
    }
    return programMap[name];
  }

  void preloadProgram(const std::string &name) { getProgram(name); }

  void release() {
    for (auto &program : programMap) {
      bgfx::destroy(program.second);
    }
  }
};
} // namespace rendering