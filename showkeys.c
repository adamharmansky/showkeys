#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "x.h"
#include "config.h"
#include "sk.h"

static SK_color* colors;
static SK_color bg_color;
static SK_color fg_color;
static SK_color title_color;
static SK_color dim_fg_color;

static SK_program* programs;
static unsigned int program_count = 0;

static int column_width = 0;

static SK_color
parse_color(const char* color) {
	unsigned int r, g, b;
	sscanf(color, "#%2x%2x%2x", &r, &g, &b);
	return (SK_color){.r = (float)r/255, .g = (float)g/255, .b = (float)b/255};
}

static void
init_colors() {
	unsigned int i;

	colors = malloc(color_count * sizeof(SK_color));

	bg_color     = parse_color(color_bg);
	fg_color     = parse_color(color_fg);
	title_color  = parse_color(color_title);
	dim_fg_color = parse_color(color_dim_fg);

	for (i = 0; i < color_count; i++)
		colors[i] = parse_color(title_colors[i]);
}

static void
load_keys() {
	DIR* mdir;
	DIR* dir;
	FILE* file;
	FILE* pipe;
	char* wintitle = NULL;
	size_t wintitlesize;
	size_t size;
	char* line = NULL;
	struct dirent* ment;
	struct dirent* ent;
	char* pth;
	unsigned int i, j;
	SK_key* k;
	char *cmd;
	int x;
	char* kd;
	char* home;
	
	/* pain. */

	if (*key_dir == '~') {
		home = getenv("HOME");
		kd = malloc(strlen(home)+strlen(key_dir));
		strcpy(kd, home);
		strcat(kd, key_dir + 1);
	} else {
		kd = (char*)key_dir;
	}

	programs = malloc(program_count = 0);

	/* master directory */
	mdir = opendir(kd);
	if (!mdir) {
		fprintf(stderr, "could not open directory\n");
		exit(1);
	}

	while (ment = readdir(mdir)) {
		/* we are looking for sub-directories */
		if (ment->d_type == DT_DIR && *ment->d_name != '.') {
			cmd = malloc(strlen(program_detector) + strlen(ment->d_name));
			sprintf(cmd, program_detector, ment->d_name);
			pipe = popen(cmd, "r");
			free(cmd);
			fscanf(pipe, "%d", &x);
			pclose(pipe);
			if (x) {
				programs = realloc(programs, ++program_count * sizeof(SK_program));
				programs[program_count-1] = (SK_program) {
					.name = malloc(strlen(ment->d_name)+1),
					/* malloc 0 just to make sure */
					.categories = malloc(0),
					.category_count = 0
				};
				strcpy(programs[program_count-1].name, ment->d_name);
				pth = malloc(strlen(kd)+strlen(ment->d_name)+2);
				strcpy(pth, kd);
				strcat(pth, "/");
				strcat(pth, ment->d_name);
				dir = opendir(pth);
				free(pth);
				/* regular files */
				while (ent = readdir(dir)) {
					if (ent->d_type == DT_REG) {
						programs[program_count-1].categories = realloc(programs[program_count-1].categories, ++programs[program_count-1].category_count*sizeof(SK_category));
						programs[program_count-1].categories[programs[program_count-1].category_count-1] = (SK_category) {
							.name = malloc(strlen(ent->d_name)+1),
							/* malloc 0 just to make sure */
							.keys = malloc(0),
							.key_count = 0
						};
						strcpy(programs[program_count-1].categories[programs[program_count-1].category_count-1].name, ent->d_name);
						pth = malloc(strlen(kd)+strlen(ment->d_name)+strlen(ent->d_name)+3);
						strcpy(pth, kd);
						strcat(pth, "/");
						strcat(pth, ment->d_name);
						strcat(pth, "/");
						strcat(pth, ent->d_name);
						file = fopen(pth, "r");
						free(pth);
						while (~getline(&line, &size, file)) {
							programs[program_count-1].categories[programs[program_count-1].category_count-1].keys = realloc(programs[program_count-1].categories[programs[program_count-1].category_count-1].keys, ++programs[program_count-1].categories[programs[program_count-1].category_count-1].key_count*sizeof(SK_key));
							k = &programs[program_count-1].categories[programs[program_count-1].category_count-1].keys[programs[program_count-1].categories[programs[program_count-1].category_count-1].key_count-1];
							*k = (SK_key) {
								/* malloc 0 just to make sure */
								.mod = malloc(0),
								.key = malloc(0),
								.action = malloc(0)
							};
							i = 0;
							for (j = 0; line[i] != ' ';i++,j++) {
								k->mod = realloc(k->mod, j+2);
								k->mod[j] = line[i];
							}
							k->mod[j] = 0;
							while (line[i] == ' ') i++;
							for (j = 0; line[i] != ' '; i++, j++) {
								k->key = realloc(k->key, j+2);
								k->key[j] = line[i];
							}
							k->key[j] = 0;
							while (line[i] == ' ') i++;
							for (j = 0; line[i] != '\n' && line[i]; i++, j++) {
								k->action = realloc(k->action, j+2);
								k->action[j] = line[i];
							}
							k->action[j] = 0;

							free(line);
							line = NULL;
						}
						fclose(file);
					}
				}
				closedir(dir);
			}
		}
	}

	closedir(mdir);
}

