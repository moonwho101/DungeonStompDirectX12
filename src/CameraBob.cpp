#include "CameraBob.hpp"
#include <cmath>


void CameraBob::SinWave(float speed, float amplitude, float frequency) {
    x = 0;
    y = 0;
    s = speed;
    a = amplitude;
    f = frequency;
}

void CameraBob::update(float delta) {
    x += s * delta;
    y = (float)(a * sin(x * f));
}

float CameraBob::getX() {
    return x;
}
float CameraBob::getY() {
    return y;
}
float CameraBob::getSpeed() {
    return s;
}
float CameraBob::getAmplitude() {
    return a;
}
float CameraBob::getFrequency() {
    return f;
}

void CameraBob::setX(float x2) {
    x = x2;
}
void CameraBob::setY(float y1) {
    y = y1;
}
void CameraBob::setSpeed(float speed) {
    s = speed;
}
void CameraBob::setAmplitude(float amplitude) {
    a = amplitude;
}
void CameraBob::setFrequency(float frequency) {
    f = frequency;
}





