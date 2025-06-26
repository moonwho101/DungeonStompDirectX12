#include "CameraBob.hpp"
#include <cmath>


void CameraBob::SinWave(float speed, float amplitude, float frequency) {
    x = 0;
    y = 0;
    s = speed;
    a = amplitude;
    f = frequency;
    centering = false;
    centerSpeed = 8.0f; // default smoothing speed
}

void CameraBob::startCentering(float speed) {
    centering = true;
    centerSpeed = speed;
}

void CameraBob::stopCentering() {
    centering = false;
}

void CameraBob::update(float delta) {
    if (centering) {
        // Smoothly interpolate y towards 0
        y += (0.0f - y) * (1.0f - expf(-centerSpeed * delta));
        // Optionally, also smooth x to 0 if needed
        x += (0.0f - x) * (1.0f - expf(-centerSpeed * delta));
        // Optionally stop centering if close enough
        if (fabs(y) < 0.001f && fabs(x) < 0.001f) {
            y = 0.0f;
            x = 0.0f;
            centering = false;
        }
    } else {
        x += s * delta;
        y = (float)(a * sin(x * f));
    }
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





