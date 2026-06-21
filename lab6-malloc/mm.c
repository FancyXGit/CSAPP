/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "YES",
    /* First member's full name */
    "FancyXGit",
    /* First member's email address */
    "no email",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * 概念
 * 要求对齐：ALIGNMENT=8，按照8字节对齐
 * 地址大小SIZE_T_SIZE = 8
 *
 * 隐式空闲链表：
 * 块的头大小为8字节，尾大小为8字节，0位标记分配或者空闲，1位标记前面的块是否为分配，3-63位标记块大小
 * 不使用4字节头尾是为了使得后续空闲链表指针对齐
 * 块大小是整个块的大小，按字节计数，包括头部和尾部
 * 最小的块大小：32字节，为后续显式链表指针留空间
 * 空闲块有头部和尾部，分配块尾部被用作数据占用
 * 块指针分两种标记：char* bp指向块的头部，char* ptr指向块中用户数据开始
 * 获得下一个块的头指针：bp + curr_block_size
 * 获取上一个块的头指针：检查当前块1位标记。如果前面块空闲，*(size_t*)(bp - 8) & (~0x7)拿到前一个块大小，之后bp - 块大小得到
 * 使用首次适配查找空闲堆
 * 堆空间不够使用mem_sbrk增加堆空间
 * 分割空闲块需检查剩下的块是否有32字节
 * 合并空闲块使用立刻合并
 * 当任何一个块分配状态改变，都必须修改后一个块的头第1位是否分配
 * 使用两个哨兵：头哨兵和尾哨兵，分配8字节，标记为分配
 * 哨兵只有头部
 */

#define MIN_BLOCK_SIZE 32

/* 头哨兵头部 */
static char *prologue_bp = NULL;
/* 尾哨兵头部 */
static char *epilogue_bp = NULL;

/* 掩码 */

#define ALLOC_MASK ((size_t)0x1)
#define SIZE_MASK (~(size_t)0x7)
#define PREV_ALLOC_MASK ((size_t)0x2)

/*
 * 组装块头部函数
 * block_size为块大小（字节数，32的倍数）
 * is_alloc为当前块是否分配（0或者1）
 * 前一个块是否分配的位由前一个块单独设置
 */
static inline size_t PackHead(size_t block_size, int is_alloc)
{
    return block_size | (!!is_alloc);
}

/*
 * 设置当前块的是否分配，即0位
 */
static inline void setAlloc(char *bp, int is_alloc)
{
    *((size_t *)bp) &= ~ALLOC_MASK;
    *((size_t *)bp) |= !!is_alloc;
}

/*
 * 设置当前块的前一个块是否标记，即1位
 */
static inline void setPrevAlloc(char *bp, int is_prev_alloc)
{
    *((size_t *)bp) &= ~PREV_ALLOC_MASK;
    *((size_t *)bp) |= !!is_prev_alloc << 1;
}

/*
 * 写入块头数据
 * bp为块头指针
 * val为要写入的值
 */
static inline void WriteHead(char *bp, size_t val)
{
    *((size_t *)bp) = val;
}

/*
 * 获取块的大小(字节数，包括头与可能的尾)
 * bp为块头指针
 */
static inline size_t getSize(char *bp)
{
    return *((size_t *)bp) & SIZE_MASK;
}

/*
 * 写入块尾（自动取头复制到尾部）
 * 调用前必须确定头部被正常写入
 * 且该块是空闲的
 */
static inline void WriteTail(char* bp)
{
    size_t head = *((size_t *)bp);
    size_t *tail = (size_t *)(bp + getSize(bp) - SIZE_T_SIZE);
    *tail = head;
}

/*
 * 获取块是否分配
 */
static inline int isAlloc(char *bp)
{
    return (int)(*((size_t *)bp) & ALLOC_MASK);
}

/*
 * 获取前一个块是否分配
 */
static inline int isPrevAlloc(char *bp)
{
    return (int)(*((size_t *)bp) & PREV_ALLOC_MASK);
}

/*
 * 传入块头指针，返回数据位置
 */
static inline char *bp2ptr(char *bp)
{
    // 隐式空闲链表数据指针 = 头指针 + 8
    return bp + SIZE_T_SIZE;
}

/*
 * 传入数据指针，返回块头指针
 */
