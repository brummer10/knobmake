#include <cairo.h>
#include <cairo-svg.h>
#include <math.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// gcc -Wall -g switch_make.c -lm `pkg-config --cflags --libs cairo` -o switchmake


static void rounded_rectangle(cairo_t *cr,double x0, double y0, double x1, double y1) {
	cairo_new_path (cr);
	cairo_move_to  (cr, x0, (y0 + y1)/2);
	cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
	cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
	cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
	cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
	cairo_close_path (cr);
}

static void paint_knob_state(cairo_t *cr, int knob_size, int knob_offset, double knobstate)
{

	// base calculation
	double x0      = 5.0;
	double y0      = 0.0;
	double rect_width  = knob_size-knob_offset-10.0;
	double rect_height = knob_size-knob_offset;
	double x1=x0+rect_width;
	double y1=y0+rect_height;

	// patterns
	cairo_pattern_t* 	pat = cairo_pattern_create_linear (x0+rect_width/2, y0,x0+rect_width/2,rect_height);
	cairo_pattern_add_color_stop_rgba (pat, 0,  0.0, 0.0, 0.0, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.75,  0.15, 0.15, 0.15, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.5,  0.2, 0.2, 0.2, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.25,  0.15, 0.15, 0.15, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 1,  0.0, 0.0, 0.0, 1.0);

	cairo_pattern_t* 	pat2 = cairo_pattern_create_linear (x0+rect_width/2, y0,x0+rect_width/2,rect_height);
	cairo_pattern_add_color_stop_rgba (pat2, 1,  0.0, 0.0, 0.0,1.0);
	cairo_pattern_add_color_stop_rgba (pat2, 0.5,  0.1, 0.1, 0.1, 1.0);
	cairo_pattern_add_color_stop_rgba (pat2, 0,  0.0, 0.0, 0.0, 1.0);

	cairo_pattern_t*	pat3 = cairo_pattern_create_linear (x0+rect_width/2, y0,x0+rect_width/2,rect_height);
	cairo_pattern_add_color_stop_rgba (pat3, 0,  0.4, 0.4, 0.4, 1.0);
	cairo_pattern_add_color_stop_rgba (pat3, 0.45,  0.1, 0.1, 0.1, 1.0);
	cairo_pattern_add_color_stop_rgba (pat3, 0.5,  0.1, 0.1, 0.1, 1.0);
	cairo_pattern_add_color_stop_rgba (pat3, 0.55,  0.1, 0.1, 0.1, 1.0);
	cairo_pattern_add_color_stop_rgba (pat3, 1,  0.4, 0.4, 0.4, 1.0);

	// base
	rounded_rectangle(cr, x0, y0, x1, y1);
	cairo_set_source (cr, pat);
	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr, 0.1, 0.1, 0.1, 1.0);
	cairo_set_line_width (cr, 2.0);
	cairo_stroke (cr);

	// inner frame and switch top
	x0 = 13.0;
	y0 = 8.0;
	x1=x0+rect_width-16.0;
	y1=y0+rect_height-16.0;
	rounded_rectangle(cr, x0, y0, x1, y1);
	cairo_set_source (cr, pat3);
	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr, 0.1, 0.1, 0.1, 0.8);
	cairo_set_line_width (cr, 2.0);
	cairo_stroke (cr);

	// 3d switch top
	x0 = 13.0;
	y0 = 10.0;
	x1=x0+rect_width-16.0;
	y1=y0+rect_height-20.0;
	rounded_rectangle(cr, x0, y0, x1, y1);
	cairo_set_source (cr, pat3);
	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr, 0.1, 0.1, 0.1, 0.8);
	cairo_set_line_width (cr, 2.0);
	cairo_stroke (cr);

	// inner and switch base
	x0 = 15.0;
	y0 = 10.0 +(rect_height-20.0)*knobstate;
	x1=x0+rect_width-20.0;
	y1=y0+(rect_height-20.0)/2;
	rounded_rectangle(cr, x0, y0, x1, y1);
	cairo_set_source (cr, pat2);
	cairo_fill(cr);

	// led indicator
	cairo_new_path (cr);
	x0 = 5.0+(rect_width/2) -(rect_width/10.0);
	y0 = 4.0 ;
	x1= x0 +(rect_width/5.0);
	y1=y0;
	cairo_set_line_width (cr, 5.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to  (cr, x0, y0);
	cairo_line_to (cr, x1 , y1);
	pat = cairo_pattern_create_linear (x0, y0,x1,y1);
	cairo_pattern_add_color_stop_rgba (pat, 1,  0.2 +(0.5*knobstate), 0.1, 0.05,1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.5,  0.2 +(0.7*knobstate), 0.05, 0.1, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0,  0.2 +(0.5*knobstate), 0.1, 0.05, 1.0);
	cairo_set_source (cr, pat);
	cairo_stroke (cr);

	cairo_pattern_destroy (pat);
	cairo_pattern_destroy (pat2);
	cairo_pattern_destroy (pat3);
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        fprintf(stdout, "usage: %s switch_size frame_count [offset] \n", basename(argv[0]));
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
    sprintf(png_file, "switch_%sx%s.png", sz,fr);
    sprintf(svg_file, "switch_%sx%s.svg", sz,fr);

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
