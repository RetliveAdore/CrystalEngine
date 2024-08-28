/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-07-12 20:43:02
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-08-01 23:52:32
 * @FilePath: \Crystal-Audio\src\audio.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <AudioDfs.h>
#include <stdio.h>
#include <string.h>

#define PREFTIMES_PER_SEC 100000

#ifdef CR_WINDOWS
#include <initguid.h>  //这个头文件一定要在最前面引用
#include <Windows.h>
#include <audiopolicy.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>

static IMMDeviceEnumerator* pEnumerator = NULL;

typedef struct
{
	CRUINT8 magic;
	IMMDevice* pDevice;
	IAudioClient* pAudioClient;
	IAudioRenderClient* pRenderClient;
	CRWWINFO* inf;
	CRSTRUCTURE dynPcm;
	/*专用于数据流模式，其他模式中被忽略*/
	CRAudioStreamCbk cbk;

	CRUINT32 hnsActualDuration;
	CRUINT32 bufferFrameCount;
	CRUINT32 numFramesAvailable;
	CRUINT32 numFramesPadding;
	CRUINT32 offs;

	CRTHREAD idThis;
	//symbols
	CRBOOL stop;
	CRBOOL pause;
}AUTHRINF;

#elif defined CR_LINUX
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

typedef struct
{
	CRUINT8 magic;
	snd_pcm_t* handle;
	snd_pcm_uframes_t frames;
	/*专用于数据流模式，其他模式中被忽略*/
	CRAudioStreamCbk cbk;

	CRWWINFO* inf;
	CRSTRUCTURE dynPcm;
	CRUINT32 offs;

	CRTHREAD idThis;
	//symbols
	CRBOOL stop;
	CRBOOL pause;
	int alsa_can_pause;

	CRBOOL stream;
}AUTHRINF;

#endif

CRBOOL _inner_craudio_init_()
{
    #ifdef CR_WINDOWS
    CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator,
		NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
		(void**)&pEnumerator)))
    {
        CR_LOG_ERR("auto", "Failed Create COM Instance");
        return CRFALSE;
    }
    #endif
    return CRTRUE;
}

void _inner_craudio_uninit_()
{
    #ifdef CR_WINDOWS
    CoUninitialize();
    #endif
}

static void _fill_buffer_(CRUINT8* Dst, CRSTRUCTURE Src, CRUINT32 frameCount, CRWWINFO* inf, CRUINT32* offs)
{
	CRUINT32 size = frameCount * inf->BlockAlign;
	for (int i = 0; i < size; i++)
		CRDynSeek(Src, (CRUINT8*)&Dst[i], *offs + i, DYN_MODE_8);
	*offs += size;
	if (*offs > CRStructureSize(Src))
		*offs = CRStructureSize(Src);
}

#ifdef CR_WINDOWS
static CRBOOL _inner_create_device_(AUTHRINF *thinf, CRWWINFO *inf)
{
	WAVEFORMATEX *pwfx = NULL;
	if (FAILED(
			pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
				pEnumerator,
				eRender,
				eConsole,
				&(thinf->pDevice)
			)
		)
	)
	{
		CR_LOG_ERR("auto", "failed get endpoint");
		goto Failed;
	}
	if (FAILED(
			thinf->pDevice->lpVtbl->Activate(
				thinf->pDevice,
				&IID_IAudioClient,
				CLSCTX_ALL,
				NULL,
				(void**)&(thinf->pAudioClient)
			)
		)
	)
	{
		CR_LOG_ERR("auto", "failed active client");
		goto Failed;
	}
	if (FAILED(
			thinf->pAudioClient->lpVtbl->GetMixFormat(
				thinf->pAudioClient,
				&pwfx
			)
		)
	)
	{
		CR_LOG_ERR("auto", "failed get format");
		goto Failed;
	}
	//设置音频数据流信息
	pwfx->wFormatTag = inf->AudioFormat;
	pwfx->nChannels = inf->NumChannels;
	pwfx->nSamplesPerSec = inf->SampleRate;
	pwfx->nAvgBytesPerSec = inf->ByteRate;
	pwfx->nBlockAlign = inf->BlockAlign;
	pwfx->wBitsPerSample = inf->BitsPerSample;
	pwfx->cbSize = 0;
	//
	if (FAILED(
			thinf->pAudioClient->lpVtbl->Initialize(
				thinf->pAudioClient,
				AUDCLNT_SHAREMODE_SHARED,
				AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
				PREFTIMES_PER_SEC,
				0,
				pwfx,
				NULL
			)
		)
	)
	{
		CR_LOG_ERR("auto", "failed initialize");
		goto Failed;
	}
	if (FAILED(
			thinf->pAudioClient->lpVtbl->GetBufferSize(
				thinf->pAudioClient,
				&(thinf->bufferFrameCount)
			)
		)
	)
	{
		CR_LOG_ERR("auto", "failed get buffer size");
		goto Failed;
	}
	if (FAILED(
			thinf->pAudioClient->lpVtbl->GetService(
				thinf->pAudioClient,
				&IID_IAudioRenderClient,
				(void**)&(thinf->pRenderClient)
			)
		)
	)
	{
		CR_LOG_ERR("auto", "failed get service");
		goto Failed;
	}
	CoTaskMemFree(pwfx);
	return CRTRUE;
