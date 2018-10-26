# knobmake
create knobs with cairo

build with:

gcc -g knob_make.c -lm `pkg-config --cflags --libs cairo` -o knobmake

gcc -g knob_view.c -lX11 `pkg-config --cflags --libs cairo` -o knobview

then run, for example:

./knobmake 150 101

to create a knob and try it in it's own window. 

To create a new knob, you need to edit the source of knob_make.c, 

rebuild knobmake and re-run it to check out your changes. 
