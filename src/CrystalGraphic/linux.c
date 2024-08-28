/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-06-24 16:15:22
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-06-24 16:15:28
 * @FilePath: \Crystal-Graphic\src\linux.c
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#include <GraphicDfs.h>
#include "crgl.h"
#include "vk.h"

#ifdef CR_LINUX
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <GL/glx.h>
#include <string.h>
#include <CrystalThread.h>

static Display *pDisplay = NULL;
static Window rootWindow = 0;
static Cursor cursors[4];
#define CR_X_CURSOR_DEFAULT 0
#define CR_X_CURSOR_HAND    1

static CRINT64 windowCounter = 0;
CRLOCK lock = NULL;

typedef struct crwindow_inner
{
    Window win;
    Atom protocols_quit;
    CRUINT32 fps;
    PCRWindowProperties prop;
    CRPOINTU delta;
    CR_GL* pgl_ui;
    cr_vk vkui;
    cr_vk vkpaint;
    //
    XVisualInfo *vi;
    //
    CRWindowCallback funcs[CALLBACK_FUNCS_NUM];
    CRBOOL onProcess;
    CRBOOL drag;
    CRBOOL preClose;
    CRBOOL onPaint;
    //
    CRTHREAD eventThread;
    CRTHREAD uiThread;
    CRTHREAD paintThread;
    CRTHREAD moveControl;
}CRWINDOWINNER, *PCRWINDOWINNER;

static CRCODE _inner_empty_callback_(PCRWINDOWMSG msg)
{
    return 0;
}

static void _inner_process_msg_(PCRWINDOWINNER pInner)
{
    CRWINDOWMSG inf;
    XEvent event;
    while (!pInner->pgl_ui || !pInner->vkpaint) CRSleep(1);
    while (pInner->onProcess)
    {
        inf.window = pInner->win;
        XNextEvent(pDisplay, &event);
        if (event.xany.window != pInner->win)
        {
            XPutBackEvent(pDisplay, &event);
            CRSleep(1);
            continue;
        }
        switch (event.type)
        {
        case Expose:
            pInner->funcs[CRWINDOW_PAINT_CB](&inf);
            break;
        case ConfigureNotify:
            inf.w = event.xconfigure.width;
            inf.h = event.xconfigure.height - CRUI_TITLEBAR_PIXEL;
            _inner_set_size_(pInner->pgl_ui, inf.w, inf.h);
            _inner_cr_gl_resize_(pInner->pgl_ui);
            break;
        case MotionNotify:
            if (event.xbutton.x < CRUI_TITLEBAR_PIXEL && event.xbutton.y < CRUI_TITLEBAR_PIXEL)
                XDefineCursor(pDisplay, pInner->win, cursors[CR_X_CURSOR_HAND]);
            else
            {
                XDefineCursor(pDisplay, pInner->win, cursors[CR_X_CURSOR_DEFAULT]);
                if (pInner->drag)
                    XMoveWindow(pDisplay, pInner->win, event.xmotion.x_root - pInner->delta.x, event.xmotion.y_root - pInner->delta.y);
                else
                {
                    inf.status = CRUI_STAT_MOVE;
                    inf.x = event.xbutton.x;
                    inf.y = event.xbutton.y - CRUI_TITLEBAR_PIXEL;
                    pInner->funcs[CRWINDOW_MOUSE_CB](&inf);
                }
            }
            break;
        case ButtonPress:
            inf.x = event.xbutton.x;
            inf.y = event.xbutton.y - CRUI_TITLEBAR_PIXEL;
            if (inf.y > 0)
            {
                inf.status = CRUI_STAT_DOWN;
                if (event.xbutton.button == 1) inf.status |= CRUI_STAT_LEFT;
                else if (event.xbutton.button == 2) inf.status |= CRUI_STAT_RIGHT;
                pInner->funcs[CRWINDOW_MOUSE_CB](&inf);
            }
            else if (event.xbutton.button == 1)
			{
				if (inf.x > CRUI_TITLEBAR_PIXEL * 3)
				{
					pInner->delta.x = event.xbutton.x;
					pInner->delta.y = event.xbutton.y;
					pInner->drag = CRTRUE;
				}
				else if (inf.x > CRUI_TITLEBAR_PIXEL * 2)
				{}
				else if (inf.x > CRUI_TITLEBAR_PIXEL)
				{}
				else
					pInner->preClose = CRTRUE;
			}
            break;
        case ButtonRelease:
            inf.x = event.xbutton.x;
            inf.y = event.xbutton.y - CRUI_TITLEBAR_PIXEL;
            if (inf.y > 0)
            {
                inf.status = CRUI_STAT_UP;
                if (event.xbutton.button == 1) inf.status |= CRUI_STAT_LEFT;
                else if (event.xbutton.button == 2) inf.status |= CRUI_STAT_RIGHT;
                pInner->funcs[CRWINDOW_MOUSE_CB](&inf);
            }
            else if (event.xbutton.button == 1)
			{
				if (inf.x > CRUI_TITLEBAR_PIXEL * 3)
				{
					pInner->delta.x = event.xbutton.x;
					pInner->delta.y = event.xbutton.y;
					pInner->drag = CRTRUE;
				}
				else if (inf.x > CRUI_TITLEBAR_PIXEL * 2)
				{}
				else if (inf.x > CRUI_TITLEBAR_PIXEL)
				{}
				else if (pInner->preClose)
                {
                    event.type = ClientMessage;
                    event.xany.window = pInner->win;
                    event.xclient.data.l[0] = pInner->protocols_quit;
                    XPutBackEvent(pDisplay, &event);
                }
			}
            pInner->drag = CRFALSE;
			pInner->preClose = CRTRUE;
            break;
        case KeyPress:
			inf.status = CRUI_STAT_DOWN;
			inf.keycode = event.xkey.keycode;
			pInner->funcs[CRWINDOW_KEY_CB](&inf);
			break;
		case KeyRelease:
			inf.status = CRUI_STAT_UP;
			inf.keycode = event.xkey.keycode;
			pInner->funcs[CRWINDOW_KEY_CB](&inf);
			break;
        case ClientMessage:
            if(event.xclient.data.l[0] == pInner->protocols_quit)
            {
                if (!pInner->funcs[CRWINDOW_QUIT_CB](&inf))
                {
                    XSelectInput(pDisplay, pInner->win, NoEventMask);
                    pInner->onProcess = CRFALSE;
                    while(pInner->pgl_ui || pInner->vkpaint) CRSleep(1);
                    XFlush(pDisplay);
                    XDestroyWindow(pDisplay, pInner->win);
                }
                CR_LOG_IFO("auto", "Close window");
            }
            break;
        default:
            break;
        }
        XFlush(pDisplay);
    }
    pInner->prop->window = (CRWINDOW)NULL;
    CRAlloc(pInner, 0);
    //
    CRLock(lock);
    windowCounter--;
    CRUnlock(lock);
}

