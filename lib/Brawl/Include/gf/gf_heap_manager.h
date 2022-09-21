#pragma once

#include <memory.h>

class gfHeapManager
{
public:
    static void *getMEMAllocator(HeapType heapType);
};