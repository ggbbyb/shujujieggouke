/*
 * 五岔路口交通灯管理 - 图着色问题
 *
 * 问题描述：
 *  一个五岔路口，C 和 E 为单行道，共有 13 条可通行路线。
 *  用顶点表示一条通行路线，不能同时通行的线路顶点用边连接。
 *  每个顶点染一种颜色，相邻顶点颜色不同，求最少颜色数（即最少灯组）。
 *
 * 13 条路线：AB, AC, AD, BA, BC, BD, DA, DB, DC, EA, EB, EC, ED
 * 其中 BA, DC, ED 为右转路线（不受灯控，孤立顶点）。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 13          /* 顶点数：13 条通行路线 */
#define MAX_COLOR 10  /* 最大颜色数（上限） */

/* 13 条通行路线名称（顶点） */
char *routes[N] = {
    "AB", "AC", "AD", "BA", "BC", "BD",
    "DA", "DB", "DC", "EA", "EB", "EC", "ED"
};

/* 邻接矩阵：graph[i][j] = 1 表示路线 i 和路线 j 不能同时通行（有边） */
int graph[N][N] = {0};

/* 每个顶点分配到的颜色编号（从 1 开始；0 表示未着色） */
int color[N];

/*
 * 构建冲突图（邻接矩阵）
 *
 * 冲突关系推导：
 *  - BA(B→A)、DC(D→C)、ED(E→D) 是右转路线，与其他任何路线不冲突（孤立点）
 *  - 从 A 出发的三条路线 AB、AC、AD 互相不冲突（同入口分流）
 *  - 从 B 出发的 BC、BD 互相不冲突
 *  - 从 D 出发的 DA、DB 互相不冲突
 *  - 从 E 出发的 EB、EC 互相不冲突
 *  - EA(E→A) 与 A 方向出发的三条路线 AB、AC、AD 冲突
 *  - A 方向出发的 AB、AC、AD 与 B、D、E 方向出发的非右转路线均冲突
 *  - B 方向出发的 BC、BD 与 D、E 方向出发的非右转路线均冲突
 *  - D 方向出发的 DA、DB 与 E 方向出发的 EB、EC 冲突
 */
void buildGraph(void)
{
    int i, j;
    /* 先清零 */
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            graph[i][j] = 0;

    /* 顶点索引：
     * 0=AB  1=AC  2=AD  3=BA  4=BC  5=BD
     * 6=DA  7=DB  8=DC  9=EA  10=EB 11=EC 12=ED
     */

    /* --- EA(9) 与 A 方向出发的 AB(0)、AC(1)、AD(2) 冲突 --- */
    graph[9][0] = graph[0][9] = 1;
    graph[9][1] = graph[1][9] = 1;
    graph[9][2] = graph[2][9] = 1;

    /* --- A 方向出发的 AB、AC、AD 与 B 方向出发的 BC、BD 冲突 --- */
    graph[0][4] = graph[4][0] = 1;
    graph[0][5] = graph[5][0] = 1;
    graph[1][4] = graph[4][1] = 1;
    graph[1][5] = graph[5][1] = 1;
    graph[2][4] = graph[4][2] = 1;
    graph[2][5] = graph[5][2] = 1;

    /* --- A 方向出发的 AB、AC、AD 与 D 方向出发的 DA、DB 冲突 --- */
    graph[0][6] = graph[6][0] = 1;
    graph[0][7] = graph[7][0] = 1;
    graph[1][6] = graph[6][1] = 1;
    graph[1][7] = graph[7][1] = 1;
    graph[2][6] = graph[6][2] = 1;
    graph[2][7] = graph[7][2] = 1;

    /* --- A 方向出发的 AB、AC、AD 与 E 方向出发的 EB、EC 冲突 --- */
    graph[0][10] = graph[10][0] = 1;
    graph[0][11] = graph[11][0] = 1;
    graph[1][10] = graph[10][1] = 1;
    graph[1][11] = graph[11][1] = 1;
    graph[2][10] = graph[10][2] = 1;
    graph[2][11] = graph[11][2] = 1;

    /* --- B 方向出发的 BC、BD 与 D 方向出发的 DA、DB 冲突 --- */
    graph[4][6] = graph[6][4] = 1;
    graph[4][7] = graph[7][4] = 1;
    graph[5][6] = graph[6][5] = 1;
    graph[5][7] = graph[7][5] = 1;

    /* --- B 方向出发的 BC、BD 与 E 方向出发的 EB、EC 冲突 --- */
    graph[4][10] = graph[10][4] = 1;
    graph[4][11] = graph[11][4] = 1;
    graph[5][10] = graph[10][5] = 1;
    graph[5][11] = graph[11][5] = 1;

    /* --- D 方向出发的 DA、DB 与 E 方向出发的 EB、EC 冲突 --- */
    graph[6][10] = graph[10][6] = 1;
    graph[6][11] = graph[11][6] = 1;
    graph[7][10] = graph[10][7] = 1;
    graph[7][11] = graph[11][7] = 1;

    /* 注意：BA(3)、DC(8)、ED(12) 是右转路线，
     * 与所有其他顶点都没有边（孤立点），不受灯控。
     */
}

