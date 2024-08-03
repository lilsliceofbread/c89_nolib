/* 
 * Note: Do not compile this file!
 * It is included in game.c
 * game.c should be the only file you compile
 */

#ifdef __linux__

#include "game.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <linux/time.h>
#include "syscalls.h"

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x20

#define MIN(a, b) ((a) < (b) ? a : b)
#define ALLOCATE(size) mmap(NULL, (size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
#define DEALLOCATE(addr, size) munmap((addr), (size))

static struct Platform
{
    NB_BOOL  closing;
    NB_RGB*  buffer;
    XImage*  image;   /* wrapper around our buffer to tell X11 how to handle it */
    Display* display; /* connection to X server */
    Window   window;  /* window handle */
    Atom     wm_delete_window; /* info to allow us to handle closing the window */
    Atom     wm_protocols;
    NB_DBL   time_offset;      /* timer offset to reduce floating point error (causes weird frame intervals) */
} platform;

void nb_x11_poll(void);
void nb_x11_create_image(void);
NB_DBL nb_x11_get_time(void);

void nb_x11_run(void)
{
    /* must declare variables at start of function in c89 */
    NB_INT x, y;
    NB_DBL time;
    XWindowAttributes attributes;

    platform.closing = NB_FALSE;
    platform.buffer = ALLOCATE(NB_WIDTH * NB_HEIGHT * sizeof(NB_RGB));

    /* setup timer for v-sync */
    platform.time_offset = 0;
    platform.time_offset = nb_x11_get_time();

    /* connect to X server */
    platform.display = XOpenDisplay(NULL);
    if(platform.display == NULL)
        return;

    /* open window */
    platform.window = XCreateSimpleWindow(platform.display, XDefaultRootWindow(platform.display),
                                          0, 0, NB_WIDTH * 4, NB_HEIGHT * 4,
                                          0, 0, 0);
    XStoreName(platform.display, platform.window, NB_TITLE);
    XMapWindow(platform.display, platform.window);
    XSelectInput(platform.display, platform.window, KeyPressMask | KeyReleaseMask | ExposureMask);

    /* get info to handle window close event since x11 doesn't have that natively... */
    platform.wm_protocols = XInternAtom(platform.display, "WM_PROTOCOLS", NB_FALSE);
    platform.wm_delete_window = XInternAtom(platform.display, "WM_DELETE_WINDOW", NB_FALSE);
    XSetWMProtocols(platform.display, platform.window, &platform.wm_delete_window, 1);

    /* create image (will probably be overwritten by nb_x11_create_image()) */
    XGetWindowAttributes(platform.display, platform.window, &attributes);
    platform.image = XCreateImage(platform.display, attributes.visual, attributes.depth,
                                  ZPixmap, 0, (char*)platform.buffer,
                                  NB_WIDTH, NB_HEIGHT, 32, NB_WIDTH * sizeof(NB_RGB));

    /* run the game */
    nb_init();
    while(!platform.closing)
    {
        nb_step();
        
        /* resize image (x11 doesn't give functions for that)*/
        nb_x11_create_image();

        /* put image in middle of screen */
        XGetWindowAttributes(platform.display, platform.window, &attributes);
        x = (NB_INT)((attributes.width - platform.image->width) / 2);
        y = (NB_INT)((attributes.height - platform.image->height) / 2);
        XPutImage(platform.display, platform.window, XDefaultGC(platform.display, 0), platform.image, 0, 0, x, y, platform.image->width, platform.image->height);

        /* poll events */
        nb_x11_poll();

        /* faking v-sync */
        time = nb_x11_get_time();
        while(1)
        {
            if(1000.0 / NB_FRAMERATE < nb_x11_get_time() - time)
                break;
        }
    }
    
    /* shutdown */
    if(platform.buffer != NULL)
        DEALLOCATE(platform.buffer, platform.image->width * platform.image->height * sizeof(NB_RGB));
    if(platform.image != NULL)
        XFree(platform.image);

    XDestroyWindow(platform.display, platform.window);
    XCloseDisplay(platform.display);
}

