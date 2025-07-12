#pragma once
#include <deque>
#include <cstdint>
#include <numeric>
#include <cmath>

class MispricingBuffer {
public:
    MispricingBuffer(std::size_t window_size) : max_size(window_size) {}

    void add(double delta) {
        buffer.push_back(delta);
        if (buffer.size() > max_size) buffer.pop_front();
    }

    double average() const {
        if (buffer.empty()) return 0.0;
        double sum = std::accumulate(buffer.begin(), buffer.end(), 0.0);
        return sum / buffer.size();
    }

    bool isPersistentSpike(double threshold) const {
        return std::abs(average()) >= threshold;
    }

private:
    std::deque<double> buffer;
    std::size_t max_size;
};