/*
	FT - Print, printing code
	© Alex Waugh 2000

	$Id: Print.c,v 1.9 2001/03/06 00:05:55 AJW Exp $

*/

#include "Desk/Window.h"
#include "Desk/Error.h"
#include "Desk/Error2.h"
#include "Desk/SWI.h"
#include "Desk/Event.h"
#include "Desk/EventMsg.h"
#include "Desk/Handler.h"
#include "Desk/Hourglass.h"
#include "Desk/Icon.h"
#include "Desk/Menu.h"
#include "Desk/Msgs.h"
#include "Desk/Drag.h"
#include "Desk/Resource.h"
#include "Desk/Screen.h"
#include "Desk/Template.h"
#include "Desk/File.h"
#include "Desk/Filing.h"
#include "Desk/Sprite.h"
#include "Desk/Screen.h"
#include "Desk/Keycodes.h"
#include "Desk/GFX.h"
#include "Desk/Save.h"
#include "Desk/Str.h"
#include "Desk/Font2.h"
#include "Desk/ColourTran.h"
#include "Desk/PDriver.h"
#include "Desk/Print.h"

#include "AJWLib/Error2.h"
#include "AJWLib/Window.h"
#include "AJWLib/Menu.h"
#include "AJWLib/Assert.h"
#include "AJWLib/Msgs.h"
#include "AJWLib/Icon.h"
#include "AJWLib/Flex.h"
#include "AJWLib/Font.h"
#include "AJWLib/File.h"
#include "AJWLib/Str.h"
#include "AJWLib/Draw.h"
#include "AJWLib/DrawFile.h"

#ifdef MemCheck_MEMCHECK
#include "MemCheck:MemCheck.h"
#endif

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
#define print_ALLPAGES 21
#define print_FROMPAGES 22
#define print_FROM 23
#define print_TO 25


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

static Desk_bool Print_CalcValues(Desk_event_pollblock *block,void *ref)
{
	int total;
	Desk_UNUSED(block);
	Desk_UNUSED(ref);
	data.copies=Desk_Icon_GetInteger(printwin,print_COPIES);
	data.portrait=Desk_Icon_GetSelect(printwin,print_UPRIGHT);
	data.scale=Desk_Icon_GetInteger(printwin,print_SCALE);
	if (data.scale<1) data.scale=1;
	if (data.copies<1) data.copies=1;
	data.overlap=Graphics_ConvertToOS(Desk_Icon_GetTextPtr(printwin,print_OVERLAP));
	Desk_Font_ConvertToOS(data.printablearea.max.x-data.printablearea.min.x,data.printablearea.max.y-data.printablearea.min.y,&data.pagesizeos.x,&data.pagesizeos.y);
/*	data.pagesizeos.x-=20;*/ /*So edges of output don't get clipped*/
/*	data.pagesizeos.y-=20;*/
	if (!data.portrait) SWAP(data.pagesizeos.x,data.pagesizeos.y);
	data.pages.x=0;
	total=((data.treeextent.max.x-data.treeextent.min.x)*data.scale)/100;
	while (total>0) {
		data.pages.x++;
		total-=(data.pagesizeos.x-data.overlap);
	}
	if (Desk_Icon_GetSelect(printwin,print_CENTRE)) data.offset.x=total/2; else data.offset.x=0;
	data.pages.y=0;
	total=((data.treeextent.max.y-data.treeextent.min.y)*data.scale)/100;
	while (total>0) {
		data.pages.y++;
		total-=(data.pagesizeos.y-data.overlap);
	}
	if (Desk_Icon_GetSelect(printwin,print_CENTRE)) data.offset.y=total/2; else data.offset.y=total;
	sprintf(Desk_Icon_GetTextPtr(printwin,print_NUMPAGES),"%d x %d",data.pages.x,data.pages.y);
	Desk_Icon_ForceRedraw(printwin,print_NUMPAGES);
	if (Desk_Icon_GetInteger(printwin,print_FROM)<1) Desk_Icon_SetInteger(printwin,print_FROM,1);
	if (Desk_Icon_GetInteger(printwin,print_TO)<1) Desk_Icon_SetInteger(printwin,print_TO,1);
	if (Desk_Icon_GetInteger(printwin,print_TO)>data.pages.x*data.pages.y) Desk_Icon_SetInteger(printwin,print_TO,data.pages.x*data.pages.y);
	if (Desk_Icon_GetInteger(printwin,print_FROM)>Desk_Icon_GetInteger(printwin,print_TO)) Desk_Icon_SetInteger(printwin,print_FROM,Desk_Icon_GetInteger(printwin,print_TO));
	return Desk_FALSE;
}

