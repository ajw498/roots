#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifndef DATABASE_H
#include "Database.h"
#endif

#ifndef LAYOUT_H
#include "Layout.h"
#endif

#define INFINITY 999999

typedef enum wintype {
	wintype_NORMAL,
	wintype_UNLINKED,
	wintype_DESCENDENTS,
	wintype_ANCESTORS,
	wintype_CLOSERELATIVES
} wintype;

typedef void (*plotfn)(const int minx,const int miny,const int maxx,const int maxy,const int linethickness,const unsigned int colour);
typedef void (*plottextfn)(const int x,const int y,const int handle,const char *font,const int size,const unsigned int bgcolour,const unsigned int fgcolour,const char *text);

void Graphics_Init(void);

void Graphics_Edit(void);

void Graphics_ChangedLayout(void);

void Graphics_OpenWindow(wintype type,elementptr person,int generations);

void Graphics_Relayout(void);

void Graphics_Redraw(layout *layout,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection,plotfn plotline,plotfn plotrect,plotfn plotrectfilled,plottextfn plottext);

int Graphics_GetSize(void);

void Graphics_Save(FILE *file);


#endif
