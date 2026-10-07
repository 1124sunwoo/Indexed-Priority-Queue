// Sunwoo Choi — CMPT 225 A3: Indexed Priority Queue
// Copyright (c) 2025 Sunwoo Choi. All rights reserved.
// See README.md for the original design and post-course improvements.
#ifndef INDPQ_H
#define INDPQ_H

#include <functional>
#include <utility>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

class IndPQ {
private:
    // Linear probing with tombstones: deletion must not break a probe chain.
    class HMap {
        enum class State { empty, occupied, deleted };
        struct Node {
            std::string key;
            int value = 0;
            State state = State::empty;
        };
        static constexpr std::size_t INITIAL_CAPACITY = 100;
        std::vector<Node> slots = std::vector<Node>(INITIAL_CAPACITY);
        std::size_t live = 0;
        std::size_t used = 0; // occupied + deleted slots

        std::size_t find(const std::string& key) const {
            auto i = std::hash<std::string>{}(key) % slots.size();
            for (std::size_t probes = 0; probes < slots.size(); ++probes) {
                const auto& slot = slots[i];
                if (slot.state == State::empty) break;
                if (slot.state == State::occupied && slot.key == key) return i;
                i = (i + 1) % slots.size();
            }
            return slots.size();
        }
        void place(const std::string& key, int value) {
            auto i = std::hash<std::string>{}(key) % slots.size();
            for (std::size_t probes = 0; probes < slots.size(); ++probes) {
                auto& slot = slots[i];
                if (slot.state != State::occupied) {
                    const bool wasEmpty = slot.state == State::empty;
                    slot.key = key;
                    slot.value = value;
                    slot.state = State::occupied;
                    ++live;
                    if (wasEmpty) ++used;
                    return;
                }
                i = (i + 1) % slots.size();
            }
            throw std::runtime_error("Hash table has no available slot");
        }
        void rehash(std::size_t capacity) {
            HMap replacement;
            replacement.slots.resize(capacity);
            for (const auto& slot : slots)
                if (slot.state == State::occupied)
                    replacement.place(slot.key, slot.value);
            slots.swap(replacement.slots);
            live = replacement.live;
            used = replacement.used;
        }
    public:
        bool contains(const std::string& key) const { return find(key) != slots.size(); }
        void insert(const std::string& key, int value) {
            if (contains(key)) throw std::runtime_error("Task ID already exists");
            // Grow for live entries; otherwise rebuild to discard tombstones.
            if ((live + 1) * 10 > slots.size() * 7) rehash(slots.size() * 2);
            else if ((used + 1) * 10 > slots.size() * 7) rehash(slots.size());
            place(key, value);
        }
        int getValue(const std::string& key) const {
            const auto i = find(key);
            if (i == slots.size()) throw std::runtime_error("Task ID not found");
            return slots[i].value;
        }
        void update(const std::string& key, int value) {
            const auto i = find(key);
            if (i == slots.size()) throw std::runtime_error("Task ID not found");
            slots[i].value = value;
        }
        void remove(const std::string& key) {
            const auto i = find(key);
            if (i == slots.size()) throw std::runtime_error("Task ID not found");
            slots[i].state = State::deleted;
            slots[i].key.clear();
            --live;
        }
        void clear() {
            for (auto& slot : slots) { slot.key.clear(); slot.state = State::empty; }
            live = used = 0;
        }
        std::vector<std::pair<std::string, int>> getEntries() const {
            std::vector<std::pair<std::string, int>> entries;
            for (const auto& slot : slots)
                if (slot.state == State::occupied) entries.emplace_back(slot.key, slot.value);
            return entries;
        }
    };

    // Inner class for Heap
    class Heap {
    private:
        std::vector<std::pair<std::string, int>> heap;
        HMap& map; 
        
        int parent(int i) { return (i - 1) / 2; }
        int leftChild(int i) { return 2 * i + 1; }
        int rightChild(int i) { return 2 * i + 2; }

        //move a node up 
        void percolateUp(int i) {

            while (i > 0 && heap[i].second < heap[parent(i)].second) {

                std::swap(heap[i], heap[parent(i)]);
                map.update(heap[i].first, i);
                map.update(heap[parent(i)].first, parent(i));
                i = parent(i);
            }
        }

