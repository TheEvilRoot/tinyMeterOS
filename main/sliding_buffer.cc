#include "sliding_buffer.hh"

#include "point.hh"

SlidingBuffer::SlidingBuffer(const size_t window): window{window}, values{new Point[window * 2]{}} {
}

SlidingBuffer::~SlidingBuffer() {
    delete values;
}

void SlidingBuffer::Push(const Point &value) {
    if (pos >= window * 2) {
        pos = window;
        start = 0;
    }
    values[pos++] = value;
    if (start + pos > window) {
        start++;
        values[pos - window - 1] = value;
    }
}

void SlidingBuffer::Reset() {
    start = 0;
    pos = 0;
}
