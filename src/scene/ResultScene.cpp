#include "ResultScene.h"
#include "../view/Button.h"
#include "../view/TextView.h"

#include "../rendering/Color.h"
#include "../rendering/ShaderManager.h"
#include "../rendering/common.h"
#include "bgfx/bgfx.h"
#include <sstream>

static void drawRect(float x, float y, float width, float height, Color color) {
  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer tib{};

  bgfx::VertexLayout layout = rendering::PosColorVertex::ms_decl;

  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);

  auto *vertices = (rendering::PosColorVertex *)tvb.data;
  auto *index = (uint16_t *)tib.data;

  uint32_t abgr = color.toABGR();
  vertices[0] = {x, y, 0.0f, abgr};
  vertices[1] = {x + width, y, 0.0f, abgr};
  vertices[2] = {x + width, y + height, 0.0f, abgr};
  vertices[3] = {x, y + height, 0.0f, abgr};

  index[0] = 0;
  index[1] = 1;
  index[2] = 2;
  index[3] = 2;
  index[4] = 3;
  index[5] = 0;

  uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA;
  bgfx::setState(state);

  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setIndexBuffer(&tib);

  bgfx::submit(
      rendering::main_view,
      rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE));
}

ResultScene::ResultScene(ApplicationContext &context,
                         const bms_parser::ChartMeta &meta,
                         const RhythmState &state)
    : Scene(context), meta(meta), resultState(state) {}

ResultScene::~ResultScene() { delete rootLayout; }

void ResultScene::init() {
  rootLayout =
      new View(0, 0, rendering::window_width, rendering::window_height);
  rootLayout->setFlexDirection(FlexDirection::Column);
  rootLayout->setAlignItems(YGAlignCenter);
  rootLayout->setJustifyContent(YGJustifyCenter);
  rootLayout->setPadding(Edge::All, 20);
  rootLayout->setGap(10);

  // Title & Artist
  auto titleText = new TextView("assets/fonts/notosanscjkjp.ttf", 48);
  titleText->setText(meta.Title);
  titleText->setAlign(TextView::CENTER);
  rootLayout->addView(titleText);

  auto artistText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  artistText->setText(meta.Artist);
  artistText->setAlign(TextView::CENTER);
  rootLayout->addView(artistText);

  // Score Container
  auto scoreContainer = new View();
  scoreContainer->setFlexDirection(FlexDirection::Row);
  scoreContainer->setGap(50);
  scoreContainer->setAlignItems(YGAlignCenter);

  // Grade
  // Calculate Grade
  // Total MAX score = total notes * 2 (PGreat is 2, Great is 1)
  int totalNotes = meta.TotalNotes;
  int maxScore = totalNotes * 2;
  int currentScore = resultState.getScore();
  double percentage = maxScore > 0 ? (double)currentScore / maxScore : 0.0;
  std::string grade = "F";
  if (percentage >= 8.0/9.0)
    grade = "AAA";
  else if (percentage >= 7.0/9.0)
    grade = "AA";
  else if (percentage >= 6.0/9.0)
    grade = "A";
  else if (percentage >= 5.0/9.0)
    grade = "B";
  else if (percentage >= 4.0/9.0)
    grade = "C";
  else if (percentage >= 3.0/9.0)
    grade = "D";
  else if (percentage >= 2.0/9.0)
    grade = "E";
  else
    grade = "F";

  auto gradeText = new TextView("assets/fonts/notosanscjkjp.ttf", 96);
  gradeText->setText(grade);
  gradeText->setColor({255, 200, 50, 255}); // Gold-ish
  scoreContainer->addView(gradeText);

  auto scoreDetailView = new View();
  scoreDetailView->setFlexDirection(FlexDirection::Column);
  auto scoreText = new TextView("assets/fonts/notosanscjkjp.ttf", 48);
  scoreText->setText(std::to_string(currentScore));
  scoreDetailView->addView(scoreText);

  auto comboText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  comboText->setText("Max Combo: " + std::to_string(resultState.maxCombo));
  scoreDetailView->addView(comboText);
  scoreContainer->addView(scoreDetailView);

  rootLayout->addView(scoreContainer);

  // Judgements
  auto detailsGrid = new View();
  detailsGrid->setFlexDirection(FlexDirection::Row);
  detailsGrid->setGap(40);
  
  auto leftCol = new View();
  leftCol->setFlexDirection(FlexDirection::Column);
  
  auto addJudgeText = [&](std::string label, int count, SDL_Color color) {
      std::stringstream ss;
      ss << label << ": " << count;
      auto tv = new TextView("assets/fonts/notosanscjkjp.ttf", 24);
      tv->setText(ss.str());
      tv->setColor(color);
      leftCol->addView(tv);
  };
  
  addJudgeText("PGREAT", resultState.judgeCount.at(PGreat), {200, 255, 255, 255});
  addJudgeText("GREAT", resultState.judgeCount.at(Great), {200, 255, 200, 255});
  addJudgeText("GOOD", resultState.judgeCount.at(Good), {200, 200, 255, 255});
  addJudgeText("BAD", resultState.judgeCount.at(Bad), {255, 200, 200, 255});
  addJudgeText("POOR", resultState.judgeCount.at(Poor), {255, 100, 100, 255});
  // KPOOR is usually Empty Poor (miss)
  addJudgeText("MISS", resultState.judgeCount.at(Kpoor), {255, 50, 50, 255});
  addJudgeText("BREAK", resultState.comboBreak, {255, 50, 50, 255});
  
  detailsGrid->addView(leftCol);
  
  auto rightCol = new View();
  rightCol->setFlexDirection(FlexDirection::Column);
  
  auto fastSlowText = new TextView("assets/fonts/notosanscjkjp.ttf", 24);
  std::stringstream fss;
  fss << "FAST: " << resultState.fastCount << "\nSLOW: " << resultState.slowCount;
  fastSlowText->setText(fss.str());
  rightCol->addView(fastSlowText);
  
  detailsGrid->addView(rightCol);
  rootLayout->addView(detailsGrid);

  // Graph Placeholder
  graphPlaceHolder = new View();
  graphPlaceHolder->setHeight(200);
  graphPlaceHolder->setWidth(rendering::window_width - 100);
  rootLayout->addView(graphPlaceHolder);

  // Buttons
  auto btn = new Button(0, 0, 300, 80);
  auto btnText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  btnText->setText("Back to Menu");
  btnText->setAlign(TextView::CENTER);
  btn->setContentView(btnText);
  btn->setOnClickListener([this]() {
    context.sceneManager->changeScene("MainMenu");
  });
  rootLayout->addView(btn);

  addView(rootLayout);
  rootLayout->applyYogaLayout();
}

