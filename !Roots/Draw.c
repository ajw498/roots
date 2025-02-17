/*
	Roots - Draw routines to plot to screen
	� Alex Waugh 1999

	$Id: Draw.c,v 1.14 2002/08/01 14:56:58 ajw Exp $

*/

#include "Desk/Error2.h"
#include "Desk/Font2.h"
#include "Desk/ColourTran.h"
#include "Desk/SWI.h"

#include "AJWLib/Draw.h"
#include "AJWLib/Assert.h"

#include "Config.h"
#include "Draw.h"

#include <stdlib.h>

#define SWI_ColourTrans_SetFontColours 0x4074F
#define ECF (1<<8) /*Or 0 for no ECFs*/

static os_trfm matrix;

static void Draw_SetMatrix(int scale,int originx,int originy)
{
	int factor;
	factor=scale*((1<<24)/100);
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
	Draw_SetMatrix(scale,originx+(x*scale)/100,originy+(y*scale)/100);
	AJWLib_Draw_PlotRectangle(0,0,width,height,linethickness,&matrix);
}

void Draw_PlotRectangleFilled(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Desk_UNUSED(linethickness);
	Desk_ColourTrans_SetGCOL(colour,ECF,0);
	Draw_SetMatrix(scale,originx+(x*scale)/100,originy+(y*scale)/100);
	AJWLib_Draw_PlotRectangleFilled(0,0,width,height,&matrix);
}

void Draw_PlotLine(int scale,int originx,int originy,int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,ECF,0);
	Draw_SetMatrix(scale,originx+(minx*scale)/100,originy+(miny*scale)/100);
	AJWLib_Draw_PlotLine(0,0,maxx-minx,maxy-miny,linethickness,&matrix);
}

void Draw_PlotText(int scale,int originx,int originy,int x,int y,int handle,char *font,int size,unsigned int bgcolour,unsigned int fgcolour,char *text)
{
	int millix,milliy,originmillix,originmilliy;
	Desk_font_transformation fontmatrix;
	int backgroundblend=0;
	Desk_bool lose=Desk_FALSE;

	if (handle==0) {
		Desk_Font_FindFont(&handle,font,size*16,size*16,0,0);
		lose=Desk_TRUE;
	}
	Desk_Font_ConvertToPoints(originx,originy,&originmillix,&originmilliy);
	/*Font matrix is slightly different as coords are in millipoints, and x and y do not get scaled by font manager*/
	fontmatrix.scale.xx=scale*((1<<16)/100);
	fontmatrix.scale.xy=0;
	fontmatrix.scale.yx=0;
	fontmatrix.scale.yy=scale*((1<<16)/100);
	fontmatrix.translate.x=0; /*Puttting the origins here seems to cause positioning errrors on large trees*/
	fontmatrix.translate.y=0;
	Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,bgcolour,fgcolour,14);
	Desk_Font_ConvertToPoints(x,y,&millix,&milliy);
	if (Config_FontBlend()) backgroundblend=1<<11;
	Desk_Font_Paint3(handle,text,Desk_font_plot_TRANSMATRIX | Desk_font_plot_CURRENTHANDLE | backgroundblend,(millix*scale)/100+originmillix,(milliy*scale)/100+originmilliy,NULL,&fontmatrix,0);
	if (lose) Desk_Font_LoseFont(handle);
}

static void Draw_SetDrawfileMatrix(int scale,int originx,int originy)
{
	int factor;
	factor=scale*((1<<16)/100);
	matrix.entries[0][0]=factor;
	matrix.entries[0][1]=0;
	matrix.entries[1][0]=0;
	matrix.entries[1][1]=factor;
	matrix.entries[2][0]=originx<<8;
	matrix.entries[2][1]=originy<<8;
}

void Draw_PlotDrawfile(int scale,int originx,int originy,int x,int y,drawfile_diagram *drawfile,int size,Desk_wimp_box *cliprect)
{
	Draw_SetDrawfileMatrix(scale,originx+(x*scale)/100,originy+(y*scale)/100);
	DrawFile_Render(0,drawfile,size,&matrix,cliprect,0);
}

void Draw_EORRectangle(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0,3);
	Draw_SetMatrix(scale,originx+(x*scale)/100,originy+(y*scale)/100);
	AJWLib_Draw_PlotRectangle(0,0,width,height,linethickness,&matrix);
}

void Draw_EORRectangleFilled(int scale,int originx,int originy,int x,int y,int width,int height,unsigned int colour)
{
	Desk_ColourTrans_SetGCOL(colour,0,3);
	Draw_SetMatrix(scale,originx+(x*scale)/100,originy+(y*scale)/100);
	AJWLib_Draw_PlotRectangleFilled(0,0,width,height,&matrix);
}

