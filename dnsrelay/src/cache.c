/**
* 实现cache
*/

#include "cache.h"
#include "log.h"
#include "thread.h"
#include "common.h"


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
static char _idx_table[256] = {0};

static inline bool validate_domain_name(const char* domain_name);
static inline void idx_table_init();
static inline int get_idx(char chr);
static inline TrieNode* get_trie_node();
static inline bool no_children(PtrTrieNode pnode);
static inline void list_append(IPListNode** plist, uint32_t val);
static inline void list_free(IPListNode** plist);
static inline void insert(PtrTrieNode root, const char* key, uint32_t val, int len);
static inline PtrTrieNode search(TrieNode* root, const char* key, int len);
static inline PtrTrieNode delete(PtrTrieNode root, const char* key, int len);
static inline void cache_remove(const char* domain_name);
static inline void ttl_remove(PtrTrieNode root, IPListNode** ip_list_ptr, const char* domain_name);



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

    idx_table_init();
}


/**
 * @brief 插入一个节点
 * @param domain_name 域名			  [key]
 * @param ip_addr	  ip地址的数值形式 [value]
*/
void cache_insert(const char* domain_name, uint32_t ip_addr)
{
    if (validate_domain_name(domain_name)) {
        WaitForSingleObject(get_cache_mutex(), INFINITE);
        PtrTrieNode root = _ip_trie.root;
        insert(root, domain_name, ip_addr, 0);
        ReleaseMutex(get_cache_mutex());
    }
}


/**
 * @brief 查找一个叶子节点
 * @param domain_name 域名  [key]
 * @param addr_list   返回包含IP信息的链表
*/
void cache_search(const char* domain_name, IPListNode** addr_list)
{
    if (validate_domain_name(domain_name)) {
        WaitForSingleObject(get_cache_mutex(), INFINITE);

        TrieNode* pnode = search(_ip_trie.root, domain_name, 0);

        if (pnode == NULL)
            *addr_list = NULL;
        else {
            ttl_remove(pnode, &pnode->ip_list, domain_name);
            *addr_list = pnode->ip_list;
//#ifdef _DEBUG
//            assert(pnode->ip_list != NULL);
//#endif
        }
        ReleaseMutex(get_cache_mutex());
    }
}


/**
 * @brief  返回缓存所占的内存大小
 * @return 缓存所占的内存大小   
*/
int cache_memory_size() {
    WaitForSingleObject(get_cache_mutex(), INFINITE);
    int ret;
    ret = _ip_trie.size * sizeof(TrieNode);
    ReleaseMutex(get_cache_mutex());
    return ret;
}

/**
* ! 比较少见的域名不存入Cache
* @brief 判断域名是否包含特殊字符, 其中英文字母、阿拉伯数字以及'.'、'-'、 '_'以外的字符一律视为非法字符
* @param domain_name 域名
* @return 合法返回true, 否则返回false
*/
static inline bool validate_domain_name(const char* domain_name)
{
    char chr;
    const char* ptr = domain_name;
    bool flag = true;

    while (*ptr != 0) {
        chr = *ptr++;
        if (is_digit(chr) || is_letter(chr) ||
            chr == '.' || chr == '-' || chr == '_')
            continue;

        // 非法字符出现
        flag = false;
        break;
    }
    return flag;
}


/**
* @brief 初始化idx_table
*/
static inline void idx_table_init()
{
    for (unsigned i = 0; i < 256; i++) {
        if (i >= (unsigned char)'a' && i <= (unsigned char)'z') {
            _idx_table[i] = 0 + i - (unsigned char)'a';
        }
        else if (i >= (unsigned char)'A' && i <= (unsigned char)'Z') {
            _idx_table[i] = 26 + i - (unsigned char)'A';
        }
        else if (i >= (unsigned char)'0' && i <= (unsigned char)'9') {
            _idx_table[i] = 52 + i - (unsigned char)'0';
        }
        else if (i == (unsigned char)'.') {
            _idx_table[i] = 62;
        }
        else if (i == (unsigned char)'-') {
            _idx_table[i] = 63;
        }
        else if (i == (unsigned char)'_') {
            _idx_table[i] = 64;
        }
        else {
            _idx_table[i] = 65;
        }
    }
}

