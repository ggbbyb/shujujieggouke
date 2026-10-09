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

// 候补队列：链式队列结构
typedef struct {
    OrderNode *front;   // 队头指针
    OrderNode *rear;    // 队尾指针
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
}

/**
 * 1. 新增候补订单：订单加入队列尾部（入队）
 */
void enQueue(WaitQueue *q, char *orderId, char *name, char *idCard) {
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
    printf(">> 候补订单提交成功，已进入排队序列\n");
}

/**
 * 2. 余票发放：输入余票数量k，从队头依次处理候补订单
 * 基础版默认每个订单对应1张车票
 */
void distributeTickets(WaitQueue *q, int k) {
    if (q->front == q->rear) {
        printf(">> 当前候补队列为空，无订单可处理\n");
        return;
    }

    int successCount = 0;
    // 余票充足且队列非空时，依次出票
    while (k > 0 && q->front != q->rear) {
        OrderNode *temp = q->front->next;

        // 输出候补成功的订单信息
        printf("\n===== 候补成功 =====\n");
        printf("订单号：%s\n", temp->orderId);
        printf("旅客姓名：%s\n", temp->name);
        printf("身份证号：%s\n", temp->idCard);
        printf("====================\n");

        // 队头订单出队
        q->front->next = temp->next;
        // 特殊处理：删除最后一个节点时，队尾指针回退到头节点
        if (temp == q->rear) {
            q->rear = q->front;
        }
        free(temp);
        k--;
        successCount++;
    }

    printf("\n>> 本次释放余票%d张，完成%d个候补订单\n", successCount, successCount);
    if (k > 0) {
        printf(">> 剩余%d张余票，但候补队列已全部处理完毕\n", k);
    }
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

    printf("====== 12306车票候补模拟系统（基础版）======\n");

    while (1) {
        printf("\n---------- 操作菜单 ----------\n");
        printf("1. 提交候补订单\n");
        printf("2. 释放余票并出票\n");
        printf("3. 查看候补队列状态\n");
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
