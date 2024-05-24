#include "SceneManager.h"
#include "Scene.h"

void SceneManager::changeScene(Scene *newScene) {
  if (currentScene) {
    currentScene->cleanup();
  }
  currentScene.reset(newScene);
  currentScene->init(context);
}

EventHandleResult SceneManager::handleEvents(SDL_Event &event) {
  EventHandleResult result;
  if (currentScene) {
    result = currentScene->handleEvents(event);
  }
  return result;
}

void SceneManager::update(float dt) {
  if (currentScene) {
    currentScene->update(dt);
  }
}

void SceneManager::render() {
  if (currentScene) {
    currentScene->render();
  }
}

void SceneManager::cleanup() {
  if (currentScene) {
    currentScene->cleanup();
    currentScene.reset();
  }
}