void nb_x11_poll(void)
{
    XEvent event;
    KeySym key;

    while(XPending(platform.display))
    {
        XNextEvent(platform.display, &event);
        switch(event.type)
        {
            case ClientMessage:
                if(event.xclient.message_type == platform.wm_protocols
                && event.xclient.data.l[0] == platform.wm_delete_window)
                    platform.closing = NB_TRUE;
                break;
            case KeyPress:
                key = XLookupKeysym(&event.xkey, 0);
                
                if (key == XK_d || key == XK_Right) nb_game.btn[NB_RIGHT] = NB_TRUE;
                if (key == XK_a || key == XK_Left) nb_game.btn[NB_LEFT] = NB_TRUE;
                if (key == XK_w || key == XK_Up) nb_game.btn[NB_UP] = NB_TRUE;
                if (key == XK_s || key == XK_Down) nb_game.btn[NB_DOWN] = NB_TRUE;
                break;

            case KeyRelease:
                key = XLookupKeysym(&event.xkey, 0);
                
                if (key == XK_d || key == XK_Right) nb_game.btn[NB_RIGHT] = NB_FALSE;
                if (key == XK_a || key == XK_Left) nb_game.btn[NB_LEFT] = NB_FALSE;
                if (key == XK_w || key == XK_Up) nb_game.btn[NB_UP] = NB_FALSE;
                if (key == XK_s || key == XK_Down) nb_game.btn[NB_DOWN] = NB_FALSE;
                break;
            case Expose:
                XClearWindow(platform.display, platform.window); /* prevent last copy of buffer from remaining in background */
                break;
        }
    }
}

void nb_x11_create_image(void)
{
    NB_INT x, y, i, j;
    NB_UINT new_width, new_height;
    NB_UINT size_multiplier;
    NB_UINT row_offset;
    NB_RGB curr_pixel;
    XWindowAttributes attributes;

    XGetWindowAttributes(platform.display, platform.window, &attributes);
    size_multiplier = MIN((NB_UINT)(attributes.width / NB_WIDTH), (NB_UINT)(attributes.height / NB_HEIGHT));

    /* window too small to display screen */
    if(size_multiplier == 0)
    {
        return;
    }

    new_width = NB_WIDTH * size_multiplier;
    new_height = NB_HEIGHT * size_multiplier;

    /* resize buffer and XImage */
    if(platform.image->width != new_width || platform.image->height != new_height)
    {
        if(platform.buffer != NULL)
        {
            DEALLOCATE(platform.buffer, platform.image->width * platform.image->height * sizeof(NB_RGB));
        }
        platform.buffer = ALLOCATE(new_width * new_height * sizeof(NB_RGB));

        if(platform.image != NULL)
        {
            XFree(platform.image);
        }
        /* could just edit the values of XImage? */
        platform.image = XCreateImage(platform.display, attributes.visual, attributes.depth,
                                      ZPixmap, 0, (char*)platform.buffer,
                                      new_width, new_height, 32, new_width * sizeof(NB_RGB));

        /* since we are replacing all pixels every frame, only need to clear window when changing size */
        XClearWindow(platform.display, platform.window);
    }

    /* copy and resize palette screen data to buffer (line by line, don't know if block by block would be faster) */
    /* 90% of computation goes to this loop according to callgrind */
    for (y = 0; y < NB_HEIGHT; y++)
    {
        for(i = 0; i < size_multiplier; i++)
        {
            /* repeat size_multiplier times */
            row_offset = (y * size_multiplier + i) * new_width;
            for (x = 0; x < NB_WIDTH; x++)
            {
                curr_pixel = nb_game.palette[nb_game.screen[x + y * NB_WIDTH]];
                for(j = 0; j < size_multiplier; j++)
                {
                    /* repeat size_multiplier times */
                    platform.buffer[(x * size_multiplier + j) + row_offset] = curr_pixel;
                }
            }
        }
    }
}

NB_DBL nb_x11_get_time(void)
{
    struct timeval os_time;

    gettimeofday(&os_time, NULL);    

    /* time in milliseconds */
    return ((NB_DBL)1000 * (NB_DBL)os_time.tv_sec + 0.001 * (NB_DBL)os_time.tv_usec) - platform.time_offset;
}

/* something about stack alignment idk i just copied this: https://stackoverflow.com/questions/2548486/compiling-without-libc*/
__attribute__((force_align_arg_pointer))
void _start(void)
{
    nb_x11_run();

    exit(0);
}

#endif /* __linux__ */