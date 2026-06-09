#include "cachelab.h"
#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MAX_FILENAME_LENGTH 64

// 缓存块结构
typedef struct
{
    unsigned long long tag;
    int valid;
    unsigned long long time_stamp;
} Block;

// 缓存组模型，内部包含一个或者多个块
typedef struct
{
    Block *blocks;
} Set;

// 缓存模型，包含一个或者多个组，还有缓存的参数
typedef struct
{
    int S; // 组数
    int E; // 相联度
    int B; // 块大小
    Set *sets;
} Cache;

// 全局缓存结构变量
Cache *cache;
// 参数
int help_flag;
int verbose_flag;
// 全局计数器，用于缓存的LRU
static unsigned long long global_counter = 0;

typedef struct
{
    int hits;
    int misses;
    int evictions;
} CacheRecord;

// 读参数函数
// cache_arg长度至少为3，储存缓存参数S,E,B
// filename传入一个字符数组，将填入读到的文件名
// return 1表示读取失败，return 0表示读取成功
int ReadCacheArgument(int argc, char *argv[], int *cache_arg, int cache_arg_len, char *filename, int filename_len)
{
    int opt;
    help_flag = 0;
    verbose_flag = 0;
    if (cache_arg_len < 3)
    {
        return 1;
    }
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            help_flag = 1;
            break;
        case 'v':
            verbose_flag = 1;
            break;
        case 's':
            cache_arg[0] = 1 << atoi(optarg);
            break;
        case 'E':
            cache_arg[1] = atoi(optarg);
            break;
        case 'b':
            cache_arg[2] = 1 << atoi(optarg);
            break;
        case 't':
            strncpy(filename, optarg, filename_len - 1);
            filename[filename_len - 1] = '\0';
            break;
        case '?':
            fprintf(stderr, "Usage: %s -hv -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
            return 1;
        }
    }
    return 0;
}

// 在堆上为缓存结构分配空间
int InitCache(int S, int E, int B)
{
    // 第1层：cache
    cache = (Cache *)malloc(sizeof(Cache));
    if (cache == NULL)
    {
        printf("InitCache:为cache分配内存失败\n");
        return 1;
    }
    cache->S = S;
    cache->E = E;
    cache->B = B;
    // 第2层：set
    cache->sets = (Set *)malloc(S * sizeof(Set));
    if (cache->sets == NULL)
    {
        printf("InitCache:为set分配内存失败\n");
        return 1;
    }
    for (int i = 0; i < S; i++)
    {
        // 第3层：block
        cache->sets[i].blocks = (Block *)malloc(E * sizeof(Block));
        if (cache->sets[i].blocks == NULL)
        {
            printf("InitCache:为block分配内存失败\n");
            return 1;
        }
        for (int j = 0; j < E; j++)
        {
            cache->sets[i].blocks[j].valid = 0;
        }
    }
    return 0;
}

// 释放缓存结构的空间
void FreeCache(void)
{
    int S = cache->S;
    // 第3层：block
    for (int i = 0; i < S; i++)
    {
        free(cache->sets[i].blocks);
    }
    // 第2层：set
    free(cache->sets);
    // 第1层：cache
    free(cache);
}

