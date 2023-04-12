#include "cache.h"
#include "log.h"
#include "thread.h"
#include <string.h>
#include <stdbool.h>

#ifdef _DEBUG
#include <assert.h>
#endif


/// @brief 字典树节点
typedef struct _TrieNode
{
	IPListNode* ip_list;
	struct _TrieNode *children[ALPHABET_SIZE];
} TrieNode, *PtrTrieNode;

typedef struct _Trie 
{
    PtrTrieNode root;
    int size;
} Trie;


static Trie _ip_trie;


static void* Malloc(unsigned long long size);

static TrieNode* getTrieNode();

static bool noChildren(PtrTrieNode pnode);

static void list_append(IPListNode** plist, UINT32 val);

static void list_free(IPListNode** plist);

static void insert(PtrTrieNode root, const char* key, UINT32 val, int len);

static PtrTrieNode search(PtrTrieNode root, const char* key, int len);

static PtrTrieNode delete(PtrTrieNode root, const char* key, int len);

static void cache_remove(const char* domain_name);

static void ttl_remove(PtrTrieNode root, IPListNode** ip_list_ptr, const char* domain_name);

/**
 * @brief 创建字典树
*/
void cache_init()
{
    int i;
    PtrTrieNode root;

    _ip_trie.root = (PtrTrieNode)Malloc(sizeof(TrieNode));
    _ip_trie.size = 0;

    root = _ip_trie.root;
    root->ip_list = NULL;

    for (i = 0; i < ALPHABET_SIZE; i++)
        root->children[i] = NULL;
}





/**
 * @brief 插入一个节点
 * @param domain_name 域名			  [key]
 * @param ip_addr	  ip地址的数值形式 [value]
*/
void cache_insert(const char* domain_name, UINT32 ip_addr)
{
    WaitForSingleObject(get_cache_mutex(), INFINITE);
    PtrTrieNode root = _ip_trie.root;
    insert(root, domain_name, ip_addr, 0);
    ReleaseMutex(get_cache_mutex());
}

/**
 * @brief 插入一个节点, 辅助函数
 * @param root 一个指向子树的根节点的指针
 * @param key  对应的键
 * @param val  对应的值
 * @param len  递归深度, 到树的根的路径长
*/
static void insert(PtrTrieNode root, const char* key, UINT32 val, int len)
{
    char c;
    int index;

    if (len == strlen(key)) {
        list_append(&root->ip_list, val);
        return;
    }

    c = key[len];
    index = GET_INDEX(c);

    if (!root->children[index])
        root->children[index] = getTrieNode();

    insert(root->children[index], key, val, len + 1);
}





/// @brief   创建一个初始化了的TrieNode
/// @return  一个初始化了的TrieNode
static TrieNode* getTrieNode()
{
    TrieNode* p = (TrieNode*)Malloc(sizeof(TrieNode));
    p->ip_list = NULL;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        p->children[i] = NULL;
    }

    return p;
}





/// @brief 在链表尾部添加一项, 检查是否重复
/// @param plist 一个指向list头结点的指针
/// @param val   添加的值(ip_addr)
static void list_append(IPListNode** plist, UINT32 val)
{
    bool repeat;
    IPListNode* curr, *prev;

    if (*plist == NULL) {
        *plist = (IPListNode*) Malloc(sizeof(IPListNode));
        (*plist)->ip_addr = val;
        (*plist)->next = NULL;
        (*plist)->start_time = time(NULL);
        (*plist)->time_to_live = 2 * SEC_PER_DAY;
        _ip_trie.size++;
    }
    else {
        repeat = false;
        curr = (*plist)->next;
        prev = *plist;

        while (curr != NULL && !repeat) {
            if (curr->ip_addr == val)   repeat = true;
            prev = curr;
            curr = curr->next;
        }

        if (curr == NULL && !repeat) {
            _ip_trie.size++;
            curr = (IPListNode*)Malloc(sizeof(IPListNode));

            curr->ip_addr = val;
            curr->next = NULL;
            curr->start_time = time(NULL);
            curr->time_to_live = 2 * SEC_PER_DAY;
            prev->next = curr;
        }
    }
}





