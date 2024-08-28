/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-23 14:46:30
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-06-26 15:07:26
 * @FilePath: \CrystalThread\src\crthread.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <ThrDefs.h>
#include <CrystalAlgorithms.h>

#ifdef CR_WINDOWS
#include <Windows.h>
#include <process.h>
#elif defined CR_LINUX
#include <pthread.h>
#include <unistd.h>
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

extern CRLVOID currentID;
extern CRSTRUCTURE threadTree;  //tree
extern CRSTRUCTURE availableID;  //linear

typedef struct crthread_inner
{
    CRThreadFunction func;
    CRLVOID userData;
    CRTHREAD idThis;
#ifdef CR_WINDOWS
    HWND thread;
    unsigned int threadIDw;
#elif defined CR_LINUX
    pthread_t thread;
#endif
}CRTHREADINNER, *PCRTHREADINNER;

typedef struct crlock_inner
{
#ifdef CR_WINDOWS
    CRITICAL_SECTION cs;
#elif defined CR_LINUX
    pthread_mutex_t cs;
#endif
    CRBOOL lock;
}CRLOCKINNER, *PCRLOCKINNER;

CRAPI void CRSleep(CRUINT64 ms)
{
    #ifdef CR_WINDOWS
    timeBeginPeriod(1);
    SleepEx((DWORD)ms, TRUE);
    timeEndPeriod(1);
    #elif defined CR_LINUX
    usleep(ms * 1000);
    #endif
}

static void* _inner_thread_(void* lp)
{
    PCRTHREADINNER pInner = lp;
    pInner->func(pInner->userData, pInner->idThis);
    CRTreeGet(threadTree, NULL, (CRUINT64)(pInner->idThis));
    CRLinPut(availableID, pInner->idThis, 0);
    CRAlloc(pInner, 0);
    return 0;
}

CRAPI CRTHREAD CRThread(CRThreadFunc func, CRLVOID data)
{
    if (!func)
    {
        CR_LOG_WAR("auto", "You need to provide a valid function rather than \"NULL\"");
        return 0;
    }
    if (!threadTree || !availableID)
    {
        CR_LOG_ERR("auto", "Please call \"CrystalThreadInit(...)\" first");
        return 0;
    }
    PCRTHREADINNER pInner = CRAlloc(NULL, sizeof(CRTHREADINNER));
    if (!pInner)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return 0;
    }
    if (!CRLinGet(availableID, &(pInner->idThis), 0))
        pInner->idThis = (CRTHREAD)currentID++;
    pInner->func = func;
    pInner->userData = data;
    CRTreePut(threadTree, pInner, (CRUINT64)(pInner->idThis));
    #ifdef CR_WINDOWS
    pInner->thread = (HWND)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)_inner_thread_, pInner, 0, &(pInner->threadIDw));
    if (pInner->thread) CloseHandle(pInner->thread);
    #elif defined CR_LINUX
    pthread_create(&(pInner->thread), NULL, _inner_thread_, pInner);
    if (pInner->thread) pthread_detach(pInner->thread);
    #endif
    return pInner->idThis;
}

CRAPI void CRWaitThread(CRTHREAD thread)
{
    while (CRTreeSeek(threadTree, NULL, (CRUINT64)thread)) CRSleep(1);
}

CRAPI CRLOCK CRLockCreate(void)
{
    PCRLOCKINNER pInner = CRAlloc(NULL, sizeof(CRLOCKINNER));
    if (!pInner)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return pInner;
    }
    InitializeCriticalSection(&(pInner->cs));
    pInner->lock = CRFALSE;
    return pInner;
}

CRAPI void CRLockRelease(CRLOCK lock)
{
    PCRLOCKINNER pInner = lock;
    if (!pInner)
    {
        CR_LOG_WAR("auto", "invalid lock");
        return;
    }
    while (pInner->lock) CRSleep(1);
    DeleteCriticalSection(&(pInner->cs));
    CRAlloc(pInner, 0);
}

CRAPI void CRLock(CRLOCK lock)
{
    PCRLOCKINNER pInner = lock;
    if (!pInner)
    {
        CR_LOG_WAR("auto", "invalid lock");
        return;
    }
Block:
    while (pInner->lock) CRSleep(1);
    EnterCriticalSection(&(pInner->cs));
    if (pInner->lock)
    {
        LeaveCriticalSection(&(pInner->cs));
        goto Block;
    }
    pInner->lock = CRTRUE;
    LeaveCriticalSection(&(pInner->cs));
}

CRAPI void CRUnlock(CRLOCK lock)
{
    PCRLOCKINNER pInner = lock;
    if (!pInner)
    {
        CR_LOG_WAR("auto", "invalid lock");
        return;
    }
    pInner->lock = CRFALSE;
}