/*
* 无边框窗口需要的东西
*
* 这些是用来创建无边框窗口使用的
* 内部的一些数据结构
*/

#define MWM_HINTS_FUNCTIONS (1L << 0)
#define MWM_HINTS_DECORATIONS  (1L << 1)

#define MWM_FUNC_ALL (1L << 0)
#define MWM_FUNC_RESIZE (1L << 1)
#define MWM_FUNC_MOVE (1L << 2)
#define MWM_FUNC_MINIMIZE (1L << 3)
#define MWM_FUNC_MAXIMIZE (1L << 4)
#define MWM_FUNC_CLOSE (1L << 5)

#define PROP_MWM_HINTS_ELEMENTS 5
typedef struct _mwmhints
{
    uint32_t flags;
    uint32_t functions;
    uint32_t decorations;
    int32_t  input_mode;
    uint32_t status;
}MWMHints;
//
static void _inner_paint_ui_thread_(CRLVOID data, CRTHREAD idThis);
static void _inner_paint_thread_(CRLVOID data, CRTHREAD idThis);
//
static void _inner_window_thread_(CRLVOID data, CRTHREAD idThis)
{
    PCRWINDOWINNER pInner = data;
    GLint att[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 24,
		GLX_SAMPLE_BUFFERS,
		None
		};
	Colormap cmap;
	XSetWindowAttributes swa;
	GLXContext glc;
	XWindowAttributes gwa;

    pInner->vi = glXChooseVisual(pDisplay, 0, att);
    cmap = XCreateColormap(pDisplay, rootWindow, pInner->vi->visual, AllocNone);
    swa.colormap = cmap;
    //选择关心的事件
    swa.event_mask = ExposureMask
	| KeyPressMask | ButtonPressMask
	| KeyReleaseMask | ButtonReleaseMask
	| PointerMotionMask
	| StructureNotifyMask;

    pInner->win = XCreateWindow(
        pDisplay, rootWindow,
        pInner->prop->x, pInner->prop->y,
        pInner->prop->w, pInner->prop->h,
        0, pInner->vi->depth, InputOutput,
        pInner->vi->visual, CWColormap | CWEventMask,
        &swa
    );
    XStoreName(pDisplay, pInner->win, pInner->prop->title);
    //制作无边框窗口
    MWMHints mwmhints;
	Atom prop;
	memset(&mwmhints,0, sizeof(MWMHints));
	prop = XInternAtom(pDisplay, "_MOTIF_WM_HINTS", False);
	mwmhints.flags = MWM_HINTS_DECORATIONS;
	mwmhints.decorations = 0;
	XChangeProperty(
	    pDisplay, pInner->win, prop, prop, 32,
	    PropModeReplace, (unsigned char*)&mwmhints,
	    PROP_MWM_HINTS_ELEMENTS
	);
    //

    XMapWindow(pDisplay, pInner->win);
    pInner->protocols_quit = XInternAtom(pDisplay, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(pDisplay, pInner->win, &(pInner->protocols_quit), 1);
    CR_LOG_IFO("auto", "Create Window");
    /**/
    pInner->uiThread = CRThread(_inner_paint_ui_thread_, pInner);
    pInner->paintThread = CRThread(_inner_paint_thread_, pInner);
    _inner_process_msg_(pInner);
    /**/
}

static void _inner_paint_ui_thread_(CRLVOID data, CRTHREAD idThis)
{
    PCRWINDOWINNER pInner = (PCRWINDOWINNER)data;
    pInner->pgl_ui= _inner_create_cr_gl_ui_(pDisplay, pInner->vi, pInner->win);
    CR_LOG_IFO("auto", "OpenGL Version: %s", pInner->pgl_ui->glGetString(GL_VERSION));
    while (pInner->onProcess)
    {
        _inner_cr_gl_paint_ui_(pInner->pgl_ui);
        CRSleep(100);
    }
    //释放
    _inner_delete_cr_gl_(pInner->pgl_ui);
    pInner->pgl_ui = NULL;
}

static void _inner_paint_thread_(CRLVOID data, CRTHREAD idThis)
{
    PCRWINDOWINNER pInner = (PCRWINDOWINNER)data;
    while (!pInner->pgl_ui) CRSleep(1);
    pInner->vkpaint = _inner_create_vk_(pDisplay, pInner->win);
    while (pInner->onProcess)
    {
        CRSleep(1);
    }
    //释放
    _inner_release_vk_(pInner->vkpaint);
    pInner->vkpaint = NULL;
}

CRAPI CRINT64 CRWindowCounter(void)
{
    return windowCounter;
}

CRAPI void CRCreateWindow(PCRWindowProperties prop)
{
    if (!prop)
    {
        CR_LOG_ERR("auto", "nullptr");
        return;
    }
    prop->window = (CRWINDOW)NULL;
    PCRWINDOWINNER pInner = CRAlloc(NULL, sizeof(CRWINDOWINNER));
    if (!pInner)
    {
        CR_LOG_ERR("auto", "bad alloc");
        return;
    }
    for (int i = 0; i < CALLBACK_FUNCS_NUM; i++)
        pInner->funcs[i] = _inner_empty_callback_;
    pInner->onProcess = CRTRUE;
    pInner->drag = CRFALSE;
    pInner->preClose = CRFALSE;
    pInner->prop = prop;
    pInner->pgl_ui = NULL;
    pInner->vkui = NULL;
    pInner->vkpaint = NULL;
    pInner->eventThread = CRThread(_inner_window_thread_, pInner);
    //
    CRLock(lock);
    windowCounter++;
    CRUnlock(lock);
    //
    prop->window = (CRWINDOW)pInner;
}

CRAPI void CRCloseWindow(CRWINDOW window)
{
    if (!window)
    {
        CR_LOG_WAR("auto", "invalid window");
        return;
    }
    PCRWINDOWINNER pInner = (PCRWINDOWINNER)window;
    XEvent event;
    event.type = ClientMessage;
    event.xany.window = pInner->win;
    event.xclient.data.l[0] = pInner->protocols_quit;
    XPutBackEvent(pDisplay, &event);
}

CRAPI void CRSetWindowCbk(CRWINDOW window, CRUINT8 type, CRWindowCallback cbk)
{
    if (!window)
    {
        CR_LOG_WAR("auto", "invalid window");
        return;
    }
    PCRWINDOWINNER pInner = (PCRWINDOWINNER)window;
    if (cbk)
        pInner->funcs[type] = cbk;
    else
        pInner->funcs[type] = _inner_empty_callback_;
}

CRBOOL _inner_x_init_()
{
    if (pDisplay) return CRTRUE;
    pDisplay = XOpenDisplay(NULL);
    if (!pDisplay)
    {
        CR_LOG_ERR("auto", "Failed open display");
        return CRFALSE;
    }
    rootWindow = DefaultRootWindow(pDisplay);
    if (!rootWindow)
    {
        CR_LOG_ERR("auto", "Can't find default window");
        return CRFALSE;
    }
    cursors[CR_X_CURSOR_DEFAULT] = XCreateFontCursor(pDisplay, XC_left_ptr);
    cursors[CR_X_CURSOR_HAND] = XCreateFontCursor(pDisplay, XC_hand2);
    return CRTRUE;
}

void _inner_x_uninit_()
{
    if (pDisplay) XCloseDisplay(pDisplay);
    pDisplay = NULL;
    rootWindow = 0;
}

#endif