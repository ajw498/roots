/*
	FT - Print, printing code
	© Alex Waugh 2000

	$Id: Print.c,v 1.2 2000/06/10 22:38:47 AJW Exp $

*/

#include "Desk.Window.h"
#include "Desk.Error.h"
#include "Desk.Error2.h"
#include "Desk.SWI.h"
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
#include "Desk.Save.h"
#include "Desk.Str.h"
#include "Desk.Font2.h"
#include "Desk.ColourTran.h"
#include "Desk.PDriver.h"
#include "Desk.Print.h"

#include "AJWLib.Error2.h"
#include "AJWLib.Window.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Assert.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Icon.h"
#include "AJWLib.Flex.h"
#include "AJWLib.Font.h"
#include "AJWLib.File.h"
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
#include "Draw.h"
#include "File.h"
#include "Print.h"

#define print_OK 14
#define print_CANCEL 13
#define print_UPRIGHT 11
#define print_SIDEWAYS 12
#define print_COPIES 1
#define print_COPIESDOWN 2
#define print_COPIESUP 3
#define print_SCALE 4
#define print_SCALEDOWN 5
#define print_SCALEUP 6
#define print_OVERLAP 8
#define print_OVERLAPDOWN 9
#define print_OVERLAPUP 10
#define print_NUMPAGES 19
#define print_CENTRE 20

#define SWI_PDriver_PageSize 0x80143

#define SWAP(x,y) {\
	int temp=x;\
	x=y;\
	y=temp;\
}


typedef struct printdata {
	int copies;
	int scale;
	Desk_bool portrait;
	int overlap;
	layout *layout;
	Desk_wimp_rect printablearea;
	Desk_wimp_rect treeextent;
	Desk_wimp_point pagesize;
	Desk_wimp_point pagesizeos;
	Desk_wimp_point offset;
	Desk_wimp_point pages;
} printdata;

static Desk_window_handle printwin;
static printdata data;

static void Print_PlotText(const int scale,const int originx,const int originy,const int x,const int y,const int handle,const char *font,const int size,const unsigned int bgcolour,const unsigned int fgcolour,const char *text)
{
}

static void Print_CalcValues(void)
{
	int total;
	data.copies=1;
	data.portrait=Desk_TRUE;
	data.scale=100;
	data.overlap=10;
	Desk_Font_ConvertToOS(data.printablearea.max.x-data.printablearea.min.x,data.printablearea.max.y-data.printablearea.min.y,&data.pagesizeos.x,&data.pagesizeos.y);
	if (!data.portrait) SWAP(data.pagesizeos.x,data.pagesizeos.y);
	/*Scale factor?*/
	data.pages.x=0;
	total=data.treeextent.max.x-data.treeextent.min.x;
	while (total>0) {
		data.pages.x++;
		total-=data.pagesizeos.x-data.overlap;
	}
	if (/*Centre*/0) data.offset.x=(-total)/2; else data.offset.x=0;
	data.pages.y=0;
	total=data.treeextent.max.y-data.treeextent.min.y;
	while (total>0) {
		data.pages.y++;
		total-=data.pagesizeos.y-data.overlap;
	}
	if (/*Centre*/0) data.offset.y=(-total)/2; else data.offset.y=-total;
}

static Desk_bool Print_StartPrinting(Desk_print_block *printblk)
{
	Desk_wimp_point position;
	Desk_wimp_rect rect,cliprect;
	Desk_print_transformation matrix;
	int more,rectid,pagex,pagey;
	printdata *ref=printblk->reference;

	if (ref->portrait) {
	/*Scale factor?*/
		matrix.xx=1<<16;
		matrix.xy=0;
		matrix.yy=1<<16;
		matrix.yx=0;
	} else {
		matrix.xx=0;
		matrix.xy=1<<16;
		matrix.yy=0;
		matrix.yx=1<<16;
	}
	Desk_Error2_Try {
		Desk_printer_info infoblk;
		Desk_PDriver_Info(&infoblk);
		if (infoblk.features.data.declarefont) Graphics_DeclareFonts();
		for (pagey=data.pages.y-1;pagey>=0;pagey--) {
			for (pagex=0;pagex<data.pages.x;pagex++) {
				rect.min.x=0; /*Minus a point as in the PRMS?*/
				rect.min.y=0;
				rect.max.x=rect.min.x+ref->pagesizeos.x;
				rect.max.y=rect.min.y+ref->pagesizeos.y;
				position.x=ref->printablearea.min.x;
				position.y=ref->printablearea.min.y;
				Desk_PDriver_GiveRectangle(1,&rect,&matrix,&position,0xFFFFFF00);
				Desk_PDriver_DrawPage(ref->copies,&cliprect,0,NULL,&more,&rectid);
				while (more) {
/*					Desk_ColourTrans_SetGCOL(0x0,0,0);
					Desk_GFX_Rectangle(0,0,100,100);
*/					Graphics_Redraw(ref->layout,ref->scale,ref->offset.x-ref->treeextent.min.x-(ref->pagesizeos.x-ref->overlap)*pagex,ref->offset.y-ref->treeextent.min.y-(ref->pagesizeos.y-ref->overlap)*pagey,&cliprect,Desk_FALSE,Draw_PlotLine,Draw_PlotRectangle,Draw_PlotRectangleFilled,Print_PlotText);
					Desk_PDriver_GetRectangle(&cliprect,&more,&rectid);
				}
			}
		}
	} Desk_Error2_Catch {
		Desk_PDriver_AbortJob(printblk->job);
		Desk_File_Close(printblk->job);
		AJWLib_Error2_ReportMsgs("Error.Print:%s");
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
	return Desk_TRUE;
}

static void Print_Result(Desk_print_block *printblk,Desk_print_result result)
{
	/**/
}

static Desk_bool Print_Ok(Desk_event_pollblock *block,void *ref)
{
	/*Check block for select button*/
	Desk_Print_StartPrint(Print_StartPrinting,NULL,Print_Result,&data,0,0,NULL,"Roots file");
	return Desk_TRUE;
}

void Print_OpenWindow(layout *layout)
{
	data.layout=layout;
/*	Desk_PDriver_PageSize(&pagesize,&printablearea);*/ /*Desk_PDriver_PageSize() seems to be broken*/
	Desk_SWI(0,7,SWI_PDriver_PageSize,NULL,&data.pagesize.x,&data.pagesize.y,&data.printablearea.min.x,&data.printablearea.min.y,&data.printablearea.max.x,&data.printablearea.max.y);
	data.treeextent=Layout_FindExtent(data.layout,Desk_FALSE);

	Print_CalcValues();

	Desk_Window_Show(printwin,Desk_open_CENTERED);
}

void Print_CloseWindow(void)
{
	Desk_Window_Hide(printwin);
}

void Print_Init(void)
{
	printwin=Desk_Window_Create("Print",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,printwin,print_OK,Print_Ok,NULL);
}
