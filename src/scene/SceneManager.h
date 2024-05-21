#pragma once
#include "Scene.h"
#include <SDL2/SDL.h>
#include <memory>

class SceneManager {
private:
  std::unique_ptr<Scene> currentScene;

public:
  void changeScene(Scene *newScene);
  EventHandleResult handleEvents(SDL_Event &event);

  void update(float dt);

  void render();
};