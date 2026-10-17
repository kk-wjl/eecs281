// Project identifier: 43DE0E0C4C76BFAA6D8C2F5AEAE0518A9C261F4E

#ifndef PAIRINGPQ_H
#define PAIRINGPQ_H

#include <deque>
#include <utility>

#include "Eecs281PQ.hpp"

// A specialized version of the priority queue ADT implemented as a pairing
// heap.
template<typename TYPE, typename COMP_FUNCTOR = std::less<TYPE>>
class PairingPQ : public Eecs281PQ<TYPE, COMP_FUNCTOR> {
    // This is a way to refer to the base class object.
    using BaseClass = Eecs281PQ<TYPE, COMP_FUNCTOR>;

public:
    // Each node within the pairing heap
    class Node {
    public:
        // Description: Custom constructor that creates a node containing
        //              the given value.
        explicit Node(const TYPE &val)
            : elt { val } {}

        // Description: Allows access to the element at that Node's position.
        //              There are two versions, getElt() and a dereference
        //              operator, use whichever one seems more natural to you.
        // Runtime: O(1) - this has been provided for you.
        const TYPE &getElt() const { return elt; }
        const TYPE &operator*() const { return elt; }

        // The following line allows you to access any private data
        // members of this Node class from within the PairingPQ class.
        // (ie: myNode.elt is a legal statement in PairingPQ's add_node()
        // function).
        friend PairingPQ;

    private:
        TYPE elt;
        Node *child = nullptr;
        Node *sibling = nullptr;
        Node *previous = nullptr;
    };  // Node


    // Description: Construct an empty pairing heap with an optional
    //              comparison functor.
    // Runtime: O(1)
    explicit PairingPQ(COMP_FUNCTOR comp = COMP_FUNCTOR())
        : BaseClass { comp } {}  // PairingPQ()


    // Description: Construct a pairing heap out of an iterator range with an
    //              optional comparison functor.
    // Runtime: O(n) where n is number of elements in range.
    template<typename InputIterator>
    PairingPQ(InputIterator start, InputIterator end, COMP_FUNCTOR comp = COMP_FUNCTOR())
        : BaseClass { comp } {
        for (; start != end; ++start) {
            addNode(*start);
        }
    }  // PairingPQ()


    // Description: Copy constructor.
    // Runtime: O(n)
    PairingPQ(const PairingPQ &other)
        : BaseClass { other.compare } {
        std::deque<const Node *> pending;
        if (other.root != nullptr) {
            pending.push_back(other.root);
        }
        while (!pending.empty()) {
            const Node *node = pending.front();
            pending.pop_front();
            addNode(node->elt);
            if (node->child != nullptr) {
                pending.push_back(node->child);
            }
            if (node->sibling != nullptr) {
                pending.push_back(node->sibling);
            }
        }
    }  // PairingPQ()


    // Description: Copy assignment operator.
    // Runtime: O(n)
    PairingPQ &operator=(const PairingPQ &rhs) {
        PairingPQ copy(rhs);
        std::swap(this->compare, copy.compare);
        std::swap(root, copy.root);
        std::swap(nodeCount, copy.nodeCount);
        return *this;
    }  // operator=()


    // Description: Destructor
    // Runtime: O(n)
    ~PairingPQ() {
        clear();
    }  // ~PairingPQ()


    // Description: Move constructor and assignment operators don't need any
    //              code, the members will be reused automatically.
    PairingPQ(PairingPQ &&other) noexcept
        : BaseClass { std::move(other.compare) }
        , root { other.root }
        , nodeCount { other.nodeCount } {
        other.root = nullptr;
        other.nodeCount = 0;
    }

    PairingPQ &operator=(PairingPQ &&rhs) noexcept {
        if (this != &rhs) {
            clear();
            this->compare = std::move(rhs.compare);
            root = rhs.root;
            nodeCount = rhs.nodeCount;
            rhs.root = nullptr;
            rhs.nodeCount = 0;
        }
        return *this;
    }


    // Description: Assumes that all elements inside the pairing heap are out
    //              of order and 'rebuilds' the pairing heap by fixing the
    //              pairing heap invariant.  You CANNOT delete 'old' nodes
    //              and create new ones!
    // Runtime: O(n)
    virtual void updatePriorities() {
        std::deque<Node *> pending;
        if (root != nullptr) {
            pending.push_back(root);
        }

        root = nullptr;
        while (!pending.empty()) {
            Node *node = pending.front();
            pending.pop_front();
            if (node->child != nullptr) {
                pending.push_back(node->child);
            }
            if (node->sibling != nullptr) {
                pending.push_back(node->sibling);
            }
            node->child = nullptr;
            node->sibling = nullptr;
            node->previous = nullptr;
            root = meld(root, node);
        }
    }  // updatePriorities()


