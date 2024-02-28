#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "CircularBuffer.h"

bool CircularBuffer::write(const float *data, size_t numSamples) {
    std::unique_lock<std::mutex> lock(mutex);
    if (numSamples > capacity - size) {
        // Not enough space
        return false;
    }

    for (size_t i = 0; i < numSamples; ++i) {
        buffer[(head + i) % capacity] = data[i];
    }

    head = (head + numSamples) % capacity;
    size += numSamples;
    dataAvailable.notify_one();
    return true;
}

size_t CircularBuffer::read(float *data, size_t numSamples) {
    std::unique_lock<std::mutex> lock(mutex);
    dataAvailable.wait(lock, [&] { return size > 0; });

    size_t samplesRead = std::min(numSamples, size);
    for (size_t i = 0; i < samplesRead; ++i) {
        data[i] = buffer[(tail + i) % capacity];
    }

    tail = (tail + samplesRead) % capacity;
    size -= samplesRead;
    return samplesRead;
}

