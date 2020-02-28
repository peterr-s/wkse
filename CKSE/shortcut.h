#ifndef SHORTCUT_H
#define SHORTCUT_H

// for storing shortcuts
typedef struct shortcut shortcut;
struct shortcut
{
	unsigned int ccode;
	bool shift;
	unsigned int* keys;
	unsigned int key_ct;
};

struct shortcut* append_to_array(shortcut shortcuts[], unsigned int l, shortcut* new_element);

#endif