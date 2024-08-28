/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-10 18:04:45
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-06-24 02:19:29
 * @FilePath: \CrystalAlgorithms\src\crstructure.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <AlgDefs.h>
#include <string.h>

#ifdef CR_WINDOWS
#include <Windows.h>
#elif defined CR_LINUX
#include <pthread.h>
void InitializeCriticalSection(pthread_mutex_t* mt)
{
	pthread_mutex_init(mt, NULL);
}
void DeleteCriticalSection(pthread_mutex_t* mt)
{
	pthread_mutex_destroy(mt);
}
void EnterCriticalSection(pthread_mutex_t* mt)
{
	pthread_mutex_lock(mt);
}
void LeaveCriticalSection(pthread_mutex_t* mt)
{
	pthread_mutex_unlock(mt);
}
#endif

#define DYN 0x00
#define TRE 0x01
#define LIN 0x02
#define QUA 0x03

#define DYN_MAX (sizeof(CRUINT8) << 29)
#define DYN_MAX_PTR ((sizeof(CRUINT8) << 29) / sizeof(CRLVOID))

void** CRCoreFunList;

typedef struct crstructurepub
{
    #ifdef CR_WINDOWS
    CRITICAL_SECTION cs;  //确保多线程安全
    #elif defined CR_LINUX
    pthread_mutex_t cs;  //确保多线程安全
    #endif
    CRUINT8 type;  //用于表明类型，防止错误操作
    CRUINT64 total;  //用于指明当前元素数量
}CRSTRUCTUREPUB;

typedef struct dyn
{
    CRSTRUCTUREPUB pub;
    union
	{
		CRUINT64* p64;
		CRUINT32* p32;
		CRUINT16* p16;
		CRUINT8* p8;
		CRUINT8* arr;
	};
    CRUINT32 capacity;
}CRDYN, * PCRDYN;

//

typedef struct treenode
{
	CRUINT64 key;
	CRLVOID data;
	CRBOOL red;
	struct treenode* parent;
	struct treenode* left;
	struct treenode* right;
}TREENODE, *PTREENODE;
typedef struct tree
{
	CRSTRUCTUREPUB pub;
	PTREENODE root;
}CRTREE, *PCRTREE;

//

typedef struct linearnode
{
	CRLVOID data;
	CRUINT64 key;  //用来支持排序的
	struct linearnode* prior;
	struct linearnode* after;
}LINEARNODE, * PLINEARNODE;

typedef struct
{
	CRSTRUCTUREPUB pub;
	PLINEARNODE hook;
}CRLINEAR, * PCRLINEAR;

//

typedef struct quadtreenode
{
	CRPOINTU point;
	struct quadtreenode* LT;  //left top
	struct quadtreenode* RT;  //right top
	struct quadtreenode* LB;  //left bottom
	struct quadtreenode* RB;  //right bottom
	struct quadtreenode* parent;  //parent
	CRRECTU range;
	//存储每个元素的信息
	CRSTRUCTURE items;  //此数组用于存储键值以及覆盖范围结构体的堆指针
}QUADTREENODE, *PQUADTREENODE;

//一个具体，准确的空间实体
typedef struct quaditems
{
	CRLVOID data;
	CRRECTU cover;
	PQUADTREENODE hook;
}CRQUADITEMS, *PCRQUADITEMS;

typedef struct
{
	CRSTRUCTUREPUB pub;
	PQUADTREENODE root;
	CRUINT8 maxItem;
	CRSTRUCTURE keyTree;  //要想实现快速删除，就需要快速查找，通过ID找到范围进而找到对应的结点
}CRQUADINNER, *PCRQUADINNER;

//

//如果要调用Core的函数，需要先获取函数指针，初始化
CRAPI CRBOOL CRModInit(void** ptr)
{
	if (ptr[0] == ptr[2])  //只有core没初始化才会出现这种情况
		return CRFALSE;
    CRCoreFunList = ptr;
    return CRTRUE;
}

CRAPI void CRModUninit(void)
{}

CRAPI CRSTRUCTURE CRDynamic(CRUINT64 size)
{
    CRUINT64 capacity = 8;
    size += 8 - size % 8;
    if (capacity > DYN_MAX) capacity = DYN_MAX;
    while (capacity < size) capacity <<= 2;
    PCRDYN pInner = CRAlloc(NULL, sizeof(CRDYN));
    if (!pInner)
    {
        CR_LOG_ERR("auto", "pInner is NULL");
        return NULL;
    }
    pInner->arr = CRAlloc(NULL, capacity);
    if (!pInner->arr)
    {
        CRAlloc(pInner, 0);
        CR_LOG_ERR("auto", "pInner->arr is NULL");
        return NULL;
    }
    pInner->pub.type = DYN;
    pInner->pub.total = 0;
    InitializeCriticalSection(&(pInner->pub.cs));
    pInner->arr[0] = 0;
    pInner->capacity = capacity;
    return pInner;
}

CRAPI CRSTRUCTURE CRTree(void)
{
	PCRTREE pInner = CRAlloc(NULL, sizeof(CRTREE));
	if (!pInner)
		return NULL;
	pInner->pub.type = TRE;
	pInner->pub.total = 0;
	InitializeCriticalSection(&(pInner->pub.cs));
	pInner->root = NULL;
	return pInner;
}

CRAPI CRSTRUCTURE CRLinear(void)
{
	PCRLINEAR pInner = CRAlloc(NULL, sizeof(CRLINEAR));
	if (!pInner)
		return NULL;
	pInner->pub.type = LIN;
	pInner->pub.total = 0;
	InitializeCriticalSection(&(pInner->pub.cs));
	pInner->hook = NULL;
	return pInner;
}

//创建一个空的四叉树结点
static PQUADTREENODE _inner_create_node_(CRRECTU range, PQUADTREENODE parent)
{
	PQUADTREENODE node = CRAlloc(NULL, sizeof(QUADTREENODE));
	if (!node)
		return NULL;
	node->LT = NULL;
	node->LB = NULL;
	node->RT = NULL;
	node->RB = NULL;
	node->parent = parent;
	node->items = CRDynamic(0);
	//
	node->range.left = range.left;
	node->range.top = range.top;
	node->range.right = range.right;
	node->range.bottom = range.bottom;
	return node;
}
CRAPI CRSTRUCTURE CRQuadtree(CRUINT64 w, CRUINT64 h, CRUINT8 max)
{
	if (max == 0)
		max = 4;
	PCRQUADINNER pInner = CRAlloc(NULL, sizeof(CRQUADINNER));
	if (pInner)
	{
		pInner->pub.total = 0;
		pInner->pub.type = QUA;
		pInner->maxItem = max;
		InitializeCriticalSection(&(pInner->pub.cs));
		CRRECTU range;
		range.left = -w;
		range.right = w * 2;
		range.top = -h;
		range.bottom = h * 2;
		pInner->root = _inner_create_node_(range, NULL);
		pInner->keyTree = CRTree();
		if (!pInner->root)
		{
			DeleteCriticalSection(&pInner->pub.cs);
			CRAlloc(pInner, 0);
			return NULL;
		}
		return pInner;
	}
	return NULL;
}

