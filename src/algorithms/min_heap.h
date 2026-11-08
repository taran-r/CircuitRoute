#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <cstddef>
#include <stdexcept>
#include <vector>

struct HeapEntry {
    double priority;
    int vertex;
};

class MinHeap {
public:
    void insert(double priority, int vertex) {
        data.push_back(HeapEntry{priority, vertex});
        siftUp(data.size() - 1);
    }

    HeapEntry extractMin() {
        if (data.empty()) {
            throw std::out_of_range("MinHeap is empty");
        }

        HeapEntry minEntry = data[0];
        data[0] = data.back();
        data.pop_back();

        if (!data.empty()) {
            siftDown(0);
        }

        return minEntry;
    }

    bool empty() const {
        return data.empty();
    }
    std::size_t size() const {
        return data.size();
    }

private:
    std::vector<HeapEntry> data;
    bool lessThan(const HeapEntry& a, const HeapEntry& b) const {
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.vertex < b.vertex;
    }

    void siftUp(std::size_t index) {
        while (index > 0) {
            std::size_t parent = (index - 1) / 2;
            if (!lessThan(data[index], data[parent])) {
                break;
            }
            HeapEntry temp = data[index];
            data[index] = data[parent];
            data[parent] = temp;
            index = parent;
        }
    }

    void siftDown(std::size_t index) {
        while (true) {
            std::size_t left = 2 * index + 1;
            std::size_t right = 2 * index + 2;
            std::size_t smallest = index;

            if (left < data.size() && lessThan(data[left], data[smallest])) {
                smallest = left;
            }
            if (right < data.size() && lessThan(data[right], data[smallest])) {
                smallest = right;
            }
            if (smallest == index) {
                break;
            }

            
            HeapEntry temp = data[index];
            data[index] = data[smallest];
            data[smallest] = temp;
            index = smallest;
        }
    }
};

#endif
