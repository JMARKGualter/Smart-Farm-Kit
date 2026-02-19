#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

int centerX = 95;
int centerY = 60;
int size = 95;

int screenW = 128;
int screenH = 128;

void drawThickLine(int x1, int y1, int x2, int y2) {
  u8g2.drawLine(x1, y1, x2, y2);
  u8g2.drawLine(x1+1, y1, x2+1, y2);
}

void animateSegment(int x1, int y1, int x2, int y2,
                    bool p1, bool p2,
                    bool p3, bool p4,
                    bool t1) {

  int steps = 15;

  for (int i = 0; i <= steps; i++) {

    float t = (float)i / steps;
    int x = x1 + t * (x2 - x1);
    int y = y1 + t * (y2 - y1);

    u8g2.clearBuffer();

    int half = size / 2;

    int leftX   = centerX - half;
    int rightX  = centerX + half;
    int topY    = centerY - half;
    int bottomY = centerY + half;

    if (p1) drawThickLine(centerX, bottomY, leftX, centerY);
    if (p2) drawThickLine(leftX, centerY, centerX, topY);
    if (p3) drawThickLine(centerX, topY, rightX, centerY);
    if (p4) drawThickLine(rightX, centerY, centerX, bottomY);
    if (t1) drawThickLine(leftX, centerY, rightX, centerY);

    drawThickLine(x1, y1, x, y);

    u8g2.sendBuffer();
    delay(1);
  }
}

void setup() {
  u8g2.begin();
}

void loop() {

  int half = size / 2;

  if (centerX + half > screenW-1) centerX = screenW-1 - half;
  if (centerX - half < 0) centerX = half;
  if (centerY + half > screenH-1) centerY = screenH-1 - half;
  if (centerY - half < 0) centerY = half;

  int leftX   = centerX - half;
  int rightX  = centerX + half;
  int topY    = centerY - half;
  int bottomY = centerY + half;

  animateSegment(centerX, bottomY, leftX, centerY,
                 false,false,false,false,false);

  animateSegment(leftX, centerY, centerX, topY,
                 true,false,false,false,false);

  animateSegment(centerX, topY, rightX, centerY,
                 true,true,false,false,false);

  animateSegment(rightX, centerY, centerX, bottomY,
                 true,true,true,false,false);

  animateSegment(leftX, centerY, rightX, centerY,
                 true,true,true,true,false);

  animateSegment(centerX, centerY, centerX, bottomY,
                 true,true,true,true,true);

  delay(600);
}
