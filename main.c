#include <coreinit/time.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/mutex.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/debug.h>
#include <coreinit/memexpheap.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_console.h>

#include <stdlib.h>

#define USE_DL_PREFIX
#include "malloc.h"

#define TEST_COUNT      8192
#define TEST_SIZE       32768
#define TEST_ALIGNMENT  128

extern void __init_wut_malloc();

static OSMutex malloc_lock;
static MEMHeapHandle mem2_handle;
static uint32_t mem2_size;
static void* custom_heap_base;
static uint32_t custom_heap_size;

static void *CustomAllocFromDefaultHeap(uint32_t size)
{
    OSLockMutex(&malloc_lock);
    void* ptr = dlmalloc(size);
    OSUnlockMutex(&malloc_lock);
    return ptr;
}

static void *CustomAllocFromDefaultHeapEx(uint32_t size, int32_t alignment)
{
    OSLockMutex(&malloc_lock);
    void* ptr = dlmemalign(alignment, size);
    OSUnlockMutex(&malloc_lock);
    return ptr;
}

static void CustomFreeToDefaultHeap(void* ptr)
{
    OSLockMutex(&malloc_lock);
    dlfree(ptr);
    OSUnlockMutex(&malloc_lock);
}

void *wiiu_morecore(long size)
{
    uint32_t old_size = custom_heap_size;
    uint32_t new_size = old_size + size;
    if (new_size > mem2_size) {
        return (void*) -1;
    }

    custom_heap_size = new_size;
    return ((uint8_t*) custom_heap_base) + old_size;
}

void __preinit_user(MEMHeapHandle *mem1, MEMHeapHandle *foreground, MEMHeapHandle *mem2)
{
    MEMAllocFromDefaultHeap = CustomAllocFromDefaultHeap;
    MEMAllocFromDefaultHeapEx = CustomAllocFromDefaultHeapEx;
    MEMFreeToDefaultHeap = CustomFreeToDefaultHeap;

    OSInitMutex(&malloc_lock);

    mem2_handle = *mem2;
    mem2_size = MEMGetAllocatableSizeForExpHeapEx(mem2_handle, 4);
    custom_heap_base = MEMAllocFromExpHeapEx(mem2_handle, mem2_size, 4);
    if (!custom_heap_base) {
        OSFatal("Cannot allocate custom heap");
    }

    custom_heap_size = 0;

    __init_wut_malloc();
}

void test_malloc(void)
{    
    void* buffers[TEST_COUNT] = {0};

    WHBLogPrintf("==========================================");
    WHBLogPrintf("Testing malloc for %d buffers with max size %d...", TEST_COUNT, TEST_SIZE);

    OSTime start_time = OSGetTime();

    for (int i = 0; i < TEST_COUNT; i++) {
        buffers[i] = malloc(rand() % TEST_SIZE + 1);
        if (!buffers[i]) {
            WHBLogPrintf("Out of memory");
        }

        // free random buffers to shuffle the heap
        if (rand() % 2 == 0) {
            int idx = rand() % TEST_COUNT;
            free(buffers[idx]);
            buffers[idx] = NULL;
        }
    }

    OSTime alloc_time = OSGetTime() - start_time;

    WHBLogPrintf("Freeing buffers...");

    start_time = OSGetTime();

    for (int i = 0; i < TEST_COUNT; i++) {
        free(buffers[i]);
    }

    OSTime free_time = OSGetTime() - start_time;

    WHBLogPrintf("Malloc test completed");
    WHBLogPrintf("Alloc took: %u microseconds", OSTicksToMicroseconds(alloc_time));
    WHBLogPrintf("Free took: %u microseconds", OSTicksToMicroseconds(free_time));
    WHBLogPrintf("==========================================");
}

void test_memalign(void)
{
    void* buffers[TEST_COUNT] = {0};

    WHBLogPrintf("==========================================");
    WHBLogPrintf("Testing memalign for %u buffers with max size %u and %u alignment...", TEST_COUNT, TEST_SIZE, TEST_ALIGNMENT);

    OSTime start_time = OSGetTime();

    for (int i = 0; i < TEST_COUNT; i++) {
        buffers[i] = memalign(TEST_ALIGNMENT, rand() % TEST_SIZE + 1);
        if (!buffers[i]) {
            WHBLogPrintf("Out of memory");
        }

        // free random buffers to shuffle the heap
        if (rand() % 2 == 0) {
            int idx = rand() % TEST_COUNT;
            free(buffers[idx]);
            buffers[idx] = NULL;
        }
    }

    OSTime alloc_time = OSGetTime() - start_time;

    WHBLogPrintf("Freeing buffers...");

    start_time = OSGetTime();

    for (int i = 0; i < TEST_COUNT; i++) {
        free(buffers[i]);
    }

    OSTime free_time = OSGetTime() - start_time;

    WHBLogPrintf("Memalign test completed");
    WHBLogPrintf("Alloc took: %u microseconds", OSTicksToMicroseconds(alloc_time));
    WHBLogPrintf("Free took: %u microseconds", OSTicksToMicroseconds(free_time));
    WHBLogPrintf("==========================================");
}

int main(int argc, char const *argv[])
{
    WHBProcInit();
    WHBLogUdpInit();
    WHBLogConsoleInit();

#ifdef USE_DEFAULTHEAP
    WHBLogPrint("Heap tester using default heap");
#else
    WHBLogPrint("Heap tester using custom heap");
#endif

    // init with a constant value to achieve similar results between runs
    srand(0xab12cd34);

    test_malloc();
    test_memalign();

    while (WHBProcIsRunning()) {
        WHBLogConsoleDraw();
    }

    WHBLogConsoleFree();
    WHBLogUdpDeinit();
    WHBProcShutdown();
    return 0;
}
