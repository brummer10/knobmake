#include <cairo.h>
#include <cairo-svg.h>
#include <math.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// gcc -g knob_make.c -lm `pkg-config --cflags --libs cairo` -o knobmake

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef max
#define max(x, y) ((x) < (y) ? (y) : (x))
#endif

const double scale_zero = 20 * (M_PI/180); // defines "dead zone" for knobs

static void draw_indicator_ring( cairo_t *cr, double knobstate, double ind_radius, 
								 double angle, double x_center, double y_center) {
 
	double add_angle = 90 * (M_PI / 180.);
	
	double dashes[] = {4.0, 6.0};
	cairo_set_dash(cr, dashes, sizeof(dashes)/sizeof(dashes[0]), 0);

	// draw background
	cairo_set_source_rgb(cr,0.2, 0.2, 0.2);
	cairo_set_line_width(cr, 4.0);
	cairo_arc (cr, x_center , y_center, ind_radius,
		  add_angle + scale_zero, add_angle + scale_zero + 320 * (M_PI/180));
	cairo_stroke(cr);

	// draw foreground
	if (scale_zero < angle) {
		cairo_set_source_rgb(cr,0.2, 0.5, 0.2);
		cairo_arc (cr, x_center, y_center, ind_radius,
			  add_angle + scale_zero, add_angle + angle);
		cairo_stroke(cr);
	}
	cairo_set_dash(cr, NULL, 0, 0);
 
}
static void shading(cairo_t *cr, int arc_offset, double knobx1, double knoby1, double knob_x) {
	cairo_arc(cr,knobx1+arc_offset/2, knoby1+arc_offset/2, knob_x/2.1, 0, 2 * M_PI );
	cairo_pattern_t* pat =
		cairo_pattern_create_radial (knobx1+arc_offset-knob_x/6,knoby1+arc_offset-knob_x/6,
									 1,knobx1+arc_offset,knoby1+arc_offset,knob_x/2.1 );
	cairo_pattern_add_color_stop_rgba (pat, 1,  0.0, 0.0, 0.0, 0.6);
	cairo_pattern_add_color_stop_rgba (pat, 0.3,  0.3, 0.3, 0.3, 0.6);
	cairo_pattern_add_color_stop_rgba (pat, 0,  0.4, 0.4, 0.4, 0.6);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_pattern_destroy (pat);
}

static void inner_ring(cairo_t *cr, int arc_offset, double knobx1, double knoby1, double knob_x) {
	cairo_arc(cr,knobx1+arc_offset/2, knoby1+arc_offset/2, knob_x/5.1, 0, 2 * M_PI );
	cairo_pattern_t* pat = cairo_pattern_create_radial (knobx1+arc_offset/2, knoby1+arc_offset/2,
											  1,knobx1+arc_offset,knoby1+arc_offset,knob_x/2.1 );
	cairo_pattern_add_color_stop_rgba (pat, 0,  0.1, 0.1, 0.1, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 1,  0.0, 0.0, 0.0, 1.0);
	cairo_set_source (cr, pat);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb (cr, 0.05, 0.15, 0.05); // knob pointer color
	cairo_set_line_width(cr,4);
	cairo_stroke(cr);
	cairo_pattern_destroy (pat);
}

static void gear(cairo_t *cr, double radius, int teeth, double tooth_depth) {

	int i;
	double r1, r2;
	double angle, da;

	r1 = radius - tooth_depth / 2.0;
	r2 = radius + tooth_depth / 2.0;

	da = 2.0 * M_PI / (double) teeth / 4.0;
	
	cairo_new_path (cr);

	angle = 0.0;
	cairo_move_to (cr, r1 * cos (angle + 3 * da), r1 * sin (angle + 3 * da));

	for (i = 1; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / (double) teeth;

		cairo_line_to (cr, r1 * cos (angle), r1 * sin (angle));
		cairo_line_to (cr, r2 * cos (angle + da), r2 * sin (angle + da));
		cairo_line_to (cr, r2 * cos (angle + 2 * da), r2 * sin (angle + 2 * da));

		if (i < teeth)
			cairo_line_to (cr, r1 * cos (angle + 3 * da),
						   r1 * sin (angle + 3 * da));
	}
	cairo_close_path (cr);
}


