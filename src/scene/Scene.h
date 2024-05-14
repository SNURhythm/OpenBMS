#pragma once
#include "../view/View.h"
#include <SDL2/SDL.h>
#include <vector>
struct EventHandleResult {
  bool quit = false;
};
class Scene {
public:
  Scene(SDL_Renderer *renderer) : renderer(renderer) {}
  std::vector<View *> views;
  virtual void init() = 0; // Initialize the scene
  virtual EventHandleResult handleEvents(SDL_Event &event) = 0; // Handle input
  virtual void update(float dt) = 0; // Update the scene logic
  // Render the scene (non-virtual public method)
  inline void render(SDL_Renderer *renderer) {
    for (auto view : views) {
      view->render();
    }
    renderScene(); // Additional custom rendering
  }

  // Cleanup resources when exiting the scene (non-virtual public method)
  inline void cleanup() {
    for (auto view : views) {
      delete view;
    }
    cleanupScene(); // Additional custom cleanup
  }

  inline void addView(View *view) { views.push_back(view); }

  virtual ~Scene() {}

protected:
  SDL_Renderer *renderer;
  // Protected virtual methods for customization by derived classes
  virtual void renderScene() = 0;

  virtual void cleanupScene() = 0;
};