#include <coreinit/time.h>
#include <coreinit/memdefaultheap.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_console.h>

#include <malloc.h>
#include <stdlib.h>

#define TEST_COUNT      8192
#define TEST_SIZE       32768
#define TEST_ALIGNMENT  128

extern void __init_wut_malloc();

//#define USE_DEFAULTHEAP
#ifdef USE_DEFAULTHEAP
void
__preinit_user(MEMHeapHandle *outMem1,
               MEMHeapHandle *outFG,
               MEMHeapHandle *outMem2)
{
    __init_wut_malloc();
}
#endif

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
