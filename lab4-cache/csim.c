#include "cachelab.h"
#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_FILENAME_LENGTH 64

// 缓存块结构
typedef struct
{
    int tag;
    int valid;
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
            cache_arg[0] = atoi(optarg);
            break;
        case 'E':
            cache_arg[1] = atoi(optarg);
            break;
        case 'b':
            cache_arg[2] = atoi(optarg);
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
        return 1;
    }
    cache->S = S;
    cache->E = E;
    cache->B = B;
    // 第2层：set
    cache->sets = (Set *)malloc(S * sizeof(Set));
    for (int i = 0; i < S; i++)
    {
        // 第3层：block
        cache->sets[i].blocks = (Block *)malloc(E * sizeof(Block));
        if (cache->sets[i].blocks == NULL)
        {
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

// 

int main(int argc, char *argv[])
{
    // 读参数
    int cache_args[3];
    char filename[MAX_FILENAME_LENGTH];
    int arg_res = ReadCacheArgument(argc, argv, cache_args, sizeof(cache_args), filename, sizeof(filename));
    if (arg_res != 0)
    {
        return 0;
    }
    

    // printSummary(0, 0, 0);
    return 0;
}