CRAPI CRUINT64 CRStructureSize(CRSTRUCTURE s)
{
	if (s)
		return ((CRSTRUCTUREPUB*)s)->total;
	return 0;
}

CRAPI CRBOOL CRFreeStructure(CRSTRUCTURE s, DSCallback cal);

//
//
//动态数组实现
//

static CRUINT8* _inner_dyn_sizeup_(CRUINT8* arr, CRUINT64 size, CRUINT64 capacity)
{
    CRUINT8* tmp = CRAlloc(NULL, capacity);
    if (!tmp) return NULL;
    for (int i = 0; i < capacity; i++) tmp[i] = 0;
    memcpy(tmp, arr, size);
    CRAlloc(arr, 0);
    return tmp;
}

static CRUINT8* _inner_dyn_sizedown_(CRUINT8* arr, CRUINT64 capacity)
{
    return CRAlloc(arr, capacity);
}

static CRBOOL _inner_dyn_push_(CRSTRUCTURE dyn, void* data, CRDynEnum mode)
{
	PCRDYN pInner = dyn;
	//需要扩容的情况，不得超过容量限制
	if (pInner->pub.total >= pInner->capacity)
	{
		if (pInner->capacity >= DYN_MAX)
			return CRFALSE;
		pInner->capacity <<= 1;
		CRUINT8* tmp = _inner_dyn_sizeup_(pInner->arr, pInner->pub.total, pInner->capacity);
		if (!tmp)
			return CRFALSE;
		pInner->arr = tmp;
	}
	else  //扩容之后回归正常流程
	{
		switch (mode)
		{
		case DYN_MODE_8:
			pInner->p8[pInner->pub.total++] = *(CRUINT8*)data;
			break;
		case DYN_MODE_16:
			pInner->pub.total >>= 1;
			pInner->pub.total <<= 1;
			pInner->p16[pInner->pub.total] = *(CRUINT16*)data;
			pInner->pub.total += 2;
			break;
		case DYN_MODE_32:
			pInner->pub.total >>= 2;
			pInner->pub.total <<= 2;
			pInner->p32[pInner->pub.total] = *(CRUINT32*)data;
			pInner->pub.total += 4;
			break;
		case DYN_MODE_64:
			pInner->pub.total >>= 3;
			pInner->pub.total <<= 3;
			pInner->p64[pInner->pub.total] = *(CRUINT64*)data;
			pInner->pub.total += 8;
			break;
		default:
			break;
		}
	}
    return CRTRUE;
}

static CRBOOL _inner_dyn_pop_(CRSTRUCTURE dyn, void* data, CRENUM mode)
{
    PCRDYN pInner = dyn;
	if (pInner->pub.total == 0)
		return CRFALSE;
	//尚有元素可取的情况
	switch (mode)
	{
	case DYN_MODE_8:
		pInner->pub.total--;
		if (data) *(CRUINT8*)data = pInner->p8[pInner->pub.total];
		break;
	case DYN_MODE_16:
		if (pInner->pub.total % 2)
			pInner->pub.total = (pInner->pub.total >> 1) << 1;
		else pInner->pub.total -= 2;
		if (data) *(CRUINT16*)data = pInner->p16[pInner->pub.total >> 1];
		break;
	case DYN_MODE_32:
		if (pInner->pub.total % 4)
			pInner->pub.total = (pInner->pub.total >> 2) << 2;
		else pInner->pub.total -= 4;
		if (data) *(CRUINT16*)data = pInner->p32[pInner->pub.total >> 2];
		break;
	case DYN_MODE_64:
		if (pInner->pub.total % 8)
			pInner->pub.total = (pInner->pub.total >> 3) << 3;
		else pInner->pub.total -= 8;
		if (data) *(CRUINT16*)data = pInner->p64[pInner->pub.total >> 3];
		break;
	default:
		break;
	}
	if (pInner->pub.total < pInner->capacity >> 1 && pInner->capacity > 32)//可以释放一些空间
	{
		pInner->capacity >>= 1;
		pInner->arr = _inner_dyn_sizedown_(pInner->arr, pInner->capacity);
	}
	return CRTRUE;
}

CRAPI CRBOOL CRDynPush(CRSTRUCTURE dyn, void* data, CRDynEnum mode)
{
    PCRDYN pInner = dyn;
    if (!pInner || pInner->pub.type != DYN)
    {
        CR_LOG_WAR("auto", "invalid structure");
        return CRFALSE;
    }
    CRBOOL back;
    EnterCriticalSection(&(pInner->pub.cs));
    if (back = _inner_dyn_push_(dyn, data, mode)) CR_LOG_ERR("auto", "dyn_push failed");
    LeaveCriticalSection(&(pInner->pub.cs));
    return back;
}

CRAPI CRBOOL CRDynPop(CRSTRUCTURE dyn, void* data, CRDynEnum mode)
{
    PCRDYN pInner = dyn;
    if (!pInner || pInner->pub.type != DYN)
    {
        CR_LOG_WAR("auto", "invalid structure");
        return CRFALSE;
    }
    CRBOOL back;
    EnterCriticalSection(&(pInner->pub.cs));
    back = _inner_dyn_pop_(dyn, data, mode);
    LeaveCriticalSection(&(pInner->pub.cs));
    return back;
}

CRAPI CRBOOL CRDynSet(CRSTRUCTURE dyn, void* data, CRUINT64 sub, CRDynEnum mode)
{
	PCRDYN pInner = dyn;
	if (!pInner || pInner->pub.type != DYN)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	switch (mode)
	{
	case DYN_MODE_8:
		if (sub < pInner->pub.total)
			pInner->p8[sub] = *(CRUINT8*)data;
		else goto Push;
		LeaveCriticalSection(&(pInner->pub.cs));
		return 0;
	case DYN_MODE_16:
		if (sub * 2 < pInner->pub.total)
			pInner->p16[sub] = *(CRUINT16*)data;
		else goto Push;
		LeaveCriticalSection(&(pInner->pub.cs));
		return 0;
	case DYN_MODE_32:
		if (sub * 4 < pInner->pub.total)
			pInner->p32[sub] = *(CRUINT32*)data;
		else goto Push;
		LeaveCriticalSection(&(pInner->pub.cs));
		return 0;
	case DYN_MODE_64:
		if (sub * 8 < pInner->pub.total)
			pInner->p64[sub] = *(CRUINT64*)data;
		else goto Push;
		LeaveCriticalSection(&(pInner->pub.cs));
		return 0;
	default:
		break;
	}
Push:
	//如果超过容量了，就忽略下标，在末尾处插入扩容
	CRBOOL back = _inner_dyn_push_(dyn, data, mode);
	LeaveCriticalSection(&(pInner->pub.cs));
	return back;
}

