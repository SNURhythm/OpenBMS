#pragma once
#include "context.h"
int main(int argv, char **args);
void run();
void resetViewTransform();
void createFrameBuffers(uint16_t windowW, uint16_t windowH);
void destroyFrameBuffers();
void blurHorizontal();
void blurVertical();
void drawFinal(uint16_t windowW, uint16_t windowH);
