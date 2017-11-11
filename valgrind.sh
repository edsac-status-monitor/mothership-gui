#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

make
#valgrind --leak-check=full --suppressions=valgrind/{base,gail,gtksourceview,pango,glib,gio,gdk,gtk,gtk3}.supp ./mothership_gui  2>&1 | grep -v "Theme parsing error"
valgrind --suppressions=valgrind/{base,gail,gtksourceview,pango,glib,gio,gdk,gtk,gtk3}.supp ./mothership_gui  2>&1 | grep -v "Theme parsing error"
