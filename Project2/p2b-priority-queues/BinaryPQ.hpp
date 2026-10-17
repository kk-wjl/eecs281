// Project identifier: 43DE0E0C4C76BFAA6D8C2F5AEAE0518A9C261F4E

#ifndef BINARYPQ_H
#define BINARYPQ_H


#include <algorithm>

#include "Eecs281PQ.hpp"

// A specialized version of the priority queue ADT implemented as a binary heap.
template<typename TYPE, typename COMP_FUNCTOR = std::less<TYPE>>
class BinaryPQ : public Eecs281PQ<TYPE, COMP_FUNCTOR> {
    // This is a way to refer to the base class object.
    using BaseClass = Eecs281PQ<TYPE, COMP_FUNCTOR>;

public:
    // Description: Construct an empty PQ with an optional comparison functor.
    // Runtime: O(1)
    explicit BinaryPQ(COMP_FUNCTOR comp = COMP_FUNCTOR())
        : BaseClass { comp } {}  // BinaryPQ


    // Description: Construct a PQ out of an iterator range with an optional
    //              comparison functor.
    // Runtime: O(n) where n is number of elements in range.
    template<typename InputIterator>
    BinaryPQ(InputIterator start, InputIterator end, COMP_FUNCTOR comp = COMP_FUNCTOR())
        : BaseClass { comp }
        , data { start, end } {
        updatePriorities();
    }  // BinaryPQ


    // Description: Destructor doesn't need any code, the data vector will
    //              be destroyed automatically.
    virtual ~BinaryPQ() = default;


    // Description: Copy constructors don't need any code, the data vector
    //              will be copied automatically.
    BinaryPQ(const BinaryPQ &) = default;
    BinaryPQ(BinaryPQ &&) noexcept = default;


    // Description: Copy assignment operators don't need any code, the data
    //              vector will be copied automatically.
    BinaryPQ &operator=(const BinaryPQ &) = default;
    BinaryPQ &operator=(BinaryPQ &&) noexcept = default;


    // Description: Assumes that all elements inside the heap are out of order and
    //              'rebuilds' the heap by fixing the heap invariant.
    // Runtime: O(n)
    virtual void updatePriorities() {
        for (std::size_t i = data.size() / 2; i > 0; --i) {
            fixDown(i - 1);
        }
    }  // updatePriorities()


    // Description: Add a new element to the PQ.
    // Runtime: O(log(n))
    virtual void push(const TYPE &val) {
        data.push_back(val);
        fixUp(data.size() - 1);
    }  // push()


    // Description: Remove the most extreme (defined by 'compare') element
    //              from the PQ.
    // Note: We will not run tests on your code that would require it to pop
    // an element when the PQ is empty. Though you are welcome to if you are
    // familiar with them, you do not need to use exceptions in this project.
    // Runtime: O(log(n))
    virtual void pop() {
        std::swap(data.front(), data.back());
        data.pop_back();
        if (!data.empty()) {
            fixDown(0);
        }
    }  // pop()


    // Description: Return the most extreme (defined by 'compare') element of
    //              the PQ. This should be a reference for speed. It MUST
    //              be const because we cannot allow it to be modified, as
    //              that might make it no longer be the most extreme element.
    // Runtime: O(1)
    virtual const TYPE &top() const {
        return data.front();
    }  // top()


    // Description: Get the number of elements in the PQ.
    // Runtime: O(1)
    [[nodiscard]] virtual std::size_t size() const {
        return data.size();
    }  // size()


    // Description: Return true if the PQ is empty.
    // Runtime: O(1)
    [[nodiscard]] virtual bool empty() const {
        return data.empty();
    }  // empty()


private:
    // Note: This vector *must* be used for your PQ implementation.
    std::vector<TYPE> data;
    // NOTE: You are not allowed to add any member variables. You don't need
    //       a "heapSize", since you can call your own size() member
    //       function, or check data.size().

    // Restore the heap invariant after insertion.
    void fixUp(std::size_t index) {
        while (index > 0) {
            const std::size_t parent = (index - 1) / 2;
            if (!this->compare(data[parent], data[index])) {
                break;
            }
            std::swap(data[parent], data[index]);
            index = parent;
        }
    }

    void fixDown(std::size_t index) {
        while (true) {
            const std::size_t left = index * 2 + 1;
            if (left >= data.size()) {
                return;
            }

            const std::size_t right = left + 1;
            std::size_t extremeChild = left;
            if (right < data.size() && this->compare(data[left], data[right])) {
                extremeChild = right;
            }

            if (!this->compare(data[index], data[extremeChild])) {
                return;
            }
            std::swap(data[index], data[extremeChild]);
            index = extremeChild;
        }
    }
};  // BinaryPQ


#endif  // BINARYPQ_H
