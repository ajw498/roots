#ifndef DRAWFILE_H
#define DRAWFILE_H

void Drawfile_PlotLine(int scale,int originx,int originy,int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour);

void Drawfile_PlotRectangle(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour);

void Drawfile_PlotRectangleFilled(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour);

void Drawfile_Save(char *filename,layout *layout);

void Drawfile_Free(void);

void Drawfile_Print(layout *layout);

void Drawfile_Redraw(int scale,int originx,int originy,Desk_wimp_rect *cliprect);

void Drawfile_Create(layout *layout,Desk_wimp_rect *printcliprect);

#endif
