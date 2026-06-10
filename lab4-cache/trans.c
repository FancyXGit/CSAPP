/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void transpose_32(int M, int N, int A[N][M], int B[M][N]);
void transpose_64(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 64 && N == 64)
    {
        transpose_64(M, N, A, B);
    }
    else
    {
        transpose_32(M, N, A, B);
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */
#define block_size 8

// 32x32 288 misses
// 61x67 1997 misses
char transpose_32_desc[] = "Transpose 32";
void transpose_32(int M, int N, int A[N][M], int B[M][N])
{
    int block_count_col = M / block_size;
    int block_count_row = N / block_size;
    // the row i col j block of blocking A
    for (int i = 0; i < block_count_row; i++)
    {
        for (int j = 0; j < block_count_col; j++)
        {
            int start_row = i * block_size;
            int start_col = j * block_size;
            for (int h = 0; h < block_size; h++)
            {

                // 我认为这里仍然会发生缓存冲突
                // 只不过由从B[start_col + h][start_row + 0] = A[start_row + 0][start_col + h];的
                // 每一次交换元素都会冲突
                // 改成了每一次读完1列就会冲突
                // 冲突次数从64次变成了8次
                int a0 = A[start_row + 0][start_col + h];
                int a1 = A[start_row + 1][start_col + h];
                int a2 = A[start_row + 2][start_col + h];
                int a3 = A[start_row + 3][start_col + h];
                int a4 = A[start_row + 4][start_col + h];
                int a5 = A[start_row + 5][start_col + h];
                int a6 = A[start_row + 6][start_col + h];
                int a7 = A[start_row + 7][start_col + h];

                B[start_col + h][start_row + 0] = a0;
                B[start_col + h][start_row + 1] = a1;
                B[start_col + h][start_row + 2] = a2;
                B[start_col + h][start_row + 3] = a3;
                B[start_col + h][start_row + 4] = a4;
                B[start_col + h][start_row + 5] = a5;
                B[start_col + h][start_row + 6] = a6;
                B[start_col + h][start_row + 7] = a7;
            }
        }
    }
    for (int j = 0; j < M; j++)

    {
        for (int i = block_count_row * block_size; i < N; i++)
        {
            B[j][i] = A[i][j];
        }
    }
    for (int j = block_count_col * block_size; j < M; j++)
    {
        for (int i = 0; i < block_count_row * block_size; i++)
        {
            B[j][i] = A[i][j];
        }
    }
}

#undef block_size

#define block_size 8
#define half_block_size 4

// 64x64 1172 misses
// 61x67 2194 misses
char transpose_64_desc[] = "Transpose 64";
void transpose_64(int M, int N, int A[N][M], int B[M][N])
{
    int block_count_col = M / block_size;
    int block_count_row = N / block_size;
    // the row i col j block of blocking A
    for (int i = 0; i < block_count_row; i++)
    {
        for (int j = 0; j < block_count_col; j++)
        {
            int start_row = i * block_size;
            int start_col = j * block_size;
            /*
                A 的子块:             期望转置到 B 的位置:
                    +-----+-----+         +-----+-----+
                    |  1  |  2  |         |  1' |  3' |
                    +-----+-----+   =>    +-----+-----+
                    |  3  |  4  |         |  2' |  4' |
                    +-----+-----+         +-----+-----+

                */
            // A的1,2一起存在缓存中，3,4与1,2冲突。B同理，1，3与2，4冲突
            // 如果A,B不是同一位置，那么A的1，2映射缓存位置和B的1，3映射缓存位置是不一样的，可以同时存到缓存中
            // 1. 缓存A的块1，2，B的块1，3，将A1拷贝到B1，将A2拷贝到B3。此时缓存A1,A2,B1,B3
            // 2. 缓存A的块3，4，将B3读出存到临时变量，B3填入A3，此时缓存A3,A4,B1,B3
            // 3. 缓存B的块3，4，将B3的临时变量填入B2，A4填入B4，此时缓存A3,A4,B2,B4
            // 开启A的下一行，理论上一共4轮，每轮交替时会发生缓存冲突，较不分块的8x8效率提高
            // 使用临时变量避免了对角线上的矩阵（即AB同位置）时的冲突
            int a1, a2, a3, a4, a5, a6, a7, a8;
            // STEP 1:
            for (int h = 0; h < half_block_size; h++)
            {
                a1 = A[start_row + 0][start_col + h];
                a2 = A[start_row + 1][start_col + h];
                a3 = A[start_row + 2][start_col + h];
                a4 = A[start_row + 3][start_col + h];

                a5 = A[start_row + 0][start_col + h + 4];
                a6 = A[start_row + 1][start_col + h + 4];
                a7 = A[start_row + 2][start_col + h + 4];
                a8 = A[start_row + 3][start_col + h + 4];

                B[start_col + h][start_row + 0] = a1;
                B[start_col + h][start_row + 1] = a2;
                B[start_col + h][start_row + 2] = a3;
                B[start_col + h][start_row + 3] = a4;

                // 坐标交换之后，row - 4 , col + 4让本来转置到3的平移到2
                B[start_col + h][start_row + 4] = a5;
                B[start_col + h][start_row + 5] = a6;
                B[start_col + h][start_row + 6] = a7;
                B[start_col + h][start_row + 7] = a8;
            }

            // STEP 2:
            for (int h = 0; h < half_block_size; h++)
            {
                a1 = A[start_row + 0 + 4][start_col + h];
                a2 = A[start_row + 1 + 4][start_col + h];
                a3 = A[start_row + 2 + 4][start_col + h];
                a4 = A[start_row + 3 + 4][start_col + h];

                a5 = B[start_col + h][start_row + 0 + 4];
                a6 = B[start_col + h][start_row + 1 + 4];
                a7 = B[start_col + h][start_row + 2 + 4];
                a8 = B[start_col + h][start_row + 3 + 4];

                B[start_col + h][start_row + 0 + 4] = a1;
                B[start_col + h][start_row + 1 + 4] = a2;
                B[start_col + h][start_row + 2 + 4] = a3;
                B[start_col + h][start_row + 3 + 4] = a4;

                // STEP 3 PART
                B[start_col + h + 4][start_row + 0] = a5;
                B[start_col + h + 4][start_row + 1] = a6;
                B[start_col + h + 4][start_row + 2] = a7;
                B[start_col + h + 4][start_row + 3] = a8;
            }

            // STEP 3
            for (int h = 0; h < half_block_size; h++)
            {
                a1 = A[start_row + 0 + 4][start_col + h + 4];
                a2 = A[start_row + 1 + 4][start_col + h + 4];
                a3 = A[start_row + 2 + 4][start_col + h + 4];
                a4 = A[start_row + 3 + 4][start_col + h + 4];

                B[start_col + h + 4][start_row + 0 + 4] = a1;
                B[start_col + h + 4][start_row + 1 + 4] = a2;
                B[start_col + h + 4][start_row + 2 + 4] = a3;
                B[start_col + h + 4][start_row + 3 + 4] = a4;
            }
        }
    }

    for (int j = 0; j < M; j++)

    {
        for (int i = block_count_row * block_size; i < N; i++)
        {
            B[j][i] = A[i][j];
        }
    }
    for (int j = block_count_col * block_size; j < M; j++)
    {
        for (int i = 0; i < block_count_row * block_size; i++)
        {
            B[j][i] = A[i][j];
        }
    }
}

#undef block_size
#undef half_block_size

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);
    registerTransFunction(transpose_32, transpose_32_desc);
    registerTransFunction(transpose_64, transpose_64_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