static void calcVertexes(double start_x, double start_y,
						double end_x, double end_y, 
						double arrow_degrees_, double arrow_lenght_,
						double *x1, double *y1, double *x2, double *y2,
						double *x3, double *y3) {

	double angle = atan2 (end_y - start_y, end_x - start_x) + M_PI;

	*(x1) = end_x + arrow_lenght_ * cos(angle - arrow_degrees_);
	*(y1) = end_y + arrow_lenght_ * sin(angle - arrow_degrees_);
	*(x2) = end_x + arrow_lenght_ * cos(angle + arrow_degrees_);
	*(y2) = end_y + arrow_lenght_ * sin(angle + arrow_degrees_);
	*(x3) = end_x + arrow_lenght_ * 0.1 * cos(angle);
	*(y3) = end_y + arrow_lenght_ * 0.1 * sin(angle);
}

static void paint_knob_state(cairo_t *cr, int knob_size, int knob_offset, double knobstate)
{
	/** set knob size **/
	int arc_offset = knob_offset;
	double knob_x = knob_size-arc_offset;
	double knob_y = knob_size-arc_offset;
	double knobx = arc_offset/2;
	double knobx1 = knob_x/2;
	double knoby = arc_offset/2;
	double knoby1 = knob_y/2;

	cairo_pattern_t* pat;
	cairo_new_path (cr);

	/** create the knob, set the knob and border color to your needs,
	 *  or set knob color alpa to 0.0 to draw only the border **/

	cairo_arc(cr,knobx1+arc_offset/2, knoby1+arc_offset/2, knob_x/2.1, 0, 2 * M_PI );
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0); // knob color
	cairo_fill_preserve (cr);
 	cairo_set_source_rgb (cr, 0.1, 0.2, 0.1); // knob border color
	cairo_set_line_width(cr,4);
	cairo_stroke(cr);
	cairo_new_path (cr);

	/** calculate the pointer **/
	double angle = scale_zero + knobstate * 2 * (M_PI - scale_zero);
	double pointer_off =knob_x/10;
	double radius = min(knob_x-pointer_off, knob_y-pointer_off) / 2;
	double length_x = (knobx+radius+pointer_off/2) - radius * sin(angle);
	double length_y = (knoby+radius+pointer_off/2) + radius * cos(angle);
	double radius_x = (knobx+radius+pointer_off/2) - radius/ 1.7 * sin(angle);
	double radius_y = (knoby+radius+pointer_off/2) + radius/ 1.7 * cos(angle);

	/** create a rotating gear on the knob,
	 * set the color to your needs **/

 	cairo_save (cr);
	cairo_translate (cr, knobx1, knoby1);
	cairo_rotate (cr, angle-0.08); // adjust tooth to pointer

	gear (cr, radius-10.0, 9, 10.0);

	pat = cairo_pattern_create_radial (0.0,0.0, 1,0.0,0.0,knob_x/2.0 );
	cairo_pattern_add_color_stop_rgba (pat, 1,  0.1, 0.2, 0.1, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0,  0.05, 0.15, 0.05, 1.0);
	cairo_set_source (cr, pat);
	cairo_fill_preserve (cr);
	cairo_set_line_width(cr,1);
	cairo_set_source_rgb (cr, 0.2, 0.2, 0.2);
	cairo_stroke (cr);
	cairo_restore (cr);
	cairo_save (cr);
 
	/** create a 2. smaller rotating gear on the knob,
	 * set the color to your needs **/

	cairo_translate (cr, knobx1, knoby1);
	cairo_rotate (cr, angle-0.08); // adjust tooth to pointer

	gear (cr, radius-15.0, 9, 6.0);

	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_fill (cr);
	cairo_restore (cr);
	cairo_save (cr);


	/** create the rotating pointer on the knob,
	 * set the color to your needs **/

	double x1 = 0;
	double y1 = 0;
	double x2 = 0;
	double y2 = 0;
	double x3 = 0;
	double y3 = 0;
	double degrees_ = 0.35;
	double lenght_ = 10.0;
	
	/** create a arrow for given lengh and degrees **/

	calcVertexes(knobx1+arc_offset/2, knoby1+arc_offset/2, radius_x, radius_y,
				 degrees_, lenght_, &x1, &y1, &x2, &y2, &x3, &y3);
	
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
	cairo_move_to(cr, length_x, length_y);
	cairo_curve_to (cr,length_x, length_y,x1,y1,x3,y3);
	cairo_curve_to (cr,x3,y3,x2,y2,length_x, length_y);
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr,1);
	cairo_stroke(cr);

	// draw a ring indicator around the knob
	draw_indicator_ring(cr, knobstate, radius, angle, knobx1+arc_offset/2,knoby1+arc_offset/2);

	/**  use this for a simple pointer **/
   // cairo_move_to(cr, radius_x, radius_y);
   // cairo_line_to(cr,length_x,length_y);

	/** create a inner ring on the knob **/
	//inner_ring(cr, arc_offset, knobx1, knoby1, knob_x);

	/** 3d shading comment out, or set alpa to 0.0, for flat knobs
	 * or set alpa to a higher value for more shading effect **/
	shading(cr, arc_offset, knobx1, knoby1, knob_x);

}

