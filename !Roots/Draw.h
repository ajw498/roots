#ifndef DRAW_H
#define DRAW_H

void Draw_PlotRectangle(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour);

void Draw_PlotRectangleFilled(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour);

void Draw_PlotLine(int scale,int originx,int originy,int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour);

void Draw_PlotText(int scale,int originx,int originy,int x,int y,int handle,char *font,int size,unsigned int bgcolour,unsigned int fgcolour,char *text);

void Draw_EORRectangle(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour);

void Draw_EORRectangleFilled(int scale,int originx,int originy,int x,int y,int width,int height,unsigned int colour);

#endif
