/*
	FT - Drawfile
	© Alex Waugh 1999

	$Log: Drawfile.c,v $
	Revision 1.6  2000/01/09 17:50:08  AJW
	Added text functions

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
#include "Desk.DeskMem.h"
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

static char *fontarray[256];
static int numberoffonts=0;

static void Drawfile_FreeFontArray(void)
{
	int i;
	for (i=0;i<numberoffonts;i++) Desk_DeskMem_Free(fontarray[i]);
	numberoffonts=0;
}

static int Drawfile_AddFont(char *font)
{
	int i;
	int fontnumber=0;
	for (i=0;i<numberoffonts;i++) {
		if (strcmp(fontarray[i],font)==0) {
			fontnumber=i+1; /*font number 0 is the system font*/
		}
	}
	if (fontnumber==0) {
		fontarray[numberoffonts]=Desk_DeskMem_Malloc(strlen(font)+1);
		/*freeing if an error?*/
		strcpy(fontarray[numberoffonts++],font);
		fontnumber=numberoffonts;
	}
	return fontnumber;
}

void Drawfile_CreateTable(void)
{
	int sizeoftable=8;
	int i;
	int *object;
	char *items;
	for (i=0;i<numberoffonts;i++) {
		int sizeofentry=strlen(fontarray[i])+1+1; /*strlen+terminator+fontnumber*/
		sizeofentry=(sizeofentry+3)&~3; /*Word align*/
		sizeoftable+=sizeofentry;
	}
	AJWLib_Flex_MidExtend((flex_ptr)&drawfile,40,sizeoftable);
	object=(int *)((char *)drawfile+40);
	items=(char *)drawfile+48;
	object[0]=0; /*Font table tag*/
	object[1]=sizeoftable;
	for (i=0;i<numberoffonts;i++) {
		int sizeofentry=strlen(fontarray[i])+1; /*strlen+terminator*/
		*(items++)=i+1;
		strcpy(items,fontarray[i]);
		items+=sizeofentry;
		items=(char *)(((unsigned int)items+3)&~3); /*Word align*/
	}
}

void Drawfile_PlotText(int x,int y,int handle,char *font,int size,unsigned int bgcolour,unsigned int fgcolour,char *text)
{
	int fontnumber=Drawfile_AddFont(font);
	int *object;
	int sizeofpath=52;
	int currentsize=AJWLib_Flex_Size((flex_ptr)&drawfile);
	sizeofpath+=strlen(text)+4;
	sizeofpath&=~3; /*word align the size*/
	AJWLib_Flex_Extend((flex_ptr)&drawfile,currentsize+sizeofpath);
	object=(int *)(((char*)drawfile)+currentsize); /*Casting to get addition correct*/
	object[0]=1; /*Text object*/
	object[1]=sizeofpath;
	object[2]=x<<8; /*Boundingbox*/
	object[3]=y<<8;
	object[4]=(x+1000)<<8;
	object[5]=(y+1000)<<8;
	object[6]=fgcolour; /*unsigned???*/
	object[7]=bgcolour;
	object[8]=fontnumber;
	object[9]=size*640; /*X size*/
	object[10]=size*640; /*Y size*/
	object[11]=x<<8;
	object[12]=y<<8;
	strcpy(((char *)object)+52,text);
}

void Drawfile_Save(char *filename,layout *layout)
{
	Desk_wimp_rect box;
	Drawfile_FreeFontArray();
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
	Graphics_Redraw(layout,-box.min.x,-box.min.y,&box,Desk_FALSE,Drawfile_PlotLine,Drawfile_PlotRectangle,Drawfile_PlotRectangleFilled,Drawfile_PlotText);
	Drawfile_CreateTable();
	Desk_File_SaveMemory2(filename,drawfile,AJWLib_Flex_Size((flex_ptr)&drawfile),Desk_filetype_DRAWFILE);
	AJWLib_Flex_Free((flex_ptr)&drawfile);
	Drawfile_FreeFontArray();
}