void ResultScene::update(float dt) {}

void ResultScene::renderScene() {
  // Draw Gauge Graph
  if (graphPlaceHolder && !resultState.gaugeHistory.empty()) {
    float x = graphPlaceHolder->getX();
    float y = graphPlaceHolder->getY();
    float w = graphPlaceHolder->getWidth();
    float h = graphPlaceHolder->getHeight();

    // Draw background
    drawRect(x, y, w, h, Color(50, 50, 50, 200));

    // Draw graph
    size_t count = resultState.gaugeHistory.size();
    if (count > 1) {
      bgfx::TransientVertexBuffer tvb{};
      bgfx::TransientIndexBuffer tib{};

      if (bgfx::getAvailTransientVertexBuffer(count * 4,
                                              rendering::PosColorVertex::ms_decl) ==
              count * 4 &&
          bgfx::getAvailTransientIndexBuffer(count * 6) == count * 6) {

        bgfx::allocTransientVertexBuffer(
            &tvb, count * 4, rendering::PosColorVertex::ms_decl);
        bgfx::allocTransientIndexBuffer(&tib, count * 6);

        auto *vertices = (rendering::PosColorVertex *)tvb.data;
        auto *index = (uint16_t *)tib.data;

        float step = w / static_cast<float>(count);

        for (size_t i = 0; i < count; ++i) {
          float val = resultState.gaugeHistory[i]; // 0 to 100
          float barH = (val / 100.0f) * h;
          // Color based on value
          Color barColor = val > 80.0f ? Color(0, 255, 255, 200)
                                       : (val > 30.0f ? Color(0, 255, 0, 200)
                                                      : Color(255, 0, 0, 200));
          uint32_t abgr = barColor.toABGR();
          float bx = x + i * step;
          float by = y + h - barH;

          vertices[i * 4 + 0] = {bx, by, 0.0f, abgr};
          vertices[i * 4 + 1] = {bx + step, by, 0.0f, abgr};
          vertices[i * 4 + 2] = {bx + step, by + barH, 0.0f, abgr};
          vertices[i * 4 + 3] = {bx, by + barH, 0.0f, abgr};

          index[i * 6 + 0] = i * 4 + 0;
          index[i * 6 + 1] = i * 4 + 1;
          index[i * 6 + 2] = i * 4 + 2;
          index[i * 6 + 3] = i * 4 + 2;
          index[i * 6 + 4] = i * 4 + 3;
          index[i * 6 + 5] = i * 4 + 0;
        }

        uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                         BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA;
        bgfx::setState(state);
        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setIndexBuffer(&tib);
        bgfx::submit(
            rendering::ui_view,
            rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE));
      } else {
        // Fallback or just don't draw if too many
        SDL_Log("Too many points in gauge history to draw: %zu", count);
      }
    }
  }
}

void ResultScene::cleanupScene() {
    // Resources are cleaned up by Scene logic usually, but we have Views.
    // Scene::cleanup() deletes all views.
}
