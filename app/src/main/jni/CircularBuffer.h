#ifndef OBOEDEMO_CIRCULARBUFFER_H
#define OBOEDEMO_CIRCULARBUFFER_H

#include "vector"

class CircularBuffer {
public:
    CircularBuffer(size_t capacity)
            : buffer(capacity), head(0), tail(0), size(0), capacity(capacity) {}

    // Add data to the buffer
    bool write(const float *data, size_t numSamples);

    // Read data from the buffer
    size_t read(float *data, size_t numSamples);

    // Flush the buffer
    void flush();

    // Check if the buffer is empty
    bool isEmpty() const {
        return size == 0;
    }

    // Check how many samples can be read
    size_t available() const {
        return size;
    }

    size_t head, tail, size, capacity;
private:
    std::vector<float> buffer;
    std::mutex mutex;
    std::condition_variable dataAvailable;
};


#endif //OBOEDEMO_CIRCULARBUFFER_H
