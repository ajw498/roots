/*
	FT - Print, printing code
	© Alex Waugh 2000

	$Id: Print.c,v 1.1 2000/05/16 22:23:01 AJW Exp $

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

#define print_OK 19
#define print_CANCEL 18
#define print_UPRIGHT 16
#define print_SIDEWAYS 17
#define print_SCALETEXT 7
#define print_SCALETOFITTEXT 8
#define print_COPIES 1
#define print_COPIESDOWN 2
#define print_COPIESUP 3
#define print_SCALE 4
#define print_SCALEDOWN 5
#define print_SCALEUP 6
#define print_SCALEFIT 9
#define print_SCALEFITDOWN 10
#define print_SCALEFITUP 11
#define print_OVERLAP 13
#define print_OVERLAPDOWN 14
#define print_OVERLAPUP 15

typedef struct printdata {
	int copies;
	int scale;
	Desk_bool portrait;
    int overlap;
    layout *layout;
} printdata;

static Desk_window_handle printwin;
static printdata data;

static Desk_bool Print_StartPrinting(Desk_print_block *printblk)
{
	Desk_wimp_point pagesize,position,pagesizeos;
	Desk_wimp_rect printablearea,rect,cliprect,treeextent;
	Desk_print_transformation matrix;
	int more,rectid,pagex,pagey;
	printdata *ref=printblk->reference;

	Desk_PDriver_PageSize(&pagesize,&printablearea);
	treeextent=Layout_FindExtent(ref->layout,Desk_FALSE);
	Desk_Font_ConvertToOS(pagesize.x-(printablearea.min.x+printablearea.max.x),pagesize.y-(printablearea.min.y+printablearea.max.y),&pagesizeos.x,&pagesizeos.y);
	if (ref->portrait) {
	/*Scale factor?*/
		matrix.xx=1<<16;
		matrix.xy=0;
		matrix.yy=1<<16;
		matrix.yx=0;
	} else {
		int temp=pagesizeos.x;
		pagesizeos.x=pagesizeos.y;
		pagesizeos.y=temp;
		matrix.xx=0;
		matrix.xy=1<<16;
		matrix.yy=0;
		matrix.yx=1<<16;
	}
	Desk_Error2_Try {
		/*Declare fonts*/
		for (pagex=0;pagex<1/**/;pagex++) {
			for (pagey=0;pagey<1/**/;pagey++) {
				rect.min.x=treeextent.min.x+(pagesizeos.x-ref->overlap)*pagex; /*Minus a point as in the PRMS?*/
				rect.min.y=treeextent.min.y+(pagesizeos.y-ref->overlap)*pagey;
				rect.max.x=rect.min.x+pagesizeos.x;
				rect.max.y=rect.min.y+pagesizeos.y;
				position.x=printablearea.min.x;
				position.y=printablearea.min.y;
				Desk_PDriver_GiveRectangle(1,&rect,&matrix,&position,0xFFFFFF00);
				Desk_PDriver_DrawPage(ref->copies,&cliprect,0,NULL,&more,&rectid);
				while (more) {
					Graphics_Redraw(ref->layout,ref->scale,0,pagesizeos.y,&cliprect,Desk_FALSE,Draw_PlotLine,Draw_PlotRectangle,Draw_PlotRectangleFilled,Draw_PlotText);
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
	data.copies=1;
	data.portrait=Desk_TRUE;
	data.scale=100;
	data.overlap=10;
	data.layout=layout;
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
