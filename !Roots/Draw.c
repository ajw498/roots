/*
	FT - Draw
	© Alex Waugh 1999

	$Log: Draw.c,v $
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

extern os_trfm matrix;

void Draw_PlotRectangle(int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0/*or 1<<8 to use ECFs ?*/,0);
	AJWLib_Draw_PlotRectangle(x,y,width,height,linethickness,&matrix);
}

void Draw_PlotRectangleFilled(int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0/*or 1<<8 to use ECFs ?*/,0);
	AJWLib_Draw_PlotRectangleFilled(x,y,width,height,&matrix);
}

void Draw_PlotLine(int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0/*or 1<<8 to use ECFs ?*/,0);
	AJWLib_Draw_PlotLine(minx,miny,maxx,maxy,linethickness,&matrix);
}

void Draw_PlotText(int x,int y,int handle,char *font,int size,unsigned int bgcolour, unsigned int fgcolour,char *text)
{
	Desk_Font_SetFont(handle);
	Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,bgcolour,fgcolour,14);
	Desk_Font_Paint(text,Desk_font_plot_OSCOORS,x,y);
}
