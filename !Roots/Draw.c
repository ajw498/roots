/*
	FT - Draw
	© Alex Waugh 1999

	$Id: Draw.c,v 1.8 2000/06/17 21:25:45 AJW Exp $

*/

#include "Desk.Error2.h"
#include "Desk.Font2.h"
#include "Desk.ColourTran.h"
#include "Desk.SWI.h"

#include "AJWLib.Draw.h"
#include "AJWLib.Assert.h"

#include "Draw.h"

#include <stdlib.h>

#define SWI_ColourTrans_SetFontColours 0x4074F
#define ECF (1<<8) /*Or 0 for no ECFs*/

static os_trfm matrix;

static void Draw_SetMatrix(int scale,int originx,int originy)
{
	int factor;
	factor=(scale<<24)/100;
	matrix.entries[0][0]=factor;
	matrix.entries[0][1]=0;
	matrix.entries[1][0]=0;
	matrix.entries[1][1]=factor;
	matrix.entries[2][0]=originx<<8;
	matrix.entries[2][1]=originy<<8;
}

void Draw_PlotRectangle(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,ECF,0);
	Draw_SetMatrix(scale,originx,originy);
	AJWLib_Draw_PlotRectangle(x,y,width,height,linethickness,&matrix);
}

void Draw_PlotRectangleFilled(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Desk_UNUSED(linethickness);
	Desk_ColourTrans_SetGCOL(colour,ECF,0);
	Draw_SetMatrix(scale,originx,originy);
	AJWLib_Draw_PlotRectangleFilled(x,y,width,height,&matrix);
}

void Draw_PlotLine(int scale,int originx,int originy,int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,ECF,0);
	Draw_SetMatrix(scale,originx,originy);
	AJWLib_Draw_PlotLine(minx,miny,maxx,maxy,linethickness,&matrix);
}

void Draw_PlotText(int scale,int originx,int originy,int x,int y,int handle,char *font,int size,unsigned int bgcolour,unsigned int fgcolour,char *text)
{
	int millix,milliy;
	Desk_font_transformation fontmatrix;
	Desk_UNUSED(font);
	Desk_UNUSED(size);
	Desk_Font_ConvertToPoints(originx,originy,&millix,&milliy);
	/*Font matrix is slightly different as coords are in millipoints, and x and y do not get scaled by font manager*/
	fontmatrix.scale.xx=scale*((1<<16)/100);
	fontmatrix.scale.xy=0;
	fontmatrix.scale.yx=0;
	fontmatrix.scale.yy=scale*((1<<16)/100);
	fontmatrix.translate.x=millix;
	fontmatrix.translate.y=milliy;
	Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,bgcolour,fgcolour,14);
	Desk_Font_ConvertToPoints(x,y,&millix,&milliy);
	millix=(millix*scale)/100;
	milliy=(milliy*scale)/100;
	/*Background blending?*/
	Desk_Font_Paint3(handle,text,Desk_font_plot_TRANSMATRIX | Desk_font_plot_CURRENTHANDLE,millix,milliy,NULL,&fontmatrix,0);
}

void Draw_EORRectangle(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0,3);
	Draw_SetMatrix(scale,originx,originy);
	AJWLib_Draw_PlotRectangle(x,y,width,height,linethickness,&matrix);
}

void Draw_EORRectangleFilled(int scale,int originx,int originy,int x,int y,int width,int height,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0,3);
	Draw_SetMatrix(scale,originx,originy);
	AJWLib_Draw_PlotRectangleFilled(x,y,width,height,&matrix);
}

