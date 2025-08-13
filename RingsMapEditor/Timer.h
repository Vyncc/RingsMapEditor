#pragma once
#include <chrono>

class Timer {
public:
    Timer() : running(false), elapsed(0) {}

    void Start() {
        if (!running)
        {
            running = true;
            startTime = std::chrono::steady_clock::now();
        }
    }

    void Stop() {
        if (running)
        {
            auto endTime = std::chrono::steady_clock::now();
            elapsed += std::chrono::duration<double>(endTime - startTime).count();
            running = false;
        }
    }

    double GetElapsedSeconds() const {
        if (running)
        {
            auto currentTime = std::chrono::steady_clock::now();
            return elapsed + std::chrono::duration<double>(currentTime - startTime).count();
        }
        return elapsed;
    }

    void Reset() {
        running = false;
        elapsed = 0;
    }

private:
    bool running;
    double elapsed;
    std::chrono::steady_clock::time_point startTime;
};