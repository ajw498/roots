/*
	FT - Drawfile
	© Alex Waugh 1999

	$Log: Drawfile.c,v $
	Revision 1.5  2000/01/08 20:00:38  AJW
	Removed custom PlotPerson etc.

	Revision 1.4  2000/01/08 16:09:05  AJW
	Changed to Drawfile

	Revision 1.3  2000/01/07 17:52:27  AJW
	Got Lines and boxes outputting to a drawfile correctly

	Revision 1.2  2000/01/06 17:17:47  AJW
	Saves Child lines (but coords currently wrong)

	Revision 1.1  1999/10/24 13:36:22  AJW
	Initial revision


*/

/*	Includes  */

#include "Desk.Window.h"
#include "Desk.Error2.h"
#include "Desk.Event.h"
#include "Desk.EventMsg.h"
#include "Desk.Handler.h"
#include "Desk.Hourglass.h"
#include "Desk.Icon.h"
#include "Desk.Menu.h"
#include "Desk.Msgs.h"
#include "Desk.Drag.h"
#include "Desk.Resource.h"
#include "Desk.Screen.h"
#include "Desk.Template.h"
#include "Desk.File.h"
#include "Desk.Filing.h"
#include "Desk.Sprite.h"
#include "Desk.Screen.h"
#include "Desk.GFX.h"
#include "Desk.Font2.h"
#include "Desk.ColourTran.h"

#include "AJWLib.Window.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Assert.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Icon.h"
#include "AJWLib.Flex.h"
#include "AJWLib.Font.h"
#include "AJWLib.Str.h"
#include "AJWLib.Draw.h"
#include "AJWLib.DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "Modules.h"
#include "GConfig.h"
#include "Config.h"
#include "Layout.h"
#include "Drawfile.h"

extern graphics graphicsdata;
static drawfile_diagram *drawfile=NULL;

void Drawfile_PlotRectangle2(int x,int y,int width,int height,int linethickness,unsigned int colour,Desk_bool filled)
{
	int *object;
	const int sizeofpath=96;
	int currentsize=AJWLib_Flex_Size((flex_ptr)&drawfile);
	AJWLib_Flex_Extend((flex_ptr)&drawfile,currentsize+sizeofpath);
	object=(int *)(((char*)drawfile)+currentsize); /*Casting to get addition correct*/
	object[0]=2; /*Path object*/
	object[1]=sizeofpath;
	object[2]=x<<8; /*Boundingbox*/
	object[3]=y<<8;
	object[4]=(x+width)<<8;
	object[5]=(y+height)<<8;
	object[6]=filled ? colour : -1; /*Fill colour*/
	object[7]=filled ? -1 : colour; /*Line colour*/
	object[8]=linethickness;
	object[9]=0; /*? Path style*/
	object[10]=2; /*Move*/
	object[11]=x<<8;
	object[12]=y<<8;
	object[13]=8; /*Line*/
	object[14]=(x+width)<<8;
	object[15]=y<<8;
	object[16]=8; /*Line*/
	object[17]=(x+width)<<8;
	object[18]=(y+height)<<8;
	object[19]=8; /*Line*/
	object[20]=x<<8;
	object[21]=(y+height)<<8;
	object[22]=5; /*Close subpath*/
	object[23]=0; /*End path*/
}

void Drawfile_PlotRectangle(int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Drawfile_PlotRectangle2(x,y,width,height,linethickness,colour,Desk_FALSE);
}

void Drawfile_PlotRectangleFilled(int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Drawfile_PlotRectangle2(x,y,width,height,linethickness,colour,Desk_TRUE);
}

void Drawfile_PlotLine(int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour)
{
	int *object;
	const int sizeofpath=68;
	int currentsize=AJWLib_Flex_Size((flex_ptr)&drawfile);
	AJWLib_Flex_Extend((flex_ptr)&drawfile,currentsize+sizeofpath);
	object=(int *)(((char*)drawfile)+currentsize); /*Casting to get addition correct*/
	object[0]=2; /*Path object*/
	object[1]=sizeofpath;
	object[2]=minx<<8; /*Boundingbox*/
	object[3]=miny<<8;
	object[4]=maxx<<8;
	object[5]=maxy<<8;
	object[6]=-1; /*Fill colour*/
	object[7]=colour;
	object[8]=linethickness;
	object[9]=0; /*? Path style*/
	object[10]=2; /*Move*/
	object[11]=minx<<8;
	object[12]=miny<<8;
	object[13]=8; /*Line*/
	object[14]=maxx<<8;
	object[15]=maxy<<8;
	object[16]=0; /*End path*/
}

/*char fontarraythingy*/
void Drawfile_PlotText(int x,int y,int handle,char *font,int size,unsigned int bgcolour, unsigned int fgcolour,char *text)
{
	/**/
}

void Drawfile_Save(char *filename,layout *layout)
{
	Desk_wimp_rect box;
	AJWLib_Flex_Alloc((flex_ptr)&drawfile,40);
	strcpy(drawfile->tag,"Draw");
	drawfile->major_version=201;
	drawfile->minor_version=0;
	strcpy(drawfile->source,"Roots       "); /*Padded with spaces*/
	box=Layout_FindExtent(layout,Desk_FALSE);
	drawfile->bbox.min.x=0; /*offset?*/
	drawfile->bbox.min.y=0;
	drawfile->bbox.max.x=(box.max.x-box.min.x)<<8;
	drawfile->bbox.max.y=(box.max.y-box.min.y)<<8;
	Graphics_Redraw(layout,-box.min.x,-box.min.y,&box,Desk_TRUE,Drawfile_PlotLine,Drawfile_PlotRectangle,Drawfile_PlotRectangleFilled,Drawfile_PlotText);
	Desk_File_SaveMemory2(filename,drawfile,AJWLib_Flex_Size((flex_ptr)&drawfile),Desk_filetype_DRAWFILE);
	AJWLib_Flex_Free((flex_ptr)&drawfile);
}
