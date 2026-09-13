/*
 * ============================================================================
 * 五岔路口交通灯管理（C 语言实现）—— 只计算最少需要几种灯
 * ----------------------------------------------------------------------------
 * 问题：
 *   五岔路口，C、E 为单行道，共 13 条可通行路线。有的路线不能同时放行
 *   （如 E->B 与 A->D），有的可以同时放行（如 A->B 与 E->C）。
 *   问：最少需要设置几种交通灯（即把路线分几批放行）？每种灯放行哪些路线？
 *
 * 数学模型 = 图着色：
 *   顶点 = 一条路线；边 = 两条路线冲突（不能同时放行）；
 *   颜色 = 一种灯（一个相位），同一种灯（同色）下的路线可同时放行；
 *   最少颜色数 = 最少需要的灯种数。
 *
 * 冲突判定规则（依据路口几何）：
 *   a) 出口相同        -> 合流冲突（如 A->C 与 B->C）
 *   b) 对向车流        -> 中心对撞（如 A->B 与 B->A）
 *   c) 行驶弧线交叉    -> 交叉冲突（题目例子：E->B 与 A->D）
 *   d) 同入口分叉/接力 -> 不冲突（题目例子：A->B 与 E->C）
 *
 * 算法：
 *   1) 构建 13x13 冲突矩阵；
 *   2) 贪心图着色（Welsh-Powell：按冲突度降序分配最小可用颜色）求一个可行上界；
 *   3) 迭代加深回溯验证并求出"最少灯种数"，输出每种灯放行的路线。
 *
 * 编译运行：
 *   gcc -O2 -Wall traffic_light.c -o traffic_light
 *   ./traffic_light
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define N_DIR     5      /* 路口方向数 */
#define N_ROUTE   13     /* 可通行路线数 */
#define MAX_LIGHT 8      /* 灯种数上限 */

/* ---- 方向定义：按题目示意图顺时针排列（角度取整便于计算） ---- */
enum { C = 0, D = 1, E = 2, A = 3, B = 4 };

static const char *DIR_NAME[N_DIR] = { "C", "D", "E", "A", "B" };
static const int   DIR_ANG[N_DIR]  = { 0, 72, 144, 216, 288 };

/* ---- 路线结构 ---- */
typedef struct {
    int  in, out;         /* 入口方向、出口方向 */
    char name[8];         /* 路线名，如 "A->B" */
} Route;

static Route routes[N_ROUTE];

static int  color[N_ROUTE];                 /* 每条路线所属灯种(1..k) */
static bool conflict[N_ROUTE][N_ROUTE];     /* 冲突矩阵 */
static int  deg[N_ROUTE];                   /* 每条路线的冲突度数 */
static int  order[N_ROUTE];                 /* 着色顺序（按度数降序） */

/* --------------------------------------------------------------------------
 * 1. 路线定义
 * --------------------------------------------------------------------------
 * C 为"入口单行"（只能从 C 进入，不能从 C 离开），
 * E 为"出口单行"（只能从 E 离开，不能进入 E），
 * 故共 3+3+3+4 = 13 条可通行路线。
 */
static void init_routes(void)
{
    static const int in[N_ROUTE]  = { A,A,A, B,B,B, D,D,D, E,E,E,E };
    static const int out[N_ROUTE] = { B,C,D, A,C,D, A,B,C, A,B,C,D };

    for (int i = 0; i < N_ROUTE; i++) {
        routes[i].in  = in[i];
        routes[i].out = out[i];
        snprintf(routes[i].name, sizeof(routes[i].name), "%s->%s",
                 DIR_NAME[in[i]], DIR_NAME[out[i]]);
    }
}

/* --------------------------------------------------------------------------
 * 2. 冲突判定
 * -------------------------------------------------------------------------- */
/* 点 p 是否位于顺时针弧 [s,e] 的严格内部（不含两端点） */
static bool on_arc(int s, int e, int p)
{
    if (s == e) return false;
    if (s < e)  return (p > s && p < e);   /* 不跨 0 度 */
    return (p > s || p < e);               /* 跨 0 度，如 [288,72] */
}

/* 两条弧是否共享端点（同入口分叉 / 接力车流） */
static bool share_endpoint(int s1, int e1, int s2, int e2)
{
    return (s1 == s2 || e1 == e2 || s1 == e2 || e1 == s2);
}

/* 两条路线能否同时放行？返回 true 表示冲突（不能同时放行） */
static bool is_conflict(const Route *r1, const Route *r2)
{
    if (r1 == r2) return false;

    /* a) 出口相同：在出口处合流，冲突 */
    if (r1->out == r2->out) return true;

    /* b) 对向车流：如 A->B 与 B->A，在路口中心对撞，冲突 */
    if (r1->in == r2->out && r1->out == r2->in) return true;

    int s1 = DIR_ANG[r1->in], e1 = DIR_ANG[r1->out];
    int s2 = DIR_ANG[r2->in], e2 = DIR_ANG[r2->out];

    /* c) 共享端点（同入口分叉、接力车流）：不交叉、不冲突 */
    if (share_endpoint(s1, e1, s2, e2)) return false;

    /* d) 圆弧端点交错：两条路径在路口内部交叉，冲突
     *    （E->B 与 A->D 冲突、A->B 与 E->C 不冲突，题目例子均由本规则得到） */
    return (on_arc(s1, e1, s2) != on_arc(s1, e1, e2));
}

/* --------------------------------------------------------------------------
 * 3. 构建冲突矩阵，并按冲突度降序确定着色顺序（Welsh-Powell）
 * -------------------------------------------------------------------------- */
