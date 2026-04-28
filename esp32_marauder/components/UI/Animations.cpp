#include "Animations.h"
#include <math.h>

float easeInOutCubic(float t) {
    if (t < 0.5f)
        return 4.0f * t * t * t;
    float u = -2.0f * t + 2.0f;
    return 1.0f - (u * u * u) / 2.0f;
}

float easeInOutQuad(float t) {
    if (t < 0.5f)
        return 2.0f * t * t;
    float u = -2.0f * t + 2.0f;
    return 1.0f - (u * u) / 2.0f;
}

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

// ----------------------------------------------------------------

Animation::Animation(int durationMs)
    : _duration(durationMs), _startTime(0), _running(false) {}

void Animation::start() {
    _startTime = millis();
    _running   = true;
}

bool Animation::isRunning() const {
    if (!_running) return false;
    return (millis() - _startTime) < (unsigned long)_duration;
}

float Animation::getProgress() const {
    if (!_running) return 1.0f;
    unsigned long elapsed = millis() - _startTime;
    if (elapsed >= (unsigned long)_duration) return 1.0f;
    return (float)elapsed / (float)_duration;
}

void Animation::reset() {
    _running = false;
}
