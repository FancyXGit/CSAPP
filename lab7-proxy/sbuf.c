#include "csapp.h"
#include "sbuf.h"

/*
 * 创建n个空槽的缓冲区
 */
void sbuf_init(sbuf_t *sp, int n)
{
    // 分配全为0的缓冲区空间
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = 0;
    sp->rear = 0;
    // 初始时互斥锁设为1，槽位锁设为n，物品锁设为0
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

/*
 * 释放缓冲区空间
 */
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

/*
 * 向队列中添加元素（加到队列尾部）
 * items和slots锁保证队列不会溢出
 */
void sbuf_insert(sbuf_t *sp, int items)
{
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[sp->rear] = items;
    sp->rear = (sp->rear + 1) % sp->n;
    V(&sp->mutex);
    V(&sp->items);
}

/*
 * 从队列中取出元素（取出队列头部）
 * items和slots锁保证队列不会取空
 */
int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[sp->front];
    sp->front = (sp->front + 1) % sp->n;
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}
