#pragma once

#include "km_debug.h"

template <typename T>
struct DynamicArray
{
    uint32 size;
    uint32 capacity;
    T* data;

    DynamicArray();
    DynamicArray(uint32 capacity);

    DynamicArray<T> Copy() const;
    void Append(T element);
    // Slow, linear time
    void Remove(uint32 idx);
    void Clear();
    void Free();

    // TODO make inline
    inline T& operator[](int index) const {
#if GAME_SLOW
        DEBUG_ASSERT(0 <= index && index < (int)size);
#endif
        return data[index];
    }
};

/*
template <typename K, typename V>
struct HashNode
{
    K key;
    V value;
    HashNode* next;
}

template <typename K>
struct KeyHash
{
    unsigned long operator()(const K& key) const;
}

template <typename K, typename V, typename F = KeyHash<K>>
struct HashMap
{
    uint32 size;
    HashNode<K, V>* table;
}
*/