// 阅读文件的一行
// 自动跳过I(首个字符不是空格)
// return 0正常读取, return 1到达文件末尾或者出错
int ReadOneLine(FILE *fp, char *buffer, int size)
{
    while (fgets(buffer, size, fp) != NULL)
    {
        if (buffer[0] != ' ')
        {
            continue;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        return 0;
    }
    return 1;
}

// 缓存操作结构体
typedef struct
{
    // 进行的操作
    // 1： L 读操作 Load
    // 2： S 写操作 Store
    // 3： M 读写操作 Modify
    int operation;
    // 进行操作的地址
    unsigned long long address;
} CacheOperation;

// 对缓存操作进行解读，结果存到给定的CacheOperation结构体中
int ParseLine(char *content, CacheOperation *res)
{
    char oper;
    char hex_str[32];

    if (sscanf(content, " %c %[^,]", &oper, hex_str) != 2)
    {
        printf("PraseLine: 字符串%s解析失败\n", content);
        return 1;
    }

    switch (oper)
    {
    case 'L':
        res->operation = 1;
        break;
    case 'S':
        res->operation = 2;
        break;
    case 'M':
        res->operation = 3;
        break;
    default:
        printf("PraseLine: 操作类型%c错误\n", oper);
        return 1;
        break;
    }

    char *endptr;
    res->address = strtoull(hex_str, &endptr, 16);
    if (*endptr != '\0')
    {
        fprintf(stderr, "字符串%s包含非十六进制字符: %s\n", hex_str, endptr);
        return 1;
    }
    return 0;
}

// TESTING
void testParseline(FILE *fp, char *buffer, int size, CacheOperation *res)
{
    while (ReadOneLine(fp, buffer, size) == 0)
    {
        printf("%s\n", buffer);
        ParseLine(buffer, res);
        printf("Operation: %d\n", res->operation);
        printf("Address: %llu\n", res->address);
    }
}

typedef struct
{
    unsigned long long tag;
    int set;
    int offset;
} AddressPart;

void DisassembleAddress(unsigned long long add, int S, int E, int B, AddressPart *ap)
{
    int offset_bit = log2(B);
    int set_bit = log2(S);
    ap->offset = add & ((1ULL << offset_bit) - 1);
    ap->set = (add >> offset_bit) & ((1ULL << set_bit) - 1);
    ap->tag = add >> (offset_bit + set_bit);
}

int GetLestUse(Set *set, int set_size)
{
    int lest_stamp = set->blocks[0].time_stamp;
    int index = 0;
    for (int i = 0; i < set_size; i++)
    {
        if (set->blocks[i].time_stamp < lest_stamp)
        {
            index = i;
            lest_stamp = set->blocks[i].time_stamp;
        }
    }
    return index;
}

// return 0: hit
// return 1: miss
// return 2: miss eviction
// return 3: miss hit
// return 4: miss eviction hit
// return 5: error
int DoCache(CacheOperation *op, CacheRecord *cr)
{
    const int E = cache->E;

    AddressPart ap;
    DisassembleAddress(op->address, cache->S, cache->E, cache->B, &ap);

    const int set = ap.set;
    const unsigned long long tag = ap.tag;

    // 找到对应组
    Set *curr_set = &cache->sets[set];
    // 遍历组内所有块
    int is_set_full = 1;
    int first_empty_block;
    for (int i = 0; i < E; i++)
    {
        Block *curr_block = &curr_set->blocks[i];
        if (curr_block->valid == 1)
        {
            // 命中
            if (curr_block->tag == tag)
            {
                global_counter++;
                curr_block->time_stamp = global_counter;

                if (op->operation == 1 || op->operation == 2)
                {
                    cr->hits++;
                    return 0;
                }

                if (op->operation == 3)
                {
                    cr->hits += 2;
                    return 0;
                }
            }
        }
        else
        {
            is_set_full = 0;
            first_empty_block = i;
        }
    }
    // 未命中
    if (!is_set_full)
    {
        // 组内未满
        Block *empty_block = &curr_set->blocks[first_empty_block];
        empty_block->tag = tag;
        empty_block->valid = 1;
        global_counter++;
        empty_block->time_stamp = global_counter;

        if (op->operation == 1 || op->operation == 2)
        {
            cr->misses++;
            return 1;
        }

        if (op->operation == 3)
        {
            cr->misses++;
            cr->hits++;
            return 3;
        }
    }
    else
    {
        // 组内已满
        int lest_index = GetLestUse(curr_set, E);
        Block *replace_block = &curr_set->blocks[lest_index];
        replace_block->tag = tag;
        replace_block->valid = 1;
        global_counter++;
        replace_block->time_stamp = global_counter;

        if (op->operation == 1 || op->operation == 2)
        {
            cr->evictions++;
            cr->misses++;
            return 2;
        }

        if (op->operation == 3)
        {
            cr->evictions++;
            cr->misses++;
            cr->hits++;
            return 4;
        }
    }
    return 5;
}

int main(int argc, char *argv[])
{
    // 读参数
    int cache_args[3];
    char filename[MAX_FILENAME_LENGTH];
    int arg_res = ReadCacheArgument(argc, argv, cache_args, sizeof(cache_args) / sizeof(int), filename, sizeof(filename));
    if (arg_res != 0)
    {
        return 0;
    }

    if (help_flag)
    {
        printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\nOptions:\n  -h         Print this help message.\n  -v         Optional verbose flag.\n  -s <num>   Number of set index bits.\n  -E <num>   Number of lines per set.\n  -b <num>   Number of block offset bits.\n  -t <file>  Trace file.\n\nExamples:\n  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
        return 0;
    }

    InitCache(cache_args[0], cache_args[1], cache_args[2]);

    char line_buffer[64];
    CacheOperation cache_oper;
    CacheRecord record = {0};

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("打开文件失败\n");
        return 0;
    }

    char res_arr[5][20] = {
        "hit",
        "miss",
        "miss eviction",
        "miss hit",
        "miss evition hit"};

    while (ReadOneLine(fp, line_buffer, sizeof(line_buffer)) == 0)
    {
        ParseLine(line_buffer, &cache_oper);
        int cache_res = DoCache(&cache_oper, &record);
        if (cache_res == 5)
        {
            printf("Docache错误\n");
            return 0;
        }
        if (verbose_flag)
        {
            printf("%s %s\n", line_buffer + 1, res_arr[cache_res]);
        }
    }
    printSummary(record.hits, record.misses, record.evictions);

    FreeCache();
    fclose(fp);
    return 0;
}
