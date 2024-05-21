#pragma once
#include "../view/View.h"
#include "../context.h"
#include <SDL2/SDL.h>
#include <vector>
struct EventHandleResult {
  bool quit = false;
};
class Scene {
public:
  Scene() {}
  std::vector<View *> views;
  virtual void init(ApplicationContext &context) = 0; // Initialize the scene
  EventHandleResult handleEvents(SDL_Event &event) {
    for (auto view : views) {
      view->handleEvents(event);
    }
    return handleEventsScene(event);
  }
  virtual void update(float dt) = 0; // Update the scene logic
  // Render the scene (non-virtual public method)
  void render() {
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
  // Protected virtual methods for customization by derived classes
  virtual void renderScene() = 0;

  virtual void cleanupScene() = 0;

  virtual EventHandleResult handleEventsScene(SDL_Event &event) {
    return EventHandleResult();
  }
};
