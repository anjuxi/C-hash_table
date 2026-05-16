#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// ========== 散列表（Hash Table）数据结构实现 ==========
// 采用链地址法（Separate Chaining）处理冲突
// 支持动态扩容，保持负载因子在合理范围

#define INITIAL_CAPACITY 16
#define LOAD_FACTOR_THRESHOLD 0.75
#define SHRINK_FACTOR_THRESHOLD 0.125

// 键值对节点
typedef struct HashNode {
    char* key;              // 键（动态字符串）
    int value;              // 值
    struct HashNode* next;  // 链表下一个节点
} HashNode;

// 散列表结构
typedef struct {
    HashNode** buckets;     // 桶数组（存储链表头指针）
    int size;               // 当前元素数量
    int capacity;           // 桶数组容量
} HashTable;

// ========== 辅助函数 ==========

// DJB2哈希算法 - 简单且分布均匀的字符串哈希
static unsigned long hashString(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// 计算键的索引
static int getIndex(HashTable* ht, const char* key) {
    return hashString(key) % ht->capacity;
}

// 创建新的节点
static HashNode* createNode(const char* key, int value) {
    HashNode* node = (HashNode*)malloc(sizeof(HashNode));
    if (!node) return NULL;
    
    node->key = (char*)malloc(strlen(key) + 1);
    if (!node->key) {
        free(node);
        return NULL;
    }
    
    strcpy(node->key, key);
    node->value = value;
    node->next = NULL;
    return node;
}

// 销毁节点及其后续链表
static void destroyNodeChain(HashNode* node) {
    while (node) {
        HashNode* temp = node;
        node = node->next;
        free(temp->key);
        free(temp);
    }
}

// ========== 散列表操作 ==========

// 创建散列表
HashTable* hashTableCreate(void) {
    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) return NULL;
    
    ht->buckets = (HashNode**)calloc(INITIAL_CAPACITY, sizeof(HashNode*));
    if (!ht->buckets) {
        free(ht);
        return NULL;
    }
    
    ht->size = 0;
    ht->capacity = INITIAL_CAPACITY;
    return ht;
}

// 销毁散列表
void hashTableDestroy(HashTable* ht) {
    if (!ht) return;
    
    for (int i = 0; i < ht->capacity; i++) {
        destroyNodeChain(ht->buckets[i]);
    }
    
    free(ht->buckets);
    free(ht);
}

// 获取当前元素数量
int hashTableSize(HashTable* ht) {
    return ht ? ht->size : 0;
}

// 判断是否为空
bool hashTableIsEmpty(HashTable* ht) {
    return !ht || ht->size == 0;
}

// 获取负载因子
static double getLoadFactor(HashTable* ht) {
    return (double)ht->size / ht->capacity;
}

// 重新哈希（扩容或缩容）
static bool hashTableResize(HashTable* ht, int newCapacity) {
    HashNode** oldBuckets = ht->buckets;
    int oldCapacity = ht->capacity;
    
    // 创建新的桶数组
    HashNode** newBuckets = (HashNode**)calloc(newCapacity, sizeof(HashNode*));
    if (!newBuckets) return false;
    
    ht->buckets = newBuckets;
    ht->capacity = newCapacity;
    
    // 重新插入所有元素
    for (int i = 0; i < oldCapacity; i++) {
        HashNode* node = oldBuckets[i];
        while (node) {
            HashNode* next = node->next;
            
            // 计算新索引并头插法插入
            int newIndex = getIndex(ht, node->key);
            node->next = ht->buckets[newIndex];
            ht->buckets[newIndex] = node;
            
            node = next;
        }
    }
    
    free(oldBuckets);
    return true;
}

// 插入键值对（若键已存在则更新值）
bool hashTablePut(HashTable* ht, const char* key, int value) {
    if (!ht || !key) return false;
    
    // 检查是否需要扩容
    if (getLoadFactor(ht) >= LOAD_FACTOR_THRESHOLD) {
        if (!hashTableResize(ht, ht->capacity * 2)) {
            return false;
        }
    }
    
    int index = getIndex(ht, key);
    HashNode* current = ht->buckets[index];
    
    // 检查是否已存在该键
    while (current) {
        if (strcmp(current->key, key) == 0) {
            current->value = value;  // 更新值
            return true;
        }
        current = current->next;
    }
    
    // 创建新节点并头插
    HashNode* newNode = createNode(key, value);
    if (!newNode) return false;
    
    newNode->next = ht->buckets[index];
    ht->buckets[index] = newNode;
    ht->size++;
    
    return true;
}

// 根据键获取值
bool hashTableGet(HashTable* ht, const char* key, int* outValue) {
    if (!ht || !key || !outValue) return false;
    
    int index = getIndex(ht, key);
    HashNode* current = ht->buckets[index];
    
    while (current) {
        if (strcmp(current->key, key) == 0) {
            *outValue = current->value;
            return true;
        }
        current = current->next;
    }
    
    return false;
}

// 检查键是否存在
bool hashTableContains(HashTable* ht, const char* key) {
    if (!ht || !key) return false;
    
    int index = getIndex(ht, key);
    HashNode* current = ht->buckets[index];
    
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return true;
        }
        current = current->next;
    }
    
    return false;
}

