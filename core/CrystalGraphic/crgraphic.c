/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-23 00:35:11
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-06-26 13:18:32
 * @FilePath: \Crystal-Graphic\core\crgraphic.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <CrystalGraphic.h>

void _cr_inner_do_nothing_grh_(void){};

static void* CRGraphicFunListArr[] =
{
    _cr_inner_do_nothing_grh_, "CrystalGraphicInit",
    _cr_inner_do_nothing_grh_, "CRWindowCounter",
    _cr_inner_do_nothing_grh_, "CRCreateWindow",
    _cr_inner_do_nothing_grh_, "CRCloseWindow",
    _cr_inner_do_nothing_grh_, "CRSetWindowCbk",
    //8
    0
};

CRAPI void** CRGraphicFunList = CRGraphicFunListArr;