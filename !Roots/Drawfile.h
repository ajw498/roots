#ifndef DRAWFILE_H
#define DRAWFILE_H

void Drawfile_PlotLine(const int originx,const int originy,const int minx,const int miny,const int maxx,const int maxy,const int linethickness,const unsigned int colour);

void Drawfile_PlotRectangle(const int originx,const int originy,const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour);

void Drawfile_PlotRectangleFilled(const int originx,const int originy,const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour);

void Drawfile_Save(const char *filename,layout *layout);

#endif
