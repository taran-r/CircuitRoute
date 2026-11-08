#include "../src/algorithms/min_heap.h"

#include <cassert>
#include <iostream>

int main() {
    MinHeap emptyHeap;
    assert(emptyHeap.empty());
    assert(emptyHeap.size() == 0);

    MinHeap single;
    single.insert(5.0, 3);
    assert(!single.empty());
    assert(single.size() == 1);
    HeapEntry only = single.extractMin();
    assert(only.priority == 5.0 && only.vertex == 3);
    assert(single.empty());
    assert(single.size() == 0);

    MinHeap order;
    order.insert(4.0, 10);
    order.insert(1.0, 20);
    order.insert(3.0, 30);
    order.insert(2.0, 40);
    assert(order.size() == 4);

    HeapEntry a = order.extractMin();
    HeapEntry b = order.extractMin();
    HeapEntry c = order.extractMin();
    HeapEntry d = order.extractMin();
    assert(a.priority == 1.0 && a.vertex == 20);
    assert(b.priority == 2.0 && b.vertex == 40);
    assert(c.priority == 3.0 && c.vertex == 30);
    assert(d.priority == 4.0 && d.vertex == 10);
    assert(order.empty());

    MinHeap ties;
    ties.insert(2.0, 9);
    ties.insert(2.0, 1);
    ties.insert(2.0, 5);
    HeapEntry t1 = ties.extractMin();
    HeapEntry t2 = ties.extractMin();
    HeapEntry t3 = ties.extractMin();
    assert(t1.priority == 2.0 && t1.vertex == 1);
    assert(t2.priority == 2.0 && t2.vertex == 5);
    assert(t3.priority == 2.0 && t3.vertex == 9);

    MinHeap duplicates;
    duplicates.insert(5.0, 7);
    duplicates.insert(1.0, 7);
    duplicates.insert(3.0, 7);
    assert(duplicates.size() == 3);
    HeapEntry d1 = duplicates.extractMin();
    HeapEntry d2 = duplicates.extractMin();
    HeapEntry d3 = duplicates.extractMin();
    assert(d1.priority == 1.0 && d1.vertex == 7);
    assert(d2.priority == 3.0 && d2.vertex == 7);
    assert(d3.priority == 5.0 && d3.vertex == 7);

    MinHeap mixed;
    mixed.insert(8.0, 1);
    mixed.insert(3.0, 2);
    assert(mixed.extractMin().priority == 3.0);
    mixed.insert(1.0, 3);
    mixed.insert(4.0, 4);
    assert(mixed.size() == 3);
    assert(mixed.extractMin().priority == 1.0);
    assert(mixed.extractMin().priority == 4.0);
    assert(mixed.extractMin().priority == 8.0);
    assert(mixed.empty());

    std::cout << "Passed: empty/single heaps, extraction order, size tracking, "
                 "equal-priority ties, duplicates, and mixed insert/extract.\n";
    return 0;
}
