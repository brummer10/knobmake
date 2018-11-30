

#include <stdio.h>
#include <string.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// gcc -g knob_view.c  -lX11 `pkg-config --cflags --libs cairo` -o knobview 

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef max
#define max(x, y) ((x) < (y) ? (y) : (x))
#endif

// define controller type
typedef enum {
	KNOB,
	SWITCH,
} type;

// define controller position in window
typedef struct {
	int x;
	int y;
	int width;
	int height;
} alinment;

// define controller adjustment
typedef struct {
	float std_value;
	float value;
	float min_value;
	float max_value;
	float step;
} adjustment;

// controller struct
typedef struct {
	adjustment adj;
	alinment al;
	type tp;
} controller;

// resize window
typedef struct {
	double x;
	double y;
	double x1;
	double y1;
	double x2;
	double y2;
	double c;
} re_scale;

typedef struct {
	Display* display;
	Drawable win;
	Atom wm_delete_window;
	XEvent event;
	long event_mask;
	int width;
	int height;

	cairo_t *cr;
	cairo_surface_t *surface;
	cairo_surface_t *image;
	int w, h, s;
	re_scale rescale;

	controller knob;
	double start_value;
	int pos_x;
	int pos_y;
	
} viewport;

// redraw the window
static void _expose(viewport *v) {
	// get sate of knob and calculate the frame index to show
	double knobstate = (v->knob.adj.value - v->knob.adj.min_value) / (v->knob.adj.max_value - v->knob.adj.min_value);
	int findex = (int)(v->s * knobstate);

	// push and pop to avoid any flicker (offline drawing)
	cairo_push_group (v->cr);

	// scale window to user equest
	cairo_scale (v->cr, v->rescale.x, v->rescale.y);

	// draw background
	cairo_set_source_rgba (v->cr, 0.0, 0.0, 0.0, 1.0);
	cairo_rectangle(v->cr,0, 0, v->h, v->h);
	cairo_fill(v->cr);

	// rescale to origion
	cairo_scale (v->cr, v->rescale.x1, v->rescale.y1);
	// scale window to aspect ratio
	cairo_scale (v->cr, v->rescale.c, v->rescale.c);

	// draw knob image
	cairo_set_source_surface (v->cr, v->image, -v->h*findex, 0);
	cairo_rectangle(v->cr,0, 0, v->h, v->h);
	cairo_fill(v->cr);

	cairo_pop_group_to_source (v->cr);

	// finally paint to window
	cairo_paint (v->cr);
}

// send expose event to own window
static void send_expose(Display* display,Window win) {
	XEvent exppp;
	memset(&exppp, 0, sizeof(exppp));
	exppp.type = Expose;
	exppp.xexpose.window = win;
	XSendEvent(display,win,False,ExposureMask,&exppp);
	// we don't need to flush the desktop in a single threated application
	//XFlush(display);
}

// mouse wheel scroll event
static void scroll_event(controller *knob, int direction) {
	knob->adj.value = min(knob->adj.max_value,max(knob->adj.min_value, 
	  knob->adj.value + (knob->adj.step * direction)));
}

// mouse move while left button is pressed
static void motion_event(controller *knob, double start_value, int pos_y, int m_y) {
	if (knob->tp == SWITCH) return;
	static const double scaling = 0.5;
	// transfer any range to a range from 0 to 1 and get position in this range
	double knobstate = (start_value - knob->adj.min_value) /
					   (knob->adj.max_value - knob->adj.min_value);
	// calculate the step size to use in 0 . . 1
	double nsteps = knob->adj.step / (knob->adj.max_value-knob->adj.min_value);
	// calculate the new position in the range 0 . . 1
	double nvalue = min(1.0,max(0.0,knobstate - ((double)(pos_y - m_y)*scaling *nsteps)));
	// set new value to the knob in the knob range
	knob->adj.value = nvalue * (knob->adj.max_value-knob->adj.min_value) + knob->adj.min_value;
}

// left mouse button is pressed, generate a switch event, or set controller active
static void button1_event(controller *knob) {
	if (knob->tp != SWITCH) return;
	float value = (int)knob->adj.value ? 0.0 : 1.0;
	knob->adj.value = value;
}

static void resize_event(viewport *v) {
	// get new size
	v->width = v->event.xconfigure.width;
	v->height = v->event.xconfigure.height;
	// resize cairo surface
	cairo_xlib_surface_set_size( v->surface, v->width, v->height);
	// calculate scale factor
	v->rescale.x  = (double)v->width/v->h;
	v->rescale.y  = (double)v->height/v->h;
	// calculate rescale factor
	v->rescale.x1 = (double)v->h/v->width;
	v->rescale.y1 = (double)v->h/v->height;
	// calculate aspect ratio
	v->rescale.c = (v->rescale.x < v->rescale.y) ? v->rescale.x : v->rescale.y;
	// calculate rescale aspect ratio (ain't need here)
	v->rescale.x2 =  v->rescale.x / v->rescale.c;
	v->rescale.y2 = v->rescale.y / v->rescale.c;
}

