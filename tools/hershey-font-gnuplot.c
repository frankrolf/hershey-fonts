//
// hershey-font-gnuplot.c - Copyright (C) 2013 Kamal Mostafa <kamal@whence.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include <hersheyfont.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>

int sample_sheet_mode = 1;

char *render_text;

void
usage()
{
    fprintf(stderr,
"usage:  hershey-font-gnuplot [options] {font} 'Text to Render'\n"
"        hershey-font-gnuplot [options] {font}      - generate sample sheet\n"
"                {font}    a Hershey fontname or fontfile.jhf\n"
"                [options]\n"
"                   -T 'gnuplot_terminal_type [opts]'\n"
"                             e.g.:  -T 'pdf'\n"
"                                    -T 'png crop transparent'\n"
"                   -h font_height          (default 100)\n"
"examples:\n"
"    hershey-font-gnuplot futural   'Hello World!' | gnuplot -p\n"
"    hershey-font-gnuplot gothiceng 'Calligraphy'  | gnuplot -p\n"
"    hershey-font-gnuplot -T 'png crop' scriptc 'Nice Text' | gnuplot > out.png\n"
	);


}

int
main( int argc, char **argv )
{
    char *gnuplot_term_opts = "wxt";
    int gnuplot_height = 100;

    int opt;
    while ((opt = getopt(argc, argv, "h:T:")) != -1) {
	switch (opt) {
	case 'h':
	    gnuplot_height = strtoul(optarg, 0, 0);
	    break;
	case 'T':
	    gnuplot_term_opts = optarg;
	    break;
	default: /* '?' */
	   usage();
	   return 1;
	}
    }

    opt = argc - optind;
    if ( opt < 1 || opt > 2 ) {
	usage();
	return 1;
    }
    if ( opt == 2 )
	sample_sheet_mode = 0;


    // load a hershey_font structure either by filename or fontname
    const char *fontname = argv[optind++];
    struct hershey_font *hf;

    if ( strchr(fontname,'.') )
	hf = hershey_jhf_font_load(fontname);
    else
	hf = hershey_font_load(fontname);

    if ( !hf ) {
	perror(fontname);
	return 1;
    }

    // render the text

    printf("#\n# gnuplot input file generated by hershey-font-gnuplot\n#\n\n");

    printf("\n");

    int terminal_width, terminal_height;

    char render_text_buf[256];
    if ( sample_sheet_mode ) {
	int i;
	for ( i=0; i<256-32; i++ )
	    render_text_buf[i] = i+32;
	render_text = render_text_buf;
	terminal_width = 16 * gnuplot_height;
	terminal_height = 8 * gnuplot_height;
    } else {
	render_text = argv[optind++];
	terminal_width = 16 * gnuplot_height;
	terminal_height = gnuplot_height;
    }

    printf("set terminal %s size %d,%d\n",
	    gnuplot_term_opts, terminal_width, terminal_height);
    printf("\n");

    printf("unset xtics\n");
    printf("unset ytics\n");
    printf("unset border\n");
    printf("unset key\n");
    if ( sample_sheet_mode ) {
	printf("set title \"%s\"\n", fontname );
	printf("set size ratio 0.5\n");
	printf("set xrange [0:%d]\n", 32*32);
	printf("set yrange [0:%d]\n", 8*64);
    } else {
	printf("set size ratio 0.125\n");
	printf("set xrange [0:%d]\n", 16*16);
	printf("set yrange [0:%d]\n", 2*16);
    }
    printf("set style arrow 1 nohead\n" );
    printf("set style line 1 lc rgb \"black\" lw 0.5\n" );

    int x_render_pos = 0;
    int y_render_pos = 8;

    const char *p;
    for ( p=render_text; *p; p++ ) {

	// get the character c to be rendered
	int c = *((unsigned char *)p);

	// get the hershey_glyph for ASCII character c
	struct hershey_glyph *hg = hershey_font_glyph(hf, c);

	// check whether there actually is a glyph for this character
	if ( hg->width == 0 )
	    continue;

	// handle special placement for sample_sheet_mode
	if ( sample_sheet_mode ) {
	    x_render_pos = (c % 32) * 32;
	    y_render_pos = (c / 32) * 64;
	    y_render_pos = 8*64 - y_render_pos;
	}

	printf("#  [[ %c ]] glyph(%d) width=%u npaths=%u\n",
			c, c, hg->width, hg->npaths);

	// walk the paths-list for this glyph
	struct hershey_path *hp;
	short px, py;
	for ( hp=hg->paths; hp; hp=hp->next ) {
	    // begin draw path
	    printf("#\t\tpath: nverts=%d\n", hp->nverts);
	    int i;
	    for ( i=0; i<hp->nverts; i++ ) {
		short x = hp->verts[i].x + x_render_pos;
		short y = hp->verts[i].y + y_render_pos;
		if ( i > 0 )
		    printf("set arrow from %d,%d to %d,%d as 1 ls 1\n",
			    px, py, x, y);
		px = x;
		py = y;
	    }
	    // end draw path
	}

	// advance the x_render_pos by the width of this glyph
	x_render_pos += hg->width;
    }

    printf("plot NaN notitle\n");

    // destroy the hershey_font
    hershey_font_free(hf);

    return 0;
}
