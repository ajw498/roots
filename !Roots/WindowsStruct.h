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

typedef void (*plotfn)(int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour);
typedef void (*plottextfn)(int x,int y,int handle,char *font,int size,unsigned int bgcolour, unsigned int fgcolour,char *text);

void Graphics_Init(void);

void Graphics_Edit(void);

void Graphics_ChangedLayout(void);

void Graphics_OpenWindow(wintype type,elementptr person,int generations);

void Graphics_Relayout(void);

void Graphics_Redraw(layout *layout,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection,plotfn plotline,plotfn plotrect,plotfn plotrectfilled,plottextfn plottext);

#endif