        //move a node down
        void percolateDown(int i) {

            int smallest = i;
            int left = leftChild(i);
            int right = rightChild(i);
            int heapSize = static_cast<int>(heap.size());

            if (left < heapSize && heap[left].second < heap[smallest].second)
                smallest = left;

            if (right < heapSize && heap[right].second < heap[smallest].second)
                smallest = right;

            if (smallest != i) {

                std::swap(heap[i], heap[smallest]);
                map.update(heap[i].first, i);
                map.update(heap[smallest].first, smallest);
                percolateDown(smallest);
            }
        }

    public:
        Heap(HMap& m) : map(m) {}

        //insert a new task
        void insert(const std::string& taskid, int priority) {
            heap.push_back({taskid, priority});
            
            int index = static_cast<int>(heap.size()) - 1;

            try {
                map.insert(heap.back().first, index);
            } catch (...) {
                heap.pop_back();
                throw;
            }

            percolateUp(index);
        }

        //get the task ID with the minimum priority
        const std::string& getMin() const {

            if (heap.empty()) throw std::runtime_error("Heap is empty");

            return heap[0].first;
        }

        //remove and return the task id with the minimum priority
        std::string deleteMin() {
            if (heap.empty()) throw std::runtime_error("Heap is empty");

            std::string minTask = heap[0].first;

            if (heap.size() == 1) {
                heap.pop_back();
                map.remove(minTask);
                return minTask;
            }

            heap[0] = std::move(heap.back());
            heap.pop_back();
            map.update(heap[0].first, 0);
            map.remove(minTask);
            percolateDown(0);

            return minTask;
        }

        //update the priority of a task at a specific index
        void updatePriority(int i, int newPriority) {
            int heapSize = static_cast<int>(heap.size());

            if (i < 0 || i >= heapSize) throw std::out_of_range("Index out of range");

            int oldPriority = heap[i].second;
            heap[i].second = newPriority;

            if (newPriority < oldPriority) {
                percolateUp(i);
            } 
            else {
                percolateDown(i);
            }
        }

        //remove a task at a specific index
        void remove(int i) {
            int heapSize = static_cast<int>(heap.size());
            if (i < 0 || i >= heapSize) throw std::out_of_range("Index out of range");

            std::string task = heap[i].first;
            if (i != heapSize - 1) heap[i] = std::move(heap.back());
            heap.pop_back();
            map.remove(task);

            heapSize = static_cast<int>(heap.size());

            if (i < heapSize) {
                map.update(heap[i].first, i);
                if (i > 0 && heap[i].second < heap[parent(i)].second)
                    percolateUp(i);
                else
                    percolateDown(i);
            }
        }
        // check if the heap is empty
        bool isEmpty() const { return heap.empty(); }

        //get the number of tasks in the heap
        int getSize() const { return static_cast<int>(heap.size()); }

        //clear the heap
        void clear() { heap.clear(); }

        
        void print() const {
            for (const auto& task : heap) {
                std::cout << "Task: " << task.first << ", Priority: " << task.second << std::endl;
            }
        }
    };

    HMap map;
    Heap heap;

public:
    IndPQ() : heap(map) {}

    // Heap owns a reference to this object's map. Disallow implicit copies/moves
    // that would bind a new queue to the old queue's map.
    IndPQ(const IndPQ&) = delete;
    IndPQ& operator=(const IndPQ&) = delete;
    IndPQ(IndPQ&&) = delete;
    IndPQ& operator=(IndPQ&&) = delete;

    //insert a new task
    void insert(const std::string& taskid, int priority) {
        if (map.contains(taskid)) {
            throw std::runtime_error("Task ID already exists");
        }
        heap.insert(taskid, priority);
    }

    //remove and return the task
    std::string deleteMin() {
        return heap.deleteMin();
    }

    //get the task id with the minimum priority
    const std::string& getMin() const {
        return heap.getMin();
    }

    //update priority
    void updatePriority(const std::string& taskid, int newPriority) {
        int index = map.getValue(taskid);
        heap.updatePriority(index, newPriority);
    }

    void remove(const std::string& taskid) {
        int index = map.getValue(taskid);
        heap.remove(index);
    }

    bool isEmpty() const {
        return heap.isEmpty();
    }

    int size() const {
        return heap.getSize();
    }

    void clear() {
        heap.clear();
        map.clear();
    }

    //display
    void display() const {
        heap.print();
    }

    void ddisplay() const {
        std::cout << "heap contents:" << std::endl;
        heap.print();
        std::cout << "map contents:" << std::endl;
        for (const auto& entry : map.getEntries()) {
            std::cout << "Task: " << entry.first << ", Index: " << entry.second << std::endl;
        }
    }
};

#endif
