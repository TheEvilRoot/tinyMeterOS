#ifndef SLIDING_BUFFER_HH
#define SLIDING_BUFFER_HH

#include <cstdlib>

struct Point;

struct SlidingBuffer {
    size_t window;
    Point *values{};
    size_t start{0};
    size_t pos{0};

    explicit SlidingBuffer(size_t window);

    ~SlidingBuffer();

    void Push(const Point& value);

    void Reset();
};

#endif //SLIDING_BUFFER_HH
