#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Desk.Wimp.h"

#include "DatabaseStruct.h"
#include "LayoutStruct.h"

#define Graphics_ConvertToOS(x) ((int)(strtol(x,NULL,10)*7))
#define Graphics_ConvertFromOS(x) ((int)(x/7))

typedef void (*plotfn)(int scale,int originx,int originy,int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour);
typedef void (*plottextfn)(int scale,int originx,int originy,int x,int y,int handle,char *font,int size,unsigned int bgcolour,unsigned int fgcolour,char *text);

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
Desk_wimp_point *Graphics_TitleWidthAndHeight(void);
void Graphics_SetFunctions(plotfn plotline,plotfn plotrect,plotfn plotrectfilled,plottextfn plottext);
void Graphics_PlotElement(layout *layout,elementptr element,int scale,int originx,int originy,int x,int y,int width,int height,Desk_bool plotselection);
void Graphics_SaveGEDCOM(FILE *file);
void Graphics_RemoveStyle(void);
void Graphics_LoadStyle(char *style);
char *Graphics_GetCurrentStyle(void);
void Graphics_LoadPersonFileLine(char *line);
void Graphics_LoadMarriageFileLine(char *line);
void Graphics_LoadDimensionsFileLine(char *line);
void Graphics_LoadTitleFileLine(char *line);
void Graphics_SetGraphicsStyle(char *style);

#endif
