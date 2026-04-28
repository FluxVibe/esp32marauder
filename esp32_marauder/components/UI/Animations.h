#pragma once

#include <Arduino.h>

// Cubic ease-in-out: smooth acceleration and deceleration
float easeInOutCubic(float t);

// Quadratic ease-in-out: lighter version
float easeInOutQuad(float t);

// Linear interpolation between a and b by factor t (0.0 - 1.0)
float lerpf(float a, float b, float t);

// Timer-based animation helper.
// Call start() to begin. getProgress() returns 0.0 -> 1.0 over the duration,
// then clamps at 1.0. isRunning() is false once progress reaches 1.0.
class Animation {
public:
    explicit Animation(int durationMs = 300);

    void start();
    bool isRunning() const;
    float getProgress() const;   // 0.0 .. 1.0
    void reset();

private:
    int           _duration;
    unsigned long _startTime;
    bool          _running;
};
