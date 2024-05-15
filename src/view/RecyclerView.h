#pragma once

#include "View.h"
#include <SDL2/SDL.h>
#include <deque>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

template <typename T> class RecyclerView : public View {
public:
  inline RecyclerView(SDL_Renderer *renderer, int x, int y, int width,
                      int height)
      : scrollOffset(0), itemHeight(100), topMargin(1), bottomMargin(1),
        View(renderer, x, y, width, height) {}
  inline ~RecyclerView() {
    for (auto entry : viewEntries) {
      recycleView(entry.first);
    }
    for (auto view : recycledViewEntries) {
      delete view;
    }
  }

  // scroll offset in pixels
  float scrollOffset;

  // fixed height of all items in the list
  int itemHeight;
  int topMargin;    // Number of items to keep ready above the visible area
  int bottomMargin; // Number of items to keep ready below the visible area

  inline void setItems(std::vector<T> items) {
    this->items = items;
    updateVisibleItems();
  }

  inline void push(T item) {
    items.push_back(item);
    updateVisibleItems();
  }

  inline void pop() {
    items.pop_back();
    updateVisibleItems();
  }

  inline void remove(int index) {
    items.erase(items.begin() + index);
    updateVisibleItems();
  }

  inline void clear() {
    items.clear();
    for (auto view : viewEntries) {
      recycleView(view);
    }
    viewEntries.clear();
  }

  inline T get(int index) { return items[index]; }

  inline int size() { return items.size(); }

  inline std::vector<T> getItems() { return items; }

  // on bound to the view (delegate)
  std::function<void(View *, T, int, bool isSelected)> onBind;
  std::function<View *(T)> onCreateView;
  std::function<bool(const T &, const T &)> itemComparator =
      [](const T &a, const T &b) { return a == b; };

  // on click
  std::function<void(T, int)> onSelected;
  std::function<void(T, int)> onUnselected;
  int selectedIndex = -1;

  inline void render() override {
    // clip the rendering area
    SDL_Rect clip = {this->getX(), this->getY(), this->getWidth(),
                     this->getHeight()};
    SDL_RenderSetClipRect(renderer, &clip);
    for (auto entry : viewEntries) {
      entry.first->render();
    }

    // scroll bar
    SDL_Rect scrollBar = {this->getX() + this->getWidth() - 10, this->getY(),
                          10, this->getHeight()};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &scrollBar);

    // scroll bar thumb
    int itemsSize = std::max(6, static_cast<int>(items.size())) * itemHeight;
    int thumbHeight = this->getHeight() * this->getHeight() / itemsSize;
    int thumbY = this->getY() + scrollOffset * this->getHeight() / itemsSize;
    SDL_Rect scrollBarThumb = {this->getX() + this->getWidth() - 10, thumbY, 10,
                               thumbHeight};
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderFillRect(renderer, &scrollBarThumb);

    SDL_RenderSetClipRect(renderer, nullptr);
  }

  inline void handleEvents(SDL_Event &event) override {
    switch (event.type) {
    case SDL_MOUSEWHEEL: {
      // check mouse position
      int x, y;
      SDL_GetMouseState(&x, &y);
      if (x < this->getX() || x > this->getX() + this->getWidth()) {
        return;
      }
      if (y < this->getY() || y > this->getY() + this->getHeight()) {
        return;
      }
      scrollOffset -= event.wheel.y * 15.0f;
      if (scrollOffset < 0) {
        scrollOffset = 0;
      }
      int itemsSize = std::max(1, static_cast<int>(items.size())) * itemHeight;
      if (scrollOffset > itemsSize - this->getHeight()) {
        scrollOffset = itemsSize - this->getHeight();
      }
      updateVisibleItems();
      break;
    }
    case SDL_MOUSEBUTTONDOWN: {
      int x, y;
      SDL_GetMouseState(&x, &y);
      if (x < this->getX() || x > this->getX() + this->getWidth()) {
        return;
      }
      if (y < this->getY() || y > this->getY() + this->getHeight()) {
        return;
      }
      int index = (y - this->getY() + scrollOffset) / itemHeight;
      if (index >= 0 && index < items.size()) {
        if (selectedIndex != -1 && onUnselected) {
          onUnselected(items[selectedIndex], selectedIndex);
        }
        selectedIndex = index;
        if (onSelected) {
          onSelected(items[selectedIndex], selectedIndex);
        }
      }
      break;
    }
    }
  }

  inline View *getViewByIndex(int index) {
    if (idxToView.find(index) != idxToView.end()) {
      return idxToView[index];
    }
    return nullptr;
  }

private:
  std::vector<T> items;
  std::deque<std::pair<View *, T>> viewEntries; // Pair of view and item

  std::deque<View *> recycledViewEntries; // Pool of recycled views
  std::map<int, View *> idxToView;
  inline void updateVisibleItems() {
    // Determine the range of visible items
    int startIndex = getStartIndex();
    int endIndex = getEndIndex();

    // Temporary container for newly visible items
    std::deque<std::pair<View *, T>> newVisibleItems;
    idxToView.clear();
    // Iterate over the range of visible items
    for (int i = startIndex; i <= endIndex; ++i) {
      T item = items[i];
      View *view = nullptr;

      // Check if the item already has a corresponding view
      auto it = std::find_if(viewEntries.begin(), viewEntries.end(),
                             [&item, this](const std::pair<View *, T> &entry) {
                               return itemComparator(entry.second, item);
                             });

      if (it != viewEntries.end()) {
        // If the view is already visible, use it
        view = it->first;
        idxToView[i] = view;
        viewEntries.erase(it); // Remove from current visible items
      } else {
        // Otherwise, get a recycled view or create a new one
        view = getViewForItem(item);
        idxToView[i] = view;
        if (onBind) {
          onBind(view, item, i,
                 selectedIndex == i); // Bind the item to the view
        }
      }
      // update the position of the view
      view->setPosition(this->getX(),
                        this->getY() + (i * itemHeight) - scrollOffset);
      view->setSize(this->getWidth(), itemHeight);
      //   SDL_Log("View position: %d, %d", view->x, view->y);

      newVisibleItems.push_back(std::make_pair(view, item));
    }

    // Recycle any views that are no longer visible
    for (auto &entry : viewEntries) {
      recycleView(entry.first);
    }

    // Update the list of visible items
    viewEntries = std::move(newVisibleItems);
  }

  inline int getStartIndex() {
    return std::max(0.0f, (scrollOffset / itemHeight) - topMargin);
  }

  inline int getEndIndex() {
    int viewportHeight =
        this->getHeight(); // Assuming RecyclerView has a getHeight method
    int lastPossibleIndex =
        (scrollOffset + viewportHeight) / itemHeight + bottomMargin;
    return std::min(static_cast<int>(items.size()) - 1, lastPossibleIndex);
  }

  inline View *getViewForItem(T item) {
    if (!recycledViewEntries.empty()) {
      View *view = recycledViewEntries.front();
      recycledViewEntries.pop_front();
      return view;
    } else {
      // Create a new view if no recycled view is available
      if (!onCreateView) {
        throw std::runtime_error("onCreateView is not set");
      }
      return onCreateView(item);
    }
  }

  inline void recycleView(View *view) { recycledViewEntries.push_back(view); }

protected:
  inline void onResize(int newWidth, int newHeight) override {
    View::onResize(newWidth, newHeight);
    SDL_Log("RecyclerView::onResize");
    updateVisibleItems();
  }
};