Failed:
	if (pwfx) CoTaskMemFree(pwfx);
	return CRFALSE;
}

static void _inner_audio_thread_(void* data, CRTHREAD idThis)
{
	AUTHRINF *auinf = (AUTHRINF*)data;
	CRUINT8* pData = NULL;
	CRBOOL paused = CRTRUE;
	while (!auinf->stop)
	{
		CRSleep(auinf->hnsActualDuration / 2);
		if (!paused)
		{
			if (auinf->pause)
			{
				auinf->pAudioClient->lpVtbl->Stop(auinf->pAudioClient);
				paused = CRTRUE;
			}
			else
			{
				//查询剩余空闲空间
				if (FAILED(auinf->pAudioClient->lpVtbl->GetCurrentPadding(auinf->pAudioClient, &auinf->numFramesPadding)))
					return;
				auinf->numFramesAvailable = auinf->bufferFrameCount - auinf->numFramesPadding;
				HRESULT hr = 0;
				if (FAILED(hr = auinf->pRenderClient->lpVtbl->GetBuffer(auinf->pRenderClient, auinf->numFramesAvailable, &pData)))
					return;
				auinf->cbk(pData, auinf->numFramesAvailable, auinf->numFramesAvailable * auinf->inf->BlockAlign);
				if (FAILED(auinf->pRenderClient->lpVtbl->ReleaseBuffer(auinf->pRenderClient, auinf->numFramesAvailable, 0)))
					return;
			}
		}
		else
		{
			if (!auinf->pause)
			{
				auinf->pAudioClient->lpVtbl->Start(auinf->pAudioClient);
				paused = CRFALSE;
			}
		}
	}
	CRSleep(auinf->hnsActualDuration / 2);
	auinf->pAudioClient->lpVtbl->Stop(auinf->pAudioClient);
	auinf->pDevice->lpVtbl->Release(auinf->pDevice);
	auinf->pAudioClient->lpVtbl->Release(auinf->pAudioClient);
	auinf->pRenderClient->lpVtbl->Release(auinf->pRenderClient);
}

