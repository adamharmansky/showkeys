#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#include "x.h"
#include "config.h"
#include "sk.h"

static SK_color* colors;
static SK_color bg_color;
static SK_color fg_color;
static SK_color title_color;
static SK_color search_bg_color;
static SK_color dim_fg_color;

static SK_program* programs;
static unsigned int program_count = 0;

static int column_width = 0;

static char** running_programs;

static struct {
	char *text;
	int head;
	int size;
} search;

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
	search_bg_color = parse_color(color_search_bg);
	fg_color     = parse_color(color_fg);
	title_color  = parse_color(color_title);
	dim_fg_color = parse_color(color_dim_fg);

	for (i = 0; i < color_count; i++)
		colors[i] = parse_color(title_colors[i]);
}

static char*
get_path() {
	static char* kd = NULL;
	char* home;

	/* do nothing if kd has already been allocated */
	if (!kd) {
		/* if the path begins with the ~ character, substitute it for $HOME. */
		if (*key_dir == '~') {
			home = getenv("HOME");
			kd = malloc(strlen(home)+strlen(key_dir));
			strcpy(kd, home);
			strcat(kd, key_dir + 1);
		} else {
			kd = malloc(strlen(key_dir)+1);
			strcpy(kd, key_dir);
		}
	}
	return kd;
}

static void
load_key(SK_key* self, char* line) {
	unsigned int i, j;
	int size;

	self->mod    = malloc(1);
	self->key    = malloc(1);
	self->action = malloc(1);

	self->mod[0] = 0;
	self->key[0] = 0;
	self->action[0] = 0;

	i = 0;
	size = 1;
	for (j = 0; line[i] != ' '; i++, j++) {
		if (line[i] == '\n' || !line[i]) break;
		if (j + 1 >= size)
			self->mod = realloc(self->mod, size = size<<1);
		self->mod[j] = line[i];
	}
	self->mod[j] = 0;
	size = 1;
	while (line[i] == ' ') i++;
	for (j = 0; line[i] != ' '; i++, j++) {
		if (line[i] == '\n' || !line[i]) break;
		if (j + 1 >= size)
			self->key = realloc(self->key, size = size<<1);
		self->key[j] = line[i];
	}
	self->key[j] = 0;
	size = 1;
	while (line[i] == ' ') i++;
	for (j = 0; line[i] != '\n' && line[i]; i++, j++) {
		if (line[i] == '\n' || !line[i]) break;
		if (j + 1 >= size)
			self->action = realloc(self->action, size = size<<1);
		self->action[j] = line[i];
	}
	self->action[j] = 0;
}

static void
load_category(SK_category* self, char* name, char* parent) {
	unsigned int i;
	FILE* file;
	char* pth;
	char* line = NULL;
	size_t size;

	self->name = malloc(strlen(name)+1);
	strcpy(self->name, name);

	self->keys = malloc(0);
	self->key_count = 0;

	/* "key_dir/program/category" */
	pth = malloc(strlen(get_path())+strlen(name)+strlen(parent)+3);
	strcpy(pth, get_path());
	strcat(pth, "/");
	strcat(pth, parent);
	strcat(pth, "/");
	strcat(pth, name);

	file = fopen(pth, "r");

	free(pth);

	while (getline(&line, &size, file) > 0) {
		self->keys = realloc(self->keys, ++self->key_count*sizeof(SK_key));
		load_key(&self->keys[self->key_count-1], line);

		free(line);
		/* set line to NULL so getline knows that it should malloc it again */
		line = NULL;
	}

	self->matches = malloc(self->key_count * sizeof(SK_key*));
	self->match_count = self->key_count;
	for (i = 0; i < self->key_count; i++)
		self->matches[i] = &self->keys[i];

	fclose(file);
}

static void
load_program(SK_program* self, char* name)
{
	char* pth;
	DIR* dir;
	struct dirent* ent;

	self->name = malloc(strlen(name)+1);
	strcpy(self->name, name);

	self->categories = malloc(0);
	self->category_count = 0;

	pth = malloc(strlen(get_path())+strlen(name)+2);
	strcpy(pth, get_path());
	strcat(pth, "/");
	strcat(pth, name);

	dir = opendir(pth);

	free(pth);

	while (ent = readdir(dir)) {
		/* we are looking for regular files */
		if (ent->d_type == DT_REG) {
			self->categories = realloc(self->categories, ++self->category_count*sizeof(SK_category));
			load_category(&self->categories[self->category_count-1], ent->d_name, name);
		}
	}
}