/**
* @brief  根据字符chr从_idx_table中获取索引
* @param  chr ascii字符
* @return chr在_idx_table中对应的索引
*/
static inline int get_idx(char chr)
{
    return _idx_table[(unsigned char)chr];
}

/**
 * @brief 插入一个节点, 辅助函数
 * @param root 一个指向子树的根节点的指针
 * @param key  对应的键
 * @param val  对应的值
 * @param len  递归深度, 到树的根的路径长
*/
static inline void insert(PtrTrieNode root, const char* key, uint32_t val, int len)
{
    char c;
    int index;

    if (len == strlen(key)) {
        list_append(&root->ip_list, val);
        return;
    }

    c = key[len];
    index = get_idx(c);

    if (!root->children[index])
        root->children[index] = get_trie_node();

    insert(root->children[index], key, val, len + 1);
}

/// @brief   创建一个初始化了的TrieNode
/// @return  一个初始化了的TrieNode
static inline TrieNode* get_trie_node()
{
    TrieNode* p = (TrieNode*)Malloc(sizeof(TrieNode));
    p->ip_list = NULL;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        p->children[i] = NULL;
    }

    return p;
}

/**
* @brief 在链表尾部添加一项, 检查是否重复
* @param plist 一个指向list头结点的指针
* @param val   添加的值(ip_addr)
*/
static inline void list_append(IPListNode** plist, uint32_t val)
{
    bool repeat;
    IPListNode* curr, * prev;

    if (*plist == NULL) {
        *plist = (IPListNode*)Malloc(sizeof(IPListNode));
        (*plist)->ip_addr = val;
        (*plist)->next = NULL;
        (*plist)->start_time = time(NULL);
        (*plist)->time_to_live = (val == 0x0)? INFINITE_TTL: TTL;
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
            curr->time_to_live = (val == 0x0) ? INFINITE_TTL : TTL;
            prev->next = curr;
        }
    }
}

#define GET_DIFFTIME(x) (difftime(time(NULL), (x)))

/**
 * @brief 根据ttl进行删除, 在搜索时检查是否要删除
 * @param root         一个指向TrieNode的指针
 * @param ip_list_ptr  一个指向链表的二级指针
 * @param domain_name  域名
*/
static inline void ttl_remove(PtrTrieNode root, IPListNode** ip_list_ptr, const char* domain_name)
{
    IPListNode* curr, * prev, * dummy;
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
static inline PtrTrieNode search(TrieNode* root, const char* key, int len)
{
    char c;
    int index;
    if (root == NULL)       return NULL;
    if (len == strlen(key)) return root;

    c = key[len];
    index = get_idx(c);
    return search(root->children[index], key, len + 1);
}

/**
 * ! 不提供外部接口
 * @brief 删除一个节点
 * @param domain_name 要删除的域名信息
*/
static inline void cache_remove(const char* domain_name)
{
    _ip_trie.root = delete(_ip_trie.root, domain_name, 0);
}

static inline PtrTrieNode delete(PtrTrieNode root, const char* key, int len)
{
    char c;
    int index;

    if (root == NULL)   return NULL;
    if (len == strlen(key)) {
        if (root->ip_list) {
            list_free(&root->ip_list);
            _ip_trie.size--;
        }

        if (no_children(root)) {
            free(root);
            root = NULL;
        }

        return root;
    }

    c = key[len];
    index = get_idx(c);

    // 左值和函数第一个参数相同, 函数对参数的修改通过返回值赋给了自身
    // 这样的做法避免了复杂的二级指针操作
    root->children[index] = delete(root->children[index], key, len + 1);

    if (no_children(root) && root->ip_list == NULL) {
        free(root);
        root = NULL;
    }

    return root;
}



/**
 * @brief 判断节点是否有孩子, 即是否有域名以该节点到根的路径字符串为前缀
 * @param pnode 一个指向 TrieNode 的指针
 * @return 有孩子返回 false, 没孩子返回 true
*/
static inline bool no_children(PtrTrieNode pnode)
{
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (pnode->children[i])     return false;
    return true;
}

/**
 * @brief 释放链表
 * @param plist 指向链表的指针
*/
static inline void list_free(IPListNode** plist)
{
    IPListNode* pnode;

    while (*plist != NULL) {
        pnode = *plist;
        *plist = pnode->next;
        free(pnode);
    }
}