#elif defined CR_LINUX
static inline CRBOOL _create_device_(AUTHRINF* thinf, CRWWINFO* inf)
{
	CRINT32 dir = 0;
	CRUINT32 val = 0;
	snd_pcm_uframes_t periodsize;
	snd_pcm_hw_params_t* params = NULL;
	//获取设备句柄
	if (snd_pcm_open(&thinf->handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
		goto Failed;
	snd_pcm_hw_params_malloc(&params);
	snd_pcm_hw_params_any(thinf->handle, params);
	thinf->alsa_can_pause = snd_pcm_hw_params_can_pause(params);
	snd_pcm_hw_params_set_access(thinf->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	//
	if (inf->BitsPerSample == 8)
		snd_pcm_hw_params_set_format(thinf->handle, params, SND_PCM_FORMAT_U8);
	else
		snd_pcm_hw_params_set_format(thinf->handle, params, SND_PCM_FORMAT_S16_LE);
	//
	snd_pcm_hw_params_set_channels(thinf->handle, params, inf->NumChannels);
	val = inf->SampleRate;
	snd_pcm_hw_params_set_rate_near(thinf->handle, params, &val, &dir);
	periodsize = 1024;
	snd_pcm_hw_params_set_period_size(thinf->handle, params, periodsize, 0);
	thinf->frames = periodsize << 1;
	snd_pcm_hw_params_set_buffer_size(thinf->handle, params, thinf->frames);
	if (snd_pcm_hw_params(thinf->handle, params) < 0)
		goto Failed;
	snd_pcm_hw_params_free(params);
	return CRTRUE;
Failed:
	if (params)
		snd_pcm_hw_params_free(params);
	return CRFALSE;
}

static void _inner_audio_thread_(void* data, CRTHREAD idThis)
{
	AUTHRINF* auinf = (AUTHRINF*)data;
	CRBOOL paused = CRTRUE;
	CRINT32 rc;
	CRUINT8* pData = CRAlloc(NULL, auinf->frames * auinf->inf->BlockAlign);
	if (!pData)
		return;
	while (!auinf->stop)
	{
		if (!paused)
		{
			if (auinf->pause)
			{
				if (auinf->alsa_can_pause)
					snd_pcm_pause(auinf->handle, 1);
				else
					snd_pcm_drop(auinf->handle);
				paused = CRTRUE;
			}
			else
			{
				auinf->cbk(pData, auinf->frames, auinf->frames * auinf->inf->BlockAlign);
				while (rc = snd_pcm_writei(auinf->handle, pData, auinf->frames) < 0)
				{
					if (rc == -EPIPE)
						snd_pcm_prepare(auinf->handle);
				}
			}
		}
		else
		{
			if (!auinf->pause)
			{
				if (auinf->alsa_can_pause)
					snd_pcm_pause(auinf->handle, 0);
				else
					snd_pcm_prepare(auinf->handle);
				paused = CRFALSE;
			}
			else
				CRSleep(1);
		}
	}
	snd_pcm_drain(auinf->handle);
	snd_pcm_close(auinf->handle);
	CRAlloc(pData, 0);
}
#endif

#ifdef CR_WINDOWS
CRAPI CRAUDIO CRAudioCreate(CRAudioStreamCbk cbk, CRWWINFO* inf)
{
	if (!cbk)
	{
		CR_LOG_WAR("auto", "callback function is NULL");
		return NULL;
	}
	if (!inf)
	{
		CR_LOG_WAR("auto", "invalid inf");
		return NULL;
	}
	AUTHRINF *thinf = CRAlloc(NULL, sizeof(AUTHRINF));
	if (!thinf)
	{
		CR_LOG_ERR("auto", "bad alloc");
		return NULL;
	}
	thinf->cbk = cbk;
	thinf->inf = inf;
	thinf->stop = CRFALSE;
	thinf->pause = CRTRUE;  //初始暂停
	thinf->pDevice = NULL;
	thinf->pAudioClient = NULL;
	thinf->pRenderClient = NULL;
	thinf->hnsActualDuration = 0;
	thinf->bufferFrameCount = 0;
	thinf->numFramesAvailable = 0;
	thinf->numFramesPadding = 0;
	thinf->offs = 0;
	//
	if (!_inner_create_device_(thinf, inf))
	{
		CR_LOG_ERR("auto", "failed create device");
		CRAlloc(thinf, 0);
		goto Failed;
	}
	BYTE *pData = NULL;
	if (FAILED(thinf->pRenderClient->lpVtbl->GetBuffer(
		thinf->pRenderClient,
		thinf->bufferFrameCount,
		&pData)))
		goto Failed;
	//写缓冲
	thinf->cbk(pData, thinf->numFramesAvailable, thinf->numFramesAvailable * thinf->inf->BlockAlign);
	if (FAILED(thinf->pRenderClient->lpVtbl->ReleaseBuffer(
		thinf->pRenderClient,
		thinf->bufferFrameCount,
		0)))
		goto Failed;
	//
	thinf->idThis = CRThread(_inner_audio_thread_, thinf);
	return thinf;
Failed:
	if (thinf->pDevice)
		thinf->pDevice->lpVtbl->Release(thinf->pDevice);
	if (thinf->pAudioClient)
		thinf->pAudioClient->lpVtbl->Release(thinf->pAudioClient);
	if (thinf->pRenderClient)
		thinf->pRenderClient->lpVtbl->Release(thinf->pRenderClient);
	CR_LOG_ERR("auto", "an error heppened while creating audio device");
	return NULL;
}
#elif defined CR_LINUX
CRAPI CRAUDIO CRAudioCreate(CRAudioStreamCbk cbk, CRWWINFO* inf)
{
	if (!cbk)
	{
		CR_LOG_WAR("auto", "callback function is NULL");
		return NULL;
	}
	if (!inf)
	{
		CR_LOG_WAR("auto", "invalid inf");
		return NULL;
	}
	AUTHRINF *thinf = CRAlloc(NULL, sizeof(AUTHRINF));
	if (!thinf)
	{
		CR_LOG_ERR("auto", "bad alloc");
		return NULL;
	}
	thinf->stream = CRTRUE;
	thinf->cbk = cbk;
	thinf->inf = inf;
	thinf->pause = CRTRUE;
	if (!_create_device_(thinf, inf))
		goto Failed;
	thinf->idThis = CRThread(_inner_audio_thread_, thinf);
	return thinf;
Failed:
	if (thinf->handle)
		snd_pcm_close(thinf->handle);
	CR_LOG_ERR("auto", "an error heppened while creating audio device");
	return NULL;
}
#endif

static CRBOOL _inner_compare_chars_(const char* chars1, const char* chars2, CRUINT32 len)
{
	for (int i = 0; i < len; i++)
		if (chars1[i] != chars2[i]) return CRFALSE;
	return CRTRUE;
}

CRAPI CRBOOL CRLoadWW(const CRCHAR* path, CRSTRUCTURE out, CRWWINFO *inf)
{
	if (!inf)
	{
		CR_LOG_ERR("auto", "invalid wwinf");
		return CRFALSE;
	}
	FILE *fp = fopen(path, "rb");
	if (!fp)
	{
		CR_LOG_WAR("auto", "couldn't open wav file");
		return CRFALSE;
	}
	CRWWHEADER header;
	CRWWBLOCK block;
	fread(&header, sizeof(CRWWHEADER), 1, fp);
	//假如不符合说明文件出错了
	if (!_inner_compare_chars_((CRUINT8*)&header.whole.ChunkID, "RIFF", 4)) goto Failed;
	if (!_inner_compare_chars_((CRUINT8*)&header.format, "WAVE", 4)) goto Failed;
	//
	fread(&block, sizeof(block), 1, fp);
	while (!_inner_compare_chars_((CRUINT8*)&block.ChunkID, "data", 4))
	{
		fseek(fp, block.ChunkSize, SEEK_CUR);
		if (!fread(&block, sizeof(block), 1, fp))
			break;
	}
	if (!_inner_compare_chars_((CRUINT8*)&block.ChunkID, "data", 4))
		goto Failed;
	//现在是正常加载情况
	//
	CRUINT8* buffer = CRAlloc(NULL, block.ChunkSize);
	if (!buffer)
	{
		fclose(fp);
		CR_LOG_ERR("auto", "bad alloc");
		return CRFALSE;
	}
	if (fread(buffer,1 ,block.ChunkSize, fp) != block.ChunkSize) //然后就遇到不正常的情况了
	{
		CRAlloc(buffer, 0);
		goto Failed;
	}
	CRCODE code = CRDynSetup(out, buffer, block.ChunkSize);
	CRAlloc(buffer, 0);
	//
	fclose(fp);
	memcpy(inf, &header.inf, sizeof(CRWWINFO));
	//
	return CRTRUE;
Failed:
	CR_LOG_WAR("auto", "invalid file");
	return CRFALSE;
}

CRAPI void CRAudioClose(CRAUDIO play)
{
	AUTHRINF *pInner = (AUTHRINF*)play;
	if (pInner) pInner->stop = CRTRUE;
	CRWaitThread(pInner->idThis);
	CRAlloc(pInner, 0);
}

CRAPI void CRAudioPause(CRAUDIO play)
{
	AUTHRINF *pInner = (AUTHRINF*)play;
	if (pInner) pInner->pause = CRFALSE;
}

CRAPI void CRAudioResume(CRAUDIO play)
{
	AUTHRINF *pInner = (AUTHRINF*)play;
	if (pInner) pInner->pause = CRFALSE;
}