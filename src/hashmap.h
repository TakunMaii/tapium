#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ==================== 类型定义 ====================

// 哈希表节点
typedef struct HashNode {
    char* key;              // 键 (字符串)
    void* value;            // 值 (Token指针 - 由外部定义)
    struct HashNode* next;  // 链表解决冲突
} HashNode;

// 哈希表结构
typedef struct TokenHashMap {
    HashNode** buckets;     // 桶数组
    size_t capacity;        // 容量
    size_t size;            // 当前元素数量
    float load_factor;      // 负载因子阈值
} TokenHashMap;

// ==================== 函数定义 ====================

/**
 * 创建哈希表
 * @param initial_capacity 初始容量
 * @return 哈希表指针，失败返回NULL
 */
static inline TokenHashMap* token_hashmap_create(size_t initial_capacity) {
    TokenHashMap* map = (TokenHashMap*)malloc(sizeof(TokenHashMap));
    if (!map) return NULL;
    
    map->capacity = initial_capacity > 0 ? initial_capacity : 16;
    map->size = 0;
    map->load_factor = 0.75f;
    
    map->buckets = (HashNode**)calloc(map->capacity, sizeof(HashNode*));
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    
    return map;
}

/**
 * 销毁哈希表
 * @param map 哈希表指针
 */
static inline void token_hashmap_destroy(TokenHashMap* map) {
    if (!map) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        HashNode* node = map->buckets[i];
        while (node) {
            HashNode* temp = node;
            node = node->next;
            
            free(temp->key);
            free(temp);  // 注意：value由调用者管理
        }
    }
    
    free(map->buckets);
    free(map);
}

/**
 * 计算字符串哈希值 (DJB2算法)
 * @param str 输入字符串
 * @return 哈希值
 */
static inline unsigned long token_hashmap_hash(const char* str) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    
    return hash;
}

/**
 * 内部重哈希函数
 */
static inline void token_hashmap_rehash(TokenHashMap* map) {
    size_t old_capacity = map->capacity;
    HashNode** old_buckets = map->buckets;
    
    map->capacity *= 2;
    map->buckets = (HashNode**)calloc(map->capacity, sizeof(HashNode*));
    map->size = 0;
    
    // 重新插入所有元素
    for (size_t i = 0; i < old_capacity; i++) {
        HashNode* node = old_buckets[i];
        while (node) {
            HashNode* next = node->next;
            
            // 重新计算位置并插入
            unsigned long h = token_hashmap_hash(node->key) % map->capacity;
            node->next = map->buckets[h];
            map->buckets[h] = node;
            map->size++;
            
            node = next;
        }
    }
    
    free(old_buckets);
}

/**
 * 插入键值对
 * @param map 哈希表指针
 * @param key 键字符串
 * @param value Token指针
 * @return true成功, false失败
 * @note 如果key已存在，会覆盖旧值但不释放旧的value
 */
static inline bool token_hashmap_put(TokenHashMap* map, const char* key, void* value) {
    if (!map || !key) return false;
    
    // 检查是否需要扩容
    if ((float)map->size / map->capacity >= map->load_factor) {
        token_hashmap_rehash(map);
    }
    
    unsigned long h = token_hashmap_hash(key) % map->capacity;
    
    // 检查是否已存在
    HashNode* node = map->buckets[h];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            // 更新已存在的键
            node->value = value;
            return true;
        }
        node = node->next;
    }
    
    // 创建新节点
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    if (!new_node) return false;
    
    new_node->key = strdup(key);
    if (!new_node->key) {
        free(new_node);
        return false;
    }
    
    new_node->value = value;
    new_node->next = map->buckets[h];
    map->buckets[h] = new_node;
    map->size++;
    
    return true;
}

/**
 * 获取值
 * @param map 哈希表指针
 * @param key 键字符串
 * @return Token指针, 不存在返回NULL
 */
static inline void* token_hashmap_get(TokenHashMap* map, const char* key) {
    if (!map || !key) return NULL;
    
    unsigned long h = token_hashmap_hash(key) % map->capacity;
    
    HashNode* node = map->buckets[h];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }
    
    printf("undefined macro : %s", key);
    exit(1);
    return NULL;
}

/**
 * 检查键是否存在
 * @param map 哈希表指针
 * @param key 键字符串
 * @return true存在, false不存在
 */
static inline bool token_hashmap_contains(TokenHashMap* map, const char* key) {
    return token_hashmap_get(map, key) != NULL;
}

/**
 * 删除键值对
 * @param map 哈希表指针
 * @param key 键字符串
 * @return 被删除的Token指针, 不存在返回NULL
 * @note 返回的value需要调用者手动释放
 */
static inline void* token_hashmap_remove(TokenHashMap* map, const char* key) {
    if (!map || !key) return NULL;
    
    unsigned long h = token_hashmap_hash(key) % map->capacity;
    
    HashNode* prev = NULL;
    HashNode* node = map->buckets[h];
    
    while (node) {
        if (strcmp(node->key, key) == 0) {
            if (prev) {
                prev->next = node->next;
            } else {
                map->buckets[h] = node->next;
            }
            
            void* value = node->value;
            free(node->key);
            free(node);
            map->size--;
            
            return value;
        }
        
        prev = node;
        node = node->next;
    }
    
    return NULL;
}

/**
 * 获取哈希表大小
 * @param map 哈希表指针
 * @return 元素数量
 */
static inline size_t token_hashmap_size(TokenHashMap* map) {
    return map ? map->size : 0;
}

/**
 * 清空哈希表
 * @param map 哈希表指针
 * @note 只清空节点，不释放value
 */
static inline void token_hashmap_clear(TokenHashMap* map) {
    if (!map) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        HashNode* node = map->buckets[i];
        while (node) {
            HashNode* temp = node;
            node = node->next;
            
            free(temp->key);
            free(temp);
        }
        map->buckets[i] = NULL;
    }
    map->size = 0;
}

/**
 * 打印哈希表统计信息 (调试用)
 * @param map 哈希表指针
 */
static inline void token_hashmap_print_stats(TokenHashMap* map) {
    if (!map) return;
    
    printf("=== TokenHashMap Stats ===\n");
    printf("Capacity: %zu\n", map->capacity);
    printf("Size: %zu\n", map->size);
    printf("Load Factor: %.2f\n", (float)map->size / map->capacity);
    
    // 统计链表长度分布
    int empty_buckets = 0;
    int max_chain = 0;
    int total_chain = 0;
    
    for (size_t i = 0; i < map->capacity; i++) {
        int chain_len = 0;
        HashNode* node = map->buckets[i];
        while (node) {
            chain_len++;
            node = node->next;
        }
        
        if (chain_len == 0) empty_buckets++;
        if (chain_len > max_chain) max_chain = chain_len;
        total_chain += chain_len;
    }
    
    printf("Empty Buckets: %d (%.1f%%)\n", 
           empty_buckets, 100.0f * empty_buckets / map->capacity);
    printf("Max Chain Length: %d\n", max_chain);
    printf("Avg Chain Length: %.2f\n", 
           map->size > 0 ? (float)total_chain / (map->capacity - empty_buckets) : 0);
    printf("==========================\n");
}

#endif
