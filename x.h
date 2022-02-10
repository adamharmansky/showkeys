#ifndef _SK_X_H
#define _SK_X_H

#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

extern Display* display;
extern Drawable window;
extern int screen;
extern cairo_surface_t* surface;
extern cairo_t* cairo;
extern unsigned int width, height;
extern unsigned int winx, winy;
extern XIC xic;

void xinit();
#endif