CRAPI CRBOOL CRDynSeek(CRSTRUCTURE dyn, void* data, CRUINT64 sub, CRDynEnum mode)
{
	PCRDYN pInner = dyn;
	if (pInner && pInner->pub.type == DYN)
	{
		if (!data)
		{
			CR_LOG_WAR("auto", "invalid structure");
			return CRFALSE;
		}
		EnterCriticalSection(&(pInner->pub.cs));
		switch (mode)
		{
		case DYN_MODE_8:
			if (sub < pInner->pub.total)
				*(CRUINT8*)data = pInner->p8[sub];
			else
				*(CRUINT8*)data = 0;
			break;
		case DYN_MODE_16:
			if (sub * 2 < pInner->pub.total)
				*(CRUINT16*)data = pInner->p16[sub];
			else
				*(CRUINT16*)data = 0;
			break;
		case DYN_MODE_32:
			if (sub * 4 < pInner->pub.total)
				*(CRUINT32*)data = pInner->p32[sub];
			else
				*(CRUINT32*)data = 0;
			break;
		case DYN_MODE_64:
			if (sub * 8 < pInner->pub.total)
				*(CRUINT64*)data = pInner->p64[sub];
			else
				*(CRUINT64*)data = 0;
			break;
		default:
			break;
		}
		LeaveCriticalSection(&(pInner->pub.cs));
		return 0;
	}
	return CRTRUE;
}

CRAPI CRBOOL CRDynSetup(CRSTRUCTURE dyn, void *buffer, CRUINT64 size)
{
	PCRDYN pInner = dyn;
	if (!pInner || pInner->pub.type != DYN)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	if (size > DYN_MAX)
	{
		CR_LOG_ERR("auto", "capacity reached upper limit");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	if (!buffer)
	{
		pInner->capacity = 8;
		pInner->pub.total = 0;
		pInner->arr = CRAlloc(NULL, 8);
		goto Done;
	}
	pInner->capacity = 1;
	while (pInner->capacity < size)
		pInner->capacity <<= 1;
	CRUINT8 *tmp = CRAlloc(NULL, pInner->capacity);
	for (int i = 0; i < pInner->capacity; i++) tmp[i] = 0;
	if (!tmp)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_ERR("auto", "bad alloc");
		return CRFALSE;
	}
	memcpy(tmp, buffer, size);
	CRAlloc(pInner->arr, 0);
	pInner->arr = tmp;
	pInner->pub.total = size;
Done:
	LeaveCriticalSection(&(pInner->pub.cs));
	return 0;
}

CRAPI void* CRDynCopy(CRSTRUCTURE dyn, CRUINT32* size)
{
	PCRDYN pInner = dyn;
	if (!pInner || pInner->pub.type != DYN)
		return NULL;
	EnterCriticalSection(&(pInner->pub.cs));
	if (!pInner->pub.total)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		return NULL;
	}
	CRUINT8* out = CRAlloc(NULL, pInner->pub.total * sizeof(CRUINT8));
	if (!out)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		return NULL;
	}
	memcpy(out, pInner->arr, pInner->pub.total * sizeof(CRUINT8));
	if (size) *size = pInner->pub.total;
	LeaveCriticalSection(&(pInner->pub.cs));
	return out;
}


CRAPI CRUINT64 CRDynGetBits(CRSTRUCTURE dyn, CRUINT64 offset, CRUINT8 len)
{
	if (len > 63)  //从0开始
		len = 63;
	PCRDYN pInner = dyn;
	if (!pInner || pInner->pub.type != DYN)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return 0;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	CRUINT64 endpos = (offset + len) >> 3;
	endpos += (offset + len) % 8 > 0 ? 1 : 0;
	if (endpos >= pInner->pub.total)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		return 0;
	}
	CRUINT64 back = 0;
	CRUINT32 size = pInner->pub.total;
	CRUINT8* buffer = pInner->arr; //寻址到需要进行读取的那个字节
	CRUINT8 offs = offset % 8;  //偏移量
	buffer += offset >> 3;
	for (; len > 0; len--)
	{
		back |= (((*buffer & CR_BIT_MASK_1 >> offs) >> (7 - offs)) << (len - 1));
		offs++;
		if (offs > 7)
		{
			offs = 0;
			buffer++;
		}
	}
	LeaveCriticalSection(&(pInner->pub.cs));
	return back;
}

