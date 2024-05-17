#include "MainMenuScene.h"

#include "../view/ChartListItemView.h"
#include "../view/TextView.h"
#include <iostream>
void MainMenuScene::init() {
  // Initialize the scene
  // get screen width
  int screenWidth, screenHeight;
  SDL_GetRendererOutputSize(renderer, &screenWidth, &screenHeight);
  recyclerView =
      new RecyclerView<std::string>(renderer, 0, 0, screenWidth, screenHeight);
  recyclerView->onCreateView = [this, screenWidth](const std::string &item) {
    return new ChartListItemView(renderer, 0, 0, screenWidth, 100, item,
                                 "Artist", "Level");
  };
  recyclerView->itemHeight = 100;
  recyclerView->onBind = [this](View *view, const std::string &item, int idx,
                                bool isSelected) {
    auto *chartListItemView = dynamic_cast<ChartListItemView *>(view);
    chartListItemView->setTitle(item);
    if (isSelected) {
      chartListItemView->onSelected();
    } else {
      chartListItemView->onUnselected();
    }
  };
  recyclerView->onSelected = [this](const std::string &item, int idx) {
    std::cout << "Selected item: " << item << " at index: " << idx << std::endl;
    auto selectedView = recyclerView->getViewByIndex(idx);
    if (selectedView) {
      selectedView->onSelected();
    }
  };
  recyclerView->onUnselected = [this](const std::string &item, int idx) {
    std::cout << "Unselected item: " << item << " at index: " << idx
              << std::endl;
    auto unselectedView = recyclerView->getViewByIndex(idx);
    if (unselectedView) {
      unselectedView->onUnselected();
    }
  };
  // create 100 items
  std::vector<std::string> items;
  for (int i = 0; i < 100; i++) {
    items.push_back("Item " + std::to_string(i));
  }
  recyclerView->setItems(items);

  addView(recyclerView);
}

EventHandleResult MainMenuScene::handleEvents(SDL_Event &event) {
  // Handle input
  EventHandleResult result;
  recyclerView->handleEvents(event);

  return result;
}

void MainMenuScene::update(float dt) {
  // Update the scene logic
  // std::cout << "Updating Main Menu Scene, dt: " << dt << std::endl;
}

void MainMenuScene::renderScene() {
  // Render the scene
  int screenWidth, screenHeight;
  SDL_GetRendererOutputSize(renderer, &screenWidth, &screenHeight);
  recyclerView->setSize(screenWidth, screenHeight);
}

void MainMenuScene::cleanupScene() {
  // Cleanup resources when exiting the scene
}