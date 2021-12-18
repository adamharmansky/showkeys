#include "config.h"
#include "x.h"

Display* display;
Drawable window;
int screen;
cairo_surface_t* surface;
cairo_t* cairo;
unsigned int width = initial_width, height = initial_height;
unsigned int winx = 0, winy = 0;

void
xinit() {
	XColor border_color, bruh;
	unsigned int i;
	Atom stop_atom;
	XineramaScreenInfo * info;
	unsigned int n;
	int di;
	Window w;
	XWindowAttributes wa;
	XSetWindowAttributes swa = {
		.override_redirect = True,
		.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask
	};

	display = XOpenDisplay(NULL);
	screen = DefaultScreen(display);

	if (info = XineramaQueryScreens(display, &n)) {
		XGetInputFocus(display, &w, &di);
		XGetWindowAttributes(display, w, &wa);
		for (i = 0; i < n; i++)
			if (info[i].x_org <= wa.x + wa.width/2 && info[i].y_org <= wa.y + wa.height/2 && info[i].x_org + info[i].width >= wa.x + wa.width/2 && info[i].y_org + info[i].height >= wa.y + wa.height/2) break;
		if (i < n) {
			winx = info[i].x_org + (info[i].width - width)/2;
			winy = info[i].y_org + (info[i].height - height)/2;
		} else {
			winx = info[0].x_org + (info[0].width - width)/2;
			winy = info[0].y_org + (info[0].height - height)/2;
		}
	}


	window = XCreateWindow(display, DefaultRootWindow(display), winx, winy,
		width, height, 2, CopyFromParent, CopyFromParent, CopyFromParent,
		CWOverrideRedirect | CWEventMask, &swa);
	XAllocNamedColor(display, DefaultColormap(display, screen), color_boder, &border_color, &bruh);
	XSetWindowBorder(display, window, border_color.pixel);
	XMapWindow(display, window);
	while (XGrabKeyboard(display, DefaultRootWindow(display), True, GrabModeAsync, GrabModeAsync, CurrentTime));

	surface = cairo_xlib_surface_create(display, window, DefaultVisual(display, screen), width, height);
	cairo_xlib_surface_set_size(surface, width, height);
	cairo = cairo_create(surface);

	stop_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &stop_atom, 1);
}
