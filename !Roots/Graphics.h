#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Desk.Wimp.h"

#include "Layout.h"

typedef void (*plotfn)(const int scale,const int originx,const int originy,const int minx,const int miny,const int maxx,const int maxy,const int linethickness,const unsigned int colour);
typedef void (*plottextfn)(const int scale,const int originx,const int originy,const int x,const int y,const int handle,const char *font,const int size,const unsigned int bgcolour,const unsigned int fgcolour,const char *text);

int Graphics_PersonHeight(void);

int Graphics_PersonWidth(void);

int Graphics_UnlinkedGapHeight(void);

int Graphics_GapHeightAbove(void);

int Graphics_GapHeightBelow(void);

int Graphics_GapWidth(void);

int Graphics_MarriageWidth(void);

int Graphics_SecondMarriageGap(void);

int Graphics_WindowBorder(void);

int Graphics_TitleHeight(void);

void Graphics_Redraw(layout *layout,int scale,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection,plotfn plotline,plotfn plotrect,plotfn plotrectfilled,plottextfn plottext);

void Graphics_PlotPerson(int scale,int originx,int originy,elementptr person,int x,int y,Desk_bool child,Desk_bool selected);

int Graphics_GetSize(void);

void Graphics_Load(FILE *file);

void Graphics_Save(FILE *file);

void Graphics_RemoveStyle(void);

void Graphics_LoadStyle(char *style);

char *Graphics_GetCurrentStyle(void);

#endif
