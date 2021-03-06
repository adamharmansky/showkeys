#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "x.h"

Display* display;
Drawable window;
int screen;
cairo_surface_t* surface;
cairo_t* cairo;
unsigned int width = initial_width, height = initial_height;
unsigned int winx = 0, winy = 0;
XIC xic;

void
xinit() {
/*
 * ⠀⠀⠀⢀⡔⠋⢉⠩⡉⠛⠛⠛⠉⣉⣉⠒⠒⡦⣄
 * ⠀⠀⢀⠎⠀⠀⠠⢃⣉⣀⡀⠂⠀⠀⠄⠀⠀⠀⠀⢱
 * ⠀⡰⠟⣀⢀⣒⠐⠛⡛⠳⢭⠆⠀⠤⡶⠿⠛⠂⠀⢈⠳⡀
 * ⢸⢈⢘⢠⡶⢬⣉⠉⠀⠀⡤⠄⠀⠀⠣⣄⠐⠚⣍⠁⢘⡇
 * ⠈⢫⡊⠀⠹⡦⢼⣍⠓⢲⠥⢍⣁⣒⣊⣀⡬⢴⢿⠈⡜
 * ⠀⠀⠹⡄⠀⠘⢾⡉⠙⡿⠶⢤⣷⣤⣧⣤⣷⣾⣿⠀⡇
 * ⠀⠀⠀⠘⠦⡠⢀⠍⡒⠧⢄⣀⣁⣀⣏⣽⣹⠽⠊⠀⡇
 * ⠀⠀⠀⠀⠀⠈⠑⠪⢔⡁⠦⠀⢀⡤⠤⠤⠄⠀⠠⠀⡇
 * ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠑⠲⠤⠤⣀⣀⣀⣀⣀⠔⠁
 */
	XIM xim;
	XColor border_color, bruh;
	unsigned int i;
	Atom stop_atom;
	XineramaScreenInfo * info;
	unsigned int n;
	int di;
	int curx, cury;
	union {Window w; int x;} junk;
	Window w;
	XWindowAttributes wa;
	XSetWindowAttributes swa = {
		.override_redirect = True,
		.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask
	};

	display = XOpenDisplay(NULL);
	if (!display) {
		fprintf(stderr, "Unable to open display\n");
		exit(1);
	}
	screen = DefaultScreen(display);

	if (info = XineramaQueryScreens(display, &n)) {
		XGetInputFocus(display, &w, &di);
		XGetWindowAttributes(display, w, &wa);
		for (i = 0; i < n; i++)
			if (info[i].x_org <= wa.x + wa.width/2 && info[i].y_org <= wa.y + wa.height/2 && info[i].x_org + info[i].width >= wa.x + wa.width/2 && info[i].y_org + info[i].height >= wa.y + wa.height/2)
				break;
		if (i < n) {
			winx = info[i].x_org + (info[i].width - width)/2;
			winy = info[i].y_org + (info[i].height - height)/2;
		} else {
			XQueryPointer(display, DefaultRootWindow(display), (Window*)&junk, (Window*)&junk, &curx, &cury, (int*)&junk, (int*)&junk, (unsigned int*)&junk);
			for (i = 0; i < n; i++)
				if (info[i].x_org <= curx && info[i].y_org <= cury && info[i].x_org + info[i].width >= curx && info[i].y_org + info[i].height >= cury)
					break;
			winx = info[i].x_org + (info[i].width - width)/2;
			winy = info[i].y_org + (info[i].height - height)/2;
		}
	}


	window = XCreateWindow(display, DefaultRootWindow(display), winx, winy,
		width, height, 2, CopyFromParent, CopyFromParent, CopyFromParent,
		CWOverrideRedirect | CWEventMask, &swa);
	XAllocNamedColor(display, DefaultColormap(display, screen), color_boder, &border_color, &bruh);
	XSetWindowBorder(display, window, border_color.pixel);
	/* DO NOT MAP THE WINDOW YET, AS IT WILL BE RESIZED LATER */
	//XMapWindow(display, window);
	while (XGrabKeyboard(display, DefaultRootWindow(display), True, GrabModeAsync, GrabModeAsync, CurrentTime));

	surface = cairo_xlib_surface_create(display, window, DefaultVisual(display, screen), width, height);
	cairo_xlib_surface_set_size(surface, width, height);
	cairo = cairo_create(surface);

	stop_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &stop_atom, 1);

	/* initialize input */
	if ((xim = XOpenIM(display, NULL, NULL, NULL)) == NULL) {
		fprintf(stderr, "Unable to open input method\n");
		exit(1);
	}
	xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window, XNFocusWindow, window, NULL);
}