CRAPI CRBOOL CRDynSetBits(CRSTRUCTURE dyn, CRUINT64 offset, CRUINT8 len, CRUINT64 bits)
{
	if (len > 63)  //从0开始
		len = 63;
	PCRDYN pInner = dyn;
	if (!pInner || pInner->pub.type != DYN)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	CRUINT64 endpos = (offset + len) >> 3;
	endpos += (offset + len) % 8 > 0 ? 1 : 0;
	if (endpos >= DYN_MAX)
	{
		CR_LOG_ERR("auto", "capacity reached upper limit");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	if (endpos >= pInner->capacity && endpos < DYN_MAX)
	{
		while (pInner->capacity <= endpos) pInner->capacity <<= 1;
		CRUINT8* tmp = _inner_dyn_sizeup_(pInner->arr, pInner->pub.total, pInner->capacity);
		if (!tmp)
		{
			LeaveCriticalSection(&(pInner->pub.cs));
			CR_LOG_ERR("auto", "bad alloc");
			return CRFALSE;
		}
		pInner->arr = tmp;
		pInner->pub.total = endpos + 1;
	}
	else if (endpos >= pInner->pub.total)
		pInner->pub.total = endpos + 1;
	CRUINT8* buffer = pInner->arr;
	CRUINT8 offs = offset % 8;
	buffer += offset >> 3;
	for (; len > 0; len--)
	{
		*buffer &= CR_BIT_MASK_0 >> offs | CR_BIT_COVER << (8 - offs);
		*buffer |= (bits >> (len - 1) & 1) << (7 - offs);
		offs++;
		if (offs > 7)
		{
			offs = 0;
			buffer++;
		}
	}
	LeaveCriticalSection(&(pInner->pub.cs));
	return CRTRUE;
}

//
//
//键值树的实现
//

static PTREENODE _create_treenode_cr_(CRINT64 key, CRLVOID data, PTREENODE parent)
{
	PTREENODE pNode = CRAlloc(NULL, sizeof(TREENODE));
	if (pNode)
	{
		pNode->parent = parent;
		pNode->red = CRTRUE;
		pNode->key = key;
		pNode->data = data;
		pNode->left = NULL;
		pNode->right = NULL;
	}
	return pNode;
}
//当且仅当node是左节点时返回CRTRUE，其余所有情况返回CRFALSE
static inline CRBOOL _left_child_(PTREENODE node)
{
	if (node && node->parent && node == node->parent->left)
		return CRTRUE;
	return CRFALSE;
}
//当且仅当node存在且颜色为红色时返回CRTRUE
static inline CRBOOL _color_(PTREENODE node)
{
	return node == NULL ? CRFALSE : node->red;
}
//兄弟结点
static inline PTREENODE _sibling_(PTREENODE node)
{
	if (_left_child_(node))
		return node->parent->right;
	else if (node->parent)
		return node->parent->left;
	return NULL;
}
//左旋操作，顺便集成了根结点的判定
static void _left_rotate_(PTREENODE node, PCRTREE tree)
{
	PTREENODE top = node, right = node->right, child = right->left;
	if (!node->parent)
		tree->root = right;
	else if (_left_child_(node))
		node->parent->left = right;
	else node->parent->right = right;
	right->parent = top->parent;
	right->left = top;
	top->parent = right;
	top->right = child;
	if (child)
		child->parent = top;
}
//右旋操作，顺便集成了根结点的判定
static void _right_rotate_(PTREENODE node, PCRTREE tree)
{
	PTREENODE top = node, left = node->left, child = left->right;
	if (!node->parent)
		tree->root = left;
	else if (_left_child_(node))
		node->parent->left = left;
	else node->parent->right = left;
	left->parent = top->parent;
	left->right = top;
	top->parent = left;
	top->left = child;
	if (child)
		child->parent = top;
}
//获取前驱结点
static PTREENODE _prev_node_(PTREENODE node)
{
	/*
	* 此处不需要考虑向上寻找结点，因为在删除情况下不会用到这种情况
	* 在删除时，只要需要寻找前驱节点，就说明这个节点下方必然有两个节点
	*/
	node = node->left;
	while (node->right) node = node->right;
	return node;
}
//寻找对应结点，假如能够找到，返回此结点
static PTREENODE _look_up_(PTREENODE root, CRINT64 key, PTREENODE* parent)
{
	*parent = root;
Find:
	if (root->key == key)
		return root;
	else if (root->key < key && root->right)
	{
		root = root->right;
		*parent = root;
		goto Find;
	}
	else if (root->key > key && root->left)
	{
		root = root->left;
		*parent = root;
		goto Find;
	}
	return NULL;
}

//吐槽：插入操作比删除操作简单多了
CRAPI CRBOOL CRTreePut(CRSTRUCTURE tree, CRLVOID data, CRINT64 key)
{
	PCRTREE pInner = tree;
	if (!pInner || pInner->pub.type != TRE)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	if (!pInner->root)
	{
		pInner->root = _create_treenode_cr_(key, data, NULL);
		if (!pInner->root)
		{
			LeaveCriticalSection(&(pInner->pub.cs));
			CR_LOG_ERR("auto", "bad alloc");
			return CRFALSE;
		}
		pInner->root->red = CRFALSE;
		goto Done;
	}
	PTREENODE node = NULL, parent = NULL, uncle = NULL, grandpa = NULL;
	if (node = _look_up_(pInner->root, key, &parent))
	{
		node->data = data;
		goto Done;
	}
	node = _create_treenode_cr_(key, data, parent);
	if (!node)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_ERR("auto", "bad alloc");
		return CRFALSE;
	}
	if (key < parent->key)
		parent->left = node;
	else parent->right = node;
Fix:
	if (node == pInner->root)
	{
		node->red = CRFALSE;
		goto Done;
	}
	if (!_color_(node->parent))
	{
		goto Done;
	}
	parent = node->parent;
	uncle = _sibling_(parent);
	grandpa  = parent->parent;
	if (_color_(uncle))
	{
		parent->red = CRFALSE;
		uncle->red = CRFALSE;
		grandpa->red = CRTRUE;
		node = grandpa;
		goto Fix;
	}
	else
	{
		if (_left_child_(parent))
		{
			if (!_left_child_(node))
			{
				_left_rotate_(parent, pInner);
				parent = node;
				node = node->left;
			}
			parent->red = CRFALSE;
			grandpa->red = CRTRUE;
			_right_rotate_(grandpa, pInner);
		}
		else
		{
			if (_left_child_(node))
			{
				_right_rotate_(parent, pInner);
				parent = node;
				node = node->right;
			}
			parent->red = CRFALSE;
			grandpa->red = CRTRUE;
			_left_rotate_(grandpa, pInner);
		}
	}
Done:
	pInner->pub.total++;
	LeaveCriticalSection(&(pInner->pub.cs));
	return CRTRUE;
}

//
//噩梦——删除红黑树节点

//declaration
static void _case_1_red_sibling_(PCRTREE tree, PTREENODE node);
static void _case_2_far_red_nephew_(PCRTREE tree, PTREENODE node);
static void _case_3_near_red_nephew(PCRTREE tree, PTREENODE node);
static void _case_4_red_parent_(PCRTREE tree, PTREENODE node);
static void _case_5_all_black(PCRTREE tree, PTREENODE node);

