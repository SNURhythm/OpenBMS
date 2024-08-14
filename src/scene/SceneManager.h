#pragma once

#include <SDL2/SDL.h>
#include <memory>
class Scene;
class EventHandleResult;
class ApplicationContext;
class SceneManager {
private:
  std::unique_ptr<Scene> currentScene;
  ApplicationContext &context;

public:
  SceneManager() = delete;
  ~SceneManager();
  explicit SceneManager(ApplicationContext &context);
  void changeScene(Scene *newScene);
  EventHandleResult handleEvents(SDL_Event &event);
  void cleanup();
  void update(float dt);

  void render();
};