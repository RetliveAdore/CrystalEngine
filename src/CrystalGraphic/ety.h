/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-08-09 20:59:15
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-08-09 21:46:33
 * @FilePath: \CrystalGraphic\src\ety.h
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#ifndef _INCLUDE_CRETY_H_
#define _INCLUDE_CRETY_H_

#include "crgl.h"

typedef struct entityNode
{
    CRUIENTITY *pEty;
    CRUIENTITY Ety;
    CRUINT32 VAO;
    CRUINT32 VBO;
    CRUINT32 EBO;  //顶点索引信息
    CRUINT64 Texture;  //只有Entity风格为Bitmap时会用到，其余时刻忽略
    float *arrayBuffer;  //使用arrayBuffer将顶点缓存起来就不用每次都去算了
    CRUINT32 arraysize;
    CRUINT32 *elementBuffer;  //索引缓冲
    CRUINT32 elementsize;
    CRUINT32 elementcount;
    CR_GL *pThis;
}CRUIENTITYNODE, *PCRUIENTITYNODE;

#endif
