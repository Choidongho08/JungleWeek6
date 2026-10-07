/*
 * mm-explicit.c 명시적 가용 리스트
 *
 * 헤드와 푸터가 존재함.
 * payload내에 prev와 next의 주소가 들어있음.
 *
 * 학생 참고 사항: 이 헤더 주석을 여러분의 구현을 개괄적으로 설명하는
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

#define CHUNKSIZE (1 << 12)
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
#define NEXT_(bp) *(void**)((char*)bp + ALIGNMENT)
#define PREV_(bp) *(void**)bp

static void* extend_heap(size_t words);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);
static void* coalesce(void* bp);
static void remove_free_block(void* bp);
static void push_free_block(void* bp);

/*
 * mm_init - malloc 구현을 초기화합니다.
 */
char* p; // 주소(+, *등)를 계산하기 위해 1바이트인 char 사용.
char* next_fit_p;
char* list;
int mm_init(void)
{
    p = mem_sbrk(4 * ALIGNMENT);
    if(p == (void*)-1)
        return -1;
    PUT(p, 0); // padding (4b)
    PUT(p + WSIZE, PACK((3 * ALIGNMENT), 1)); // header (4b)
    *(void**)(p + ALIGNMENT) = NULL; // prev
    *(void**)(p + (2*ALIGNMENT)) = NULL; // next
    PUT(p + ((2 * ALIGNMENT) + (2 * WSIZE)), PACK((3 * ALIGNMENT), 1)); // footer
    PUT(p + ((2 * ALIGNMENT) + (3 * WSIZE)), PACK(0, 0));
    p += (2 * WSIZE); // p => payload
    list = NULL;
    next_fit_p = list;

    if(extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void* extend_heap(size_t words) // 4096 / 4 = 1024
{
    char* bp;
    size_t size;

    size = words % 2 ? (words+1) * WSIZE/*홀수*/ : words * WSIZE;/*짝수*/ // size => 1024 * 4 = 4096
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    
    bp = coalesce(bp);
    push_free_block(bp);
    return bp;
}

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendSize;
    char* bp;

    if(size == 0)
        return NULL;

    if(size <= ALIGNMENT)
        asize = 3 * ALIGNMENT; // 헤더랑 푸터 포함인건가? ㅇㅇ
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
    for(bp = next_fit_p; bp != NULL; bp = NEXT_(bp))
    {
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }
    for(bp = list; bp != next_fit_p && bp != NULL; bp = NEXT_(bp))
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
    if((csize - asize) >= 3 * ALIGNMENT)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        remove_free_block(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK((csize - asize), 0));
        PUT(FTRP(bp), PACK((csize - asize), 0));
        push_free_block(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        remove_free_block(bp);
    }
}

static void remove_free_block(void* bp)
{
    if(PREV_(bp)) // prev가 중간이거나 마지막일 때
    {
        NEXT_(PREV_(bp)) = NEXT_(bp);
        if(next_fit_p == bp)
            next_fit_p = NEXT_(bp);
    }
    if(NEXT_(bp)) // next가 중간이거나 처음일 때
    {
        PREV_(NEXT_(bp)) = PREV_(bp);
        if(next_fit_p == bp)
            next_fit_p = NEXT_(bp);
    }
    if(!PREV_(bp)) // bp가 머리일때
    {
        list = NEXT_(bp);
    }
    
    PREV_(bp) = NULL;
    NEXT_(bp) = NULL;
}

static void push_free_block(void* bp)
{
    // 이미 분리가 된 상태로 들어옴
    PREV_(bp) = NULL;
    NEXT_(bp) = list;
    if(list)
        PREV_(list) = bp;
    list = bp;
}

void mm_free(void *ptr)
{
    if(!ptr)
        return;

    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    ptr = coalesce(ptr);
    push_free_block(ptr);
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
        remove_free_block(NEXT_BLKP(bp));
        
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if(!prev_alloc && next_alloc)
    {
        remove_free_block(PREV_BLKP(bp));
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        remove_free_block(NEXT_BLKP(bp));
        remove_free_block(PREV_BLKP(bp));
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
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