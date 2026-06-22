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

int mm_check(void);

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
static inline void WriteTail(char *bp)
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
    return (int)(*((size_t *)bp) & PREV_ALLOC_MASK) >> 1;
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
    char *next_bp = bp + getSize(bp);
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
 * 辅助函数：求最大值
 */
static inline size_t max(size_t a, size_t b)
{
    return a >= b ? a : b;
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
    setPrevAlloc(epilogue_bp, 1);

#ifdef DEBUG
    if (mm_check() == -1)
    {
        printf("正在执行函数mm_init(void))\n");
    }
#endif

    return 0;
}

/*
 * mm_malloc
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t arg1 = size;
    // 向上取整到8字节
    // 加上必须的头节点空间
    // 至少32字节大小
    size_t need_size = max(32, ALIGN(size + 8));
    // 获取头哨兵的下一个块
    char *curr_bp = prologue_bp;
    // 最后一个有效块，为后面准备
    char *final_bp = curr_bp;
    while ((curr_bp = getNextBp(curr_bp)) != NULL)
    {
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
            char *bp_2 = getNextBp(curr_bp);
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

#ifdef DEBUG
    if (mm_check() == -1)
    {
        printf("正在执行函数mm_malloc(size_t size = %zd)\n", arg1);
    }
#endif

    return bp2ptr(curr_bp);
}

/*
 * 递归合并后面的所有空闲块
 * 如果后一个块为被占用，则终止
 * 会自动设置合并完成后的下一个块的isPrevAlloc
 * 注意只有发生合并才会设置isPrevAlloc
 * bp: 当前空闲块头指针
 */
static void CombineLatter(char *bp)
{
    // 获取当前块和下一个块
    char *curr_bp = bp;
    char *next_bp = getNextBp(curr_bp);
    // 没有下一个块：终止
    if (next_bp == NULL)
    {
        return;
    }
    // 遇到下一个块被占用：终止
    if (isAlloc(next_bp))
    {
        return;
    }
    // 保存前一个块是否占用的信息
    int is_prev_alloc = isPrevAlloc(curr_bp);
    // 执行合并操作
    size_t sum_size = getSize(curr_bp) + getSize(next_bp);
    WriteHead(curr_bp, PackHead(sum_size, 0));
    setPrevAlloc(curr_bp, is_prev_alloc);
    WriteTail(curr_bp);
    // 获取合并之后的下一个块，并设置之前未分配
    next_bp = getNextBp(curr_bp);
    // 没有下一个块：终止
    if (next_bp == NULL)
    {
        setPrevAlloc(epilogue_bp, 0);
        return;
    }
    setPrevAlloc(next_bp, 0);
    // 递归
    CombineLatter(curr_bp);
}

/*
 * 递归合并前面所有空闲块
 * 如果前面的块被占用，则终止
 */
static void CombineFormer(char *bp)
{
    char *curr_bp = bp;
    char *prev_bp = getPrevBp(bp);
    // 到达堆底或者到达被占用块
    // 将当前块isPrevAlloc设为1
    // 终止递归
    if (prev_bp == NULL)
    {
        setPrevAlloc(curr_bp, 1);
        return;
    }
    // 合并操作
    int is_prev_alloc = isPrevAlloc(prev_bp);
    size_t sum_size = getSize(curr_bp) + getSize(prev_bp);
    WriteHead(prev_bp, PackHead(sum_size, 0));
    setPrevAlloc(prev_bp, is_prev_alloc);
    WriteTail(prev_bp);
    // 递归
    CombineFormer(prev_bp);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    void *arg1 = ptr;
    // 处理空指针
    if (ptr == NULL)
    {
        return;
    }
    // 释放当前块内存空间
    char *curr_bp = ptr2bp((char *)ptr);
    // 设置当前与之后两个块的状态
    setAlloc(curr_bp, 0);
    WriteTail(curr_bp);
    char *next_bp = getNextBp(curr_bp);
    if (next_bp != NULL)
        setPrevAlloc(next_bp, 0);
    else
        setPrevAlloc(epilogue_bp, 0);
    // 递归合并前后相邻空闲块
    CombineLatter(curr_bp);
    CombineFormer(curr_bp);
#ifdef DEBUG
    if (mm_check() == -1)
    {
        printf("正在执行函数mm_free(void *ptr = %p)\n", arg1);
    }
#endif
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    // 空指针
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }
    // 大小为0
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }
    char *curr_bp = ptr2bp(ptr);
    size_t need_size = max(32, ALIGN(size + 8));
    size_t curr_size = getSize(curr_bp);
    // 需要大小更小，无需改动
    if (need_size <= curr_size)
    {
        return ptr;
    }
    // 需要地址更大，另外扩充空间
    // 看看后面有没有空闲块可以合并
    char *next_bp = getNextBp(curr_bp);
    if (next_bp != NULL && !isAlloc(next_bp))
    {
        // 下一个块为空，如果加起来够大直接合并，然后重新调用
        size_t new_size = getSize(curr_bp) + getSize(next_bp);
        if (new_size >= need_size)
        {
            // 看看大小能不能分裂
            size_t left_size = new_size - need_size;
            if (left_size >= MIN_BLOCK_SIZE)
            {
                // 大小足够，执行分裂
                char *next_next_bp = curr_bp + need_size;
                WriteHead(next_next_bp, PackHead(left_size, 0));
                setPrevAlloc(next_next_bp, 1);
                WriteTail(next_next_bp);
                // 设置头
                int is_prev_alloc = isPrevAlloc(curr_bp);
                WriteHead(curr_bp, PackHead(need_size, 1));
                setPrevAlloc(curr_bp, is_prev_alloc);
                return ptr;
            }
            else
            {
                // 整段合并
                int is_prev_alloc = isPrevAlloc(curr_bp);
                WriteHead(curr_bp, PackHead(new_size, 1));
                setPrevAlloc(curr_bp, is_prev_alloc);
                next_bp = getNextBp(curr_bp);
                if (next_bp != NULL)
                {
                    setPrevAlloc(next_bp, 1);
                }
                else
                {
                    setPrevAlloc(epilogue_bp, 1);
                }
                return ptr;
            }
        }
    }
    // 没有空闲块，另开一处
    char *old_ptr = (char *)ptr;
    char *new_ptr = mm_malloc(size);
    size_t old_data_size = curr_size - SIZE_T_SIZE;
    memcpy(new_ptr, old_ptr, old_data_size);
    mm_free(old_ptr);
    return new_ptr;
}

