/*
	FT - Drawfile
	© Alex Waugh 1999

	$Id: Drawfile.c,v 1.19 2000/09/25 18:44:10 AJW Exp $

*/

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
#include "Desk.SWI.h"
#include "Desk.PDriver.h"

#include "AJWLib.Error2.h"
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
#include "Windows.h"
#include "Config.h"
#include "Layout.h"
#include "Drawfile.h"

#define SWI_Drawfile_DeclareFonts 0x45542
#define SWI_Drawfile_Render 0x45540

#define SWAP(x,y) \
{ \
	int temp=x; \
	x=y; \
	y=temp; \
}

static drawfile_diagram *drawfile=NULL;
static char *fontarray[256];
static int numberoffonts=0;

static void Drawfile_PlotRectangle2(int x,int y,int width,int height,int linethickness,unsigned int colour,Desk_bool filled)
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

void Drawfile_PlotRectangle(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Desk_UNUSED(scale);
	Drawfile_PlotRectangle2(originx+x,originy+y,width,height,linethickness,colour,Desk_FALSE);
}

void Drawfile_PlotRectangleFilled(int scale,int originx,int originy,int x,int y,int width,int height,int linethickness,unsigned int colour)
{
	Desk_UNUSED(scale);
	Drawfile_PlotRectangle2(originx+x,originy+y,width,height,linethickness,colour,Desk_TRUE);
}

void Drawfile_PlotLine(int scale,int originx,int originy,int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour)
{
	int *object;
	const int sizeofpath=68;
	int currentsize=AJWLib_Flex_Size((flex_ptr)&drawfile);
	Desk_UNUSED(scale);
	AJWLib_Flex_Extend((flex_ptr)&drawfile,currentsize+sizeofpath);
	object=(int *)(((char*)drawfile)+currentsize); /*Casting to get addition correct*/
	object[0]=2; /*Path object*/
	object[1]=sizeofpath;
	object[2]=originx+(minx<<8); /*Boundingbox*/
	object[3]=originy+(miny<<8);
	object[4]=originx+(maxx<<8);
	object[5]=originy+(maxy<<8);
	object[6]=-1; /*Fill colour*/
	object[7]=colour;
	object[8]=linethickness;
	object[9]=0; /*? Path style*/
	object[10]=2; /*Move*/
	object[11]=originx+(minx<<8);
	object[12]=originy+(miny<<8);
	object[13]=8; /*Line*/
	object[14]=originx+(maxx<<8);
	object[15]=originy+(maxy<<8);
	object[16]=0; /*End path*/
}

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
		strcpy(fontarray[numberoffonts],font);
		fontnumber=++numberoffonts;
	}
	return fontnumber;
}

static void Drawfile_CreateTable(void)
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
	sizeoftable=(sizeoftable+3)&~3; /*Word align*/
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
	}
	while ((unsigned int)items & 3) *(items++)=0; /*Word align, pad with zeros*/
}

static void Drawfile_CreateOptions(int papersize,Desk_bool landscape)
{
	const int sizeoftable=64+24;
	int *object;
	AJWLib_Flex_MidExtend((flex_ptr)&drawfile,40,sizeoftable);
	object=(int *)((char *)drawfile+40);
	object[0]=11; /*Options tag*/
	object[1]=sizeoftable;
	object[2]=0; /*Boundingbox (not used)*/
	object[3]=0;
	object[4]=0;
	object[5]=0;
	object[6]=papersize; /*Paper size A3=&400 A4=&500 etc.*/
	object[7]=landscape ? 1<<4 : 0; /*Paper limits (bit 0 show limits, bit 4 landscape)*/
	object[8]=0x3FF00000; /*Grid spacing (floating point)*/
	object[9]=0; /*2nd part of grid spacing*/
	object[10]=2; /*Grid division*/
	object[11]=0; /*Grid (rectangular=0 isometric<>0)*/
	object[12]=0; /*Grid auto ajust?*/
	object[13]=0; /*Grid Shown*/
	object[14]=0; /*Grid lock*/
	object[15]=1; /*Units 0 inches <>0 cm*/
	object[16]=1; /*Zoom multiplier*/
	object[17]=1; /*Zoom divider*/
	object[18]=0; /*Zoom locking*/
	object[19]=1; /*Toolbox*/
	object[20]=1<<7; /*Mode bit 7=select*/
	object[21]=0; /*Bytes in undo buffer*/
}

