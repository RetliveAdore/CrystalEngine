/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-08-25 19:32:45
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-09-09 23:02:20
 * @FilePath: \CrystalEngine\application\main.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */

#include <CrystalCore.h>
#include <CrystalAlgorithms.h>
#include <CrystalThread.h>
#include <CrystalAudio.h>
#include <CrystalGraphic.h>

CRMODULE core = NULL;
CRMODULE algorithms = NULL;
CRMODULE thread = NULL;
CRMODULE audio = NULL;
CRMODULE graphic = NULL;

static CRBOOL _inner_init_main_(char* argv0)
{
    core = CRImport("CrystalCore.so", CRCoreFunList, argv0);
    if (!core) return CRFALSE;
    CR_LOG_IFO("console", "Load CrystalCore succeed");

    algorithms = CRImport("CrystalAlgorithms.so", CRAlgorithmsFunList, argv0);
    if (!algorithms) return CRFALSE;
    CR_LOG_IFO("console", "Load CrystalAlgorithmsSucceed");

    thread = CRImport("CrystalThread.so", CRThreadFunList, argv0);
    if (!thread) return CRFALSE;
    if(CrystalThreadInit(CRAlgorithmsFunList)) CR_LOG_IFO("console", "Load CrystalThreadSucceed");
    else return CRFALSE;

    audio = CRImport("CrystalAudio.so", CRAudioFunList, argv0);
    if (!audio) return CRFALSE;
    if(CrystalAudioInit(CRAlgorithmsFunList, CRThreadFunList)) CR_LOG_IFO("console", "Load CrystalAudioSucceed");
    else return CRFALSE;

    graphic = CRImport("CrystalGraphic.so", CRGraphicFunList, argv0);
    if (!graphic) return CRFALSE;
    if(CrystalGraphicInit(CRAlgorithmsFunList, CRThreadFunList)) CR_LOG_IFO("console", "Load CrystalGraphicSucceed");
    else return CRFALSE;

    return CRTRUE;
}

static void _inner_uninit_main_()
{
    //卸载模块时最好按照初始化的逆序来逐个卸载
    CRUnload(graphic);
    CRUnload(audio);
    CRUnload(thread);
    CRUnload(algorithms);
    CRUnload(core);
}

int main(int argc, char** argv)
{
    _inner_init_main_(argv[0]);
    //应用程序的具体代码
    CRWindowProperties prop1;
    prop1.w = 500;
    prop1.h = 350;
    prop1.x = 100;
    prop1.y = 100;
    prop1.title = "Crystal Application";
    CRCreateWindow(&prop1);
    //开启应用循环
    while(CRWindowCounter()) CRSleep(1);

    //
    _inner_uninit_main_();
    return 0;
}
