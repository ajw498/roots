#ifndef LAYOUTSTRUCT_H
#define LAYOUTSTRUCT_H

#include "AJWLib/Drawfile.h"
#include "DatabaseStruct.h"

typedef struct flags {
	unsigned int editable   : 1;
	unsigned int moveable   : 1;
	unsigned int linkable   : 1;
	unsigned int snaptogrid : 1;
	unsigned int selectable : 1;
} flags;

typedef struct elementlayout {
	int x;
	int y;
	int xgrid;
	int ygrid;
	elementptr element;
	int width;
	int height;
	flags flags;
} elementlayout;


typedef struct drawfileholder {
	drawfile_diagram *drawfile;
} drawfileholder;

typedef struct picturelayout {
	int x;
	int y;
	int minx;
	int miny;
	int xgrid;
	int ygrid;
	drawfileholder *picture;
	int width;
	int height;
	flags flags;
} picturelayout;

typedef struct layout {
	elementlayout *person;
	int numpeople;
	elementlayout *transients;
	int numtransients;
	picturelayout *picture;
	int numpictures;
	int gridx;
	int gridy;
	flags flags;
} layout;

typedef struct mouseclickdata {
	struct windowdata *window;
	elementptr element;
	Desk_wimp_point pos;
	Desk_bool transient;
} mouseclickdata;


#endif

