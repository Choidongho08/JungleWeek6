/*
 * mm-naive.c - 가장 빠르지만 메모리 효율은 가장 낮은 malloc 패키지입니다.
 *
 * 이 단순한 방식에서는 brk 포인터를 증가시켜 블록을 할당합니다.
 * 블록에는 페이로드만 있으며 헤더나 푸터는 없습니다.
 * 블록은 병합하거나 재사용하지 않습니다. realloc은 mm_malloc과
 * mm_free를 직접 사용해 구현합니다.
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
 * 학생 참고 사항: 다른 작업을 하기 전에 아래 구조체에
 * 팀 정보를 입력하세요.
 ********************************************************/
team_t team = {
    /* 팀 이름 */
    "ateam",
    /* 첫 번째 팀원의 이름 */
    "Harry Bovik",
    /* 첫 번째 팀원의 이메일 주소 */
    "bovik@cs.cmu.edu",
    /* 두 번째 팀원의 이름 (팀원이 없으면 비워 두세요) */
    "",
    /* 두 번째 팀원의 이메일 주소 */
    ""};

/* 단일 워드(4바이트) 또는 더블 워드(8바이트) 정렬 */
#define ALIGNMENT 8

/* 크기를 ALIGNMENT의 가장 가까운 배수로 올림 */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - malloc 패키지를 초기화합니다.
 */
int mm_init(void)
{
    return 0;
}

/*
 * mm_malloc - brk 포인터를 증가시켜 블록을 할당합니다.
 *     항상 크기가 정렬 단위의 배수인 블록을 할당합니다.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - 블록을 해제해도 아무 작업도 하지 않습니다.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - mm_malloc과 mm_free를 사용해 간단히 구현합니다.
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