static int
should_be_displayed(char* name) {
	unsigned int i, j;

	for (i = 0; running_programs[i]; i++) {
		for (j = 0; running_programs[i][j] && name[j]; j++) {
			/* running_programs[i] is guaranteed to be lowercase */
			if ((isupper(name[j]) ? name[j] + 'a' - 'A' : name[j]) != running_programs[i][j]) break;
		}
		/* one AND + one NOT instead of two NOTs and one OR 😎*/
		if (!(running_programs[i][j] && name[j])) return 1;
	}
	return 0;
}

static void
load_keys() {
	DIR* dir;
	struct dirent* ent;

	programs = malloc(program_count = 0);

	/* master directory */
	dir = opendir(get_path());
	if (!dir) {
		fprintf(stderr, "could not open directory\n");
		exit(1);
	}

	while (ent = readdir(dir)) {
		/* we are looking for non-hidden subdirectories */
		if (ent->d_type == DT_DIR && *ent->d_name != '.') {
			if (should_be_displayed(ent->d_name)) {
				programs = realloc(programs, ++program_count * sizeof(SK_program));
				load_program(&programs[program_count-1], ent->d_name);
			}
		}
	}

	closedir(dir);
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

int
match_text(char* text) {
	char* q;

	char* orig = text;

	for (q = search.text;*q;q++) {
		while ((isupper(*text)?*text-'A'+'a':*text) != (isupper(*q)?*q-'A'+'a':*q) && *text) text++;
		if (!*text) break;
	}
	return !*q;
}

void
match() {
	unsigned int i, j, k;

	for (i = 0; i < program_count; i++) {
		for (j = 0; j < programs[i].category_count; j++) {
			programs[i].categories[j].match_count = 0;
			for (k = 0; k < programs[i].categories[j].key_count; k++) {
				if (match_text(programs[i].categories[j].keys[k].action)) {
					programs[i].categories[j].matches[programs[i].categories[j].match_count++] = &programs[i].categories[j].keys[k];
				}
			}
		}
	}
}

void
size_window() {
	unsigned int i, j, k, l;
	cairo_font_extents_t title_font_extents;
	cairo_font_extents_t key_font_extents;
	cairo_font_extents_t search_font_extents;
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
			if (y + title_padding*2 + padding_below_title + title_font_extents.height + border_padding > height) {
				y = border_padding;
				x = x + column_width + column_padding;
			}
			y += title_font_extents.height + title_padding * 2 + padding_below_title;
			for (k = 0; k < programs[i].categories[j].key_count; k++) {
				if (y + key_font_extents.height + key_padding*2 + border_padding > height) {
					y = border_padding;
					x = x + column_width + column_padding;
				}
				y += key_font_extents.height + key_padding*2;
			}
			if (y + padding_below_title > height) {
				y = border_padding;
				x = x + column_width + column_padding;
			} else y += padding_below_title;
			if (y > maxy) maxy = y;
		}
	}
	cairo_set_font_size(cairo, search_font_size);
	cairo_font_extents(cairo, &search_font_extents);
	maxy += border_padding + search_padding*2 + search_font_extents.height;
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
	cairo_font_extents_t search_font_extents;
	int x = border_padding, y;
	char tmp[256];
	int w;

	cairo_push_group(cairo);

	/* clear background */
	cairo_set_source_rgb(cairo, bg_color.r, bg_color.g, bg_color.b);
	cairo_paint(cairo);

	cairo_select_font_face(cairo, font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size(cairo, title_font_size);
	cairo_font_extents(cairo, &title_font_extents);
	cairo_set_font_size(cairo, font_size);
	cairo_font_extents(cairo, &key_font_extents);

	cairo_set_font_size(cairo, search_font_size);
	cairo_font_extents(cairo, &search_font_extents);

	cairo_set_source_rgb(cairo, search_bg_color.r, search_bg_color.g, search_bg_color.b);
	cairo_rectangle(cairo, 0, 0, width, search_font_extents.height + search_padding*2 + border_padding);
	cairo_fill(cairo);
	cairo_set_source_rgb(cairo, fg_color.r, fg_color.g, fg_color.b);
	cairo_move_to(cairo, border_padding + search_padding, border_padding + search_padding + search_font_extents.ascent);
	cairo_show_text(cairo, search.text);
	cairo_set_line_width(cairo, 2);
	cairo_rel_line_to(cairo, 6, 0);
	cairo_stroke(cairo);

	y = border_padding + search_padding*2 + padding_below_title + search_font_extents.height;

	for (l = i = 0; i < program_count; i++) {
		for (j = 0; j < programs[i].category_count; j++, l++) {
			if (programs[i].categories[j].match_count) {
				if (y + title_padding*2 + padding_below_title + title_font_extents.height + border_padding > height) {
					y = border_padding + search_padding*2 + padding_below_title + search_font_extents.height;
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
				for (k = 0; k < programs[i].categories[j].match_count; k++) {
					if (y + key_font_extents.height + key_padding*2 + border_padding > height) {
						y = border_padding + search_padding*2 + padding_below_title + search_font_extents.height;
						x = x + column_width + column_padding;
					}
					cairo_move_to(cairo, x + key_padding, y + key_padding + key_font_extents.ascent);
					if (*programs[i].categories[j].matches[k]->mod) {
						cairo_set_source_rgb(cairo, dim_fg_color.r, dim_fg_color.g, dim_fg_color.b);
						cairo_show_text(cairo, programs[i].categories[j].matches[k]->mod);
						cairo_show_text(cairo, "+");
					}
					cairo_set_source_rgb(cairo, fg_color.r, fg_color.g, fg_color.b);
					cairo_show_text(cairo, programs[i].categories[j].matches[k]->key);
					cairo_text_extents(cairo, programs[i].categories[j].matches[k]->action, &extents);
					cairo_move_to(cairo, x + column_width - key_padding - extents.width, y + key_padding + key_font_extents.ascent);
					cairo_show_text(cairo, programs[i].categories[j].matches[k]->action);
					y += key_font_extents.height + key_padding*2;
				}
				if (y + padding_below_title > height) {
					y = border_padding + search_padding*2 + padding_below_title + search_font_extents.height;
					x = x + column_width + column_padding;
				} else y += padding_below_title;
			}
		}
	}

	cairo_pop_group_to_source(cairo);
	cairo_paint(cairo);
	cairo_surface_flush(surface);
}

void
keypress(XKeyEvent* ev) {
	char buf[32];
	KeySym ksym;
	Status status;
	int len;

	len = XmbLookupString(xic, ev, buf, sizeof(buf), &ksym, &status);
	switch (ksym) {
		case XK_Escape:
		case XK_Return:
			exit(0);
			break;
		case XK_BackSpace:
			if (search.head) {
				search.text[--search.head] = 0;
				match();
				redraw();
			}
			break;
		default:
			if (!iscntrl(*buf)) {
				if (search.head + strlen(buf) + 1 >= search.size) {
					search.size <<= 1;
					search.text = realloc(search.text, search.size);
				}
				search.text[search.head] = *buf;
				search.text[++search.head] = 0;
				match();
				redraw();
			}
			break;
	}
}

void
get_running_programs() {
	unsigned int i, j;
	FILE* pipe;
	char* line = 0;
	size_t n;
	unsigned int bufsize = 4;

	running_programs = malloc(bufsize*sizeof(char*));
	*running_programs = 0;

	pipe = popen(program_list, "r");
	
	for (i = 0; getline(&line, &n, pipe) > 0; i++) {
		running_programs[i] = malloc(n+1);
		strcpy(running_programs[i], line);

		/* convert to lowercase */
		for (j = 0; running_programs[i][j]; j++)
			if (isupper(running_programs[i][j]))
				running_programs[i][j] += 'a' - 'A';

		if (i + 2 >= bufsize)
			running_programs = realloc(running_programs, (bufsize <<= 1)*sizeof(char*));

		free(line);
		line = NULL;
	}
	/* end it off with a NULL */
	running_programs[i] = 0;

	pclose(pipe);
}

int
main(int argc, char** argv) {
	XEvent e;

	/* assigning 3 different variables in one line 😎 */
	*(search.text = malloc(search.size = 1)) = search.head = 0;

	get_running_programs();
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
				keypress(&e.xkey);
				break;
		}
	}
}
