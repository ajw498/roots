/*
	FT - Draw
	© Alex Waugh 1999

	$Log: Draw.c,v $
	Revision 1.2  2000/01/11 13:32:30  AJW
	Added Draw_SetScaleFactor
	Added lots of consts

	Revision 1.1  2000/01/08 20:00:17  AJW
	Initial revision


*/

/*	Includes  */

#include "Desk.Error2.h"
#include "Desk.Font2.h"
#include "Desk.ColourTran.h"
#include "Desk.SWI.h"

#include "AJWLib.Draw.h"

#define SWI_ColourTrans_SetFontColours 0x4074F

static os_trfm matrix;

void Draw_SetScaleFactor(const unsigned int factor)
{
	matrix.entries[0][0]=factor;
	matrix.entries[0][1]=0;
	matrix.entries[1][0]=0;
	matrix.entries[1][1]=factor;
	matrix.entries[2][0]=0;
	matrix.entries[2][1]=0;
}

void Draw_PlotRectangle(const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0/*or 1<<8 to use ECFs ?*/,0);
	AJWLib_Draw_PlotRectangle(x,y,width,height,linethickness,&matrix);
}

void Draw_PlotRectangleFilled(const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0/*or 1<<8 to use ECFs ?*/,0);
	AJWLib_Draw_PlotRectangleFilled(x,y,width,height,&matrix);
}

void Draw_PlotLine(const int minx,const int miny,const int maxx,const int maxy,const int linethickness,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0/*or 1<<8 to use ECFs ?*/,0);
	AJWLib_Draw_PlotLine(minx,miny,maxx,maxy,linethickness,&matrix);
}

void Draw_PlotText(const int x,const int y,const int handle,const char *font,const int size,const unsigned int bgcolour,const unsigned int fgcolour,const char *text)
{
	Desk_Font_SetFont(handle);
	Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,bgcolour,fgcolour,14);
	Desk_Font_Paint(text,Desk_font_plot_OSCOORS,x,y); /*Use transformation matrix*/
}

void Draw_EORRectangle(const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0,3);
	AJWLib_Draw_PlotRectangle(x,y,width,height,linethickness,&matrix);
}

void Draw_EORRectangleFilled(const int x,const int y,const int width,const int height,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0,3);
	AJWLib_Draw_PlotRectangle(x,y,width,height,0,&matrix);
}

