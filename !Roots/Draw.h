#ifndef DRAW_H
#define DRAW_H

void Draw_SetScaleFactor(const int scale);

int Draw_GetScaleFactor(void);

void Draw_PlotRectangle(const int originx,const int originy,const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour);

void Draw_PlotRectangleFilled(const int originx,const int originy,const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour);

void Draw_PlotLine(const int originx,const int originy,const int minx,const int miny,const int maxx,const int maxy,const int linethickness,const unsigned int colour);

void Draw_PlotText(const int originx,const int originy,const int x,const int y,const int handle,const char *font,const int size,const unsigned int bgcolour,const unsigned int fgcolour,const char *text);

void Draw_EORRectangle(const int originx,const int originy,const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour);

void Draw_EORRectangleFilled(const int originx,const int originy,const int x,const int y,const int width,const int height,const unsigned int colour);

#endif