    // Description: Add a new element to the pairing heap. This is already
    //              done. You should implement push functionality entirely
    //              in the addNode() function, and this function calls
    //              addNode().
    // Runtime: O(1)
    virtual void push(const TYPE &val) { addNode(val); }  // push()


    // Description: Remove the most extreme (defined by 'compare') element
    //              from the pairing heap.
    // Note: We will not run tests on your code that would require it to pop
    // an element when the pairing heap is empty. Though you are welcome to
    // if you are familiar with them, you do not need to use exceptions in
    // this project.
    // Runtime: Amortized O(log(n))
    virtual void pop() {
        Node *oldRoot = root;
        std::deque<Node *> firstPass;
        Node *child = oldRoot->child;

        while (child != nullptr) {
            Node *first = child;
            Node *second = child->sibling;
            child = second == nullptr ? nullptr : second->sibling;

            first->previous = nullptr;
            first->sibling = nullptr;
            if (second != nullptr) {
                second->previous = nullptr;
                second->sibling = nullptr;
                firstPass.push_back(meld(first, second));
            }
            else {
                firstPass.push_back(first);
            }
        }

        delete oldRoot;
        --nodeCount;
        root = nullptr;
        while (!firstPass.empty()) {
            Node *heap = firstPass.back();
            firstPass.pop_back();
            root = meld(heap, root);
        }
    }  // pop()


    // Description: Return the most extreme (defined by 'compare') element of
    //              the pairing heap. This should be a reference for speed.
    //              It MUST be const because we cannot allow it to be
    //              modified, as that might make it no longer be the most
    //              extreme element.
    // Runtime: O(1)
    virtual const TYPE &top() const {
        return root->elt;
    }  // top()


    // Description: Get the number of elements in the pairing heap.
    // Runtime: O(1)
    [[nodiscard]] virtual std::size_t size() const {
        return nodeCount;
    }  // size()

    // Description: Return true if the pairing heap is empty.
    // Runtime: O(1)
    [[nodiscard]] virtual bool empty() const {
        return root == nullptr;
    }  // empty()


    // Description: Updates the priority of an element already in the pairing
    //              heap by replacing the element refered to by the Node with
    //              new_value.  Must maintain pairing heap invariants.
    //
    // PRECONDITION: The new priority, given by 'new_value' must be more
    //              extreme (as defined by comp) than the old priority.
    //
    // Runtime: As discussed in reading material.
    void updateElt(Node *node, const TYPE &new_value) {
        node->elt = new_value;
        if (node == root) {
            return;
        }

        Node *previous = node->previous;
        if (previous->child == node) {
            previous->child = node->sibling;
        }
        else {
            previous->sibling = node->sibling;
        }
        if (node->sibling != nullptr) {
            node->sibling->previous = previous;
        }

        node->previous = nullptr;
        node->sibling = nullptr;
        root = meld(root, node);
    }  // updateElt()


    // Description: Add a new element to the pairing heap. Returns a Node*
    //              corresponding to the newly added element.
    // Runtime: O(1)
    // NOTE: Whenever you create a node, and thus return a Node *, you must
    //       be sure to never move or copy/delete that node in the future,
    //       until it is eliminated by the user calling pop(). Remember this
    //       when you implement updateElt() and updatePriorities().
    Node *addNode(const TYPE &val) {
        Node *node = new Node(val);
        root = meld(root, node);
        ++nodeCount;
        return node;
    }  // addNode()


private:
    // NOTE: For member variables, you are only allowed to add a "root
    //       pointer" and a "count" of the number of nodes. Anything else
    //       (such as a deque) should be declared inside of member functions
    //       as needed.
    Node *root = nullptr;
    std::size_t nodeCount = 0;

    Node *meld(Node *first, Node *second) {
        if (first == nullptr) {
            return second;
        }
        if (second == nullptr) {
            return first;
        }
        if (this->compare(first->elt, second->elt)) {
            std::swap(first, second);
        }

        second->previous = first;
        second->sibling = first->child;
        if (first->child != nullptr) {
            first->child->previous = second;
        }
        first->child = second;
        first->previous = nullptr;
        return first;
    }

    void clear() {
        std::deque<Node *> pending;
        if (root != nullptr) {
            pending.push_back(root);
        }
        while (!pending.empty()) {
            Node *node = pending.front();
            pending.pop_front();
            if (node->child != nullptr) {
                pending.push_back(node->child);
            }
            if (node->sibling != nullptr) {
                pending.push_back(node->sibling);
            }
            delete node;
        }
        root = nullptr;
        nodeCount = 0;
    }
};


#endif  // PAIRINGPQ_H
