/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * mm-naive.c - 가장 빠르지만 메모리 효율은 가장 낮은 malloc 패키지입니다.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * 이 단순한 방식에서는 블록을 할당할 때 brk 포인터를 단순히 증가시키기만 합니다.
 * the brk pointer.  A block is pure payload. There are no headers or
 * 블록은 순수한 payload(사용자 데이터 영역)만 가지며, 헤더와 푸터는 없습니다.
 * footers.  Blocks are never coalesced or reused. Realloc is
 * 블록들은 합쳐지지(coalesce)도 않고 재사용되지도 않습니다. realloc은
 * implemented directly using mm_malloc and mm_free.
 * mm_malloc과 mm_free를 직접 사용하는 방식으로 구현되어 있습니다.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * 학생들에게: 이 파일 상단 주석은 여러분의 구현 방식을 높은 수준에서 설명하는
 * comment that gives a high level description of your solution.
 * 주석으로 바꾸세요.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * 학생들에게: 다른 작업보다 먼저 아래 구조체에 팀 정보를 입력하세요.
 * provide your team information in the following struct.
 * 
 ********************************************************/
team_t team = {
    /* Team name */
    /* 팀 이름 */
    "ateam",
    /* First member's full name */
    /* 첫 번째 팀원의 전체 이름 */
    "Harry Bovik",
    /* First member's email address */
    /* 첫 번째 팀원의 이메일 주소 */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    /* 두 번째 팀원의 전체 이름 (없으면 비워 두세요) */
    "",
    /* Second member's email address (leave blank if none) */
    /* 두 번째 팀원의 이메일 주소 (없으면 비워 두세요) */
    ""};

/* single word (4) or double word (8) alignment */
/* 워드(4바이트) 또는 더블 워드(8바이트) 단위로 정렬합니다. */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
/* size를 ALIGNMENT의 배수 중 가장 가까운 큰 값으로 올림합니다. */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char*heap_listp=0;
static void*extend_heap(size_t words);
static void*coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);


/*
 * mm_init - initialize the malloc package.
 * mm_init - malloc 패키지를 초기화합니다.
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1) {
        return -1;
    }
    PUT(heap_listp, 0);                             // Alignment padding
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    // Prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    // Prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));        // Epilogue header
    heap_listp += (2*WSIZE);                       
     
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL){      // chunksize만큼 힙을 늘려서 큰 free block 생성.
        return -1;                                  // NULL이면 확장 실패, -1 반환
    }
    return 0;
}

static void *extend_heap(size_t words){ // 힙 확장
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; // words가 홀수면 1 더해서 짝수로 만들고 WSIZE 곱, 짝수면 그대로 곱 (8바이트 정렬)
    if((long)(bp = mem_sbrk(size)) == -1){                  // 힙을 size바이트만큼 확장, 실패하면 NULL 반환 
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));           // 새 free block의  header 추가
    PUT(FTRP(bp), PACK(size, 0));           // 새 free block의 footer 추가
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));   // 새 Epilogue header 추가

    return coalesce(bp);    // 인접 free blcok과 합치기
}

static void *find_fit(size_t asize){
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) { // prologue부터 epilogue 만나기 전까지
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){     // 빈 블록이면서 현재 블록의 크기가 요청 크기 이상이면
            return bp;                                                  // 찾으면 반환
        }
    }
    return NULL;    // 못 찾을 경우
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));          // 할당할 free block의 전체 크기

    if ((csize - asize) >= (2 * DSIZE)) {       // free block에 요청한 크기를 할당하고 남는 공간이 16바이트 이상인 경우
        PUT(HDRP(bp), PACK(asize, 1));          // 현재 block의 앞부분을 asize만큼 할당하고 header와 footer를 allocated block으로 표시
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);                     // 분리된 다음 block으로 이동
        PUT(HDRP(bp), PACK(csize - asize, 0));  // 다음 블럭 header와 footer를 free block으로 표시
        PUT(FTRP(bp), PACK(csize - asize, 0));  
    } else {                                    // 남는 공간이 너무 작아서 블록을 나누지 않고 그대로 할당
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


void *mm_malloc(size_t size)
{
    size_t asize;       // 실제 블록 크기 (header + footer + 요청크기 + padding크기)
    size_t extendsize;  // free block이 없을 때 늘릴 힙 크기
    char *bp;           // payload 시작 주소

    if (size == 0){
        return NULL;
    }
    if (size <= DSIZE){     // size가 8바이트보다 같거나 작을 때 
        asize = 2*DSIZE;    // 블록 사이즈를 16바이트로 고정
    }
    else{
        asize = DSIZE * ((size + (DSIZE) + (DSIZE -1)) / DSIZE);    // 8의 배수로 정렬
    }

    if ((bp = find_fit(asize)) != NULL){    // 선택할 블록 찾으면
        place(bp, asize);                   // 해당 주소에 배치
        return bp;
    }                                       // 못 찾은 경우 힙을 늘려야 함

    extendsize = MAX(asize, CHUNKSIZE);                
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){  // 워드를 넘기기 때문에 4바이트로 나눠준 값 넘김
        return NULL;                                    // extend_heap()이 힙 뒤에 새 메모리 붙이고 free block으로 초기화
    }
    place(bp, asize);   // 새로 확작한 free block에 요청 크기만큼 배치
    return bp;          // 할당한 블록의 payload 주소 반환
}

void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));  // 현재 블록의 전체 크기

    PUT(HDRP(ptr),PACK(size, 0));       // header, footer free 상태로 바꿈
    PUT(FTRP(ptr), PACK(size, 0));      
    coalesce(ptr);                      // free한 블록을 검사(이전 블록과 다음 블록이 free인지)
}

static void *coalesce(void *bp){                        // bp는 현재 free된 블록의 payload 주소
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 이전 블록이 사용 중인지(1 or 0 저장됨)
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 다음 블록이 사용 중인지(1 or 0 저장됨)
    size_t size = GET_SIZE(HDRP(bp));                   // 현재 free 블록 크기

    if (prev_alloc && next_alloc){  // 이전, 다음 블록 둘 다 사용 중이면 합칠 수 없으므로 리턴
        return bp;
    }

    else if (prev_alloc && !next_alloc){        // 앞 사용 중, 뒤 free 인 경우
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));  // 다음 블록 크기를 현재 size에 더함
        PUT(HDRP(bp), PACK(size, 0));           // 현재 블록 header를 합쳐진 크기로 바꿈
        PUT(FTRP(bp), PACK(size, 0));           // 병합된 블록 footer를 합쳐진 크기로 바꿈
    }

    else if (!prev_alloc && next_alloc){            // 앞 free, 뒤 사용 중인 경우
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));      // 이전 블록 크기를 더함
        PUT(FTRP(bp), PACK(size, 0));               // 병합된 블록 footer를 합쳐진 크기로 바꿈
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));    // 이전 블록 header를 합쳐진 크기로 바꿈
        bp = PREV_BLKP(bp);                         // 새 시작점이 이전 블록이므로 이전 블록으로 이동
    }
    else{   // 둘 다 free 인 경우
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); // 이전, 현재, 다음 블록 크기 전부 더함
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));    // 제일 앞 블록 header를 합쳐진 크기로 바꿈
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));    // 제일 뒤 블록 footer를 합쳐진 크기로 바꿈
        bp = PREV_BLKP(bp);                         // 새 시작점은 이전 블록이므로 이전 블록으로 이동
    }
    return bp;  // free block 시작 주소 반환
}


void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}