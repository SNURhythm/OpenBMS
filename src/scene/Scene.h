#pragma once
#include <SDL2/SDL.h>
struct EventHandleResult {
  bool quit = false;
};
class Scene {
public:
  virtual void init() = 0; // Called when entering the scene
  virtual EventHandleResult handleEvents(SDL_Event &event) = 0; // Handle input
  virtual void update(float dt) = 0;               // Update the scene logic
  virtual void render(SDL_Renderer *renderer) = 0; // Render the scene
  virtual void cleanup() = 0; // Cleanup resources when exiting the scene

  virtual ~Scene() {}
};