static inline char *ptr2bp(char *ptr)
{
    // 隐式空闲链表块头指针 = 数据指针 - 8
    return ptr - SIZE_T_SIZE;
}

/*
 * 查询上一个块是否为头节点块
 */
static inline int isPrevPrologue(char *bp)
{
    return bp - SIZE_T_SIZE == prologue_bp;
}

/*
 * 获取下一块的头指针
 * 如果没有下一个块，返回NULL
 */
static inline char *getNextBp(char *bp)
{
    int next_bp = bp + getSize(bp);
    if (next_bp == epilogue_bp)
    {
        return NULL;
    }
    return next_bp;
}

/*
 * 获取上一个块的头指针
 * 如果上一个块为占用或者没有上一个块，返回NULL
 */
static inline char *getPrevBp(char *bp)
{
    // 如果上一个块为头哨兵，则一定被占用
    // 这里逻辑一致
    if (isPrevAlloc(bp))
    {
        return NULL;
    }
    char *prev_foot = bp - SIZE_T_SIZE;
    return bp - getSize(prev_foot);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int init_heap_size = 2 * SIZE_T_SIZE;
    void *p = mem_sbrk(init_heap_size);
    if (p == (void *)-1)
    {
        return 1;
    }
    size_t head = PackHead(SIZE_T_SIZE, 1);
    prologue_bp = (char *)p;
    epilogue_bp = (char *)p + SIZE_T_SIZE;
    WriteHead(prologue_bp, head);
    WriteHead(epilogue_bp, head);
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // 向上取整到8字节
    // 加上必须的头节点空间
    size_t need_size = ALIGN(size + 8);
    // 获取头哨兵的下一个块
    char *curr_bp = prologue_bp;
    // 最后一个有效块，为后面准备
    char *final_bp = curr_bp;
    while (curr_bp != NULL)
    {
        curr_bp = getNextBp(curr_bp);
        // 跳过分配块
        if (isAlloc(curr_bp))
        {
            final_bp = curr_bp;
            continue;
        }
        // 跳过空间不足的块
        size_t curr_size = getSize(curr_bp);
        if (curr_size < need_size)
        {
            final_bp = curr_bp;
            continue;
        }
        // 找到足够的块
        size_t left_size = curr_size - need_size;
        if (left_size < MIN_BLOCK_SIZE)
        {
            // 没有足够的剩余空间建新块
            // 直接使用整块
            // 将当前块设为占用
            setAlloc(curr_bp, 1);
            // 将下一个块设为占用
            char *next_bp = getNextBp(curr_bp);
            if (next_bp != NULL)
            {
                setPrevAlloc(next_bp, 1);
            }
            // 返回数据指针
            return bp2ptr(curr_bp);
        }
        else
        {
            // 保存之前块的占用信息
            int is_prev_alloc = isPrevAlloc(curr_bp);
            // 分裂成两个块
            size_t head_1 = PackHead(need_size, 1);
            size_t head_2 = PackHead(left_size, 0);
            // 写前一个块的头
            WriteHead(curr_bp, head_1);
            setPrevAlloc(curr_bp, is_prev_alloc);
            // 写后一个块的头尾
            char* bp_2 = getNextBp(curr_bp);
            WriteHead(bp_2, head_2);
            setPrevAlloc(bp_2, 1);
            WriteTail(bp_2);
            // 由于原本就是空闲的，所以下一个块的isPrevAlloc不需要写
            // 返回数据指针
            return bp2ptr(curr_bp);
        }
        // 下一个块
        final_bp = curr_bp;
    }
    // 找不到空闲的块
    // 增大堆空间
    void *p = mem_sbrk(need_size);
    if (p == (void *)-1)
    {
        printf("堆空间分配失败！\n");
        return NULL;
    }
    // 替换原来的堆顶，即尾哨兵
    size_t head = PackHead(need_size, 1);
    curr_bp = epilogue_bp;
    WriteHead(curr_bp, head);
    if (isAlloc(final_bp))
    {
        setPrevAlloc(curr_bp, 1);
    }
    // 设置正确的尾哨兵
    epilogue_bp += need_size;
    WriteHead(epilogue_bp, PackHead(SIZE_T_SIZE, 1));
    setPrevAlloc(epilogue_bp, 1);
    return bp2ptr(curr_bp);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
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

int mm_check(void)
{
    printf("CHECK MM\n");
    return 1;
}
