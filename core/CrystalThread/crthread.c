#include <CrystalThread.h>

void _cr_inner_do_nothing_thr_(void){};

static void* CRThreadFunListArr[] =
{
    _cr_inner_do_nothing_thr_, "CrystalThreadInit",
    _cr_inner_do_nothing_thr_, "CRSleep",
    _cr_inner_do_nothing_thr_, "CRThread",
    _cr_inner_do_nothing_thr_, "CRWaitThread",
    _cr_inner_do_nothing_thr_, "CRLockCreate",
    _cr_inner_do_nothing_thr_, "CRLockRelease",
    _cr_inner_do_nothing_thr_, "CRLock",
    _cr_inner_do_nothing_thr_, "CRUnlock",
    0
};

CRAPI void** CRThreadFunList = CRThreadFunListArr;