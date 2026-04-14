/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 * memlib.c - 메모리 시스템을 시뮬레이션하는 모듈입니다.
 *            학생이 구현한 malloc 패키지 호출과
 *            libc의 시스템 malloc 패키지 호출을 섞어서 사용할 수 있게 해 주므로 필요합니다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "memlib.h"
#include "config.h"

/* private variables */
/* 내부에서만 사용하는 변수들 */
static char *mem_start_brk;  /* points to first byte of heap */
                            /* 힙의 첫 번째 바이트를 가리킵니다. */
static char *mem_brk;        /* points to last byte of heap */
                            /* 힙의 마지막 바이트를 가리킵니다. */
static char *mem_max_addr;   /* largest legal heap address */ 
                            /* 힙에서 허용되는 가장 큰 주소입니다. */

/* 
 * mem_init - initialize the memory system model
 * mem_init - 메모리 시스템 모델을 초기화합니다.
 */
void mem_init(void)
{
    /* allocate the storage we will use to model the available VM */
    /* 사용 가능한 가상 메모리를 모델링하는 데 사용할 저장 공간을 할당합니다. */
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
	fprintf(stderr, "mem_init_vm: malloc error\n");
	exit(1);
    }

    mem_max_addr = mem_start_brk + MAX_HEAP;  /* max legal heap address */
                                              /* 허용되는 최대 힙 주소 */
    mem_brk = mem_start_brk;                  /* heap is empty initially */
                                              /* 처음에는 힙이 비어 있습니다. */
}

/* 
 * mem_deinit - free the storage used by the memory system model
 * mem_deinit - 메모리 시스템 모델이 사용한 저장 공간을 해제합니다.
 */
void mem_deinit(void)
{
    free(mem_start_brk);
}

/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 * mem_reset_brk - 시뮬레이션된 brk 포인터를 초기화하여 빈 힙 상태로 되돌립니다.
 */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 * mem_sbrk - sbrk 함수를 단순화해 모델링한 함수입니다. 힙을
 *    incr 바이트만큼 확장하고 새 영역의 시작 주소를 반환합니다. 이
 *    모델에서는 힙을 줄일 수 없습니다.
 */
void *mem_sbrk(int incr) 
{
    char *old_brk = mem_brk;

    if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
	errno = ENOMEM;
	fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
	return (void *)-1;
    }
    mem_brk += incr;
    return (void *)old_brk;
}

/*
 * mem_heap_lo - return address of the first heap byte
 * mem_heap_lo - 힙의 첫 번째 바이트 주소를 반환합니다.
 */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/* 
 * mem_heap_hi - return address of last heap byte
 * mem_heap_hi - 힙의 마지막 바이트 주소를 반환합니다.
 */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 * mem_heapsize() - 힙의 크기를 바이트 단위로 반환합니다.
 */
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
}

/*
 * mem_pagesize() - returns the page size of the system
 * mem_pagesize() - 시스템의 페이지 크기를 반환합니다.
 */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}
