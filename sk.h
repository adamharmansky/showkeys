#ifndef _SK_H
#define _SK_H

typedef struct sk_color {
	float r, g, b;
} SK_color;

typedef struct sk_key {
	char* mod;      /* modifier */
	char* key;      /* the key itself */
	char* action;   /* what the key does */
	/* EXAMPLE: mod = "Super+Shift"; key = "c"; action = "Close Window"; program = "dwm"; category = "client"; */
} SK_key;

typedef struct sk_category {
	char* name;

	SK_key* keys;
	unsigned int key_count;

	SK_key** matches;
	unsigned int match_count;
} SK_category;

typedef struct sk_program {
	char* name;

	SK_category* categories;
	unsigned int category_count;
} SK_program;

#endif
