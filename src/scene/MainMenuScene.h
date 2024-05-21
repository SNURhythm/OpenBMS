#pragma once
#include "../view/RecyclerView.h"
#include "Scene.h"
class MainMenuScene : public Scene {
public:
  inline explicit MainMenuScene() : Scene() {}
  void init() override;
  EventHandleResult handleEvents(SDL_Event &event) override;
  void update(float dt) override;
  void renderScene() override;
  void cleanupScene() override;

private:
  RecyclerView<std::string> *recyclerView = nullptr;
};
