#include "RenderPlan.h"

namespace rendering {
void applyViewOrder(bgfx::ViewId blurViewH, bgfx::ViewId blurViewV,
                    bgfx::ViewId finalView) {
  const bgfx::ViewId order[] = {
      rendering::clear_view, rendering::bga_view,  rendering::bga_layer_view,
      blurViewH,             blurViewV,           finalView,
      rendering::main_view,  rendering::ui_view,
  };
  bgfx::setViewOrder(
      0, static_cast<uint16_t>(sizeof(order) / sizeof(order[0])), order);
}
} // namespace rendering
