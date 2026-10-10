#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 候补订单节点：存储单条候补订单信息
typedef struct Order {
    char orderId[32];   // 订单号
    char name[20];      // 旅客姓名
    char idCard[20];    // 身份证号
    struct Order *next;
} OrderNode;

// 候补队列：链式队列结构 + 全局余票池
typedef struct {
    OrderNode *front;         // 队头指针
    OrderNode *rear;          // 队尾指针
    int remainingTickets;     // 全局余票池：累计留存未使用的余票
} WaitQueue;

/**
 * 初始化队列（带头节点，简化边界操作）
 */
void initQueue(WaitQueue *q) {
    q->front = q->rear = (OrderNode*)malloc(sizeof(OrderNode));
    if (!q->front) {
        printf("内存分配失败！\n");
        exit(1);
    }
    q->front->next = NULL;
    q->remainingTickets = 0; // 初始状态无余票
}

/**
 * 1. 新增候补订单
 * 优先检查余票池：有票直接出票，无票才加入队尾排队
 */
void enQueue(WaitQueue *q, char *orderId, char *name, char *idCard) {
    // 余票池有剩余：直接候补成功，无需排队
    if (q->remainingTickets > 0) {
        q->remainingTickets--;
        printf("\n===== 候补成功（余票充足，直接出票）=====\n");
        printf("订单号：%s\n", orderId);
        printf("旅客姓名：%s\n", name);
        printf("身份证号：%s\n", idCard);
        printf("余票池剩余：%d张\n", q->remainingTickets);
        printf("========================================\n");
        return;
    }

    // 无可用余票：创建节点，加入队列尾部排队
    OrderNode *newNode = (OrderNode*)malloc(sizeof(OrderNode));
    if (!newNode) {
        printf("内存不足，添加订单失败！\n");
        return;
    }
    strcpy(newNode->orderId, orderId);
    strcpy(newNode->name, name);
    strcpy(newNode->idCard, idCard);
    newNode->next = NULL;

    q->rear->next = newNode;
    q->rear = newNode;
    printf(">> 当前无可用余票，候补订单已进入排队序列\n");
}

/**
 * 2. 余票发放
 * 本次释放的票先加入全局余票池，再从队头依次按顺序出票
 * 没用完的余票继续留在池中，供后续新增订单使用
 */
void distributeTickets(WaitQueue *q, int k) {
    // 本次释放的余票累加入全局余票池
    q->remainingTickets += k;
    printf("\n>> 本次新增余票%d张，当前余票池总计：%d张\n", k, q->remainingTickets);

    if (q->front == q->rear) {
        printf(">> 当前候补队列为空，余票已留存，等待新候补订单\n");
        return;
    }

    int successCount = 0;
    // 余票充足且队列非空时，从队头依次出票
    while (q->remainingTickets > 0 && q->front != q->rear) {
        OrderNode *temp = q->front->next;

        // 输出候补成功的订单信息
        printf("\n===== 候补成功 =====\n");
        printf("订单号：%s\n", temp->orderId);
        printf("旅客姓名：%s\n", temp->name);
        printf("身份证号：%s\n", temp->idCard);
        printf("====================\n");

        // 队头订单出队
        q->front->next = temp->next;
        // 边界处理：删除最后一个节点时，队尾指针回退到头节点
        if (temp == q->rear) {
            q->rear = q->front;
        }
        free(temp);

        q->remainingTickets--; // 余票池扣减1张
        successCount++;
    }

    printf("\n>> 本次完成%d个候补订单，余票池剩余：%d张\n", successCount, q->remainingTickets);
    if (q->front != q->rear) {
        printf(">> 候补队列仍有订单，继续等待下一批余票\n");
    }
}

/**
 * 3. 判断候补队列是否为空
 */
int isQueueEmpty(WaitQueue *q) {
    return q->front == q->rear;
}

/**
 * 销毁队列，释放所有内存（退出程序时调用）
 */
void destroyQueue(WaitQueue *q) {
    while (q->front) {
        q->rear = q->front->next;
        free(q->front);
        q->front = q->rear;
    }
}

int main() {
    WaitQueue queue;
    initQueue(&queue);
    int option;
    char orderId[32], name[20], idCard[20];
    int ticketCount;

    printf("====== 12306车票候补模拟系统======\n");

    while (1) {
        printf("\n---------- 操作菜单 ----------\n");
        printf("1. 提交候补订单\n");
        printf("2. 释放余票并出票\n");
        printf("3. 查看系统状态\n");
        printf("4. 退出系统\n");
        printf("请输入选项编号：");
        scanf("%d", &option);

        switch (option) {
            case 1:
                printf("\n请输入订单号：");
                scanf("%s", orderId);
                printf("请输入旅客姓名：");
                scanf("%s", name);
                printf("请输入身份证号：");
                scanf("%s", idCard);
                enQueue(&queue, orderId, name, idCard);
                break;

            case 2:
                printf("\n请输入本次释放的余票数量：");
                scanf("%d", &ticketCount);
                if (ticketCount <= 0) {
                    printf(">> 余票数量必须大于0！\n");
                    break;
                }
                distributeTickets(&queue, ticketCount);
                break;

            case 3:
                printf("\n>> 余票池剩余票数：%d张\n", queue.remainingTickets);
                if (isQueueEmpty(&queue)) {
                    printf(">> 当前候补队列为空\n");
                } else {
                    printf(">> 候补队列非空，仍有旅客在排队候补\n");
                }
                break;

            case 4:
                printf("\n正在退出系统，清理资源...\n");
                destroyQueue(&queue);
                printf("系统已退出，感谢使用\n");
                return 0;

            default:
                printf(">> 输入无效，请输入1-4的有效选项\n");
        }
    }
    return 0;
}
