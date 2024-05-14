#include "Scene.h"
class MainMenuScene : public Scene {
public:
  void init() override;
  EventHandleResult handleEvents(SDL_Event &event) override;
  void update(float dt) override;
  void render(SDL_Renderer *renderer) override;
  void cleanup() override;
};