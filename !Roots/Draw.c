/*
	FT - Draw
	© Alex Waugh 1999

	$Id: Draw.c,v 1.4 2000/02/25 16:59:20 uid1 Exp $

*/

#include "Desk.Error2.h"
#include "Desk.Font2.h"
#include "Desk.ColourTran.h"
#include "Desk.SWI.h"

#include "AJWLib.Draw.h"
#include "AJWLib.Assert.h"

#include <stdlib.h>

#define SWI_ColourTrans_SetFontColours 0x4074F
#define ECF (1<<8) /*Or 0 for no ECFs*/

static os_trfm matrix;
static int scalefactor=100;

void Draw_SetScaleFactor(const int scale)
{
	AJWLib_Assert(scale>0);
	scalefactor=scale;
}

int Draw_GetScaleFactor(void)
{
	return scalefactor;
}

static void Draw_SetMatrix(const int originx,const originy)
{
	int factor;
	factor=scalefactor*((1<<24)/100);
	matrix.entries[0][0]=factor;
	matrix.entries[0][1]=0;
	matrix.entries[1][0]=0;
	matrix.entries[1][1]=factor;
	matrix.entries[2][0]=originx<<8;
	matrix.entries[2][1]=originy<<8;
}

void Draw_PlotRectangle(const int originx,const int originy,const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,ECF,0);
	Draw_SetMatrix(originx,originy);
	AJWLib_Draw_PlotRectangle(x,y,width,height,linethickness,&matrix);
}

void Draw_PlotRectangleFilled(const int originx,const int originy,const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,ECF,0);
	Draw_SetMatrix(originx,originy);
	AJWLib_Draw_PlotRectangleFilled(x,y,width,height,&matrix);
}

void Draw_PlotLine(const int originx,const int originy,const int minx,const int miny,const int maxx,const int maxy,const int linethickness,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,ECF,0);
	Draw_SetMatrix(originx,originy);
	AJWLib_Draw_PlotLine(minx,miny,maxx,maxy,linethickness,&matrix);
}

void Draw_PlotText(const int originx,const int originy,const int x,const int y,const int handle,const char *font,const int size,const unsigned int bgcolour,const unsigned int fgcolour,const char *text)
{
	int millix,milliy;
	Desk_font_transformation fontmatrix;
	Desk_Font_ConvertToPoints(originx,originy,&millix,&milliy);
	/*Font matrix is slightly different as coords are in millipoints, and x and y do not get scaled by font manager*/
	fontmatrix.scale.xx=scalefactor*((1<<16)/100);
	fontmatrix.scale.xy=0;
	fontmatrix.scale.yx=0;
	fontmatrix.scale.yy=scalefactor*((1<<16)/100);
	fontmatrix.translate.x=millix;
	fontmatrix.translate.y=milliy;
	Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,bgcolour,fgcolour,14);
	Desk_Font_ConvertToPoints(x,y,&millix,&milliy);
	millix=(millix*scalefactor)/100;
	milliy=(milliy*scalefactor)/100;
	Desk_Font_Paint3(handle,text,Desk_font_plot_TRANSMATRIX,millix,milliy,NULL,&fontmatrix,0); /*Background blending?*/
}

void Draw_EORRectangle(const int originx,const int originy,const int x,const int y,const int width,const int height,const int linethickness,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0,3);
	Draw_SetMatrix(originx,originy);
	AJWLib_Draw_PlotRectangle(x,y,width,height,linethickness,&matrix);
}

void Draw_EORRectangleFilled(const int originx,const int originy,const int x,const int y,const int width,const int height,const unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0,3);
	Draw_SetMatrix(originx,originy);
	AJWLib_Draw_PlotRectangleFilled(x,y,width,height,&matrix);
}

