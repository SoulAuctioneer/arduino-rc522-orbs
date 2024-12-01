#ifndef SIMPLE_QUEUE_H
#define SIMPLE_QUEUE_H

template<typename T>
class SimpleQueue {
private:
    static const int MAX_SIZE = 10;
    T items[MAX_SIZE];
    int front;
    int rear;
    int size;

public:
    SimpleQueue() : front(0), rear(-1), size(0) {}
    
    bool push(T item) {
        if (size >= MAX_SIZE) return false;
        rear = (rear + 1) % MAX_SIZE;
        items[rear] = item;
        size++;
        return true;
    }
    
    bool pop() {
        if (size <= 0) return false;
        front = (front + 1) % MAX_SIZE;
        size--;
        return true;
    }
    
    T getFront() {
        return items[front];
    }
    
    bool empty() {
        return size == 0;
    }
};

#endif 