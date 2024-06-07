/*
 *
 * memspeed.c
 * test memory write speed
 * each byte will be set to a value between 0~255
 * neighbour bytes will have different value
 * the memory write was splitted into g_loop loops,
 * in each loop g_mem_per_loop MB will be allocated and initilized
 *
 * joehuang.sweden@gmail.com
 *
 */

#include <stdio.h>
#include <stdint.h>	/* for uint64 definition */
#include <stdlib.h>	/* for exit() definition */
#include <unistd.h>
#include <time.h>	/* for clock_gettime */
#include <string.h>

#include "mimalloc.h"
#include "static.c"

int g_mimalloc = 0;
int g_huge = 0;
int g_loop = 200;
int g_mem_per_loop = 10;

#define TOTAL_LOOP (g_loop)
#define MEM_PER_LOOP ((g_mem_per_loop)*1024*1024)
#define REPEAT_TIMES 50
#define BILLION 1000000000L

inline void *mymalloc(size_t size) {
	if (g_mimalloc) {
		return mi_malloc(size);
	}
	else {
		return malloc(size);
	}
}

void *test_mem(int loop) {
    int i;
    char *pMem = NULL;

    pMem = mymalloc(MEM_PER_LOOP);
    if (pMem == NULL) {
        printf("no enough memory %d loop \n", loop);
        return NULL;
    }

    for (i=0; i<MEM_PER_LOOP; i++) {
        *(pMem+i) = i % 255;
    }

    return pMem;
}

void repeat_test_mem(char *pMem) {
    int i;

    for (i=0; i<MEM_PER_LOOP; i++) {
        *(pMem+i) = i % 255;
    }
}

void repeat_test_loop(int n, char *pAllMem) {
        int i, j;
        char *pMem = NULL;

        for (i=0; i<n; i++) {
		printf("repeat_test_loop: %d\n", i);
		for (j=0; j<TOTAL_LOOP; j++) {
			memcpy(&pMem, pAllMem + j*sizeof(char *), sizeof(char *));
			repeat_test_mem(pMem);
		}
        }

	printf("finished all repeat \n");
}

void test_loop(void) {
    int loop;
    char *pMem;
    char *pAllMem;
    char *pNow;

    pAllMem = mymalloc((TOTAL_LOOP+2)*sizeof(char *));
    if (pAllMem == NULL) {
        printf("memory pointer space allocation failure \n");
        return;
    }

    memset(pAllMem, 0x00, (TOTAL_LOOP+2)*sizeof(char *));
    pNow = pAllMem;
    for (loop=0; loop<TOTAL_LOOP; loop++) {
        pMem = test_mem(loop);
        if (pMem == NULL) {
            return;
        }
        memcpy(pNow, &pMem, sizeof(char *));
        pNow += sizeof(char *);
    }

    //repeat_test_loop(REPEAT_TIMES, pAllMem);
}

void init_mimalloc(void) {
	if (!g_huge || !g_mimalloc) {
		return;
	}

	printf("test with huge page enabled\n");
	mi_option_enable(mi_option_allow_large_os_pages);
	mi_option_set(mi_option_reserve_huge_os_pages, 4);

	printf("mi_option_allow_large_os_pages:%ld\n",
		       	mi_option_get(mi_option_allow_large_os_pages));

	printf("mi_option_reserve_huge_os_pages:%ld\n",
			mi_option_get(mi_option_reserve_huge_os_pages));
}

bool handle_para(int argc, char **argv) {

	if (argc < 2) {
		return true;
	}

	if (argc > 5) {
		printf("too many arguments \n");
		return false;
	}

	if (argc >= 2) {
		g_mimalloc = atoi(argv[1]);
		if (!g_mimalloc) {
			return true;
		}
	}

	if (argc == 3 || argc == 4 || argc == 5) {
		g_huge = atoi(argv[2]);
		if (argc == 4 || argc == 5) {
			g_mem_per_loop = atoi(argv[3]);
			if(argc == 5) {
				g_loop = atoi(argv[4]);
			}
		}
	}

	return true;
}

int main(int argc, char **argv)
{
	uint64_t diff;
	struct timespec start, end;

	printf("memspeed [mimalloc] [huge] [mem_per_loop] [loop]\n");
	if (!handle_para(argc, argv)) {
		return 0;
	}

	init_mimalloc();

	clock_gettime(CLOCK_MONOTONIC, &start);
	test_loop();
	clock_gettime(CLOCK_MONOTONIC, &end);

	diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
	printf("elapsed time = %llu nanoseconds\n", (long long unsigned int) diff);

	return 0;
}
