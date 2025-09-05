#include "SceneManager.h"
#include "Scene.h"
SceneManager::SceneManager(ApplicationContext &context) : context(context) {
  context.sceneManager = this;
}

void SceneManager::registerScene(const std::string& name, std::unique_ptr<Scene> scene) {
  registeredScenes[name] = std::move(scene);
}

void SceneManager::changeScene(Scene *newScene, bool keepBackground) {
  // Check if the new scene is already in backgroundScenes (O(1) lookup)
  auto it = backgroundScenes.find(newScene);
  
  // Handle current scene (common logic)
  if (currentScene && !keepBackground) {
    currentScene->cleanup();
  }
  if (keepBackground && currentScene) {
    backgroundScenes.insert(currentScene);
  }
  
  if (it != backgroundScenes.end()) {
    // Scene is in background, bring it to foreground
    currentScene = newScene;
    backgroundScenes.erase(it);
    // Don't call init() again since the scene is already initialized
  } else {
    // Normal scene change for new or registered scenes
    currentScene = newScene;
    currentScene->init();
  }
}

void SceneManager::changeScene(const std::string& sceneName, bool keepBackground) {
  auto it = registeredScenes.find(sceneName);
  if (it != registeredScenes.end()) {
    // Use the existing changeScene method with the scene pointer
    Scene* scenePtr = it->second.get();
    changeScene(scenePtr, keepBackground);
  }
  // Note: Could add error handling here if scene name not found
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

void SceneManager::handleDeferred() {
  if (currentScene) {
    currentScene->handleDeferred();
  }
}

void SceneManager::render() {
  if (currentScene) {
    currentScene->render();
  }
}

void SceneManager::cleanup() {
  currentScene = nullptr;
  backgroundScenes.clear();
  
  for (auto &[name, scene] : registeredScenes) {
    scene->cleanup();
  }
  registeredScenes.clear();
}
SceneManager::~SceneManager() { cleanup(); }
