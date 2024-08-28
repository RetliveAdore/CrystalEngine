#ifndef _INCLUdE_ALGDEFS_H_
#define _INCLUdE_ALGDEFS_H_

#include <CrystalCore.h>

//1000_0000
#define CR_BIT_MASK_1 0x80
//0111_1111
#define CR_BIT_MASK_0 0x7f
#define CR_BIT_COVER  0xff
//取模
#define GET_MOD(num, mod) { if ((num) < 0) (num) = -(-(num) % (mod)); else if ((num > 0)) (num) = ((num) % (mod)); }

typedef void* CRSTRUCTURE;

/**
 * 回调函数，用于销毁数据结构时对其中每一个元素进行可选的操作
 */
typedef void(*DSCallback)(CRLVOID data);
/**
 * 回调函数，用于遍历数据结构
 */
typedef void(*DSForeach)(CRLVOID data, CRLVOID user, CRUINT64 key);

typedef enum CRDynEnum
{
    DYN_MODE_8 = 0,
    DYN_MODE_16 = 1,
    DYN_MODE_32 = 2,
    DYN_MODE_64 = 3
}CRDynEnum;

#pragma pack(push, 1)
//通用的二维坐标结构
typedef struct
{
	CRINT64 x;
	CRINT64 y;
}CRPOINTU;
typedef struct
{
	double x;
	double y;
}CRPOINTF;
//矩形结构/二维碰撞箱结构
typedef struct
{
	CRINT64 left;
	CRINT64 top;
	CRINT64 right;
	CRINT64 bottom;
}CRRECTU, CRBOXU;
typedef struct
{
	double left;
	double top;
	double right;
	double bottom;
}CRRECTF, CRBOXF;
#pragma pack(pop)

#endif