int main(int argc, char* argv[])
{
	if (argc < 3) {
		fprintf(stdout, "usage: %s knob_size frame_count [offset] \nexample:\n  ./%s 150 101\n", basename(argv[0]), basename(argv[0]));
		return 1;
	}

	int knob_size = atoi(argv[1]);
	int knob_frames = atoi(argv[2]);
	int knob_image_width = knob_size * knob_frames;
	int knob_offset = 0;
	if (argc >= 4) {
		knob_offset = atoi(argv[3]);
	}
   
	char* sz = argv[1];
	char* fr = argv[2];
	char png_file[80];
	char svg_file[80];
	sprintf(png_file, "knob_%sx%s.png", sz,fr);
	sprintf(svg_file, "knob_%sx%s.svg", sz,fr);

	/** use this instead the svg surface when you don't need the svg format **/
	//cairo_surface_t *frame = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, knob_size, knob_size);
	cairo_surface_t *frame = cairo_svg_surface_create(NULL, knob_size, knob_size);
	cairo_t *crf = cairo_create(frame);
	/** use this instead the svg surface when you don't need the svg format **/
	//cairo_surface_t *knob_img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, knob_image_widht, knob_size);
	cairo_surface_t *knob_img = cairo_svg_surface_create(svg_file, knob_image_width, knob_size);
	cairo_t *cr = cairo_create(knob_img);

	/** draw the knob per frame to image **/
	for (int i = 0; i < knob_frames; i++) {
		paint_knob_state(crf, knob_size, knob_offset, (double)((double)i/ knob_frames));
		cairo_set_source_surface(cr, frame, knob_size*i, 0);
		cairo_paint(cr);
		cairo_set_operator(crf,CAIRO_OPERATOR_CLEAR);
		cairo_paint(crf);
		cairo_set_operator(crf,CAIRO_OPERATOR_OVER);
	}

	/** save to png file **/
	cairo_surface_flush(knob_img);
	cairo_surface_write_to_png(knob_img, png_file);
	unlink ("knob.png");
	symlink(png_file,"knob.png");

	/** clean up **/
	cairo_destroy(crf);
	cairo_destroy(cr);
	cairo_surface_destroy(knob_img);
	cairo_surface_destroy(frame);

	char *arg[]={"./knobview",NULL}; 
	return execvp(arg[0],arg);
}
