#include "CameraBob.hpp"
#include <cmath>

const float DAMPING_FACTOR = 0.55f; // Adjust this value for desired smoothness

void CameraBob::SinWave(float speed, float amplitude, float frequency) {
	x = 0;
	// y is not reset here, allowing smooth transition if already bobbing
	s = speed;
	a = amplitude;
	f = frequency;
	isBobbing = true;
}

void CameraBob::update(float delta) {
	if (isBobbing) {
		x += s * delta;
		float targetY = (float)(a * sin(x * f));
		// Smoothly interpolate towards the target bobbing position
		y += (targetY - y) * (1.0f - DAMPING_FACTOR);
	} else {
		// Smoothly interpolate back to center when not bobbing
		y *= DAMPING_FACTOR;
		// If y is very close to 0, snap it to 0 to avoid tiny oscillations
		if (std::abs(y) < 0.001f) {
			y = 0.0f;
		}
	}
}

void CameraBob::stopBobbing() {
	isBobbing = false;
}

bool CameraBob::getIsBobbing() const {
	return isBobbing;
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
