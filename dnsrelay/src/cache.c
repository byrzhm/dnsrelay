/**
* ʵ��cache
*/

#include "cache.h"
#include "log.h"
#include "thread.h"
#include "common.h"


#ifdef _DEBUG
#include <assert.h>
#endif


/// @brief �ֵ����ڵ�
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
 * @brief �����ֵ���
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
 * @brief ����һ���ڵ�
 * @param domain_name ����			  [key]
 * @param ip_addr	  ip��ַ����ֵ��ʽ [value]
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
 * @brief ����һ��Ҷ�ӽڵ�
 * @param domain_name ����  [key]
 * @param addr_list   ���ذ���IP��Ϣ������
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
 * @brief  ���ػ�����ռ���ڴ��С
 * @return ������ռ���ڴ��С   
*/
int cache_memory_size() {
    WaitForSingleObject(get_cache_mutex(), INFINITE);
    int ret;
    ret = _ip_trie.size * sizeof(TrieNode);
    ReleaseMutex(get_cache_mutex());
    return ret;
}

/**
* ! �Ƚ��ټ�������������Cache
* @brief �ж������Ƿ���������ַ�, ����Ӣ����ĸ�������������Լ�'.'��'-'�� '_'������ַ�һ����Ϊ�Ƿ��ַ�
* @param domain_name ����
* @return �Ϸ�����true, ���򷵻�false
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

        // �Ƿ��ַ�����
        flag = false;
        break;
    }
    return flag;
}


/**
* @brief ��ʼ��idx_table
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
* @brief  �����ַ�chr��_idx_table�л�ȡ����
* @param  chr ascii�ַ�
* @return chr��_idx_table�ж�Ӧ������
*/
static inline int get_idx(char chr)
{
    return _idx_table[(unsigned char)chr];
}

/**
 * @brief ����һ���ڵ�, ��������
 * @param root һ��ָ�������ĸ��ڵ��ָ��
 * @param key  ��Ӧ�ļ�
 * @param val  ��Ӧ��ֵ
 * @param len  �ݹ����, �����ĸ���·����
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

/// @brief   ����һ����ʼ���˵�TrieNode
/// @return  һ����ʼ���˵�TrieNode
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
* @brief ������β�����һ��, ����Ƿ��ظ�
* @param plist һ��ָ��listͷ����ָ��
* @param val   ��ӵ�ֵ(ip_addr)
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
 * @brief ����ttl����ɾ��, ������ʱ����Ƿ�Ҫɾ��
 * @param root         һ��ָ��TrieNode��ָ��
 * @param ip_list_ptr  һ��ָ������Ķ���ָ��
 * @param domain_name  ����
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
 * @brief ����, ��������
 * @param root �����ĸ��ڵ�
 * @param key  ���ҵļ�
 * @param len  �ݹ�����
 * @return NULL ���û�ҵ�; ָ�����Ӧ��TrieNode, ����ҵ�
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
 * ! ���ṩ�ⲿ�ӿ�
 * @brief ɾ��һ���ڵ�
 * @param domain_name Ҫɾ����������Ϣ
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

    // ��ֵ�ͺ�����һ��������ͬ, �����Բ������޸�ͨ������ֵ����������
    // ���������������˸��ӵĶ���ָ�����
    root->children[index] = delete(root->children[index], key, len + 1);

    if (no_children(root) && root->ip_list == NULL) {
        free(root);
        root = NULL;
    }

    return root;
}



/**
 * @brief �жϽڵ��Ƿ��к���, ���Ƿ��������Ըýڵ㵽����·���ַ���Ϊǰ׺
 * @param pnode һ��ָ�� TrieNode ��ָ��
 * @return �к��ӷ��� false, û���ӷ��� true
*/
static inline bool no_children(PtrTrieNode pnode)
{
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (pnode->children[i])     return false;
    return true;
}

/**
 * @brief �ͷ�����
 * @param plist ָ�������ָ��
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
