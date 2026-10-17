// Project identifier: 43DE0E0C4C76BFAA6D8C2F5AEAE0518A9C261F4E

#include "BinaryPQ.hpp"
#include "PairingPQ.hpp"
#include "SortedPQ.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <queue>
#include <random>
#include <utility>
#include <vector>

struct Item {
    explicit Item(int valueIn)
        : value(valueIn) {}
    int value;
};

struct ItemLess {
    bool operator()(const Item &lhs, const Item &rhs) const {
        return lhs.value < rhs.value;
    }
};

struct IntPtrLess {
    bool operator()(const int *lhs, const int *rhs) const {
        return *lhs < *rhs;
    }
};

template<template<typename...> typename PQ, typename Compare>
void randomizedTest(Compare compare) {
    PQ<int, Compare> actual(compare);
    std::priority_queue<int, std::vector<int>, Compare> expected(compare);
    std::mt19937 generator(281);

    for (int step = 0; step < 10000; ++step) {
        if (expected.empty() || generator() % 3 != 0) {
            const int value = static_cast<int>(generator() % 2001) - 1000;
            actual.push(value);
            expected.push(value);
        }
        else {
            assert(actual.top() == expected.top());
            actual.pop();
            expected.pop();
        }
        assert(actual.size() == expected.size());
        assert(actual.empty() == expected.empty());
        if (!expected.empty()) {
            assert(actual.top() == expected.top());
        }
    }

    while (!expected.empty()) {
        assert(actual.top() == expected.top());
        actual.pop();
        expected.pop();
    }
}

template<template<typename...> typename PQ>
void commonTests() {
    const std::vector<int> values { 4, -2, 9, 9, 0, 17, 3 };
    PQ<int> ranged(values.begin(), values.end());
    assert(ranged.size() == values.size());
    assert(ranged.top() == 17);

    PQ<int, std::greater<int>> minQueue(values.begin(), values.end());
    assert(minQueue.top() == -2);

    PQ<Item, ItemLess> nonDefault;
    nonDefault.push(Item(2));
    nonDefault.push(Item(8));
    assert(nonDefault.top().value == 8);

    std::vector<int> pointed { 5, 20, -1, 7 };
    PQ<const int *, IntPtrLess> pointerQueue;
    for (const int &value : pointed) {
        pointerQueue.push(&value);
    }
    pointed[2] = 100;
    pointed[1] = -50;
    pointerQueue.updatePriorities();
    assert(*pointerQueue.top() == 100);

    randomizedTest<PQ>(std::less<int>());
    randomizedTest<PQ>(std::greater<int>());
}

void pairingSpecificTests() {
    PairingPQ<int> queue;
    auto *one = queue.addNode(1);
    auto *five = queue.addNode(5);
    auto *three = queue.addNode(3);
    auto *four = queue.addNode(4);
    assert(queue.top() == 5);
    queue.updateElt(one, 10);
    assert(queue.top() == 10);
    queue.updateElt(three, 11);
    assert(queue.top() == 11);
    queue.updateElt(five, 12);
    assert(queue.top() == 12);
    queue.updateElt(four, 13);
    assert(queue.top() == 13);

    PairingPQ<int> copied(queue);
    PairingPQ<int> assigned;
    assigned = queue;
    queue.pop();
    assert(copied.top() == 13);
    assert(assigned.top() == 13);
    assert(queue.top() == 12);

    PairingPQ<int> moved(std::move(copied));
    assert(copied.empty());
    assert(moved.top() == 13);
    PairingPQ<int> moveAssigned;
    moveAssigned.push(-1);
    moveAssigned = std::move(assigned);
    assert(assigned.empty());
    assert(moveAssigned.top() == 13);

    while (!moved.empty()) {
        moved.pop();
    }
    while (!moveAssigned.empty()) {
        moveAssigned.pop();
    }

    PairingPQ<int> manyNodes;
    std::vector<PairingPQ<int>::Node *> handles;
    handles.reserve(2000);
    for (int value = 0; value < 2000; ++value) {
        handles.push_back(manyNodes.addNode(value));
    }
    for (int index = 0; index < 2000; index += 7) {
        manyNodes.updateElt(handles[static_cast<std::size_t>(index)], 10000 + index);
    }
    assert(manyNodes.top() == 11995);
    int previous = manyNodes.top();
    while (!manyNodes.empty()) {
        const int current = manyNodes.top();
        assert(current <= previous);
        previous = current;
        manyNodes.pop();
    }

    PairingPQ<int, std::greater<int>> minPairing;
    auto *minNode = minPairing.addNode(10);
    minPairing.push(2);
    minPairing.updateElt(minNode, -5);
    assert(minPairing.top() == -5);
}

int main() {
    commonTests<SortedPQ>();
    commonTests<BinaryPQ>();
    commonTests<PairingPQ>();
    pairingSpecificTests();
    std::cout << "All comprehensive priority queue tests passed.\n";
}