/*假如兄弟结点是红色，就转化成父节点是红色的情况，以统一处理
     p              S   |      p             S
    / \            / \  |     / \           / \
   D  red   ==>  red ...|   red  D   ==>  ... red
  ... /  \       / \    |   /  \ ...          / \
 .. ...  ...    D  ...  | ...  ... ..       ...  D
*/
void _case_1_red_sibling_(PCRTREE tree, PTREENODE node)
{
	node->parent->red = CRTRUE;
	if (_left_child_(node))
	{
		node->parent->right->red = CRFALSE;
		_left_rotate_(node->parent, tree);
	}
	else
	{
		node->parent->left->red = CRFALSE;
		_right_rotate_(node->parent, tree);
	}
}
/*兄弟节点是黑色且挂了一个远端红色侄子结点
     p              p
    / \            / \
   D   S          S   D
  ... / \        / \  ...
 .. ... red    red ...  ..
*/
void _case_2_far_red_nephew_(PCRTREE tree, PTREENODE node)
{
	PTREENODE sibling = _sibling_(node);
	PTREENODE farNephew;
	sibling->red = node->parent->red;
	node->parent->red = CRFALSE;
	if (_left_child_(node))
	{
		farNephew = sibling->right;
		_left_rotate_(node->parent, tree);
	}
	else
	{
		farNephew = sibling->left;
		_right_rotate_(node->parent, tree);
	}
	farNephew->red = CRFALSE;
}
/*兄弟节点设计黑色且只挂了一个近端红色侄子节点
	 p              p
    / \            / \
   D   S          S   D
  ... / \        / \  ...
 .. red  ...   ...  red ..
转化成为情况2来处理
*/
void _case_3_near_red_nephew(PCRTREE tree, PTREENODE node)
{
	PTREENODE sibling = _sibling_(node);
	PTREENODE nearNephew;
	if (_left_child_(node))
	{
		nearNephew = sibling->left;
		_right_rotate_(sibling, tree);
	}
	else
	{
		nearNephew = sibling->right;
		_left_rotate_(sibling, tree);
	}
	sibling->red = CRTRUE;
	nearNephew->red = CRFALSE;
	//转化之后就可以使用情况2来处理了
	_case_2_far_red_nephew_(tree, node);
}
/*父结点为红色而且有兄弟结点
* 将父节点变为黑色，兄弟节点变红就能平衡
    red          red
    / \          / \
   D   S        S   D
  / \ / \      / \ / \
 .........    .........
*/
void _case_4_red_parent_(PCRTREE tree, PTREENODE node)
{
	PTREENODE sibling = _sibling_(node);
	node->parent->red = CRFALSE;
	sibling->red = CRTRUE;
}
/*唯一一种需要用到迭代的情况，全黑
   Black        Black
    / \          / \
   D Black    Black D
  / \ / \      / \ / \
 .........    .........
*/
void _case_5_all_black(PCRTREE tree, PTREENODE node)
{
	PTREENODE sibling = _sibling_(node);
	sibling->red = CRTRUE;
}

void _fix_single_black_node_(PCRTREE tree, PTREENODE node)
{
	PTREENODE sibling;
Fix:
	if (node == tree->root)
	{
		node->red = CRFALSE;
		return;
	}
	//这种情况下兄弟节点必然存在，否则就不平衡了
	sibling = _sibling_(node);
	if (_color_(sibling))
	{
		_case_1_red_sibling_(tree, node);
		goto Fix;
	}
	else if (_left_child_(node))
	{
		if (_color_(sibling->right))
			_case_2_far_red_nephew_(tree, node);
		else if (_color_(sibling->left))
			_case_3_near_red_nephew(tree, node);
		else if (_color_(node->parent))
			_case_4_red_parent_(tree, node);
		else
		{
			_case_5_all_black(tree, node);
			node = node->parent;
			goto Fix;
		}
	}
	else
	{
		if (_color_(sibling->left))
			_case_2_far_red_nephew_(tree, node);
		else if (_color_(sibling->right))
			_case_3_near_red_nephew(tree, node);
		else if (_color_(node->parent))
			_case_4_red_parent_(tree, node);
		else
		{
			_case_5_all_black(tree, node);
			node = node->parent;
			goto Fix;
		}
	}
}

void _fix_parent_(PTREENODE node)
{
	if (_left_child_(node))
		node->parent->left = NULL;
	else if (node->parent)
		node->parent->right = NULL;
}

CRAPI CRBOOL CRTreeGet(CRSTRUCTURE tree, CRLVOID* data, CRINT64 key)
{
	PCRTREE pInner = tree;
	if (!pInner || pInner->pub.type != TRE)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	if (!pInner->root)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_IFO("auto", "tree with no root node");
		return CRFALSE;
	}
	PTREENODE parent;
	PTREENODE node = _look_up_(pInner->root, key, &parent);
	if (!node)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_IFO("auto", "node not found");
		return CRFALSE;
	}
	if (data) *data = node->data;
Fix:
	if (node->left && node->right)
	{
		PTREENODE prev = _prev_node_(node);
		node->data = prev->data;
		node->key = prev->key;
		node = prev;
	}  //假如红黑树的结构是正确的，转换一次就够了
	if (node->red)
		goto Done;
	else if (node->left)
	{
		node->data = node->left->data;
		node->key = node->left->key;
		node = node->left;
	}
	else if (node->right)
	{
		node->data = node->right->data;
		node->key = node->right->key;
		node = node->right;
	}
	else
		_fix_single_black_node_(pInner, node);
Done:
	_fix_parent_(node);
	if (node == pInner->root)
		pInner->root = NULL;
	CRAlloc(node, 0);
	pInner->pub.total--;
	LeaveCriticalSection(&(pInner->pub.cs));
	return CRTRUE;
}

CRAPI CRBOOL CRTreeSeek(CRSTRUCTURE tree, CRLVOID* data, CRINT64 key)
{
	PCRTREE pInner = tree;
	if (!pInner || pInner->pub.type != TRE)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	if (!pInner->root)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_IFO("auto", "tree with no root node");
		return CRFALSE;
	}
	PTREENODE parent;
	PTREENODE node = _look_up_(pInner->root, key, &parent);
	if (!node)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_IFO("auto", "node not found");
		return CRFALSE;
	}
	if (data) *data = node->data;
	LeaveCriticalSection(&(pInner->pub.cs));
	return CRTRUE;
}

//
// 
//线性表实现
//

CRAPI CRBOOL CRLinPut(CRSTRUCTURE lin, CRLVOID data, CRINT64 seek)
{
	PCRLINEAR pInner = lin;
	if (!pInner || pInner->pub.type != LIN)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	PLINEARNODE ins = CRAlloc(NULL, sizeof(LINEARNODE));
	if (!ins)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_ERR("auto", "bad alloc");
		return CRFALSE;
	}
	ins->data = data;
	GET_MOD(seek, pInner->pub.total);
	if (!pInner->hook)
	{
		ins->prior = ins;
		ins->after = ins;
		pInner->hook = ins;
	}
	else if (seek < 0)
	{
		ins->prior = pInner->hook->prior;
		while (seek < 0) { seek++; ins->prior = ins->prior->prior; }
		ins->after = ins->prior->after;
		ins->prior->after = ins;
		ins->after->prior = ins;
	}
	else
	{
		ins->after = pInner->hook->after;
		while (seek > 0) { seek--; ins->after = ins->after->after; }
		ins->prior = ins->after->prior;
		ins->after->prior = ins;
		ins->prior->after = ins;
	}
	pInner->pub.total++;
	LeaveCriticalSection(&(pInner->pub.cs));
	return CRTRUE;
}

CRAPI CRBOOL CRLinSeek(CRSTRUCTURE lin, CRLVOID* data, CRINT64 seek)
{
	PCRLINEAR pInner = lin;
	if (!pInner || pInner->pub.type != LIN)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	if (!pInner->hook)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_IFO("auto", "node not found");
		return CRFALSE;
	}
	PLINEARNODE node = pInner->hook;
	GET_MOD(seek, pInner->pub.total);
	if (seek < 0)
		while (seek < 0) { seek++; node = node->prior; }
	else
		while (seek > 0) { seek--; node = node->after; }
	if (data) *data = node->data;
	LeaveCriticalSection(&(pInner->pub.cs));
	return CRTRUE;
}