/*
 * 获取块头的辅助函数
 */
static inline size_t getHead(char *bp)
{
    return *((size_t *)bp);
}

/*
 * 获取块尾的辅助函数
 * 不关心该块是否空闲
 */
static inline size_t getTail(char *bp)
{
    return *(size_t *)(bp + getSize(bp) - SIZE_T_SIZE);
}

/*
 * 打印前后块的信息
 */
static inline void PrintBlockInfo(char *prev_bp, char *curr_bp)
{
    printf("前一个块信息：地址: %p, 大小: %d, 分配: %d, 前面是否分配:%d\n", prev_bp, getSize(prev_bp), isAlloc(prev_bp), isPrevAlloc(prev_bp));
    printf("当前块信息：地址: %p, 大小: %d, 分配: %d, 前面是否分配:%d\n", curr_bp, getSize(curr_bp), isAlloc(curr_bp), isPrevAlloc(curr_bp));
    printf("当前堆头哨兵地址: %p , 尾哨兵地址 %p\n", prologue_bp, epilogue_bp);
    printf("仅返回最近的一次错误\n");
}

int mm_check(void)
{
    char *prev_bp = prologue_bp;
    char *curr_bp = prologue_bp;
    // 遍历检查全部堆块
    while ((curr_bp = getNextBp(curr_bp)) != NULL)
    {
        // 检查是否对齐
        if ((size_t)curr_bp % 8 != 0)
        {
            printf("\n错误！指针未能按8字节对齐！当前块指针 %p\n", curr_bp);
            PrintBlockInfo(prev_bp, curr_bp);
            return -1;
        }
        // 检查prev_alloc字段
        int curr_is_prev_alloc = isPrevAlloc(curr_bp);
        int prev_alloc = isAlloc(prev_bp);
        if (prev_alloc != curr_is_prev_alloc)
        {
            printf("\n错误！位于 %p 的块的 alloc 为 %d , 但是下一个块 %p 的 prev_alloc 为 %d , 不匹配！\n", prev_bp, prev_alloc, curr_bp, curr_is_prev_alloc);
            PrintBlockInfo(prev_bp, curr_bp);
            return -1;
        }
        // 检查空闲块头尾块是否一致正确
        if (!isAlloc(curr_bp))
        {
            size_t head = getHead(curr_bp);
            size_t tail = getTail(curr_bp);
            if (head != tail)
            {
                printf("\n错误！位于%p的空闲块块头和块尾不一致,块头为%#zx,块尾为%#zx,不匹配！\n", curr_bp, head, tail);
                PrintBlockInfo(prev_bp, curr_bp);
                return -1;
            }
        }
        // 检查两个空闲块是否相邻
        // 立即合并不能出现相邻空闲块
        if (!isAlloc(curr_bp))
        {
            if (!isAlloc(prev_bp))
            {
                printf("\n错误！相邻的两个空闲块,块1地址 %p, 块2地址 %p \n", prev_bp, curr_bp);
                PrintBlockInfo(prev_bp, curr_bp);
                return -1;
            }
        }
        // 一轮检查结束，递增prev_bp
        prev_bp = curr_bp;
    }
    return 1;
}