// 删除键值对
bool hashTableRemove(HashTable* ht, const char* key) {
    if (!ht || !key) return false;
    
    int index = getIndex(ht, key);
    HashNode* current = ht->buckets[index];
    HashNode* prev = NULL;
    
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                ht->buckets[index] = current->next;
            }
            
            free(current->key);
            free(current);
            ht->size--;
            
            // 检查是否需要缩容
            if (ht->capacity > INITIAL_CAPACITY && 
                getLoadFactor(ht) <= SHRINK_FACTOR_THRESHOLD) {
                hashTableResize(ht, ht->capacity / 2);
            }
            
            return true;
        }
        prev = current;
        current = current->next;
    }
    
    return false;
}

// 打印散列表状态
void hashTablePrint(HashTable* ht) {
    if (!ht) {
        printf("NULL HashTable\n");
        return;
    }
    
    printf("\n=== 散列表状态 ===\n");
    printf("元素数量: %d\n", ht->size);
    printf("桶数量: %d\n", ht->capacity);
    printf("负载因子: %.2f\n", getLoadFactor(ht));
    printf("-------------------\n");
    
    for (int i = 0; i < ht->capacity; i++) {
        printf("桶[%2d]: ", i);
        HashNode* current = ht->buckets[i];
        if (!current) {
            printf("NULL");
        }
        while (current) {
            printf("(%s=%d)", current->key, current->value);
            if (current->next) printf(" -> ");
            current = current->next;
        }
        printf("\n");
    }
    printf("===================\n\n");
}

// 获取所有键（调用者需释放返回的数组，但不需要释放键字符串）
char** hashTableKeys(HashTable* ht, int* outCount) {
    if (!ht || !outCount) return NULL;
    
    char** keys = (char**)malloc(sizeof(char*) * ht->size);
    if (!keys) return NULL;
    
    int count = 0;
    for (int i = 0; i < ht->capacity; i++) {
        HashNode* current = ht->buckets[i];
        while (current) {
            keys[count++] = current->key;
            current = current->next;
        }
    }
    
    *outCount = count;
    return keys;
}

// ========== 测试代码 ==========

int main() {
    printf("=== 散列表数据结构测试 ===\n\n");
    
    // 测试1：基本插入和查询
    printf("【基本操作测试】\n");
    HashTable* ht = hashTableCreate();
    
    hashTablePut(ht, "apple", 100);
    hashTablePut(ht, "banana", 200);
    hashTablePut(ht, "cherry", 300);
    hashTablePut(ht, "date", 400);
    hashTablePut(ht, "elderberry", 500);
    
    printf("插入5个键值对\n");
    hashTablePrint(ht);
    
    // 查询测试
    int value;
    if (hashTableGet(ht, "banana", &value)) {
        printf("查询 'banana': %d\n", value);
    }
    
    if (hashTableGet(ht, "apple", &value)) {
        printf("查询 'apple': %d\n", value);
    }
    
    // 测试2：更新值
    printf("\n【更新值测试】\n");
    hashTablePut(ht, "apple", 150);
    if (hashTableGet(ht, "apple", &value)) {
        printf("更新后 'apple': %d\n", value);
    }
    
    // 测试3：删除操作
    printf("\n【删除操作测试】\n");
    printf("删除 'banana'\n");
    hashTableRemove(ht, "banana");
    hashTablePrint(ht);
    
    printf("'banana' 是否存在: %s\n", 
           hashTableContains(ht, "banana") ? "是" : "否");
    
    // 测试4：扩容测试
    printf("【扩容测试】\n");
    printf("插入更多元素触发扩容...\n");
    hashTablePut(ht, "fig", 600);
    hashTablePut(ht, "grape", 700);
    hashTablePut(ht, "honeydew", 800);
    hashTablePut(ht, "kiwi", 900);
    hashTablePut(ht, "lemon", 1000);
    hashTablePut(ht, "mango", 1100);
    hashTablePut(ht, "nectarine", 1200);
    hashTablePut(ht, "orange", 1300);
    hashTablePut(ht, "papaya", 1400);
    hashTablePut(ht, "quince", 1500);
    
    hashTablePrint(ht);
    
    // 测试5：获取所有键
    printf("【遍历所有键】\n");
    int keyCount;
    char** keys = hashTableKeys(ht, &keyCount);
    printf("所有键 (%d个): ", keyCount);
    for (int i = 0; i < keyCount; i++) {
        printf("%s ", keys[i]);
    }
    printf("\n\n");
    free(keys);
    
    // 测试6：冲突处理测试
    printf("【冲突处理测试】\n");
    printf("插入可能导致冲突的键...\n");
    // 这些键在特定容量下可能产生冲突
    hashTablePut(ht, "abc", 1);
    hashTablePut(ht, "cba", 2);
    hashTablePut(ht, "bac", 3);
    
    hashTablePrint(ht);
    
    // 验证冲突键的值
    if (hashTableGet(ht, "abc", &value)) {
        printf("'abc' = %d\n", value);
    }
    if (hashTableGet(ht, "cba", &value)) {
        printf("'cba' = %d\n", value);
    }
    if (hashTableGet(ht, "bac", &value)) {
        printf("'bac' = %d\n", value);
    }
    
    // 测试7：空值查询
    printf("\n【查询不存在的键】\n");
    if (!hashTableGet(ht, "nonexistent", &value)) {
        printf("'nonexistent' 不存在（符合预期）\n");
    }
    
    hashTableDestroy(ht);
    
    printf("\n=== 所有测试完成 ===\n");
    return 0;
}
