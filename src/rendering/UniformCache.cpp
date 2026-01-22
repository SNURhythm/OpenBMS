#include "UniformCache.h"

namespace rendering {
UniformCache &UniformCache::getInstance() {
  static UniformCache instance;
  return instance;
}

bgfx::UniformHandle UniformCache::getSampler(const std::string &name) {
  auto it = samplers_.find(name);
  if (it != samplers_.end()) {
    return it->second;
  }
  auto handle = bgfx::createUniform(name.c_str(), bgfx::UniformType::Sampler);
  samplers_.emplace(name, handle);
  return handle;
}

void UniformCache::destroyAll() {
  for (auto &pair : samplers_) {
    if (bgfx::isValid(pair.second)) {
      bgfx::destroy(pair.second);
    }
  }
  samplers_.clear();
}
} // namespace rendering
