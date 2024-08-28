/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-07-23 20:18:18
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-08-01 23:54:12
 * @FilePath: \Crystal-Audio\core\cuaudio.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <CrystalAudio.h>

void _inner_do_nothing_audio_(void){}

void* CRAudioFunListArr[] =
{
    _inner_do_nothing_audio_, "CrystalAudioInit",
    _inner_do_nothing_audio_, "CRAudioCreate",
    _inner_do_nothing_audio_, "CRLoadWW",
    _inner_do_nothing_audio_, "CRAudioClose",
    _inner_do_nothing_audio_, "CRAudioPause",
    _inner_do_nothing_audio_, "CRAudioResume",
    0
};

void** CRAudioFunList = CRAudioFunListArr;