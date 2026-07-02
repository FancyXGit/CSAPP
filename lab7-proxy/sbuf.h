#ifndef __SBUF_H_
#define __SBUF_H_

#include "csapp.h"

// 循环队列缓存区，带有锁保护
typedef struct sbuf_t
{
    int *buf;    // 缓冲区
    int n;       // 缓冲区大小
    int front;   // 队列头，指向队列的第一个元素，buf[front % n]
    int rear;    // 队列尾，指向队列下一个插入的元素，队列不为空时不允许和头指针重叠，最后一个元素buf[(rear - 1 + n) % n]
    sem_t mutex; // 互斥锁
    sem_t slots; // 槽位锁：当填入时P减少，当取出时V增加
    sem_t items; // 物品锁：当取出时P减少，当填入时V增加
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int items);
int sbuf_remove(sbuf_t *sp);

#endif