CRAPI CRBOOL CRLinGet(CRSTRUCTURE lin, CRLVOID* data, CRINT64 seek)
{
	PCRLINEAR pInner = lin;
	if (!pInner || pInner->pub.type != LIN)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	if (!pInner->hook)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_IFO("auto", "empty list");
		return CRFALSE;
	}
	PLINEARNODE node = pInner->hook;
	GET_MOD(seek, pInner->pub.total);
	if (seek < 0)
		while (seek < 0) { seek++; node = node->prior; }
	else
		while (seek > 0) { seek--; node = node->after; }
	if (data) *data = node->data;
	if (pInner->pub.total == 1)
	{
		CRAlloc(pInner->hook, 0);
		pInner->hook = NULL;
	}
	else
	{
		if (node == pInner->hook)
			pInner->hook = node->after;
		node->prior->after = node->after;
		node->after->prior = node->prior;
		CRAlloc(node, 0);
	}
	pInner->pub.total--;
	LeaveCriticalSection(&(pInner->pub.cs));
	return CRTRUE;
}

//
//
//四叉树（空间索引树）实现
//

#define CRQUAD_N 0x00
#define CRQUAD_L 0x01
#define CRQUAD_R 0x02
#define CRQUAD_T 0x04
#define CRQUAD_B 0x08

/*定位函数
* 此函数将告诉你以当前的cover处于base的哪个部分
* 不仅可以处于单个区域，还可以跨区
*/
static CRUINT8 _quad_partition_(CRRECTU cover, CRRECTU base, CRBOOL* crossed)
{
	CRUINT8 partition = CRQUAD_N;
	if (crossed) *crossed = CRFALSE;
	CRUINT32 x_m = base.left + (base.right - base.left) / 2;
	CRUINT32 y_m = base.top + (base.bottom - base.top) / 2;
	if (cover.right <= x_m)
		partition |= CRQUAD_L;
	else if (cover.left > x_m)
		partition |= CRQUAD_R;
	else
	{
		if (crossed) *crossed = CRTRUE;
		partition |= CRQUAD_L | CRQUAD_R;
	}
	if (cover.bottom <= y_m)
		partition |= CRQUAD_T;
	else if (cover.top > y_m)
		partition |= CRQUAD_B;
	else
	{
		if (crossed) *crossed = CRTRUE;
		partition |= CRQUAD_T | CRQUAD_B;
	}
	return partition;
}

/*
* 假如返回CRFALSE，就表明不可再分（拆分到单个像素了）或者范围有误
* 分裂不仅会拆分区域，还会将元素分配到子区域
* 对于那种刚好在边界横跨两个区域的元素，就留在父节点
* 
* 拆分时会检查拆分后的结点是否需要继续拆分（少见的情况）
* 满足拆分条件的进一步拆分，直到拆到满足数量条件或全部跨区无法拆分
*/
static CRBOOL _quad_split_(PQUADTREENODE node)
{
	if (node->range.right - node->range.left <= 1 || node->range.bottom - node->range.top <= 1)
		return CRFALSE;
	//先对已有元素进行分区判断在决定是否分裂结点
	CRUINT32 cross = 0;  //统计跨区元素数量，只要不是全部跨区且结点可分裂，就有必要分裂结点
	PCRQUADITEMS titem;
	CRBOOL crossed;
	//
	for (int i = 0; i < CRStructureSize(node->items); i++)
	{
		CRDynSeek(node->items, &titem, i, DYN_MODE_64);
		_quad_partition_(titem->cover, node->range, &crossed);
		if (crossed) cross++;
	}
	if (cross >= CRStructureSize(node->items))
		return CRFALSE;
	//确定要分裂结点了
	CRRECTU range;
	CRUINT64 x_m = node->range.left + (node->range.right - node->range.left) / 2;
	CRUINT64 y_m = node->range.top + (node->range.bottom - node->range.top) / 2;
	range.left = node->range.left;
	range.top = node->range.top;
	range.right = x_m;
	range.bottom = y_m;
	node->LT = _inner_create_node_(range, node);
	//
	range.left = x_m + 1;
	range.right = node->range.right;
	node->RT = _inner_create_node_(range, node);
	//
	range.top = y_m + 1;
	range.bottom = node->range.bottom;
	node->RB = _inner_create_node_(range, node);
	//
	range.left = node->range.left;
	range.right = x_m;
	node->LB = _inner_create_node_(range, node);
	//使用新的dynPtr来承接新的item列表，就列表pop完毕之后销毁
	CRSTRUCTURE tmp = node->items;
	node->items = CRDynamic(0);  //即使内存申请失败也不处理了，太繁琐了
	CRUINT8 p;
	while (!CRDynPop(tmp, &titem, DYN_MODE_64))
	{
		p = _quad_partition_(titem->cover, node->range, &crossed);
		if (crossed)
			CRDynPush(node->items, &titem, DYN_MODE_64);
		else
		{
			switch (p)
			{
			case CRQUAD_L | CRQUAD_T:
				if (CRDynPush(node->LT->items, &titem, DYN_MODE_64))
					CRAlloc(titem, 0);
				else titem->hook = node->LT;
				break;
			case CRQUAD_R | CRQUAD_T:
				if (CRDynPush(node->RT->items, &titem, DYN_MODE_64))
					CRAlloc(titem, 0);
				else titem->hook = node->RT;
				break;
			case CRQUAD_L | CRQUAD_B:
				if (CRDynPush(node->RB->items, &titem, DYN_MODE_64))
					CRAlloc(titem, 0);
				else titem->hook = node->RB;
				break;
			case CRQUAD_R | CRQUAD_B:
				if (CRDynPush(node->LB->items, &titem, DYN_MODE_64))
					CRAlloc(titem, 0);
				else titem->hook = node->LB;
				break;
			default:
				CRAlloc(titem, 0);
				break;
			}
		}
	}
	//
	CRFreeStructure(tmp, NULL);
	return CRTRUE;
}

static void _fix_quadnode_split_(PQUADTREENODE node, CRUINT8 max)
{
	if (CRStructureSize(node->items) <= max || node->LT)
		return;
	if (_quad_split_(node))
	{
		_fix_quadnode_split_(node->LT, max);
		_fix_quadnode_split_(node->RT, max);
		_fix_quadnode_split_(node->RB, max);
		_fix_quadnode_split_(node->LB, max);
	}
}