static Desk_bool Print_StartPrinting(Desk_print_block *printblk)
{
	Desk_wimp_point position;
	Desk_wimp_rect rect,cliprect;
	Desk_print_transformation matrix;
	int more,rectid,pagex,pagey,minpage,maxpage;
	printdata *ref=printblk->reference;

	Desk_Error2_Try {
		Desk_Hourglass_On();
		if (ref->portrait) {
			matrix.xx=1<<16;
			matrix.xy=0;
			matrix.yy=1<<16;
			matrix.yx=0;
			position.x=ref->printablearea.min.x;
			position.y=ref->printablearea.min.y;
		} else {
			matrix.xx=0;
			matrix.xy=-(1<<16);
			matrix.yx=(1<<16);
			matrix.yy=0;
			position.x=ref->printablearea.min.x;
			position.y=ref->printablearea.max.y;
		}
		if (Desk_Icon_GetSelect(printwin,print_ALLPAGES)) {
			minpage=0;
			maxpage=data.pages.x*data.pages.y-1;
		} else {
			minpage=Desk_Icon_GetInteger(printwin,print_FROM)-1;
			maxpage=Desk_Icon_GetInteger(printwin,print_TO)-1;
		}
		Drawfile_Print(ref->layout);
		for (pagey=data.pages.y-1;pagey>=0;pagey--) {
			for (pagex=0;pagex<data.pages.x;pagex++) {
				if (((data.pages.y-1-pagey)*data.pages.x+pagex>=minpage) && ((data.pages.y-1-pagey)*data.pages.x+pagex<=maxpage)) {
					Desk_wimp_rect page;
					
					page.min.x=(ref->offset.x*100)/ref->scale+((ref->pagesizeos.x*100)/ref->scale-ref->overlap)*pagex;
					page.min.y=(ref->offset.y*100)/ref->scale+((ref->pagesizeos.y*100)/ref->scale-ref->overlap)*pagey;
					page.max.x=page.min.x+(ref->pagesizeos.x*100)/ref->scale;
					page.max.y=page.min.y+(ref->pagesizeos.y*100)/ref->scale;
					rect.min.x=-10; /*So edges of output don't get clipped*/
					rect.min.y=-10;
					rect.max.x=ref->pagesizeos.x+10;
					rect.max.y=ref->pagesizeos.y+10;
					Drawfile_Create(ref->layout,&page);
					Desk_PDriver_GiveRectangle(1,&rect,&matrix,&position,0xFFFFFF00);
					Desk_PDriver_DrawPage(ref->copies,&cliprect,0,NULL,&more,&rectid);
					while (more) {
						cliprect.min.x-=100;
						cliprect.max.x+=100;
						cliprect.min.y-=100;
						cliprect.max.y+=100;
						Drawfile_Redraw(ref->scale,0,0,&cliprect);
						Desk_PDriver_GetRectangle(&cliprect,&more,&rectid);
					}
					Drawfile_Free();
				}
			}
		}
	} Desk_Error2_Catch {
		Desk_PDriver_AbortJob(printblk->job);
		Desk_File_Close(printblk->job);
		Desk_Hourglass_Off();
		Drawfile_Free();
		AJWLib_Error2_ReportMsgs("Error.Print:%s");
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
	Desk_Hourglass_Off();
	return Desk_TRUE;
}

static void Print_Result(Desk_print_block *printblk,Desk_print_result result)
{
	Desk_UNUSED(printblk);
	Desk_UNUSED(result);
}

static Desk_bool Print_Ok(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	Print_CalcValues(NULL,NULL);
	Desk_Print_StartPrint(Print_StartPrinting,NULL,Print_Result,&data,0,0,NULL,"Roots file");
	if (block->data.mouse.button.data.select) Print_CloseWindow();
	return Desk_TRUE;
}

void Print_OpenWindow(layout *layout)
{
	char *driver,name[]="Unknown";

	Desk_Error2_Try {
		data.layout=layout;
		driver=Desk_PDriver_PrinterName();
		if (driver) {
#ifdef MemCheck_MEMCHECK
			MemCheck_RegisterMiscBlock_String(driver);
#endif
		} else {
			driver=name;
		}
	/*	Desk_PDriver_PageSize(&pagesize,&printablearea);*/ /*Desk_PDriver_PageSize() seems to be broken*/
		Desk_Error2_CheckOS(Desk_SWI(0,7,SWI_PDriver_PageSize,NULL,&data.pagesize.x,&data.pagesize.y,&data.printablearea.min.x,&data.printablearea.min.y,&data.printablearea.max.x,&data.printablearea.max.y));
		data.treeextent=Layout_FindExtent(data.layout,Desk_FALSE);
		if (Config_Title()) data.treeextent.max.y+=Graphics_TitleHeight();
		Desk_Window_Show(printwin,Desk_open_CENTERED);
		Desk_Icon_SetText(printwin,print_FROM,"1");
		Desk_Icon_SetText(printwin,print_TO,"999");
		Print_CalcValues(NULL,NULL);
		Desk_Window_SetTitle(printwin,driver);
		Desk_Icon_SetCaret(printwin,print_COPIES);
	} Desk_Error2_Catch {
		if (Desk_Error2_globalblock.type==Desk_error2_type_OSERROR && Desk_Error2_globalblock.data.oserror->errnum==486) {
			/* SWI number not known error - printer driver not loaded*/
			Desk_Msgs_Report(1,"Error.PDrvr:Please load a printer driver and try again");
		} else {
			Desk_Error2_ReThrow();
		}
	} Desk_Error2_EndCatch
}

void Print_CloseWindow(void)
{
	Desk_Window_Hide(printwin);
}

static Desk_bool Print_KeyPressed(Desk_event_pollblock *block,void *ref)
{
	Desk_caret_block caret;
	Desk_Wimp_GetCaretPosition(&caret);
	if (caret.icon!=print_FROM && caret.icon!=print_TO) Print_CalcValues(block,ref);
	switch (block->data.key.code) {
		case Desk_keycode_RETURN:
			block->data.mouse.button.value=Desk_button_SELECT;
			switch (caret.icon) {
				case print_COPIES:
					Desk_Icon_SetCaret(printwin,print_SCALE);
					break;
				case print_SCALE:
					Desk_Icon_SetCaret(printwin,print_OVERLAP);
					break;
				case print_OVERLAP:
					Desk_Icon_SetCaret(printwin,print_FROM);
					break;
				case print_FROM:
					Desk_Icon_SetCaret(printwin,print_TO);
					break;
				case print_TO:
					Print_Ok(block,ref);
					return Desk_TRUE;
					break;
			}
			break;
		case Desk_keycode_ESCAPE:
			Print_CloseWindow();
			return Desk_TRUE;
			break;
	}
	return Desk_FALSE;
}

void Print_Init(void)
{
	printwin=Desk_Window_Create("Print",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,printwin,Desk_event_ANY,Print_CalcValues,NULL);
	Desk_Event_Claim(Desk_event_KEY,printwin,Desk_event_ANY,Print_KeyPressed,NULL);
	Desk_Event_Claim(Desk_event_CLICK,printwin,print_OK,Print_Ok,NULL);
	Desk_Event_Claim(Desk_event_CLICK,printwin,print_CANCEL,Windows_Cancel,NULL);
	Desk_Icon_Select(printwin,print_CENTRE);
	Desk_Icon_SetRadios(printwin,print_UPRIGHT,print_SIDEWAYS,print_UPRIGHT);
	Desk_Icon_SetRadios(printwin,print_ALLPAGES,print_FROMPAGES,print_ALLPAGES);
	AJWLib_Icon_RegisterCheckAdjust(printwin,print_UPRIGHT);
	AJWLib_Icon_RegisterCheckAdjust(printwin,print_SIDEWAYS);
	AJWLib_Icon_RegisterCheckAdjust(printwin,print_ALLPAGES);
	AJWLib_Icon_RegisterCheckAdjust(printwin,print_FROMPAGES);
	Desk_Icon_InitIncDecHandler(printwin,print_COPIES,print_COPIESUP,print_COPIESDOWN,Desk_FALSE,1,1,99,1);
	Desk_Icon_InitIncDecHandler(printwin,print_SCALE,print_SCALEUP,print_SCALEDOWN,Desk_FALSE,1,1,999,100);
	Desk_Icon_InitIncDecHandler(printwin,print_OVERLAP,print_OVERLAPUP,print_OVERLAPDOWN,Desk_FALSE,1,0,99,0);
}