static void Drawfile_PlotText(int scale,int originx,int originy,int x,int y,int handle,char *font,int size,unsigned int bgcolour,unsigned int fgcolour,char *text)
{
	int fontnumber=Drawfile_AddFont((char *)font);
	int *object;
	int sizeofpath=52;
	Desk_wimp_point *bbox;
	int currentsize=AJWLib_Flex_Size((flex_ptr)&drawfile);
	Desk_UNUSED(scale);
	Desk_UNUSED(handle);
	sizeofpath+=strlen(text)+4;
	sizeofpath&=~3; /*word align the size*/
	AJWLib_Flex_Extend((flex_ptr)&drawfile,currentsize+sizeofpath);
	bbox=AJWLib_Font_GetWidthAndHeightGiven(font,size*16,text);
	object=(int *)(((char*)drawfile)+currentsize); /*Casting to get addition correct*/
	object[0]=1; /*Text object*/
	object[1]=sizeofpath;
	object[2]=originx+(x<<8); /*Boundingbox*/
	object[3]=originy+(y<<8);
	object[4]=(originx+x+bbox->x)<<8;
	object[5]=(originy+y+bbox->y)<<8;
	object[6]=fgcolour; /*unsigned???*/
	object[7]=bgcolour;
	object[8]=fontnumber;
	object[9]=size*640; /*X size*/
	object[10]=size*640; /*Y size*/
	object[11]=originx+(x<<8);
	object[12]=originy+(y<<8);
	strcpy(((char *)object)+52,text);
}

static void Drawfile_Create(layout *layout,Desk_bool printing)
{
	Desk_wimp_rect box,cliprect;
	int paperwidth=21*70,paperheight=30*70; /*Get correct values*/
	int papersize=0x500;
	int width,height;
	int xoffset=0,yoffset=0;
	Desk_bool landscape=Desk_FALSE;
	AJWLib_Assert(drawfile==NULL);
	Drawfile_FreeFontArray();
	AJWLib_Flex_Alloc((flex_ptr)&drawfile,40);
	strcpy(drawfile->tag,"Draw");
	drawfile->major_version=201;
	drawfile->minor_version=0;
	strcpy(drawfile->source,"Roots       "); /*Padded with spaces*/
	box=Layout_FindExtent(layout,Desk_FALSE);
	width=box.max.x-box.min.x;
	height=box.max.y-box.min.y;
	cliprect.min.x=-INFINITY;
	cliprect.min.y=-INFINITY;
	cliprect.max.x=INFINITY;
	cliprect.max.y=INFINITY;
	if (!printing) {
		if (width>height) {
			SWAP(width,height);
			landscape=Desk_TRUE;
		}
		for (papersize=0x500;papersize>0x100;papersize-=0x100) {
			if (width<paperwidth && height<paperheight) break;
			SWAP(paperwidth,paperheight);
			paperheight*=2;
		}
		xoffset=(paperwidth-width)/2;
		yoffset=(paperheight-height)/2;
		if (landscape) SWAP(xoffset,yoffset);
	}
	drawfile->bbox.min.x=xoffset;
	drawfile->bbox.min.y=yoffset;
	drawfile->bbox.max.x=(xoffset+box.max.x-box.min.x)<<8;
	drawfile->bbox.max.y=(yoffset+box.max.y-box.min.y)<<8;
	Graphics_Redraw(layout,100,xoffset-box.min.x,yoffset-box.min.y,&cliprect,Desk_FALSE,Drawfile_PlotLine,Drawfile_PlotRectangle,Drawfile_PlotRectangleFilled,Drawfile_PlotText);
	if (!printing) Drawfile_CreateOptions(papersize,landscape);
	Drawfile_CreateTable();
}

void Drawfile_Free(void)
{
	if (drawfile) AJWLib_Flex_Free((flex_ptr)&drawfile);
	Drawfile_FreeFontArray();
}

void Drawfile_Print(layout *layout)
{
	Desk_printer_info infoblk;
	AJWLib_Assert(layout!=NULL);
	Desk_Error2_Try {
		Drawfile_Create(layout,Desk_TRUE);
		Desk_PDriver_Info(&infoblk);
		if (infoblk.features.data.declarefont) Desk_Error2_CheckOS(Desk_SWI(3,0,SWI_Drawfile_DeclareFonts,0,drawfile,AJWLib_Flex_Size((flex_ptr)&drawfile)));
	} Desk_Error2_Catch {
		Drawfile_Free();
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
}

void Drawfile_Redraw(int scale,int originx,int originy,Desk_wimp_rect *cliprect)
{
	int matrix[6];
	AJWLib_Assert(drawfile!=NULL);
	matrix[0]=(scale<<16)/100;
	matrix[1]=0;
	matrix[2]=0;
	matrix[3]=(scale<<16)/100;
	matrix[4]=originx<<8;
	matrix[5]=originy<<8;
	Desk_Error2_CheckOS(Desk_SWI(5,0,SWI_Drawfile_Render,0,drawfile,AJWLib_Flex_Size((flex_ptr)&drawfile),matrix,cliprect));
}

void Drawfile_Save(char *filename,layout *layout)
{
	Desk_Error2_Try {
		Drawfile_Create(layout,Desk_FALSE);
		Desk_File_SaveMemory2(filename,drawfile,AJWLib_Flex_Size((flex_ptr)&drawfile),Desk_filetype_DRAWFILE);
	} Desk_Error2_Catch {
		Drawfile_Free();
		AJWLib_Error2_ReportMsgs("Error.DrawS:%s");
	} Desk_Error2_EndCatch
	Drawfile_Free();
}