static void build_conflict_matrix(void)
{
    for (int i = 0; i < N_ROUTE; i++) {
        deg[i] = 0;
        for (int j = 0; j < N_ROUTE; j++) {
            conflict[i][j] = is_conflict(&routes[i], &routes[j]);
            if (conflict[i][j]) deg[i]++;
        }
    }
    for (int i = 0; i < N_ROUTE; i++) order[i] = i;
    for (int i = 0; i < N_ROUTE; i++)
        for (int j = i + 1; j < N_ROUTE; j++)
            if (deg[order[j]] > deg[order[i]]) {
                int t = order[i]; order[i] = order[j]; order[j] = t;
            }
}

/* --------------------------------------------------------------------------
 * 4. 图着色：求最少灯种数
 * -------------------------------------------------------------------------- */
/* 贪心着色：为每个顶点分配"不与已着色且冲突顶点同色"的最小颜色，返回颜色数 */
static int greedy_coloring(void)
{
    int maxc = 0;
    memset(color, 0, sizeof(color));

    for (int k = 0; k < N_ROUTE; k++) {
        int  v = order[k];
        bool used[MAX_LIGHT + 1] = { false };

        for (int i = 0; i < k; i++) {
            int u = order[i];
            if (conflict[v][u] && color[u] > 0) used[color[u]] = true;
        }
        int c = 1;
        while (c <= MAX_LIGHT && used[c]) c++;
        color[v] = c;
        if (c > maxc) maxc = c;
    }
    return maxc;
}

/* 回溯（DFS）：能否用 maxc 种颜色给 order[idx..] 全部着色（迭代加深用） */
static bool try_color(int idx, int maxc)
{
    if (idx == N_ROUTE) return true;

    int v = order[idx];
    for (int c = 1; c <= maxc; c++) {
        bool ok = true;
        for (int i = 0; i < idx; i++) {
            int u = order[i];
            if (conflict[v][u] && color[u] == c) { ok = false; break; }
        }
        if (ok) {
            color[v] = c;
            if (try_color(idx + 1, maxc)) return true;
        }
    }
    return false;
}

/* 迭代加深：从 1 开始逐级尝试，求"最少灯种数"并生成一组最优划分 */
static int minimal_lights(int upper)
{
    for (int k = 1; k < upper; k++) {
        memset(color, 0, sizeof(color));
        if (try_color(0, k)) return k;
    }
    greedy_coloring();   /* 兜底：贪心方案本身就是可行解 */
    return upper;
}

/* --------------------------------------------------------------------------
 * 5. 输出
 * -------------------------------------------------------------------------- */
static void print_header(void)
{
    printf("====================================================================\n");
    printf(" 五岔路口交通灯管理 —— 最少灯种计算\n");
    printf(" 方向布置(顺时针): C(0°) D(72°) E(144°) A(216°) B(288°)\n");
    printf(" 数学模型: 图着色（顶点=路线，边=冲突，颜色=灯种）\n");
    printf("====================================================================\n");

    printf("\n----- 可通行路线（13 条） -----\n");
    for (int i = 0; i < N_ROUTE; i++)
        printf("  %2d. %s  (入口 %s 出口 %s)\n", i, routes[i].name,
               DIR_NAME[routes[i].in], DIR_NAME[routes[i].out]);
}

static void print_conflict_matrix(void)
{
    printf("\n----- 冲突矩阵（1=冲突, . =可同时放行） -----\n");
    printf("      ");
    for (int j = 0; j < N_ROUTE; j++)
        printf("%-5s", routes[j].name);
    printf("\n");
    for (int i = 0; i < N_ROUTE; i++) {
        printf("%-5s ", routes[i].name);
        for (int j = 0; j < N_ROUTE; j++)
            printf("%-5s", conflict[i][j] ? "1" : ".");
        printf("  deg=%d\n", deg[i]);
    }
}

/* 输出每种灯放行的路线，并自检灯内两两无冲突 */
static void print_lights(int light_count)
{
    printf("\n----- 最少需要 %d 种灯，每种灯放行的路线 -----\n", light_count);
    for (int k = 1; k <= light_count; k++) {
        printf("第 %d 种灯: ", k);
        bool first = true;
        for (int i = 0; i < N_ROUTE; i++) {
            if (color[i] == k) {
                printf("%s%s", first ? "" : ", ", routes[i].name);
                first = false;
            }
        }
        printf("\n");
    }

    bool ok = true;
    for (int i = 0; i < N_ROUTE && ok; i++)
        for (int j = i + 1; j < N_ROUTE; j++)
            if (color[i] == color[j] && conflict[i][j]) {
                printf("[自检] 第 %d 种灯内 %s 与 %s 冲突！\n",
                       color[i], routes[i].name, routes[j].name);
                ok = false;
            }
    printf("[自检] 每种灯内路线两两可同时放行：%s\n", ok ? "通过" : "失败");
}

/* -------------------------------------------------------------------------- */
int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   /* Windows 控制台使用 UTF-8 输出 */
#endif

    init_routes();
    build_conflict_matrix();

    print_header();
    print_conflict_matrix();

    int greedy = greedy_coloring();   /* 贪心上界 */
    int best   = minimal_lights(greedy);   /* 迭代加深求最少灯种数 */

    printf("\n[结论] 贪心方案需要 %d 种灯；经迭代加深回溯验证，"
           "最少需要 %d 种灯。\n", greedy, best);

    print_lights(best);

    return 0;
}