//暂时不改变四叉树的结构
static void _fix_quadnode_merge_(PQUADTREENODE node, CRUINT8 max)
{
	//递归处理，检查这下面一系列结点是否可以融合
	if (node->LT)
	{
		_fix_quadnode_merge_(node->LT, max);
		_fix_quadnode_merge_(node->RT, max);
		_fix_quadnode_merge_(node->RB, max);
		_fix_quadnode_merge_(node->LB, max);
	}
Merge:  //对当前结点做出判断，符合条件的进行融合
	if (node->parent)
	{
		node = node->parent;
	}
	else if (node->LT)
	{
		CRUINT32 numLT = CRStructureSize(node->LT->items);
		CRUINT32 numRT = CRStructureSize(node->RT->items);
		CRUINT32 numRB = CRStructureSize(node->RB->items);
		CRUINT32 numLB = CRStructureSize(node->LB->items);
		CRUINT32 numME = CRStructureSize(node->items);
		if (!numLT || !numRT || !numRB || !numLB)
			goto Melt;
		else if (numLT + numRT + numRB + numLB + numME <= max)
		{
			PCRQUADITEMS item;
			while (!CRDynPop(node->LT->items, &item, DYN_MODE_64))
			{
				item->hook = node;
				CRDynPush(node->items, &item, DYN_MODE_64);
			}
			while (!CRDynPop(node->RT->items, &item, DYN_MODE_64))
			{
				item->hook = node;
				CRDynPush(node->items, &item, DYN_MODE_64);
			}
			while (!CRDynPop(node->RB->items, &item, DYN_MODE_64))
			{
				item->hook = node;
				CRDynPush(node->items, &item, DYN_MODE_64);
			}
			while (!CRDynPop(node->LB->items, &item, DYN_MODE_64))
			{
				item->hook = node;
				CRDynPush(node->items, &item, DYN_MODE_64);
			}
			goto Melt;
		}
	}
	return;
Melt:
	CRFreeStructure(node->LT->items, NULL);
	CRAlloc(node->LT, 0);
	node->LT = NULL;
	CRFreeStructure(node->RT->items, NULL);
	CRAlloc(node->RT, 0);
	node->RT = NULL;
	CRFreeStructure(node->RB->items, NULL);
	CRAlloc(node->RB, 0);
	node->RB = NULL;
	CRFreeStructure(node->LB->items, NULL);
	CRAlloc(node->LB, 0);
	node->LB = NULL;
}

CRAPI CRBOOL CRQuadtreePushin(CRSTRUCTURE tree, CRRECTU range, CRLVOID key)
{
	PCRQUADINNER pInner = tree;
	if (!pInner || pInner->pub.type != QUA || !CRTreeSeek(pInner->keyTree, NULL, (CRUINT64)key))
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	PCRQUADITEMS item = CRAlloc(NULL, sizeof(CRQUADITEMS));
	if (!item)
	{
		CR_LOG_ERR("auto", "bad alloc");
		return CRFALSE;
	}
	//
	item->cover.left = range.left;
	item->cover.top = range.top;
	item->cover.right = range.right;
	item->cover.bottom = range.bottom;
	item->data = key;
	//寻找item的所属结点
	CRBOOL crossed;
	CRUINT8 part;
	PQUADTREENODE node = pInner->root;
	while (1)
	{
		part = _quad_partition_(item->cover, node->range, &crossed);
		if (crossed)
			break;
		if (node->LT)  //只要有一个子节点，就说明另外三个也在
		{
			switch (part)
			{
			case CRQUAD_L | CRQUAD_T:
				node = node->LT;
				break;
			case CRQUAD_R | CRQUAD_T:
				node = node->RT;
				break;
			case CRQUAD_R | CRQUAD_B:
				node = node->RB;
				break;
			case CRQUAD_L | CRQUAD_B:
				node = node->LB;
				break;
			default:
				break;
			}
		}
		else
			break;
	}
	item->hook = node;
	CRDynPush(node->items, &item, DYN_MODE_64);
	CRTreePut(pInner->keyTree, item, (CRUINT64)key);
	//开始检查结点是否可调整
	_fix_quadnode_split_(node, pInner->maxItem);
	return CRTRUE;
}

//通过键值树直接找到此元素，然后从结点中移除此元素
//最后检查临近结点包括父节点的元素分布，满足数量的就融合结点
CRAPI CRBOOL CRQuadtreeRemove(CRSTRUCTURE tree, CRLVOID key)
{
	PCRQUADINNER pInner = tree;
	if (!pInner || pInner->pub.type != QUA)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	PCRQUADITEMS item;
	if (CRTreeGet(pInner->keyTree, (CRLVOID*)&item, (CRUINT64)key))
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		CR_LOG_IFO("auto", "node not found");
		return CRFALSE;
	}
	//命中结点之后开始移除并调整
	PQUADTREENODE node = item->hook;
	CRSTRUCTURE tmp = CRDynamic(0);
	while (!CRDynPop(node->items, &item, DYN_MODE_64))
	{
		if (item->data != key)
			CRDynPush(tmp, &item, DYN_MODE_64);
		else CRAlloc(item, 0);
	}
	CRFreeStructure(node->items, NULL);
	node->items = tmp;
	_fix_quadnode_merge_(node, pInner->maxItem);
	LeaveCriticalSection(&(pInner->pub.cs));
	return CRTRUE;
}

CRAPI CRBOOL CRQuadtreeSearch(CRSTRUCTURE tree, CRPOINTU p, CRSTRUCTURE dynOut)
{
	PCRQUADINNER pInner = tree;
	if (!pInner || pInner->pub.type != QUA)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pInner->pub.cs));
	PQUADTREENODE node = pInner->root;
	if (p.x < pInner->root->range.left
		|| p.x > pInner->root->range.right
		|| p.y < pInner->root->range.top
		|| p.y > pInner->root->range.bottom)
	{
		LeaveCriticalSection(&(pInner->pub.cs));
		return CRTRUE;
	}
	PCRQUADITEMS item;
	CRUINT32 x_m, y_m;
	while (1)
	{
		for (int i = 0; i < CRStructureSize(node->items); i++)
		{
			CRDynSeek(node->items, &item, i, DYN_MODE_64);
			if (p.x >= item->cover.left
				&& p.x <= item->cover.right
				&& p.y >= item->cover.top
				&& p.y <= item->cover.bottom)
				CRDynPush(dynOut, &item->data, DYN_MODE_64);
		}
		x_m = node->range.left + (node->range.right - node->range.left) / 2;
		y_m = node->range.top + (node->range.bottom - node->range.top) / 2;
		if (node->LT)
		{
			if (p.x <= x_m && p.y <= y_m)
				node = node->LT;
			else if (p.x > x_m && p.y <= y_m)
				node = node->RT;
			else if (p.x > x_m && p.y > y_m)
				node = node->RB;
			else if (p.x <= x_m && p.y > y_m)
				node = node->LB;
		}
		else break;
	}
	LeaveCriticalSection(&(pInner->pub.cs));
	return CRTRUE;
}

