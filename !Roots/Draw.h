#ifndef DRAW_H
#define DRAW_H

void Draw_PlotRectangle(int x,int y,int width,int height,int linethickness,unsigned int colour);

void Draw_PlotRectangleFilled(int x,int y,int width,int height,int linethickness,unsigned int colour);

void Draw_PlotLine(int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour);

void Draw_PlotText(int x,int y,int handle,char *font,int size,unsigned int bgcolour, unsigned int fgcolour,char *text);

#endif