/*
 * 贪心图着色算法
 * 按顶点顺序依次着色：给每个顶点分配与其相邻顶点不同的最小颜色编号。
 * 颜色从 1 开始编号。
 */
void greedyColoring(void)
{
    int i, j, cr;
    int used[MAX_COLOR + 1];

    /* 所有顶点初始未着色（0 表示未着色） */
    for (i = 0; i < N; i++)
        color[i] = 0;

    /* 按顺序处理每个顶点 */
    for (i = 0; i < N; i++) {
        /* 记录相邻顶点已使用的颜色 */
        memset(used, 0, sizeof(used));
        for (j = 0; j < N; j++) {
            if (graph[i][j] && color[j] != 0) {
                used[color[j]] = 1;
            }
        }
        /* 找到第一个未被相邻顶点使用的颜色 */
        for (cr = 1; cr <= MAX_COLOR; cr++) {
            if (!used[cr]) {
                color[i] = cr;
                break;
            }
        }
    }
}

/* 判断顶点是否为右转路线（孤立点，不受灯控） */
int isRightTurn(int v)
{
    /* BA=3, DC=8, ED=12 */
    return (v == 3 || v == 8 || v == 12);
}

int main(void)
{
    int i, c, maxColor = 0;

    buildGraph();
    greedyColoring();

    /* 统计最大颜色编号 */
    for (i = 0; i < N; i++) {
        if (color[i] > maxColor)
            maxColor = color[i];
    }

    printf("=============================================\n");
    printf("  五岔路口交通灯管理 — 图着色求解结果\n");
    printf("=============================================\n\n");

    /* 输出右转路线（不受灯控） */
    printf("【右转路线（不受灯控限制，可随时通行）】\n  ");
    for (i = 0; i < N; i++) {
        if (isRightTurn(i)) {
            printf("%s  ", routes[i]);
        }
    }
    printf("\n\n");

    /* 按颜色分组输出 */
    printf("【按灯组（颜色）分组结果】\n");
    for (c = 1; c <= maxColor; c++) {
        int first = 1;
        printf("  颜色 %d（灯组 %d）：", c, c);
        for (i = 0; i < N; i++) {
            if (color[i] == c && !isRightTurn(i)) {
                if (!first) printf("、");
                printf("%s", routes[i]);
                first = 0;
            }
        }
        printf("\n");
    }

    printf("\n---------------------------------------------\n");
    printf("结论：最少需要 %d 种颜色（即 %d 个灯组）即可控制该路口。\n",
           maxColor, maxColor);
    printf("（另有 3 条右转路线不受灯控限制。）\n");
    printf("=============================================\n");

    return 0;
}