void
calculate_column_width() {
	unsigned int i, j, k, l;
	int w;
	cairo_text_extents_t extents;
	cairo_font_extents_t key_font_extents;

	cairo_select_font_face(cairo, font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size(cairo, font_size);
	cairo_font_extents(cairo, &key_font_extents);

	for (l = i = 0; i < program_count; i++) {
		for (j = 0; j < programs[i].category_count; j++, l++) {
			for (k = 0; k < programs[i].categories[j].key_count; k++) {
				w = 0;
				cairo_text_extents(cairo, programs[i].categories[j].keys[k].mod, &extents);
				w += extents.width;
				if (*programs[i].categories[j].keys[k].mod) {
					cairo_text_extents(cairo, "+", &extents);
					w += extents.width;
				}
				cairo_text_extents(cairo, programs[i].categories[j].keys[k].key, &extents);
				w += extents.width;
				cairo_text_extents(cairo, programs[i].categories[j].keys[k].action, &extents);
				w += extents.width;
				w += key_padding*2 + 10;
				if (w > column_width) column_width = w;
			}
		}
	}
}

void
size_window()
{
	unsigned int i, j, k, l;
	cairo_font_extents_t title_font_extents;
	cairo_font_extents_t key_font_extents;
	int x = border_padding, y = border_padding;
	int maxy = 0;
	int w;

	/* clear background */
	cairo_set_source_rgb(cairo, bg_color.r, bg_color.g, bg_color.b);
	cairo_paint(cairo);

	cairo_select_font_face(cairo, font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size(cairo, title_font_size);
	cairo_font_extents(cairo, &title_font_extents);
	cairo_set_font_size(cairo, font_size);
	cairo_font_extents(cairo, &key_font_extents);

	for (l = i = 0; i < program_count; i++) {
		for (j = 0; j < programs[i].category_count; j++, l++) {
			if (y + programs[i].categories[j].key_count * (key_font_extents.height + key_padding*2) + title_padding*2 + padding_below_title*2 + title_font_extents.height > height) {
				y = border_padding;
				x = x + column_width + column_padding;
			}
			y += title_font_extents.height + title_padding * 2 + padding_below_title*2 + (key_font_extents.height + key_padding * 2) * programs[i].categories[j].key_count;
			if (y > maxy) maxy = y;
		}
	}
	maxy += border_padding - padding_below_title;
	XResizeWindow(display, window, x + column_width + border_padding, maxy);
	XMoveWindow(display, window, winx + (width - (x + column_width + border_padding))/2, winy + (height - maxy)/2);
	cairo_xlib_surface_set_size(surface, width, height);
	winx = winx + (width - (x + column_width + border_padding))/2;
	winy = winy + (height - maxy)/2;
	width = x + column_width + border_padding;
}

void
redraw()
{
	unsigned int i, j, k, l;
	cairo_text_extents_t extents;
	cairo_font_extents_t title_font_extents;
	cairo_font_extents_t key_font_extents;
	int x = border_padding, y = border_padding;
	char tmp[256];
	int w;

	/* clear background */
	cairo_set_source_rgb(cairo, bg_color.r, bg_color.g, bg_color.b);
	cairo_paint(cairo);

	cairo_select_font_face(cairo, font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size(cairo, title_font_size);
	cairo_font_extents(cairo, &title_font_extents);
	cairo_set_font_size(cairo, font_size);
	cairo_font_extents(cairo, &key_font_extents);

	for (l = i = 0; i < program_count; i++) {
		for (j = 0; j < programs[i].category_count; j++, l++) {
			if (y + programs[i].categories[j].key_count * (key_font_extents.height + key_padding*2) + title_padding*2 + padding_below_title*2 + title_font_extents.height > height) {
				y = border_padding;
				x = x + column_width + column_padding;
			}
			strcpy(tmp, programs[i].name);
			strcat(tmp, ": ");
			strcat(tmp, programs[i].categories[j].name);
			cairo_set_font_size(cairo, title_font_size);
			cairo_set_source_rgb(cairo, colors[l%color_count].r, colors[l%color_count].g, colors[l%color_count].b);
			cairo_rectangle(cairo, x, y, column_width, title_font_extents.height + title_padding*2);
			cairo_fill(cairo);
			cairo_set_source_rgb(cairo, title_color.r, title_color.g, title_color.b);
			cairo_move_to(cairo, x + title_padding, y + title_padding + title_font_extents.ascent);
			cairo_show_text(cairo, tmp);
			y += title_font_extents.height + title_padding * 2 + padding_below_title;
			cairo_set_font_size(cairo, font_size);
			for (k = 0; k < programs[i].categories[j].key_count; k++) {
				cairo_move_to(cairo, x + key_padding, y + key_padding + key_font_extents.ascent);
				if (*programs[i].categories[j].keys[k].mod) {
					cairo_set_source_rgb(cairo, dim_fg_color.r, dim_fg_color.g, dim_fg_color.b);
					cairo_show_text(cairo, programs[i].categories[j].keys[k].mod);
					cairo_show_text(cairo, "+");
				}
				cairo_set_source_rgb(cairo, fg_color.r, fg_color.g, fg_color.b);
				cairo_show_text(cairo, programs[i].categories[j].keys[k].key);
				cairo_text_extents(cairo, programs[i].categories[j].keys[k].action, &extents);
				cairo_move_to(cairo, x + column_width - key_padding - extents.width, y + key_padding + key_font_extents.ascent);
				cairo_show_text(cairo, programs[i].categories[j].keys[k].action);
				y += key_font_extents.height + key_padding * 2;
			}
			y += padding_below_title;
		}
	}
}

int
main(int argc, char** argv) {
	XEvent e;

	load_keys();
	xinit();
	calculate_column_width();
	size_window();
	/* MAP THE WINDOW NOW, NOT IN XINIT */
	XMapWindow(display, window);
	init_colors();

	for (;;) {
		XNextEvent(display, &e);
		switch (e.type) {
			case ConfigureNotify:
				width = e.xconfigure.width;
				height = e.xconfigure.height;
				cairo_xlib_surface_set_size(surface, width, height);
				break;
			case Expose:
				redraw();
				break;
			case KeyPress:
				exit(0);
		}
	}
}
