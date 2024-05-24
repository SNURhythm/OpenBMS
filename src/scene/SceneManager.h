#pragma once
#include "Scene.h"
#include <SDL2/SDL.h>
#include <memory>
#include "../context.h"
class SceneManager {
private:
  std::unique_ptr<Scene> currentScene;
  ApplicationContext &context;

public:
  SceneManager() = delete;
  ~SceneManager() { cleanup(); }
  explicit SceneManager(ApplicationContext &context) : context(context) {}
  void changeScene(Scene *newScene);
  EventHandleResult handleEvents(SDL_Event &event);
  void cleanup();
  void update(float dt);

  void render();
};