/**
 * @brief 查找一个叶子节点
 * @param domain_name 域名  [key]
 * @param addr_list   返回包含IP信息的链表
*/
void cache_search(const char* domain_name, IPListNode** addr_list)
{
    WaitForSingleObject(get_cache_mutex(), INFINITE);
    TrieNode* pnode = search(_ip_trie.root, domain_name, 0);

    if (pnode == NULL)
        *addr_list = NULL;
    else {
        ttl_remove(pnode, &pnode->ip_list, domain_name);
        *addr_list = pnode->ip_list;
#ifdef _DEBUG
        assert(pnode->ip_list != NULL);
#endif
    }
    ReleaseMutex(get_cache_mutex());
}


#define GET_DIFFTIME(x) (difftime(time(NULL), (x)))

/**
 * @brief 根据ttl进行删除, 在搜索时检查是否要删除
 * @param root         一个指向TrieNode的指针
 * @param ip_list_ptr  一个指向链表的二级指针
 * @param domain_name  域名
*/
static void ttl_remove(PtrTrieNode root, IPListNode** ip_list_ptr, const char* domain_name)
{
    IPListNode* curr, *prev, *dummy;
    dummy = (IPListNode*)Malloc(sizeof(IPListNode));
    dummy->next = *ip_list_ptr;

    prev = dummy;
    curr = *ip_list_ptr;
    while (curr != NULL) {
        if (GET_DIFFTIME(curr->start_time) > curr->time_to_live) {
            prev->next = curr->next;
            free(curr);
            curr = prev->next;
        }
        else {
            prev = curr;
            curr = prev->next;
        }
    }
    *ip_list_ptr = dummy->next;
    free(dummy);
}



/**
 * @brief 搜索, 辅助函数
 * @param root 子树的根节点
 * @param key  查找的键
 * @param len  递归的深度
 * @return NULL 如果没找到; 指向键对应的TrieNode, 如果找到
*/
static PtrTrieNode search(TrieNode* root, const char*key, int len)
{
    char c;
    int index;
    if (root == NULL)       return NULL;
    if (len == strlen(key)) return root;

    c = key[len];
    index = GET_INDEX(c);
    return search(root->children[index], key, len + 1);
}




/**
 * ! 不提供外部接口
 * @brief 删除一个节点
 * @param domain_name 要删除的域名信息
*/
static void cache_remove(const char* domain_name) 
{
    _ip_trie.root = delete(_ip_trie.root, domain_name, 0);
}





static PtrTrieNode delete(PtrTrieNode root, const char* key, int len)
{
    char c;
    int index;

    if (root == NULL)   return NULL;
    if (len  == strlen(key)) {
        if (root->ip_list) {
            list_free(&root->ip_list);
            _ip_trie.size--;
        }
        
        if (noChildren(root)) {
            free(root);
            root = NULL;
        }

        return root;
    }

    c = key[len];
    index = GET_INDEX(c);

    // 左值和函数第一个参数相同, 函数对参数的修改通过返回值赋给了自身
    // 这样的做法避免了复杂的二级指针操作
    root->children[index] = delete(root->children[index], key, len + 1);

    if (noChildren(root) && root->ip_list == NULL) {
        free(root);
        root = NULL;
    }

    return root;
}


/**
 * @brief 释放链表
 * @param plist 指向链表的指针
*/
static void list_free(IPListNode** plist)
{
    IPListNode* pnode;

    while (*plist != NULL) {
        pnode = *plist;
        *plist = pnode->next;
        free(pnode);
    }
}




/**
 * @brief  返回缓存的大小
 * @return 缓存的大小    
*/
int cache_size() {
    WaitForSingleObject(get_cache_mutex(), INFINITE);
    int ret;
    ret = _ip_trie.size;
    ReleaseMutex(get_cache_mutex());
    return ret;
}


/**
 * @brief 判断节点是否有孩子, 即是否有域名以该节点到根的路径字符串为前缀
 * @param pnode 一个指向 TrieNode 的指针
 * @return 有孩子返回 false, 没孩子返回 true                           
*/
static bool noChildren(PtrTrieNode pnode)
{
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (pnode->children[i])     return false;
    return true;
}



/**
 * @brief malloc 辅助函数
 * @param size 申请的空间大小
 * @return 申请到的内存空间的地址
*/
static void* Malloc(unsigned long long size)
{
    void* p = malloc(size);
    if (p == NULL) {
        log_error_message("cache.c: Malloc()");
    }
    return p;
}