CRAPI CRBOOL CRQuadtreeCheck(CRSTRUCTURE tree, CRLVOID key)
{
	PCRQUADINNER pInner = tree;
	if (pInner && pInner->pub.type == QUA)
	{
		EnterCriticalSection(&(pInner->pub.cs));
		if (!CRTreeSeek(pInner->keyTree, NULL, (CRUINT64)key))
		{
			LeaveCriticalSection(&(pInner->pub.cs));
			return CRTRUE;
		}
		LeaveCriticalSection(&(pInner->pub.cs));
	}
	return CRFALSE;
}

//
//
//遍历数据结构实现
//

static void _for_dyn_(CRSTRUCTURE s, DSForeach cal, CRLVOID user)
{
	PCRDYN dyn = s;
	int tmp = 0;
	if (dyn->pub.total % 8)
		tmp = dyn->pub.total >> 3 + 1;
	else tmp = dyn->pub.total >> 3;
	for (int i = 0; i < tmp; i++) cal((CRLVOID)dyn->p64[i], user, i);
}

static void _for_tree_node_(PTREENODE node, DSForeach cal, CRLVOID user)
{
	if (node->left)
		_for_tree_node_(node->left, cal, user);
	cal(node->data, user, node->key);  //中序遍历当然是放在中间（这一点十分形象）
	if (node->right)
		_for_tree_node_(node->right, cal, user);
}

static void _for_tree_(CRSTRUCTURE s, DSForeach cal, CRLVOID user)
{
	PCRTREE tree = s;
	if (tree->root)
		_for_tree_node_(tree->root, cal, user);
}

static void _for_linear_(CRSTRUCTURE s, DSForeach cal, CRLVOID user)
{
	PCRLINEAR lin = s;
	if (!lin->hook)
		return;
	PLINEARNODE node = lin->hook, p = node;
	node->prior->after = NULL;
	CRUINT64 i = 0;
	while (node->after)
	{
		p = node;
		node = node->after;
		cal(node->data, user, i++);
		CRAlloc(p, 0);
	}
	cal(node->data, user, i);
}

typedef void (*_Struct_For_Each_)(CRSTRUCTURE s, DSForeach cal, CRLVOID user);
static void _for_nothing_(CRSTRUCTURE s, DSForeach cal, CRLVOID user) { return; }

const _Struct_For_Each_ forFuncs[] =
{
	_for_dyn_,
	_for_tree_,
	_for_linear_,
	_for_nothing_
};

CRAPI CRBOOL CRStructureForEach(CRSTRUCTURE s, DSForeach cal, CRLVOID user)
{
	CRSTRUCTUREPUB* pub = s;
	if (!pub || !cal)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	EnterCriticalSection(&(pub->cs));
	if (pub->type < 0 || pub->type > 3)
	{
		LeaveCriticalSection(&(pub->cs));
		return CRFALSE;
	}
	forFuncs[pub->type](s, cal, user);
	LeaveCriticalSection(&(pub->cs));
	return CRTRUE;
}

//
//
//销毁释放数据结构
//

//什么也不做，仅占位
static void _struc_do_nothing_(CRLVOID data) { return; }
static void _clear_nothing_(CRSTRUCTURE s, DSCallback cal) { return; }

static void _clear_dyn_(CRSTRUCTURE s, DSCallback cal)
{
	PCRDYN dyn = s;
	if(dyn->pub.total % 8)
		dyn->pub.total = dyn->pub.total >> 3 + 1;
	else dyn->pub.total >>= 3;
	for (int i = 0; i < dyn->pub.total; i++) cal((CRLVOID)dyn->p64[i]);
	CRAlloc(dyn->arr, 0);
}

static void _clear_tree_node_(PTREENODE node, DSCallback cal)
{
	if (node->left)
		_clear_tree_node_(node->left, cal);
	if (node->right)
		_clear_tree_node_(node->right, cal);
	cal(node->data);
	CRAlloc(node, 0);
}
static void _clear_tree_(CRSTRUCTURE s, DSCallback cal)
{
	PCRTREE tree = s;
	if (tree->root)
		_clear_tree_node_(tree->root, cal);
}

static void _clear_linear_(CRSTRUCTURE s, DSCallback cal)
{
	PCRLINEAR lin = s;
	if (!lin->hook)
		return;
	PLINEARNODE node = lin->hook, p = node;
	node->prior->after = NULL;
	while (node->after)
	{
		p = node;
		node = node->after;
		cal(node->data);
		CRAlloc(p, 0);
	}
	cal(node->data);
	CRAlloc(node, 0);
}

void _cover_clear_cbk_(CRLVOID data)
{
	CRAlloc((PCRQUADITEMS)data, 0);
}

void _free_quad_node_(PQUADTREENODE node)
{
	if (node->LT)
		_free_quad_node_(node->LT);
	if (node->RT)
		_free_quad_node_(node->RT);
	if (node->LB)
		_free_quad_node_(node->LB);
	if (node->RB)
		_free_quad_node_(node->RB);
	CRFreeStructure(node->items, _cover_clear_cbk_);
	CRAlloc(node, 0);
}

static void _free_quad_(PCRQUADINNER quad, DSCallback cbk)
{
	CRFreeStructure(quad->keyTree, cbk);
	_free_quad_node_(quad->root);
}

static void _clear_quad_(CRSTRUCTURE s, DSCallback cal)
{
	_free_quad_(s, cal);
}

//使用一下数据思想
typedef void (*_Struct_Clear_Func_)(CRSTRUCTURE s, DSCallback cal);
const _Struct_Clear_Func_ clearFuncs[] =
{
	_clear_dyn_,
	_clear_tree_,
	_clear_linear_,
	_clear_quad_
};

CRBOOL CRFreeStructure(CRSTRUCTURE s, DSCallback cal)
{
	CRSTRUCTUREPUB* pub = s;
	if (!pub)
	{
		CR_LOG_WAR("auto", "invalid structure");
		return CRFALSE;
	}
	//如此操作只是为了确保最后一个操作完成，假如后面还有操作，造成的错误概不负责
	EnterCriticalSection(&(pub->cs));
	if (!cal)
		cal = _struc_do_nothing_;
	if (pub->type < 0 || pub->type > 3)
	{
		LeaveCriticalSection(&(pub->cs));
		CR_LOG_ERR("auto", "unknown type of structure");
		return CRFALSE;
	}
	clearFuncs[pub->type](s, cal);
	LeaveCriticalSection(&(pub->cs));
	DeleteCriticalSection(&(pub->cs));
	CRAlloc(s, 0);
	return CRTRUE;
}
