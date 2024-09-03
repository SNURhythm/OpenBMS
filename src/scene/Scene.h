#pragma once
#include "../view/View.h"
#include "../context.h"
#include <SDL2/SDL.h>
#include <vector>
#include <set>
struct EventHandleResult {
  bool quit = false;
};
class Scene {

public:
  Scene() = delete;
  Scene(ApplicationContext &context) : context(context) {}
  std::vector<View *> views;
  std::map<Uint64, std::pair<Uint64, std::vector<std::function<bool()>>>>
      deferred;
  virtual void init() = 0; // Initialize the scene
  EventHandleResult handleEvents(SDL_Event &event) {
    for (auto view : views) {
      view->handleEvents(event);
    }
    return {};
  }
  virtual void update(float dt) = 0; // Update the scene logic
  void defer(const std::function<bool()> &func, Uint64 delay,
             bool shouldWaitFrame = false) {
    Uint64 time = SDL_GetTicks64() + delay;
    if (deferred.find(time) == deferred.end()) {
      deferred[time] = {};
    }
    deferred[time].first =
        shouldWaitFrame ? context.currentFrame + 1 : context.currentFrame;
    deferred[time].second.push_back(func);
  }
  void handleDeferred() {

    Uint64 time = SDL_GetTicks64();
    auto it = deferred.begin();
    while (it != deferred.end()) {
      if (it->first <= time) {
        if (it->second.first <= context.currentFrame) {
          SDL_Log("Handling deferred");
          for (const auto &func : it->second.second) {
            if(!func()) return;
            if (isDead) {
              return;
            }
          }
          SDL_Log("Done");
          it = deferred.erase(it);
        } else {
          ++it;
        }
      } else {
        ++it;
      }
    }
  }
  // Render the scene (non-virtual public method)
  void render() {
    RenderContext context;
    for (auto view : views) {
      view->render(context);
    }
    renderScene(); // Additional custom rendering
  }

  // Cleanup resources when exiting the scene (non-virtual public method)
  inline void cleanup() {
    isDead = true;
    SDL_Log("Cleaning up");
    cleanupScene(); // Additional custom cleanup
    for (auto view : views) {
      delete view;
    }
  }

  inline void addView(View *view) { views.push_back(view); }

  virtual ~Scene() {}

protected:
  // Protected virtual methods for customization by derived classes
  virtual void renderScene() = 0;

  virtual void cleanupScene() = 0;

  ApplicationContext &context;

private:
  bool isDead = false;
};
