#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifndef DATABASE_H
#include "Database.h"
#endif

#ifndef LAYOUT_H
#include "Layout.h"
#endif

#define INFINITY 999999

#define EORCOLOUR 0xFFFFFF00
#define EORCOLOURRED 0xFFFF0000

typedef enum wintype {
	wintype_NORMAL,
	wintype_UNLINKED,
	wintype_DESCENDENTS,
	wintype_ANCESTORS,
	wintype_CLOSERELATIVES
} wintype;

typedef void (*plotfn)(const int minx,const int miny,const int maxx,const int maxy,const int linethickness,const unsigned int colour);
typedef void (*plottextfn)(const int x,const int y,const int handle,const char *font,const int size,const unsigned int bgcolour,const unsigned int fgcolour,const char *text);

void Windows_Init(void);

void Windows_Edit(void);

void Windows_ChangedLayout(void);

void Windows_OpenWindow(wintype type,elementptr person,int generations);

void Windows_Relayout(void);

int Windows_GetSize(void);

void Windows_Save(FILE *file);


#endif
