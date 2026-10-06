/*
 * mm-naive.c - 가장 빠르지만 메모리 효율은 가장 낮은 malloc 구현.
 *
 * 이 단순한 방식에서는 brk 포인터를 증가시켜 블록을 할당합니다.
 * 블록은 사용자 데이터(payload)만으로 이루어지며, 헤더나 푸터가 없습니다.
 * 블록을 병합하거나 재사용하지 않습니다. realloc은 mm_malloc과 mm_free를
 * 직접 사용하여 구현합니다.
 * 
 * ->
 *
 * 책 내용대로 헤더와 푸터를 사용하여 메모리 효율을 높인 방식.
 *
 * 학생 안내: 이 머리말을 자신의 구현 방식을 전반적으로 설명하는
 * 주석으로 교체하세요.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * 학생 안내: 다른 작업을 시작하기 전에 아래 구조체에
 * 팀 정보를 입력하세요.
 ********************************************************/
team_t team = {
    /* 팀 이름 */
    "team12",
    /* 첫 번째 팀원의 전체 이름 */
    "dongho choi",
    /* 첫 번째 팀원의 이메일 주소 */
    "jijiji0809160@gmail.com",
    /* 두 번째 팀원의 전체 이름 (없으면 빈 문자열) */
    "haihai1222555@gmail.com",
    /* 두 번째 팀원의 이메일 주소 (없으면 빈 문자열) */
    "beeeeyayeyao@gmail.com"};

/* 단일 워드(4바이트) 또는 더블 워드(8바이트) 경계에 정렬 */
#define WSIZE 4
#define ALIGNMENT 8

/* size를 ALIGNMENT의 가장 가까운 배수로 올림 */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define CHUNKSIZE 1 << 12
#define MAX(x, y) (x > y ? x : y)
#define PACK(size, alloc) (size|alloc)
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char*)bp - WSIZE)
#define FTRP(bp) ((char*)bp + GET_SIZE(HDRP(bp)) - ALIGNMENT)
#define NEXT_BLKP(bp) ((char*)bp + GET_SIZE(((char*)bp - WSIZE)))
#define PREV_BLKP(bp) ((char*)bp - GET_SIZE((char*)bp - ALIGNMENT))

static void* extend_heap(size_t words);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);
static void* coalesce(void* bp);

/*
 * mm_init - malloc 구현을 초기화합니다.
 */
 char* p;
int mm_init(void)
{
    p = mem_sbrk(4*WSIZE);
    if(p == (void*)-1)
        return -1;

    PUT(p, 0);
    PUT(p + WSIZE, PACK(ALIGNMENT, 1));
    PUT(p + (2*WSIZE), PACK(ALIGNMENT, 1));
    PUT(p + (3 * WSIZE), PACK(0, 1));
    p += (2*WSIZE);

    if(extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void* extend_heap(size_t words)
{
    char* bp;
    size_t size;

    size = words % 2 ? (words+1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/*
 * mm_malloc - brk 포인터를 증가시켜 블록을 할당합니다.
 *     항상 정렬 단위의 배수 크기로 블록을 할당합니다.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendSize;
    char* bp;

    if(size == 0)
        return NULL;

    if(size <= ALIGNMENT)
        asize = 2 * ALIGNMENT;
    else
        asize = ALIGNMENT * ((size + ALIGNMENT + ALIGNMENT-1) / ALIGNMENT);
    
    if((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    extendSize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendSize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

static void* find_fit(size_t asize)
{
    void* bp;
    for(bp = p; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }
    return NULL;
}

static void place(void* bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    if((csize- asize) >= 2*ALIGNMENT)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK((csize - asize), 0));
        PUT(FTRP(bp), PACK((csize - asize), 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_free - 이 구현에서는 블록을 해제할 때 아무 작업도 하지 않습니다.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc)
        return bp;
    else if(prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if(!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

/*
 * mm_realloc - mm_malloc과 mm_free를 사용하여 단순하게 구현합니다.
 */
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