/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-07-12 20:20:42
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-08-01 23:58:46
 * @FilePath: \Crystal-Audio\include\CrystalAudio.h
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#ifndef _INCLUDE_CRYSTALAUTIO_H_
#define _INCLUDE_CRYSTALAUTIO_H_

#include "AudioDfs.h"

extern void** CRAudioFunList;

/**
 * 手动初始化额外的依赖项
 */
typedef CRBOOL(*CRAUDIOINIT)(void** alg, void** thr);
#define CrystalAudioInit ((CRAUDIOINIT)CRAudioFunList[0])

/**
 * 创建一个音频对象
 */
typedef CRAUDIO(*CRAUDIOCREATE)(CRAudioStreamCbk cbk, CRWWINFO* inf);
#define CRAudioCreate ((CRAUDIOCREATE)CRAudioFunList[2])

/**
 * 自动加载WAV文件并以动态数组形式存储
 */
typedef CRBOOL(*CRLOADWW)(const CRCHAR* path, CRSTRUCTURE out, CRWWINFO *inf);
#define CRLoadWW ((CRLOADWW)CRAudioFunList[4])

/**
 * 关闭一个音频对象
 */
typedef void(*CRAUDIOCLOSE)(CRAUDIO play);
#define CRAudioClose ((CRAUDIOCLOSE)CRAudioFunList[6])

/**
 * 暂停音频
 */
typedef void(*CRAUDIOPAUSE)(CRAUDIO play);
#define CRAudioPause ((CRAUDIOPAUSE)CRAudioFunList[8])

/**
 * 恢复音频
 */
typedef void(*CRAUDIORESUME)(CRAUDIO play);
#define CRAudioResume ((CRAUDIORESUME)CRAudioFunList[10])

#endif
