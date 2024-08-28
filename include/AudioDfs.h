/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-07-12 20:32:45
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-07-28 22:49:57
 * @FilePath: \Crystal-Audio\include\AudioDfs.h
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#ifndef _INCLUDE_CRYSTALAUDIODFS_H_
#define _INCLUDE_CRYSTALAUDIODFS_H_

#include <CrystalCore.h>
#include <CrystalAlgorithms.h>
#include <CrystalThread.h>

typedef CRLVOID CRAUDIO;
typedef void (*CRAudioStreamCbk)(CRUINT8* buffer, CRUINT32 frames, CRUINT32 size);

#pragma pack(push, 1)  //千万别使用默认的字节对齐，否则无法正确读取文件
/*
* WAVE格式的文件相关结构体
*/

typedef struct
{
	CRUINT32 ChunkID;
	CRUINT32 ChunkSize;
}CRWWBLOCK;

typedef struct
{
	CRUINT16 AudioFormat;   //PCM固定为1
	CRUINT16 NumChannels;   //声道数量
	CRUINT32 SampleRate;    //采样率，如：44100
	CRUINT32 ByteRate;
	CRUINT16 BlockAlign;
	CRUINT16 BitsPerSample; //采样位宽，如16，24，32
}CRWWINFO;

typedef struct
{
	//
	CRWWBLOCK whole;  //ChunkID is 'RIFF' or it`s wrong file
	CRUINT32 format;  //'WAVE' or wrong data
	//
	CRWWBLOCK block2;  //ChunkID is 'fmt '
	CRWWINFO inf;
	//
	//...data...  //Search a chunk whose id is 'data', then you find the
	//            //actual PCM data
}CRWWHEADER;

#pragma pack(pop)

#endif
