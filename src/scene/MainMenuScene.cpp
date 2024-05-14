#include "MainMenuScene.h"
#include <iostream>
void MainMenuScene::init() {
  // Initialize the scene
}

EventHandleResult MainMenuScene::handleEvents(SDL_Event &event) {
  // Handle input
  EventHandleResult result;

  if (event.type == SDL_KEYDOWN) {
    result.quit = true;
  }
  if (event.type == SDL_MOUSEBUTTONDOWN) {
    result.quit = true;
  }
  return result;
}

void MainMenuScene::update(float dt) {
  // Update the scene logic
  std::cout << "Updating Main Menu Scene, dt: " << dt << std::endl;
}

void MainMenuScene::render(SDL_Renderer *renderer) {
  // Render the scene
}

void MainMenuScene::cleanup() {
  // Cleanup resources when exiting the scene
}