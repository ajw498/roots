#ifndef DRAWFILE_H
#define DRAWFILE_H

void Drawfile_PlotLine(int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour);

void Drawfile_PlotRectangle(int x,int y,int width,int height,int linethickness,unsigned int colour);

void Drawfile_PlotRectangleFilled(int x,int y,int width,int height,int linethickness,unsigned int colour);

void Drawfile_Save(char *filename,layout *layout);

#endif