int main(int argc, char* argv[])
{
	viewport v;
	v.display = XOpenDisplay(NULL);

	v.image = cairo_image_surface_create_from_png ("./knob.png");
	v.w = cairo_image_surface_get_width (v.image);
	v.h = cairo_image_surface_get_height (v.image);
	if (!v.w ||!v.h) {
		XCloseDisplay(v.display);
		fprintf(stderr, "./knob.png not found\n");
		return 1;
	}
	v.s = (v.w/v.h)-1;
	fprintf(stderr, "width %i height %i steps %i\n", v.w,v.h,v.s);

	v.knob = (controller) {{0.5,0.5,0.0,1.0, 0.01},{0,0,v.w,v.h}};
	v.knob.tp = (v.s<2) ? SWITCH : KNOB;
	v.knob.adj.step = (v.knob.tp == SWITCH) ? 1.0 : 0.01;

	v.win = XCreateWindow(v.display, DefaultRootWindow(v.display), 0, 0, v.h,v.h, 0,
						CopyFromParent, InputOutput, CopyFromParent, CopyFromParent, 0);

	v.event_mask = StructureNotifyMask|ExposureMask|KeyPressMask 
					|EnterWindowMask|LeaveWindowMask|ButtonReleaseMask
					|ButtonPressMask|Button1MotionMask;

	XSelectInput(v.display, v.win, v.event_mask);

	v.wm_delete_window = XInternAtom(v.display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(v.display, v.win, &v.wm_delete_window, 1);

	v.surface = cairo_xlib_surface_create (v.display, v.win,DefaultVisual(v.display, DefaultScreen (v.display)), v.h, v.h);
	v.cr = cairo_create(v.surface);

	XMapWindow(v.display, v.win);

	int keep_running = 1;

	while (keep_running) {
		XNextEvent(v.display, &v.event);

		switch(v.event.type) {
			case ConfigureNotify:
				// configure event, we only check for resize events here
				resize_event(&v);
			break;
			case Expose:
				// only redraw on the last expose event
				if (v.event.xexpose.count == 0) {
					_expose(&v);
				}
			break;
			case ButtonPress:
				// save mouse position and knob value
				v.pos_x = v.event.xbutton.x;
				v.pos_y = v.event.xbutton.y;
				v.start_value = v.knob.adj.value;

				switch(v.event.xbutton.button) {
					case  Button1:
						// left button pressed
						button1_event(&v.knob);
						send_expose(v.display,v.win);
					break;
					case  Button4:
						// mouse wheel scroll up
						scroll_event(&v.knob, 1);
						send_expose(v.display,v.win);
					break;
					case Button5:
						// mouse wheel scroll down
						scroll_event(&v.knob, -1);
						send_expose(v.display,v.win);
					break;
					default:
					break;
				}
			break;
			case MotionNotify:
				// mouse move while button1 is pressed
				if(v.event.xmotion.state & Button1Mask) {
					motion_event(&v.knob, v.start_value, v.event.xmotion.y, v.pos_y);
					send_expose(v.display,v.win);
				}
			break;
			case KeyPress:
				if (v.event.xkey.keycode == XKeysymToKeycode(v.display,XK_Up)) {
					scroll_event(&v.knob, 1);
					send_expose(v.display,v.win);
				} else if (v.event.xkey.keycode == XKeysymToKeycode(v.display,XK_Right)) {
					scroll_event(&v.knob, 1);
					send_expose(v.display,v.win);
				} else if (v.event.xkey.keycode == XKeysymToKeycode(v.display,XK_Down)) {
					scroll_event(&v.knob, -1);
					send_expose(v.display,v.win);
				} else if (v.event.xkey.keycode == XKeysymToKeycode(v.display,XK_Left)) {
					scroll_event(&v.knob, -1);
					send_expose(v.display,v.win);
				}
			break;
			case ClientMessage:
				if (v.event.xclient.message_type == XInternAtom(v.display, "WM_PROTOCOLS", 1) &&
				   (Atom)v.event.xclient.data.l[0] == XInternAtom(v.display, "WM_DELETE_WINDOW", 1))
					keep_running = 0;
				break;
			default:
				break;
		}
	}

	cairo_destroy(v.cr);
	cairo_surface_destroy(v.surface);
	cairo_surface_destroy(v.image);
	XDestroyWindow(v.display, v.win);
	XCloseDisplay(v.display);
	return 0;
}
