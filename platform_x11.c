/* 
 * Note: Do not compile this file!
 * It is included in game.c
 * game.c should be the only file you compile
 */

#ifdef __linux__

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "game.h"
#include "sleep_x11.h"

struct Platform
{
    NB_FLT   framecounter;
    NB_BOOL  closing;
    Window   window;
    NB_RGB   buffer[NB_WIDTH * NB_HEIGHT];
    Display* display; // connection to X server
    Atom wm_delete_window; // info to allow us to handle closing the window
} platform;

void ls_linux_poll(void);

void ls_linux_run(void)
{
    NB_INT x, y;
    //XSetWindowAttributes attributes;

    platform.closing = NB_FALSE;
    platform.framecounter = 0;

    /* open window */
    platform.display = XOpenDisplay(NULL); // ! should check for errors on all these
    Window root = DefaultRootWindow(platform.display);
    platform.window = XCreateSimpleWindow(platform.display, root, 0, 0, NB_WIDTH * 4, NB_HEIGHT * 4, 0, 0, 0x00000000);

    //attributes.event_mask = 0;
    //attributes.event_mask |= KeyPressMask;
    //attributes.event_mask |= KeyReleaseMask;
    //XChangeWindowAttributes(platform.display, platform.window, CWEventMask, &attributes);
    XStoreName(platform.display, platform.window, NB_TITLE);
    XMapWindow(platform.display, platform.window);

    /* get info to handle window close event since x11 doesn't have that natively... */
    platform.wm_delete_window = XInternAtom(platform.display, "WM_DELETE_WINDOW", NB_FALSE);
    XSetWMProtocols(platform.display, platform.window, &platform.wm_delete_window, 1);

    /* get screen buffer */
    // pixmap?

    /* run the game */
    nb_init();
    while (!platform.closing)
    {
        nb_step();
        
        /* Copy Palette Screen data to our RGB buffer */

        /* Copy to buffer */

        /* send Expose event  */

        /* poll events */
        nb_linux_poll();

        /* faking v-sync */
        //platform.framecounter += (1000.0 / NB_FRAMERATE);
        //sleep((NB_INT)(platform.framecounter / 1000.0));
        //platform.framecounter -= (NB_INT)platform.framecounter;
    }
    
    /* shutdown */
    XDestroyWindow(platform.display, platform.window);
    XCloseDisplay(platform.display);
}

void ls_linux_poll(void)
{
    XEvent event;

    /* loop over events until there are none left */
    while (XCheckWindowEvent(platform.display, platform.window, KeyPressMask | KeyReleaseMask, &event))
    {
        switch(event.type)
        {
            case ClientMessage:
                if(event.xclient.data.l[0] == platform.wm_delete_window)
                {
                    platform.closing = NB_TRUE;
                }
                break;
            case KeyPress:
                KeySym key = XKeysymToKeycode(platform.display, event.xkey.keycode);
                
                if (key == XK_d || key == XK_Right) nb_game.btn[NB_RIGHT] = NB_TRUE;
                if (key == XK_a || key == XK_Left) nb_game.btn[NB_LEFT] = NB_TRUE;
                if (key == XK_w || key == XK_Up) nb_game.btn[NB_UP] = NB_TRUE;
                if (key == XK_s || key == XK_Down) nb_game.btn[NB_DOWN] = NB_TRUE;
                break;
            case KeyRelease:
                KeySym key = XKeysymToKeycode(platform.display, event.xkey.keycode);
                
                if (key == XK_d || key == XK_Right) nb_game.btn[NB_RIGHT] = NB_FALSE;
                if (key == XK_a || key == XK_Left) nb_game.btn[NB_LEFT] = NB_FALSE;
                if (key == XK_w || key == XK_Up) nb_game.btn[NB_UP] = NB_FALSE;
                if (key == XK_s || key == XK_Down) nb_game.btn[NB_DOWN] = NB_FALSE;
                break;
            case Expose: // equivalent to WM_PAINT
                
                break;
        }
    }
}

/* without libc wrapping it, _start() is the correct entry point */
void _start(void)
{
    ls_linux_run();
}

#endif /* __linux__ */