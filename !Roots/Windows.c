/*
	FT - Windows, menus and interface
	© Alex Waugh 1999

	$Id: Windows.c,v 1.77 2000/09/11 11:08:22 AJW Exp $

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

#include "Graphics.h"
#include "Modules.h"
#include "Windows.h"
#include "Config.h"
#include "Layout.h"
#include "Database.h"
#include "Drawfile.h"
#include "Draw.h"
#include "File.h"
#include "Print.h"

#define MAXWINDOWS 10
#define REDRAWOVERLAP 4

#define mainmenu_FILE 0
#define mainmenu_PERSON 1
#define mainmenu_ADDPERSON 2
#define mainmenu_SELECT 3
#define mainmenu_GRAPHICSSTYLE 4
#define mainmenu_NEWVIEW 5
#define mainmenu_SCALE 6
#define mainmenu_SEARCH 7
#define mainmenu_REPORTS 8

#define filemenu_INFO 0
#define filemenu_SAVE 1
#define filemenu_EXPORT 2
#define filemenu_CHOICES 3
#define filemenu_PRINT 4

#define exportmenu_GEDCOM 0
#define exportmenu_DRAW 1

#define personmenu_EDIT 0
#define personmenu_DELETE 1

#define selectmenu_DESCENDENTS 0
#define selectmenu_ANCESTORS 1
#define selectmenu_SIBLINGS 2
#define selectmenu_SPOUSES 3

/*#define newview_UNLINKED 1*/
#define newview_NORMAL 0
#define newview_ANCESTOR 2
#define newview_DESCENDENT 3
#define newview_CLOSERELATIVES 4
#define newview_CLOSERELATIVESPERSON 14
#define newview_ANCESTORPERSON 13
#define newview_DESCENDENTPERSON 5
#define newview_UPTO 8
#define newview_GENERATIONSTEXT 12
#define newview_GENERATIONS 9
#define newview_UP 11
#define newview_DOWN 10
#define newview_OK 7
#define newview_CANCEL 6

#define scale_TEXT 1
#define scale_UP 4
#define scale_DOWN 3
#define scale_SIZE1 6
#define scale_SIZE2 7
#define scale_SIZE3 8
#define scale_SIZE4 9
#define scale_FIT 5
#define scale_CANCEL 10
#define scale_OK 11

#define addparents_SECONDMARRIAGE 1

#define info_MODIFIED 1
#define info_TYPE 2
#define info_FILENAME 0
#define info_SIZE 3
#define info_PEOPLE 4
#define info_DATE 10
#define info_ICON 5

#define save_ICON 3
#define save_OK 0
#define save_CANCEL 1
#define save_FILENAME 2

#define unsaved_DISCARD 2
#define unsaved_CANCEL 3
#define unsaved_SAVE 0

#define fileconfig_USER1 1
#define fileconfig_USER2 2
#define fileconfig_USER3 3
#define fileconfig_OK 8
#define fileconfig_CANCEL 9

typedef struct windowdata {
	Desk_window_handle handle;
	wintype type;
	elementptr person;
	int generations;
	layout *layout;
	int scale;
} windowdata;

typedef struct savedata {
	wintype type;
	elementptr person;
	int generations;
	int scale;
	Desk_convert_block coords;
	int reserved[4]; /*Should be set to zero*/
} savedata;

typedef enum dragtype {
	drag_MOVE,
	drag_UNLINK,
	drag_LINK
} dragtype;

typedef struct dragdata {
	elementptr person;
	int origmousey;
	int personoffset;
	windowdata *windowdata;
	Desk_wimp_rect  coords;
	int origmousex,oldmousex,oldoffset,oldmousey,centered;
	Desk_bool plotted,marriage;
	dragtype type;
} dragdata;

typedef struct mouseclickdata {
	windowdata *window;
	elementptr element;
	Desk_wimp_point pos;
	elementtype type;
	int layoutptr;
} mouseclickdata;

#ifdef DEBUG
extern layout *debuglayout;
#endif

static windowdata windows[MAXWINDOWS];
static Desk_bool menusdeletedvalid=Desk_FALSE;
static int numwindows;
static Desk_window_handle newviewwin,fileinfowin,savewin,savedrawwin,savegedcomwin,scalewin,unsavedwin,fileconfigwin;
static Desk_menu_ptr mainmenu,filemenu,exportmenu,personmenu,selectmenu,fileconfigmenu=NULL;
static elementptr newviewperson;
static mouseclickdata mousedata;

static void Windows_RedrawPerson(windowdata *windowdata,personlayout *person)
{
	Desk_Window_ForceRedraw(windowdata->handle,(windowdata->scale*person->x)/100-REDRAWOVERLAP,(windowdata->scale*person->y)/100-REDRAWOVERLAP,(windowdata->scale*(person->x+Graphics_PersonWidth()))/100+REDRAWOVERLAP,(windowdata->scale*(person->y+Graphics_PersonHeight()))/100+REDRAWOVERLAP);
}

static void Windows_RedrawMarriage(windowdata *windowdata,marriagelayout *marriage)
{
	Desk_Window_ForceRedraw(windowdata->handle,(windowdata->scale*marriage->x)/100-REDRAWOVERLAP,(windowdata->scale*marriage->y)/100-REDRAWOVERLAP,(windowdata->scale*(marriage->x+Graphics_MarriageWidth()))/100+REDRAWOVERLAP,(windowdata->scale*(marriage->y+Graphics_PersonHeight()))/100+REDRAWOVERLAP);
}

void Windows_ForceRedraw(void)
{
	int i;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle) Desk_Window_ForceWholeRedraw(windows[i].handle);
		/*Make this more efficient*/
	}
}

static Desk_bool Windows_RedrawWindow(Desk_event_pollblock *block,windowdata *windowdata)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=block->data.openblock.window;
	Desk_Wimp_RedrawWindow(&blk,&more);
	while (more) {
#ifdef DEBUG
		Desk_ColourTrans_SetGCOL(0x00000000,0,0);
		Desk_GFX_RectangleFill(blk.rect.min.x-blk.scroll.x+1000,blk.rect.max.y-blk.scroll.y-10000,10,20000);
		Desk_GFX_RectangleFill(blk.rect.min.x-blk.scroll.x-1000,blk.rect.max.y-blk.scroll.y-10000,10,20000);
		Desk_ColourTrans_SetGCOL(0x0000FF00,0,0);
		Desk_GFX_RectangleFill(blk.rect.min.x-blk.scroll.x+2000,blk.rect.max.y-blk.scroll.y-10000,10,20000);
		Desk_GFX_RectangleFill(blk.rect.min.x-blk.scroll.x-2000,blk.rect.max.y-blk.scroll.y-10000,10,20000);
		Desk_ColourTrans_SetGCOL(0x00FF0000,0,0);
		Desk_GFX_RectangleFill(blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y-10000,10,20000);
#endif
		Graphics_Redraw(windowdata->layout,windowdata->scale,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,&(blk.cliprect),Desk_TRUE,Draw_PlotLine,Draw_PlotRectangle,Draw_PlotRectangleFilled,Draw_PlotText);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
	return Desk_TRUE;
}

static void Windows_UnselectAll(windowdata *windowdata)
{
	int i;
	AJWLib_Assert(windowdata!=NULL);
	for (i=0;i<windowdata->layout->numpeople;i++) {
		if (Database_GetSelect(windowdata->layout->person[i].person)) {
			Database_DeSelect(windowdata->layout->person[i].person);
			Windows_RedrawPerson(windowdata,windowdata->layout->person+i);
		}
	}
	for (i=0;i<windowdata->layout->nummarriages;i++) {
		if (Database_GetSelect(windowdata->layout->marriage[i].marriage)) {
			Database_DeSelect(windowdata->layout->marriage[i].marriage);
			Windows_RedrawMarriage(windowdata,windowdata->layout->marriage+i);
		}
	}
}

static void Windows_AddSelected(layout *layout,int amountx,int amounty)
{
	int i;
	for (i=0;i<layout->numpeople;i++) {
		if (Database_GetSelect(layout->person[i].person)) {
			layout->person[i].x+=amountx;
			layout->person[i].y+=amounty;
		}
	}
	for (i=0;i<layout->nummarriages;i++) {
		if (Database_GetSelect(layout->marriage[i].marriage)) {
			layout->marriage[i].x+=amountx;
			layout->marriage[i].y+=amounty;
		}
	}
	Modules_ChangedLayout();
}

static void Windows_ResizeWindow(windowdata *windowdata)
{
	Desk_wimp_rect box;
	Desk_window_info infoblk;
	Desk_window_outline outlineblk;
	Desk_bool maxxextent=Desk_FALSE,maxyextent=Desk_FALSE;
	box=Layout_FindExtent(windowdata->layout,Desk_FALSE);
	Desk_Window_GetInfo3(windowdata->handle,&infoblk);
	outlineblk.window=windowdata->handle;
	if (infoblk.block.workarearect.max.x-infoblk.block.workarearect.min.x==infoblk.block.screenrect.max.x-infoblk.block.screenrect.min.x) {
		/*We are currently open at maximum x extend*/
		maxxextent=Desk_TRUE;
	}
	if (infoblk.block.workarearect.max.y-infoblk.block.workarearect.min.y==infoblk.block.screenrect.max.y-infoblk.block.screenrect.min.y) {
		/*We are currently open at maximum y extend*/
		maxyextent=Desk_TRUE;
	}
	/*Set window extent to fit layout size*/
	Desk_Window_SetExtent(windowdata->handle,(windowdata->scale*(box.min.x-Graphics_WindowBorder()))/100,(windowdata->scale*(box.min.y-Graphics_WindowBorder()))/100,(windowdata->scale*(box.max.x+Graphics_WindowBorder()))/100,(windowdata->scale*(box.max.y+Graphics_WindowBorder()+((Config_Title() && windowdata->type==wintype_NORMAL) ? Graphics_TitleHeight() : 0)))/100);
	/*Reread window position as it might have changed*/
	Desk_Window_GetInfo3(windowdata->handle,&infoblk);
	Desk_Wimp_GetWindowOutline(&outlineblk);
	if ((maxxextent || Config_AutoIncreaseAlways()) && outlineblk.screenrect.max.x<=Desk_screen_size.x && Config_AutoIncreaseSize()) {
		/*Enlarge window to show new extent in x direction*/
		Desk_window_openblock openblk;
		int amountmax=0,amountmin=0,amount=0;
		openblk.window=windowdata->handle;
		openblk.screenrect=infoblk.block.screenrect;
		/*Find amount we nedd to increase by*/
		amount=(infoblk.block.workarearect.max.x-infoblk.block.workarearect.min.x)-(openblk.screenrect.max.x-openblk.screenrect.min.x);
		amountmax=amount;
		/*Find maximum +ve x we can increase by*/
		if (outlineblk.screenrect.max.x+amount>Desk_screen_size.x) amountmax=Desk_screen_size.x-outlineblk.screenrect.max.x;
		amountmin=amount-amountmax;
		/*Find maximum -ve x we can increase by*/
		if (outlineblk.screenrect.min.x-amountmin<0) amountmin=outlineblk.screenrect.min.x;
		if (outlineblk.screenrect.min.x<0) amountmin=0;
		openblk.screenrect.max.x+=amountmax;
		openblk.screenrect.min.x-=amountmin;
		openblk.scroll=infoblk.block.scroll;
		openblk.behind=infoblk.block.behind;
		Desk_Wimp_OpenWindow(&openblk);
	}
	/*Reread window position as it might have changed*/
	Desk_Window_GetInfo3(windowdata->handle,&infoblk);
	Desk_Wimp_GetWindowOutline(&outlineblk);
	if ((maxyextent || Config_AutoIncreaseAlways()) && outlineblk.screenrect.max.y<=Desk_screen_size.y && Config_AutoIncreaseSize()) {
		/*Enlarge window to show new extent in y direction*/
		Desk_window_openblock openblk;
		int amountmax=0,amountmin=0,amount=0;
		openblk.window=windowdata->handle;
		openblk.screenrect=infoblk.block.screenrect;
		/*Find amount we nedd to increase by*/
		amount=(infoblk.block.workarearect.max.y-infoblk.block.workarearect.min.y)-(openblk.screenrect.max.y-openblk.screenrect.min.y);
		amountmax=amount;
		/*Find maximum -ve y we can increase by*/
		if (outlineblk.screenrect.min.y-amount<0) amountmax=outlineblk.screenrect.min.y;
		amountmin=amount-amountmax;
		/*Find maximum +ve y we can increase by*/
		if (outlineblk.screenrect.max.y+amountmin>Desk_screen_size.y) amountmin=Desk_screen_size.y-outlineblk.screenrect.max.y;
		if (outlineblk.screenrect.max.y>Desk_screen_size.y) amountmin=0;
		openblk.screenrect.min.y-=amountmax;
		openblk.screenrect.max.y+=amountmin;
		openblk.scroll=infoblk.block.scroll;
		openblk.behind=infoblk.block.behind;
		Desk_Wimp_OpenWindow(&openblk);
	}
}

Desk_bool Windows_BringToFront(void)
{
	int i;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle) {
			if (windows[i].type==wintype_NORMAL || windows[i].type==wintype_CLOSERELATIVES) {
				Desk_Window_BringToFront(windows[i].handle);
				return Desk_TRUE;
			}
		}
	}
	return Desk_FALSE;
}

void Windows_ChangedLayout(void)
{
	int i;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle) {
			Desk_Window_ForceWholeRedraw(windows[i].handle);
			Windows_ResizeWindow(&(windows[i]));
			/*Make this more efficient*/
		}
	}
}

static void Windows_PlotDragBox(dragdata *dragdata)
{
	/*Plot or remove a drag box*/
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=dragdata->windowdata->handle;
	blk.rect.min.x=-INFINITY;
	blk.rect.max.x=INFINITY;
	blk.rect.min.y=-INFINITY;
	blk.rect.max.y=INFINITY;
	Desk_Wimp_UpdateWindow(&blk,&more);
	while (more) {
		Draw_EORRectangle(dragdata->windowdata->scale,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,dragdata->oldmousex+dragdata->coords.min.x+dragdata->oldoffset,dragdata->oldmousey+dragdata->coords.min.y,dragdata->coords.max.x-dragdata->coords.min.x,dragdata->coords.max.y-dragdata->coords.min.y,0,EORCOLOUR);
		Draw_EORRectangle(dragdata->windowdata->scale,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,dragdata->oldmousex+dragdata->oldoffset-dragdata->personoffset,dragdata->oldmousey,dragdata->marriage ? Graphics_MarriageWidth() : Graphics_PersonWidth(),Graphics_PersonHeight(),0,EORCOLOURRED);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
}

static void Windows_DragEnd(void *ref)
{
	/*User has finished a drag*/
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	int window=-1,i;
	Desk_Wimp_GetPointerInfo(&mouseblk);
	/*Find destination window*/
	for (i=0;i<MAXWINDOWS;i++) if (mouseblk.window==windows[i].handle) window=i;
	if (window==-1) return;
	Desk_Icon_SetCaret(mouseblk.window,-1);
	if (dragdata->windowdata->type==wintype_NORMAL) {
		/*We are moving people/marriages*/
		int mousex,mousey;
		/*Act as if drag had ended on same win as started on*/
		if (dragdata->plotted) Windows_PlotDragBox(dragdata);
		/*Find mouse position relative to window origin and independant of current scale*/
		Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
		mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/dragdata->windowdata->scale;
		mousey=Layout_NearestGeneration(((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/dragdata->windowdata->scale);
		/*Check to see if we are unlinking people*/
		if (mousey!=dragdata->origmousey) Database_UnlinkSelected(dragdata->windowdata->layout);
		/*Move all people/marriages that were selected*/
		Windows_AddSelected(dragdata->windowdata->layout,mousex+dragdata->oldoffset-dragdata->origmousex,mousey-dragdata->origmousey);
		Layout_LayoutLines(dragdata->windowdata->layout);
		Layout_LayoutTitle(dragdata->windowdata->layout);
		Windows_ResizeWindow(dragdata->windowdata);
		Desk_Window_ForceWholeRedraw(dragdata->windowdata->handle);
	}
}

static void Windows_SelectDragEnd(void *ref)
/*A select drag box has ended, so select everything enclosed by it*/
{
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	int mousex,mousey,i;
	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/dragdata->windowdata->scale;
	for (i=0;i<dragdata->windowdata->layout->numpeople;i++) {
		if (mousex>dragdata->windowdata->layout->person[i].x && dragdata->oldmousex<dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth() || mousex<dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth() && dragdata->oldmousex>dragdata->windowdata->layout->person[i].x) {
			if (mousey>dragdata->windowdata->layout->person[i].y && dragdata->oldmousey<dragdata->windowdata->layout->person[i].y+Graphics_PersonHeight() || mousey<dragdata->windowdata->layout->person[i].y+Graphics_PersonHeight() && dragdata->oldmousey>dragdata->windowdata->layout->person[i].y) {
				if (Database_GetSelect(dragdata->windowdata->layout->person[i].person)) {
					Database_DeSelect(dragdata->windowdata->layout->person[i].person);
				} else {
					Database_Select(dragdata->windowdata->layout->person[i].person);
				}
				Windows_RedrawPerson(dragdata->windowdata,dragdata->windowdata->layout->person+i);
			}
		}
	}
	for (i=0;i<dragdata->windowdata->layout->nummarriages;i++) {
		if (mousex>dragdata->windowdata->layout->marriage[i].x && dragdata->oldmousex<dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth() || mousex<dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth() && dragdata->oldmousex>dragdata->windowdata->layout->marriage[i].x) {
			if (mousey>dragdata->windowdata->layout->marriage[i].y && dragdata->oldmousey<dragdata->windowdata->layout->marriage[i].y+Graphics_PersonHeight() || mousey<dragdata->windowdata->layout->marriage[i].y+Graphics_PersonHeight() && dragdata->oldmousey>dragdata->windowdata->layout->marriage[i].y) {
				if (Database_GetSelect(dragdata->windowdata->layout->marriage[i].marriage)) {
					Database_DeSelect(dragdata->windowdata->layout->marriage[i].marriage);
				} else {
					Database_Select(dragdata->windowdata->layout->marriage[i].marriage);
				}
				Windows_RedrawMarriage(dragdata->windowdata,dragdata->windowdata->layout->marriage+i);
			}
		}
	}
}

static void Windows_LinkDragEnd(void *ref)
/*A link drag has ended, so link the person if possible*/
{
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	int mousex,mousey,i;
	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/dragdata->windowdata->scale;
	/*See who we were dropped on*/
	for (i=dragdata->windowdata->layout->numpeople-1;i>=0;i--) {
		if (mousex>dragdata->windowdata->layout->person[i].x && mousex<dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth()) {
			if (mousey>dragdata->windowdata->layout->person[i].y && mousey<dragdata->windowdata->layout->person[i].y+Graphics_PersonHeight()) {
				/*Check that the dragged person is not the same as the destination person*/
				if (dragdata->person==dragdata->windowdata->layout->person[i].person) return;
				/*Check that both people are in the same generation*/
				if (dragdata->origmousey!=Layout_NearestGeneration(mousey)) return;
				Desk_Error2_Try {
					volatile elementptr marriage;
					marriage=Database_Marry(dragdata->windowdata->layout->person[i].person,dragdata->person);
					Desk_Error2_Try {
						int startx,finishx,marriagex;
						/*Marriage is put next to the person that the drag was started on*/
						/*Find out if we need to put it to the left or right of the person*/
						startx=Layout_FindXCoord(dragdata->windowdata->layout,dragdata->person);
						finishx=Layout_FindXCoord(dragdata->windowdata->layout,dragdata->windowdata->layout->person[i].person);
						if (startx<finishx) marriagex=startx+Graphics_PersonWidth(); else marriagex=startx-Graphics_MarriageWidth();
						Layout_AddMarriage(dragdata->windowdata->layout,marriage,marriagex,dragdata->origmousey);
						Windows_UnselectAll(dragdata->windowdata);
					} Desk_Error2_Catch {
						Database_RemoveMarriage(marriage);
						Desk_Error2_ReThrow();
					} Desk_Error2_EndCatch
				} Desk_Error2_Catch {
					AJWLib_Error2_Report("%s");
				} Desk_Error2_EndCatch
				return;
			}
		}
	}
	for (i=dragdata->windowdata->layout->nummarriages-1;i>=0;i--) {
		if (mousex>dragdata->windowdata->layout->marriage[i].x && mousex<dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth()) {
			if (mousey>dragdata->windowdata->layout->marriage[i].y && mousey<dragdata->windowdata->layout->marriage[i].y+Graphics_PersonHeight()) {
				/*Check that the person does not have parents already*/
				if (Database_GetMother(dragdata->person)) return;
				/*Check that the person is in the right generation*/
				if (Layout_NearestGeneration((dragdata->origmousey)+Graphics_PersonHeight()+Graphics_GapHeightAbove()+Graphics_GapHeightBelow())!=Layout_NearestGeneration(mousey)) return;
				Database_AddChild(dragdata->windowdata->layout->marriage[i].marriage,dragdata->person);
				Windows_UnselectAll(dragdata->windowdata);
				break;
			}
		}
	}
}

static void Windows_GetOffset(dragdata *dragdata)
{
	int i,distance;
	dragdata->oldoffset=0;
	if (!Config_Snap()) return;
	for (i=0;i<dragdata->windowdata->layout->numpeople;i++) {
		if (dragdata->windowdata->layout->person[i].y==dragdata->oldmousey && !Database_GetSelect(dragdata->windowdata->layout->person[i].person)) {
			/*Look for right hand edge of person or marriage*/
			distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth());
			if (!dragdata->marriage) distance-=Graphics_GapWidth();
			if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
			/*Look for second marriage on right*/
			if (dragdata->marriage) {
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth()+Graphics_SecondMarriageGap());
				if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
			}
			/*Look for left hand edge of person or marriage*/
			distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x);
			if (!dragdata->marriage) distance+=Graphics_GapWidth()+Graphics_PersonWidth(); else distance+=Graphics_MarriageWidth();
			if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
			/*Look for second marriage on left*/
			if (dragdata->marriage) {
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x-Graphics_MarriageWidth()-Graphics_SecondMarriageGap());
				if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
			}
		}
	}
	if (!dragdata->marriage) {
		for (i=0;i<dragdata->windowdata->layout->nummarriages;i++) {
			if (dragdata->windowdata->layout->marriage[i].y==dragdata->oldmousey && !Database_GetSelect(dragdata->windowdata->layout->marriage[i].marriage)) {
				/*Look for right hand edge of marriage*/
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth());
				if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
				/*Look for left hand edge of marriage*/
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->marriage[i].x-Graphics_PersonWidth());
				if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
			}
		}
	}
	/*Look for siblings centered under marriage and marriages centered over siblings*/
	distance=dragdata->oldmousex-dragdata->centered;
	if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
}

static void Windows_DragFn(void *ref)
{
	/*Called every null poll when dragging*/
	dragdata *dragdata=ref;
	Desk_mouse_block mouseblk;
	Desk_window_state blk;
	Desk_window_info infoblk;
	int mousex,mousey;
	if (dragdata->windowdata->type!=wintype_NORMAL) return;
	if (!dragdata->plotted) {
		/*Plot drag box if not already plotted*/
		Windows_GetOffset(dragdata);
		Windows_PlotDragBox(dragdata);
		dragdata->plotted=Desk_TRUE;
	}
	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Wimp_GetWindowState(dragdata->windowdata->handle,&blk);
	Desk_Window_GetInfo3(dragdata->windowdata->handle,&infoblk);
	mousex=((mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.openblock.screenrect.max.y-blk.openblock.scroll.y))*100)/dragdata->windowdata->scale;
	if (mousex!=dragdata->oldmousex || Layout_NearestGeneration(mousey)!=dragdata->oldmousey) {
		/*Unplot drag box if it has moved*/
		Windows_PlotDragBox(dragdata);
		dragdata->oldmousex=mousex;
		dragdata->oldmousey=Layout_NearestGeneration(mousey);
		Windows_GetOffset(dragdata);
		/*Replot it in new position*/
		Windows_PlotDragBox(dragdata);
	}
	if (mouseblk.pos.x-blk.openblock.screenrect.min.x<Config_ScrollDistance()) {
		/*We are near left edge of window*/
		Windows_PlotDragBox(dragdata);
		dragdata->plotted=Desk_FALSE;
		if (infoblk.block.scroll.x<=infoblk.block.workarearect.min.x) {
			/*Increase window size*/
			Desk_wimp_box extent;
			extent=infoblk.block.workarearect;
			extent.min.x-=(Config_ScrollSpeed()*(Config_ScrollDistance()-(mouseblk.pos.x-blk.openblock.screenrect.min.x)))/20;
			Desk_Wimp_SetExtent(dragdata->windowdata->handle,&extent);
		}
		/*Scroll window*/
		blk.openblock.scroll.x-=(Config_ScrollSpeed()*(Config_ScrollDistance()-(mouseblk.pos.x-blk.openblock.screenrect.min.x)))/20;
		Desk_Wimp_OpenWindow(&blk.openblock);
		mousex=mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x);
		dragdata->oldmousex=mousex;
	} else if (blk.openblock.screenrect.max.x-mouseblk.pos.x<Config_ScrollDistance()) {
		/*We are near right edge of window*/
		Windows_PlotDragBox(dragdata);
		dragdata->plotted=Desk_FALSE;
		if (infoblk.block.scroll.x-infoblk.block.workarearect.min.x+(infoblk.block.screenrect.max.x-infoblk.block.screenrect.min.x)>=infoblk.block.workarearect.max.x-infoblk.block.workarearect.min.x) {
			/*Increase window size*/
			Desk_wimp_box extent;
			extent=infoblk.block.workarearect;
			extent.max.x+=(Config_ScrollSpeed()*(Config_ScrollDistance()-(blk.openblock.screenrect.max.x-mouseblk.pos.x)))/20;
			Desk_Wimp_SetExtent(dragdata->windowdata->handle,&extent);
		}
		/*Scroll window*/
		blk.openblock.scroll.x+=(Config_ScrollSpeed()*(Config_ScrollDistance()-(blk.openblock.screenrect.max.x-mouseblk.pos.x)))/20;
		Desk_Wimp_OpenWindow(&blk.openblock);
		mousex=mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x);
		dragdata->oldmousex=mousex;
	} else if (mouseblk.pos.y-blk.openblock.screenrect.min.y<Config_ScrollDistance()) {
		/*We are near bottom edge of window*/
		Windows_PlotDragBox(dragdata);
		dragdata->plotted=Desk_FALSE;
		if (-(infoblk.block.scroll.y-infoblk.block.workarearect.max.y-(infoblk.block.screenrect.max.y-infoblk.block.screenrect.min.y))>=infoblk.block.workarearect.max.y-infoblk.block.workarearect.min.y) {
			/*Increase window size*/
			Desk_wimp_box extent;
			extent=infoblk.block.workarearect;
			extent.min.y-=(Config_ScrollSpeed()*(Config_ScrollDistance()-(mouseblk.pos.y-blk.openblock.screenrect.min.y)))/20;
			Desk_Wimp_SetExtent(dragdata->windowdata->handle,&extent);
		}
		/*Scroll window*/
		blk.openblock.scroll.y-=(Config_ScrollSpeed()*(Config_ScrollDistance()-(mouseblk.pos.y-blk.openblock.screenrect.min.y)))/20;
		Desk_Wimp_OpenWindow(&blk.openblock);
		mousey=mouseblk.pos.y-(blk.openblock.screenrect.min.y-blk.openblock.scroll.y);
		dragdata->oldmousey=Layout_NearestGeneration(mousey);
	} else if (blk.openblock.screenrect.max.y-mouseblk.pos.y<Config_ScrollDistance()) {
		/*We are near top edge of window*/
		Windows_PlotDragBox(dragdata);
		dragdata->plotted=Desk_FALSE;
		if (infoblk.block.scroll.y>=infoblk.block.workarearect.max.y) {
			/*Increase window size*/
			Desk_wimp_box extent;
			extent=infoblk.block.workarearect;
			extent.max.y+=(Config_ScrollSpeed()*(Config_ScrollDistance()-(blk.openblock.screenrect.max.y-mouseblk.pos.y)))/20;
			Desk_Wimp_SetExtent(dragdata->windowdata->handle,&extent);
		}
		/*Scroll window*/
		blk.openblock.scroll.y+=(Config_ScrollSpeed()*(Config_ScrollDistance()-(blk.openblock.screenrect.max.y-mouseblk.pos.y)))/20;
		Desk_Wimp_OpenWindow(&blk.openblock);
		mousey=mouseblk.pos.y-(blk.openblock.screenrect.min.y-blk.openblock.scroll.y);
		dragdata->oldmousex=Layout_NearestGeneration(mousey);
	}
}

static void Windows_StartDragNormal(elementptr person,int x,int y,windowdata *windowdata,Desk_bool marriage)
{
	static dragdata dragdata;
	Desk_drag_block dragblk;
	Desk_convert_block blk;
	Desk_mouse_block mouseblk;
	int mousex,mousey;
	Desk_Window_GetCoords(windowdata->handle,&blk);
	Desk_Wimp_GetPointerInfo(&mouseblk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/windowdata->scale;
	mousey=Layout_NearestGeneration(((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windowdata->scale);
	dragblk.type=Desk_drag_INVISIBLE;
	dragdata.coords=Layout_FindExtent(windowdata->layout,Desk_TRUE);
	dragdata.coords.min.x-=mousex;
	dragdata.coords.max.x-=mousex;
	dragdata.coords.min.y-=mousey;
	dragdata.coords.max.y-=mousey;
	if (blk.screenrect.min.x<0) dragblk.parent.min.x=0; else dragblk.parent.min.x=blk.screenrect.min.x;
	if (blk.screenrect.max.x>Desk_screen_size.x) dragblk.parent.max.x=Desk_screen_size.x; else dragblk.parent.max.x=blk.screenrect.max.x;
	if (blk.screenrect.min.y<0) dragblk.parent.min.y=0; else dragblk.parent.min.y=blk.screenrect.min.y;
	if (blk.screenrect.max.y>Desk_screen_size.y) dragblk.parent.max.y=Desk_screen_size.y; else dragblk.parent.max.y=blk.screenrect.max.y;
	dragblk.screenrect.min.x=0;
	dragblk.screenrect.max.x=0;
	dragblk.screenrect.min.y=0;
	dragblk.screenrect.max.y=0;
	dragdata.person=person;
	dragdata.personoffset=mousex-x;
	dragdata.windowdata=windowdata;
	dragdata.origmousex=mousex;
	dragdata.origmousey=mousey;
	dragdata.oldmousex=mousex;
	dragdata.oldmousey=mousey;
	dragdata.oldoffset=0;
	dragdata.marriage=marriage;
	dragdata.plotted=Desk_FALSE;
	if (!marriage) {
		Desk_bool allsiblings=Desk_TRUE;
		elementptr person1=person,person2=person;
		int x,rightx=-INFINITY,leftx=INFINITY;
		while ((person1=Database_GetSiblingLtoR(person1))!=none) {
			if (!Database_GetSelect(person1)) allsiblings=Desk_FALSE;
			if ((x=Layout_FindXCoord(windowdata->layout,person1))<leftx) leftx=x;
			if (x>rightx) rightx=x;
		}
		do {
			if (!Database_GetSelect(person2)) allsiblings=Desk_FALSE;
			if ((x=Layout_FindXCoord(windowdata->layout,person2))<leftx) leftx=x;
			if (x>rightx) rightx=x;
		} while ((person2=Database_GetSiblingRtoL(person2))!=none);
		if (allsiblings) {
			int centre=(rightx+leftx+Graphics_PersonWidth())/2;
			int marriagepos=Layout_FindMarriageXCoord(windowdata->layout,Database_GetMarriage(Database_GetMother(person)))+Graphics_MarriageWidth()/2;
			dragdata.centered=marriagepos+(mousex-centre);
		} else {
			dragdata.centered=INFINITY;
		}
	} else {
		if (Database_GetRightChild(person)) {
			int tempx,rightx=-INFINITY,leftx=INFINITY,centre,marriagepos;
			elementptr person1=Database_GetRightChild(person);
			do {
				if ((tempx=Layout_FindXCoord(windowdata->layout,person1))<leftx) leftx=tempx;
				if (tempx>rightx) rightx=tempx;
			} while ((person1=Database_GetSiblingRtoL(person1))!=none);
			centre=(rightx+leftx+Graphics_PersonWidth())/2;
			marriagepos=Layout_FindMarriageXCoord(windowdata->layout,person)+Graphics_MarriageWidth()/2;
			dragdata.centered=centre+(mousex-marriagepos);
		} else {
			dragdata.centered=INFINITY;
		}
	}
	Desk_Wimp_DragBox(&dragblk);
	Desk_Drag_SetHandlers(Windows_DragFn,Windows_DragEnd,&dragdata);
}

static void Windows_StartDragSelect(windowdata *windowdata)
{
	static dragdata dragdata;
	Desk_drag_block dragblk;
	Desk_convert_block blk;
	Desk_mouse_block mouseblk;
	int mousex,mousey;
	Desk_Window_GetCoords(windowdata->handle,&blk);
	Desk_Wimp_GetPointerInfo(&mouseblk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windowdata->scale;
	dragblk.type=Desk_drag_RUBBERBOX;
	dragblk.screenrect.min.x=mouseblk.pos.x;
	dragblk.screenrect.max.x=mouseblk.pos.x;
	dragblk.screenrect.min.y=mouseblk.pos.y;
	dragblk.screenrect.max.y=mouseblk.pos.y;
	if (blk.screenrect.min.x<0) dragblk.parent.min.x=0; else dragblk.parent.min.x=blk.screenrect.min.x;
	if (blk.screenrect.max.x>Desk_screen_size.x) dragblk.parent.max.x=Desk_screen_size.x; else dragblk.parent.max.x=blk.screenrect.max.x;
	if (blk.screenrect.min.y<0) dragblk.parent.min.y=0; else dragblk.parent.min.y=blk.screenrect.min.y;
	if (blk.screenrect.max.y>Desk_screen_size.y) dragblk.parent.max.y=Desk_screen_size.y; else dragblk.parent.max.y=blk.screenrect.max.y;
	dragdata.windowdata=windowdata;
	dragdata.origmousex=mousex;
	dragdata.oldmousex=mousex;
	dragdata.oldmousey=mousey;
	Desk_Wimp_DragBox(&dragblk);
	Desk_Drag_SetHandlers(NULL,Windows_SelectDragEnd,&dragdata);
}

static void Windows_StartDragLink(windowdata *windowdata,elementptr person)
{
	static dragdata dragdata;
	Desk_drag_block dragblk;
	Desk_convert_block blk;
	Desk_mouse_block mouseblk;
	int mousex,mousey,i;
	Desk_Window_GetCoords(windowdata->handle,&blk);
	Desk_Wimp_GetPointerInfo(&mouseblk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windowdata->scale;
	dragblk.type=Desk_drag_INVISIBLE;
	dragblk.screenrect.min.x=mouseblk.pos.x;
	dragblk.screenrect.max.x=mouseblk.pos.x;
	dragblk.screenrect.min.y=mouseblk.pos.y;
	dragblk.screenrect.max.y=mouseblk.pos.y;
	if (blk.screenrect.min.x<0) dragblk.parent.min.x=0; else dragblk.parent.min.x=blk.screenrect.min.x;
	if (blk.screenrect.max.x>Desk_screen_size.x) dragblk.parent.max.x=Desk_screen_size.x; else dragblk.parent.max.x=blk.screenrect.max.x;
	if (blk.screenrect.min.y<0) dragblk.parent.min.y=0; else dragblk.parent.min.y=blk.screenrect.min.y;
	if (blk.screenrect.max.y>Desk_screen_size.y) dragblk.parent.max.y=Desk_screen_size.y; else dragblk.parent.max.y=blk.screenrect.max.y;
	dragdata.type=drag_LINK;
	dragdata.windowdata=windowdata;
	dragdata.person=person;
	dragdata.origmousex=mousex;
	dragdata.origmousey=Layout_NearestGeneration(mousey);
	dragdata.oldmousex=mousex;
	dragdata.oldmousey=mousey;
	Windows_UnselectAll(windowdata);
	Database_Select(person);
	for (i=0;i<windowdata->layout->numpeople;i++) {
		if (windowdata->layout->person[i].person==person) Windows_RedrawPerson(windowdata,windowdata->layout->person+i);
	}
	Desk_Wimp_DragBox(&dragblk);
	Desk_Drag_SetHandlers(NULL,Windows_LinkDragEnd,&dragdata);
}

static void Windows_StyleMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	Graphics_RemoveStyle();
	Graphics_LoadStyle(Desk_Menu_GetText(fileconfigmenu,entry));
	Modules_ChangedStructure();
}

static void Windows_SetUpMenu(void)
{
	char buffer[20];
	Desk_filing_dirdata dir;
	char *name=NULL;
	char dirname[256];
	int i=0;
	Desk_Icon_SetText(savewin,save_FILENAME,File_GetFilename());
	Desk_Icon_SetText(savedrawwin,save_FILENAME,AJWLib_Msgs_TempLookup("File.Draw:Drawfile"));
	sprintf(buffer,"%d",Database_GetNumPeople());
	Desk_Icon_SetText(fileinfowin,info_PEOPLE,buffer);
	Desk_Icon_SetText(fileinfowin,info_FILENAME,File_GetFilename());
	Desk_Icon_SetText(fileinfowin,info_MODIFIED,File_GetModified() ? AJWLib_Msgs_TempLookup("Mod.Yes:Yes") : AJWLib_Msgs_TempLookup("Mod.No:No"));
	Desk_Error2_CheckOS(Desk_SWI(3,0,Desk_SWI_OS_ConvertFileSize,File_GetSize(),buffer,sizeof(buffer)));
	Desk_Icon_SetText(fileinfowin,info_SIZE,buffer);
	Desk_Icon_SetText(fileinfowin,info_DATE,File_GetDate());
	Desk_Icon_SetInteger(scalewin,scale_TEXT,mousedata.window->scale);
	if (fileconfigmenu) {
		AJWLib_Menu_FullDispose(fileconfigmenu);
		fileconfigmenu=NULL;
	}
	sprintf(dirname,"%s.%s",choicesread,GRAPHICSDIR);
	if (Desk_File_IsDirectory(dirname)) {
		Desk_Filing_OpenDir(dirname,&dir,256,Desk_readdirtype_NAMEONLY);
		do {
			name=Desk_Filing_ReadDir(&dir);
			if (name) {
				if (fileconfigmenu) {
					fileconfigmenu=Desk_Menu_Extend(fileconfigmenu,name);
					i++;
				} else {
					fileconfigmenu=Desk_Menu_New(AJWLib_Msgs_TempLookup("Title.Config:"),name);
					i=0;
				}
				if (Desk_stricmp(name,Graphics_GetCurrentStyle())) {
					Desk_Menu_SetFlags(fileconfigmenu,i,0,0);
				} else {
					Desk_Menu_SetFlags(fileconfigmenu,i,1,0);
				}
			}
		} while (name);
		Desk_Filing_CloseDir(&dir);
	}
	AJWLib_Menu_Register(fileconfigmenu,Windows_StyleMenuClick,NULL);
	Desk_Menu_AddSubMenu(mainmenu,mainmenu_GRAPHICSSTYLE,fileconfigmenu);
}

static Desk_bool Windows_MenusDeleted(Desk_event_pollblock *block,void *ref)
{
	windowdata *windowdata=ref;
	Desk_UNUSED(block);
	if (menusdeletedvalid) {
		Windows_UnselectAll(windowdata);
		Desk_EventMsg_Release(Desk_message_MENUSDELETED,Desk_event_ANY,Windows_MenusDeleted);
	}
	menusdeletedvalid=Desk_FALSE;
	return Desk_TRUE;
}

static Desk_bool Windows_MouseClick(Desk_event_pollblock *block,void *ref)
{
	windowdata *windowdata=ref;
	int mousex,mousey,i;
	Desk_convert_block blk;
	elementtype selected;
	menusdeletedvalid=Desk_FALSE;
	if (!block->data.mouse.button.data.menu) Desk_Icon_SetCaret(block->data.mouse.window,-1);
	Desk_Window_GetCoords(windowdata->handle,&blk);
	mousex=((block->data.mouse.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/windowdata->scale;
	mousey=((block->data.mouse.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windowdata->scale;
	mousedata.element=none;
	mousedata.type=element_NONE;
	mousedata.window=windowdata;
	mousedata.pos.x=mousex;
	mousedata.pos.y=mousey;
	AJWLib_Menu_Shade(mainmenu,mainmenu_ADDPERSON);
	AJWLib_Menu_Shade(personmenu,personmenu_EDIT);
	AJWLib_Menu_Shade(personmenu,personmenu_DELETE);
	AJWLib_Menu_Shade(mainmenu,mainmenu_SELECT);
	AJWLib_Menu_Shade(mainmenu,mainmenu_PERSON);
	Desk_Menu_SetText(mainmenu,mainmenu_PERSON,AJWLib_Msgs_TempLookup("Item.Person:Person"));
	/*See if we clicked on a person*/
	for (i=windowdata->layout->numpeople-1;i>=0;i--) {
		if (mousex>=windowdata->layout->person[i].x && mousex<=windowdata->layout->person[i].x+Graphics_PersonWidth()) {
			if (mousey>=windowdata->layout->person[i].y && mousey<=windowdata->layout->person[i].y+Graphics_PersonHeight()) {
				mousedata.type=element_PERSON;
				mousedata.layoutptr=i;
				mousedata.element=windowdata->layout->person[i].person;
				break;
			}
		}
	}
	/*See if we clicked on a marriage*/
	/*Marriages take priority over people, as they are generally smaller*/
	for (i=windowdata->layout->nummarriages-1;i>=0;i--) {
		if (mousex>=windowdata->layout->marriage[i].x && mousex<=windowdata->layout->marriage[i].x+Graphics_MarriageWidth()) {
			if (mousey>=windowdata->layout->marriage[i].y && mousey<=windowdata->layout->marriage[i].y+Graphics_PersonHeight()) {
				mousedata.type=element_MARRIAGE;
				mousedata.layoutptr=i;
				mousedata.element=windowdata->layout->marriage[i].marriage;
				break;
			}
		}
	}
	/*See if we clicked on the title*/
	if (Config_Title() && mousey>windowdata->layout->title.y-Graphics_TitleHeight()/2) {
		mousedata.type=element_TITLE;
	}
	
	/*Check for a double click*/
	if (block->data.mouse.button.data.select) {
		Windows_UnselectAll(windowdata);
		switch (mousedata.type) {
			case element_PERSON:
			case element_MARRIAGE:
				Database_Edit(mousedata.element);
				break;
			case element_TITLE:
				Database_EditTitle();
		}
		return Desk_TRUE;
	}
    switch (windowdata->type) {
		case wintype_ANCESTORS:
		case wintype_DESCENDENTS:
			/*Only menu and double clicks are relevent for these windows*/
			if (block->data.mouse.button.data.menu) {
				Windows_SetUpMenu();
				switch (mousedata.type) {
					case element_MARRIAGE:
						Desk_Menu_SetText(mainmenu,mainmenu_PERSON,AJWLib_Msgs_TempLookup("Item.Marriage:Marriage"));
						/*No break*/
					case element_PERSON:
						AJWLib_Menu_UnShade(personmenu,personmenu_EDIT);
						AJWLib_Menu_UnShade(mainmenu,mainmenu_PERSON);
						break;
				}
				Desk_Menu_Show(mainmenu,block->data.mouse.pos.x,block->data.mouse.pos.y);
			}
			break;
		case wintype_NORMAL:
			switch (block->data.mouse.button.value) {
				case Desk_button_MENU:
					Windows_SetUpMenu();
					AJWLib_Menu_UnShade(mainmenu,mainmenu_ADDPERSON);
					/*AJWLib_Menu_UnShade(mainmenu,mainmenu_SELECT);*/ /*Temporary, until selected descendents etc. works again*/
					selected=Database_AnyoneSelected();
					if (selected==element_NONE) {
						switch (mousedata.type) {
							case element_PERSON:
								Database_Select(mousedata.element);
								Windows_RedrawPerson(windowdata,windowdata->layout->person+mousedata.layoutptr);
								selected=element_PERSON;
								Desk_EventMsg_Claim(Desk_message_MENUSDELETED,Desk_event_ANY,Windows_MenusDeleted,windowdata);
								menusdeletedvalid=Desk_TRUE;
								break;
							case element_MARRIAGE:
								Database_Select(mousedata.element);
								Windows_RedrawMarriage(windowdata,windowdata->layout->marriage+mousedata.layoutptr);
								selected=element_MARRIAGE;
								Desk_EventMsg_Claim(Desk_message_MENUSDELETED,Desk_event_ANY,Windows_MenusDeleted,windowdata);
								menusdeletedvalid=Desk_TRUE;
								break;
						}
					}
					switch (selected) {
						case element_PERSON:
							AJWLib_Menu_UnShade(personmenu,personmenu_DELETE);
							AJWLib_Menu_UnShade(personmenu,personmenu_EDIT);
							AJWLib_Menu_UnShade(mainmenu,mainmenu_PERSON);
							break;
						case element_MARRIAGE:
							AJWLib_Menu_UnShade(personmenu,personmenu_DELETE);
							AJWLib_Menu_UnShade(personmenu,personmenu_EDIT);
							AJWLib_Menu_UnShade(mainmenu,mainmenu_PERSON);
							Desk_Menu_SetText(mainmenu,mainmenu_PERSON,AJWLib_Msgs_TempLookup("Item.Marriage:Marriage"));
							break;
						case element_SELECTION:
							AJWLib_Menu_UnShade(personmenu,personmenu_DELETE);
							AJWLib_Menu_UnShade(mainmenu,mainmenu_PERSON);
							Desk_Menu_SetText(mainmenu,mainmenu_PERSON,AJWLib_Msgs_TempLookup("Item.Select:Selection"));
							break;
						case element_NONE:
							AJWLib_Menu_Shade(mainmenu,mainmenu_SELECT);
							break;
					}
					Desk_Menu_Show(mainmenu,block->data.mouse.pos.x,block->data.mouse.pos.y);
					break;
				case Desk_button_CLICKSELECT:
					switch (mousedata.type) {
						case element_PERSON:
							if (!Database_GetSelect(windowdata->layout->person[mousedata.layoutptr].person)) {
								Windows_UnselectAll(windowdata);
								Database_Select(windowdata->layout->person[mousedata.layoutptr].person);
								Windows_RedrawPerson(windowdata,windowdata->layout->person+mousedata.layoutptr);
							}
							break;
						case element_MARRIAGE:
							if (!Database_GetSelect(windowdata->layout->marriage[mousedata.layoutptr].marriage)) {
								Windows_UnselectAll(windowdata);
								Database_Select(windowdata->layout->marriage[mousedata.layoutptr].marriage);
								Windows_RedrawMarriage(windowdata,windowdata->layout->marriage+mousedata.layoutptr);
							}
							break;
						default:
							Windows_UnselectAll(windowdata);
					}
					break;
				case Desk_button_CLICKADJUST:
					switch (mousedata.type) {
						case element_PERSON:
							if (Database_GetSelect(windowdata->layout->person[mousedata.layoutptr].person)) {
								Database_DeSelect(windowdata->layout->person[mousedata.layoutptr].person);
							} else {
								Database_Select(windowdata->layout->person[mousedata.layoutptr].person);
							}
							Windows_RedrawPerson(windowdata,windowdata->layout->person+mousedata.layoutptr);
							break;
						case element_MARRIAGE:
							if (Database_GetSelect(windowdata->layout->marriage[mousedata.layoutptr].marriage)) {
								Database_DeSelect(windowdata->layout->marriage[mousedata.layoutptr].marriage);
							} else {
								Database_Select(windowdata->layout->marriage[mousedata.layoutptr].marriage);
							}
							Windows_RedrawMarriage(windowdata,windowdata->layout->marriage+mousedata.layoutptr);
							break;
					}
					break;
				case Desk_button_DRAGSELECT:
					switch (mousedata.type) {
						case element_PERSON:
							Windows_StartDragNormal(windowdata->layout->person[mousedata.layoutptr].person,windowdata->layout->person[mousedata.layoutptr].x,windowdata->layout->person[mousedata.layoutptr].y,windowdata,Desk_FALSE);
							break;
						case element_MARRIAGE:
							Windows_StartDragNormal(windowdata->layout->marriage[mousedata.layoutptr].marriage,windowdata->layout->marriage[mousedata.layoutptr].x,windowdata->layout->marriage[mousedata.layoutptr].y,windowdata,Desk_TRUE);
							break;
						default:
							Windows_UnselectAll(windowdata);
							Windows_StartDragSelect(windowdata);
					}
					break;
				case Desk_button_DRAGADJUST:
					switch (mousedata.type) {
						case element_PERSON:
							Windows_StartDragLink(windowdata,mousedata.element);
							break;
						case element_NONE:
							Windows_StartDragSelect(windowdata);
					}
			}
	}
	return Desk_TRUE;
}

void Windows_Relayout(void)
{
	int i;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle!=0) {
			Desk_Error2_Try {
				switch (windows[i].type) {
					case wintype_NORMAL:
						Layout_LayoutLines(windows[i].layout);
						Layout_LayoutTitle(windows[i].layout);
						break;
					case wintype_DESCENDENTS:
						Layout_Free(windows[i].layout);
						windows[i].layout=NULL;
						windows[i].layout=Layout_LayoutDescendents(windows[i].person,windows[i].generations);
						break;
					case wintype_ANCESTORS:
						Layout_Free(windows[i].layout);
						windows[i].layout=NULL;
						windows[i].layout=Layout_LayoutAncestors(windows[i].person,windows[i].generations);
						break;
					default:
						AJWLib_Assert(0);
				}
			} Desk_Error2_Catch {
				AJWLib_Error2_Report("%s");
			} Desk_Error2_EndCatch
			Windows_ResizeWindow(&windows[i]);
			Desk_Window_ForceWholeRedraw(windows[i].handle);
		}
	}
}

int Windows_GetSize(void)
{
	int i,size=0;
	Desk_bool donelayout=Desk_FALSE;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle!=0) {
			size+=sizeof(savedata)+sizeof(tag)+sizeof(int);
			if (windows[i].type==wintype_NORMAL || !donelayout) {
				size+=Layout_GetSize(windows[i].layout);
				donelayout=Desk_TRUE;
			}
		}
	}
	return size;
}

void Windows_Load(FILE *file)
{
	savedata data;
	AJWLib_Assert(file!=NULL);
	AJWLib_File_fread(&data,sizeof(savedata),1,file);
	Windows_OpenWindow(data.type,data.person,data.generations,data.scale,&(data.coords));
}

layout *Windows_Save(FILE *file,int *index)
{
	int i=(*index)++;
	savedata data;
	tag tag=tag_WINDOW;
	int size=sizeof(savedata)+sizeof(tag)+sizeof(int);
	AJWLib_Assert(file!=NULL);
	if (i>=MAXWINDOWS) {
		*index=-1;
		return NULL;
	}
	AJWLib_Assert(i>=0);
	if (windows[i].handle) {
		Desk_Window_GetCoords(windows[i].handle,&(data.coords));
		data.type=windows[i].type;
		data.person=windows[i].person;
		data.generations=windows[i].generations;
		data.scale=windows[i].scale;
		data.reserved[0]=0;
		data.reserved[1]=0;
		data.reserved[2]=0;
		data.reserved[3]=0;
		AJWLib_File_fwrite(&tag,sizeof(tag),1,file);
		AJWLib_File_fwrite(&size,sizeof(int),1,file);
		AJWLib_File_fwrite(&data,sizeof(savedata),1,file);
		if (windows[i].type==wintype_NORMAL) return windows[i].layout;
	}
	return NULL;
}

layout *Windows_SaveGEDCOM(FILE *file,int *index)
{
	Desk_convert_block coords;
	int i=(*index)++;

	AJWLib_Assert(file!=NULL);
	AJWLib_Assert(i>=0);
	if (i>=MAXWINDOWS) {
		*index=-1;
		return NULL;
	}

	if (i==0) fprintf(file,"0 @W1@ _WINDOWS\n");

	if (windows[i].handle) {
		fprintf(file,"1 _TYPE %d\n",windows[i].type);
		Desk_Window_GetCoords(windows[i].handle,&(coords));
		fprintf(file,"1 _COORDS\n2 _SCREENRECT\n3 _MIN\n4 _X %d\n4 _Y %d\n3 _MAX\n4 _X %d\n4 _Y %d\n2 _SCROLL\n3 _X %d\n3 _Y %d\n",coords.screenrect.min.x,coords.screenrect.min.y,coords.screenrect.max.x,coords.screenrect.max.y,coords.scroll.x,coords.scroll.y);
		fprintf(file,"1 _PERSON @%d@\n",windows[i].person);
		fprintf(file,"1 _GENERATIONS %d\n",windows[i].generations);
		fprintf(file,"1 _SCALE %d\n",windows[i].scale);

		if (windows[i].type==wintype_NORMAL) return windows[i].layout;
	}
	return NULL;
}

static void Windows_OpenSaveWindow(void)
{
	if (!strcmp(File_GetFilename(),AJWLib_Msgs_TempLookup("File.Tree:Untitled"))) {
		AJWLib_Window_OpenTransient(savewin);
		/*How do I close all windows once savebox has been dealt with?*/
	} else {
		File_SaveFile(NULL,NULL);
		Windows_CloseAllWindows();
	}
}

void Windows_CloseAllWindows(void)
/*Tidy up quietly*/
{
	int i;
	menusdeletedvalid=Desk_FALSE;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle) {
			Desk_Error2_TryCatch(if (windows[i].layout) Layout_Free(windows[i].layout);,)
			Desk_Error2_TryCatch(Desk_Window_Delete(windows[i].handle);,)
			windows[i].handle=NULL;
		}
	}
	Desk_Error2_TryCatch(Desk_Window_Hide(fileconfigwin);,)
	Desk_Error2_TryCatch(Print_CloseWindow();,)
	Desk_Error2_TryCatch(File_Remove();,)
}

static Desk_bool Windows_CloseWindow(Desk_event_pollblock *block,windowdata *windowdata)
{
	int i,found=0;
	Desk_UNUSED(block);
	menusdeletedvalid=Desk_FALSE;
	for (i=0;i<MAXWINDOWS;i++)
		if (windows[i].handle!=0)
			if (windows[i].type==wintype_NORMAL) found++;
	AJWLib_Assert(found>0);
	switch (windowdata->type) {
		case wintype_NORMAL:
			if (found<=1) {
				/*Warn if unsaved data*/
				if (File_GetModified()) {
					AJWLib_Window_OpenTransient(unsavedwin);
				} else {
					Windows_CloseAllWindows();
				}
			} else {
				Desk_Window_Delete(windowdata->handle);
				windowdata->handle=0;
			}
			break;
		default:
			Layout_Free(windowdata->layout);
			Desk_Window_Delete(windowdata->handle);
			windowdata->handle=0;
	}
	return Desk_TRUE;
}

void Windows_FileModified(void)
{
	int i;
	char title[256+2];
	strcpy(title,File_GetFilename());
	if (File_GetModified()) strcat(title," *");
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle) {
			if (windows[i].type==wintype_NORMAL) Desk_Window_SetTitle(windows[i].handle,title);
		}
	}
}

void Windows_CloseNewView(void)
{
	Desk_Window_Hide(newviewwin);
}

static void Windows_OpenWindowCentered(windowdata *windowdata,Desk_convert_block *coords)
{
	Desk_window_openblock blk;
	Desk_wimp_rect bbox;
	AJWLib_Assert(windowdata!=NULL);
	blk.window=windowdata->handle;
	blk.behind=-1;
	blk.screenrect.min.x=0;
	blk.screenrect.max.x=INFINITY;
	blk.screenrect.min.y=-INFINITY;
	blk.screenrect.max.y=Desk_screen_size.y;
	bbox=Layout_FindExtent(windowdata->layout,Desk_FALSE);
	blk.screenrect.min.x=Desk_screen_size.x/2-((Graphics_WindowBorder()+bbox.max.x-bbox.min.x)*windowdata->scale)/200;
	blk.screenrect.max.y=Desk_screen_size.y/2+((Graphics_WindowBorder()+(Config_Title() ? Graphics_TitleHeight() : 0)+bbox.max.y-bbox.min.y)*windowdata->scale)/200;
	blk.scroll.x=0;
	blk.scroll.y=0;
	if (coords) {
		blk.screenrect=coords->screenrect;
		blk.scroll=coords->scroll;
	}
	Desk_Wimp_OpenWindow(&blk);

}

void Windows_LayoutNormal(layout *layout,Desk_bool opencentred)
{
	int i;
	if (layout==NULL) Desk_Error2_TryCatch(layout=Layout_LayoutNormal();,AJWLib_Error2_Report("%s");)
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle && windows[i].type==wintype_NORMAL && windows[i].layout==NULL) {
			windows[i].layout=layout;
			Windows_ResizeWindow(&windows[i]);
			if (opencentred) Windows_OpenWindowCentered(&windows[i],NULL);
		}
	}
}

void Windows_OpenWindow(wintype type,elementptr person,int generations,int scale,Desk_convert_block *coords)
{
	/*Let any Error2s be handled by the calling function*/
	int newwindow;
	char str[256]="";
	layout *layoutnormal=NULL;
	int i;
	if (numwindows>MAXWINDOWS) {
		Desk_Msgs_Report(1,"Error.NufWin:Too many windows (%d)",MAXWINDOWS);
		return;
	}
	for (newwindow=0;windows[newwindow].handle!=0 && newwindow<MAXWINDOWS;newwindow++);
	if (newwindow==MAXWINDOWS) {
		Desk_Msgs_Report(1,"Error.NufWin:Too many windows (%d)",MAXWINDOWS);
		return;
	}
	for (i=0;i<MAXWINDOWS;i++) if (windows[i].handle!=0 && windows[i].type==wintype_NORMAL) layoutnormal=windows[i].layout;
	windows[newwindow].handle=Desk_Window_Create("Main",Desk_template_TITLEMIN);
	windows[newwindow].type=type;
	windows[newwindow].person=person;
	windows[newwindow].generations=generations;
	windows[newwindow].scale=scale;
	switch (type) {
		case wintype_NORMAL:
			Desk_Window_SetTitle(windows[newwindow].handle,File_GetFilename());
#ifdef DEBUG
			Desk_Event_Claim(Desk_event_REDRAW,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Windows_RedrawWindow,&windows[newwindow]);
			windows[newwindow].layout=debuglayout;
#endif
			windows[newwindow].layout=layoutnormal;
			break;
		case wintype_DESCENDENTS:
			Desk_Msgs_Lookup("Win.Desc:",str,255);
			strcat(str," ");
			strcat(str,Database_GetName(person));
			Desk_Window_SetTitle(windows[newwindow].handle,str);
			windows[newwindow].layout=NULL;
			windows[newwindow].layout=Layout_LayoutDescendents(person,generations);
			break;
		case wintype_ANCESTORS:
			Desk_Msgs_Lookup("Win.Anc:",str,255);
			strcat(str," ");
			strcat(str,Database_GetName(person));
			Desk_Window_SetTitle(windows[newwindow].handle,str);
			windows[newwindow].layout=NULL;
#ifdef DEBUG
			Desk_Event_Claim(Desk_event_REDRAW,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Windows_RedrawWindow,&windows[newwindow]);
			windows[newwindow].layout=debuglayout;
#endif
			windows[newwindow].layout=Layout_LayoutAncestors(person,generations);
			break;
		default:
			windows[newwindow].layout=NULL;
			AJWLib_Assert(0);
	}
	if (type!=wintype_NORMAL || layoutnormal!=NULL) Windows_ResizeWindow(&windows[newwindow]);
	Desk_Event_Claim(Desk_event_CLICK,windows[newwindow].handle,Desk_event_ANY,Windows_MouseClick,&windows[newwindow]);
	Desk_Event_Claim(Desk_event_REDRAW,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Windows_RedrawWindow,&windows[newwindow]);
	Desk_Event_Claim(Desk_event_CLOSE,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Windows_CloseWindow,&windows[newwindow]);
	Windows_OpenWindowCentered(&windows[newwindow],coords);
	File_Modified();
}

static void Windows_MainMenuClick(int entry,void *ref)
{
	volatile elementptr newperson;
	char buffer[10]="";
	Desk_UNUSED(ref);
	switch (entry) {
		case mainmenu_NEWVIEW:
			if (mousedata.element==none) {
				Desk_Icon_Shade(newviewwin,newview_ANCESTOR);
				Desk_Icon_Shade(newviewwin,newview_ANCESTORPERSON);
				Desk_Icon_Shade(newviewwin,newview_DESCENDENT);
				Desk_Icon_Shade(newviewwin,newview_DESCENDENTPERSON);
				Desk_Icon_Shade(newviewwin,newview_CLOSERELATIVES);
				Desk_Icon_Shade(newviewwin,newview_CLOSERELATIVESPERSON);
			} else {
				Desk_Icon_Unshade(newviewwin,newview_ANCESTOR);
				Desk_Icon_Unshade(newviewwin,newview_ANCESTORPERSON);
				Desk_Icon_Unshade(newviewwin,newview_DESCENDENT);
				Desk_Icon_Unshade(newviewwin,newview_DESCENDENTPERSON);
/*				Desk_Icon_Unshade(newviewwin,newview_CLOSERELATIVES);
				Desk_Icon_Unshade(newviewwin,newview_CLOSERELATIVESPERSON);
*/				strcpy(buffer,Database_GetName(mousedata.element));
			}
			Desk_Icon_Shade(newviewwin,newview_UPTO);
			Desk_Icon_Shade(newviewwin,newview_GENERATIONS);
			Desk_Icon_Shade(newviewwin,newview_UP);
			Desk_Icon_Shade(newviewwin,newview_DOWN);
			Desk_Icon_Shade(newviewwin,newview_GENERATIONSTEXT);
			Desk_Icon_SetText(newviewwin,newview_ANCESTORPERSON,buffer);
			Desk_Icon_SetText(newviewwin,newview_DESCENDENTPERSON,buffer);
			Desk_Icon_SetText(newviewwin,newview_CLOSERELATIVESPERSON,buffer);
			Desk_Window_Show(newviewwin,Desk_open_CENTERED);
			Desk_Icon_SetCaret(newviewwin,-1);
			Desk_Icon_SetRadios(newviewwin,newview_NORMAL,newview_CLOSERELATIVES,newview_NORMAL);
			newviewperson=mousedata.element;
			break;
		case mainmenu_SCALE:
			Desk_Window_Show(scalewin,Desk_open_CENTERED);
			Desk_Icon_SetCaret(scalewin,scale_TEXT);
			break;
		case mainmenu_ADDPERSON:
			newperson=Database_Add();
			Desk_Error2_Try {
				Layout_AddPerson(mousedata.window->layout,newperson,mousedata.pos.x,Layout_NearestGeneration(mousedata.pos.y));
			} Desk_Error2_Catch {
				Database_Delete(newperson);
				AJWLib_Error2_Report("%s");
			} Desk_Error2_EndCatch
	}
}

Desk_bool Windows_Cancel(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.select) {
		Desk_Window_Hide(block->data.mouse.window);
		return Desk_TRUE;
	}
	return Desk_FALSE;
}

static Desk_bool Windows_FileConfigOk(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	Database_SetUserDesc(0,Desk_Icon_GetTextPtr(fileconfigwin,fileconfig_USER1));
	Database_SetUserDesc(1,Desk_Icon_GetTextPtr(fileconfigwin,fileconfig_USER2));
	Database_SetUserDesc(2,Desk_Icon_GetTextPtr(fileconfigwin,fileconfig_USER3));
	if (block->data.mouse.button.data.select) Desk_Window_Hide(block->data.mouse.window);
	return Desk_TRUE;
}

static void Windows_OpenFileConfig(void)
{
	Desk_Icon_SetText(fileconfigwin,fileconfig_USER1,Database_GetUserDesc(0));
	Desk_Icon_SetText(fileconfigwin,fileconfig_USER2,Database_GetUserDesc(1));
	Desk_Icon_SetText(fileconfigwin,fileconfig_USER3,Database_GetUserDesc(2));
	Desk_Window_Show(fileconfigwin,Desk_open_CENTERED);
	Desk_Icon_SetCaret(fileconfigwin,fileconfig_USER1);
}

static void Windows_FileMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	switch (entry) {
		case filemenu_CHOICES:
			Windows_OpenFileConfig();
			break;
		case filemenu_PRINT:
			Print_OpenWindow(mousedata.window->layout);
	}
}

static Desk_bool Windows_NewViewOk(Desk_event_pollblock *block,void *ref)
{
	wintype wintype;
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	switch (Desk_Icon_WhichRadio(newviewwin,newview_NORMAL,newview_DESCENDENT)) {
		case newview_NORMAL:
			wintype=wintype_NORMAL;
			break;
		case newview_ANCESTOR:
			wintype=wintype_ANCESTORS;
			break;
		case newview_DESCENDENT:
			wintype=wintype_DESCENDENTS;
			break;
		case newview_CLOSERELATIVES:
			wintype=wintype_CLOSERELATIVES;
			break;
		default:
			wintype=wintype_UNKNOWN;
			AJWLib_Assert(0);
	}
	if (block->data.mouse.button.data.select) Desk_Window_Hide(newviewwin);
	Desk_Error2_Try {
		Windows_OpenWindow(wintype,newviewperson,atoi(Desk_Icon_GetTextPtr(newviewwin,newview_GENERATIONS)),100,NULL);
		if (wintype==wintype_NORMAL) Windows_LayoutNormal(NULL,Desk_TRUE);
	} Desk_Error2_Catch {
		AJWLib_Error2_Report("%s");
	} Desk_Error2_EndCatch
	return Desk_TRUE;
}

static Desk_bool Windows_SaveDraw(char *filename,void *ref)
{
	Desk_UNUSED(ref);
	Drawfile_Save(filename,mousedata.window->layout);
	return Desk_TRUE;
}

static void Windows_PersonMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	switch (entry) {
		case personmenu_EDIT:
			Database_Edit(mousedata.element);
			break;
		case personmenu_DELETE:
			Database_DeleteSelected(mousedata.window->layout);
			AJWLib_Menu_Shade(personmenu,entry);
			break;
	}
}

static void Windows_SelectMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	switch (entry) {
		case selectmenu_DESCENDENTS:
			Layout_SelectDescendents(mousedata.window->layout,mousedata.element);
			Desk_Window_ForceWholeRedraw(mousedata.window->handle);
			break;
		case selectmenu_ANCESTORS:
			Layout_SelectAncestors(mousedata.window->layout,mousedata.element);
			Desk_Window_ForceWholeRedraw(mousedata.window->handle);
			break;
		case selectmenu_SIBLINGS:
			Layout_SelectSiblings(mousedata.window->layout,mousedata.element);
			Desk_Window_ForceWholeRedraw(mousedata.window->handle);
			break;
		case selectmenu_SPOUSES:
			Layout_SelectSpouses(mousedata.window->layout,mousedata.element);
			Desk_Window_ForceWholeRedraw(mousedata.window->handle);
			break;
	}
}

static Desk_bool Windows_NewViewClick(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	switch (block->data.mouse.icon) {
		case newview_NORMAL:
			Desk_Icon_Shade(newviewwin,newview_UPTO);
			Desk_Icon_Shade(newviewwin,newview_GENERATIONS);
			Desk_Icon_Shade(newviewwin,newview_UP);
			Desk_Icon_Shade(newviewwin,newview_DOWN);
			Desk_Icon_Shade(newviewwin,newview_GENERATIONSTEXT);
			Desk_Icon_SetCaret(newviewwin,-1);
			break;
		case newview_ANCESTOR:
		case newview_DESCENDENT:
		case newview_CLOSERELATIVES:
			Desk_Icon_Unshade(newviewwin,newview_UPTO);
			Desk_Icon_Unshade(newviewwin,newview_GENERATIONS);
			Desk_Icon_Unshade(newviewwin,newview_UP);
			Desk_Icon_Unshade(newviewwin,newview_DOWN);
			Desk_Icon_Unshade(newviewwin,newview_GENERATIONSTEXT);
			Desk_Icon_SetCaret(newviewwin,newview_GENERATIONS);
			break;
	}
	return Desk_FALSE;
}

static Desk_bool Windows_ScaleClick(Desk_event_pollblock *block,void *ref)
{
	Desk_wimp_rect bbox;
	int scale,xscale,yscale,xwindowborders,ywindowborders;
	Desk_window_state stateblk;
	Desk_window_outline outlineblk;
	Desk_UNUSED(ref);
	outlineblk.window=mousedata.window->handle;
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	switch (block->data.mouse.icon) {
		case scale_SIZE1:
			AJWLib_Msgs_SetText(scalewin,scale_TEXT,"Scale.1:100");
			Desk_Icon_SetCaret(scalewin,scale_TEXT);
			return Desk_TRUE;
			break;
		case scale_SIZE2:
			AJWLib_Msgs_SetText(scalewin,scale_TEXT,"Scale.2:100");
			Desk_Icon_SetCaret(scalewin,scale_TEXT);
			return Desk_TRUE;
			break;
		case scale_SIZE3:
			AJWLib_Msgs_SetText(scalewin,scale_TEXT,"Scale.3:100");
			Desk_Icon_SetCaret(scalewin,scale_TEXT);
			return Desk_TRUE;
			break;
		case scale_SIZE4:
			AJWLib_Msgs_SetText(scalewin,scale_TEXT,"Scale.4:100");
			Desk_Icon_SetCaret(scalewin,scale_TEXT);
			return Desk_TRUE;
			break;
		case scale_FIT:
			bbox=Layout_FindExtent(mousedata.window->layout,Desk_FALSE);
			Desk_Wimp_GetWindowState(mousedata.window->handle,&stateblk);
			Desk_Wimp_GetWindowOutline(&outlineblk);
			xwindowborders=(outlineblk.screenrect.max.x-outlineblk.screenrect.min.x)-(stateblk.openblock.screenrect.max.x-stateblk.openblock.screenrect.min.x);
			ywindowborders=(outlineblk.screenrect.max.y-outlineblk.screenrect.min.y)-(stateblk.openblock.screenrect.max.y-stateblk.openblock.screenrect.min.y);
			xscale=(Desk_screen_size.x-xwindowborders)*100/(bbox.max.x-bbox.min.x+2*Graphics_WindowBorder());
			yscale=(Desk_screen_size.y-ywindowborders)*100/(bbox.max.y-bbox.min.y+2*Graphics_WindowBorder());
			scale=Desk_MIN(xscale,yscale);
			if (scale<1) scale=1;
			if (scale>999) scale=999;
			Desk_Icon_SetInteger(scalewin,scale_TEXT,scale);
			return Desk_TRUE;
			break;
		case scale_OK:
			scale=Desk_Icon_GetInteger(scalewin,scale_TEXT);
			if (scale<1) scale=1;
			mousedata.window->scale=scale;
			Windows_ResizeWindow(mousedata.window);
			Desk_Window_ForceWholeRedraw(mousedata.window->handle);
			if (block->data.mouse.button.data.select) Desk_Window_Hide(scalewin);
			break;
	}
	return Desk_FALSE;
}

void Windows_Init(void)
{
	int i;
	numwindows=0;
	for (i=0;i<MAXWINDOWS;i++) windows[i].handle=0;
	Desk_Drag_Initialise(Desk_TRUE);
	newviewwin=Desk_Window_Create("NewView",Desk_template_TITLEMIN);
	Desk_Icon_InitIncDecHandler(newviewwin,newview_GENERATIONS,newview_UP,newview_DOWN,Desk_FALSE,1,1,999,10);
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_NORMAL);
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_ANCESTOR);
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_DESCENDENT);
	Desk_Event_Claim(Desk_event_CLICK,newviewwin,Desk_event_ANY,Windows_NewViewClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,newviewwin,newview_OK,Windows_NewViewOk,NULL);
	Desk_Event_Claim(Desk_event_CLICK,newviewwin,newview_CANCEL,Windows_Cancel,NULL);
	fileinfowin=Desk_Window_Create("FileInfo",Desk_template_TITLEMIN);
	exportmenu=AJWLib_Menu_CreateFromMsgs("Title.Export:","Menu.Export:",NULL,NULL);
	filemenu=AJWLib_Menu_CreateFromMsgs("Title.File:","Menu.File:",Windows_FileMenuClick,NULL);
	personmenu=AJWLib_Menu_CreateFromMsgs("Title.Person:","Menu.Person:",Windows_PersonMenuClick,NULL);
	mainmenu=AJWLib_Menu_CreateFromMsgs("Title.Main:","Menu.Main:",Windows_MainMenuClick,NULL);
	selectmenu=AJWLib_Menu_CreateFromMsgs("Title.Select:","Menu.Select:",Windows_SelectMenuClick,NULL);
	Desk_Menu_AddSubMenu(mainmenu,mainmenu_PERSON,personmenu);
	Desk_Menu_AddSubMenu(mainmenu,mainmenu_FILE,filemenu);
	Desk_Menu_AddSubMenu(filemenu,filemenu_EXPORT,exportmenu);
	Desk_Menu_AddSubMenu(filemenu,filemenu_INFO,(Desk_menu_ptr)fileinfowin);
	Desk_Menu_AddSubMenu(mainmenu,mainmenu_SELECT,selectmenu);
	Desk_Icon_Shade(newviewwin,newview_ANCESTOR);
	Desk_Icon_Shade(newviewwin,newview_ANCESTORPERSON);
	Desk_Icon_Shade(newviewwin,newview_DESCENDENT);
	Desk_Icon_Shade(newviewwin,newview_DESCENDENTPERSON);
	Desk_Icon_Shade(newviewwin,newview_CLOSERELATIVES);
	Desk_Icon_Shade(newviewwin,newview_CLOSERELATIVESPERSON);
	unsavedwin=Desk_Window_Create("Unsaved",Desk_template_TITLEMIN);
	scalewin=Desk_Window_Create("Scale",Desk_template_TITLEMIN);
	Desk_Icon_InitIncDecHandler(scalewin,scale_TEXT,scale_UP,scale_DOWN,Desk_FALSE,1,1,999,100);
	Desk_Event_Claim(Desk_event_CLICK,scalewin,Desk_event_ANY,Windows_ScaleClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,scalewin,scale_CANCEL,Windows_Cancel,NULL);
	AJWLib_Window_KeyHandler(scalewin,scale_OK,Windows_ScaleClick,scale_CANCEL,Windows_Cancel,NULL);
	savewin=Desk_Window_Create("Save",Desk_template_TITLEMIN);
	Desk_Menu_AddSubMenu(filemenu,filemenu_SAVE,(Desk_menu_ptr)savewin);
	Desk_Save_InitSaveWindowHandler(savewin,Desk_TRUE,Desk_TRUE,Desk_FALSE,save_ICON,save_OK,save_CANCEL,save_FILENAME,File_SaveFile,NULL,File_Result,1024*10/*Filesize estimate?*/,0x090/*Filetype*/,NULL);
	savedrawwin=Desk_Window_Create("Save",Desk_template_TITLEMIN);
	savegedcomwin=Desk_Window_Create("Save",Desk_template_TITLEMIN);
	Desk_Menu_AddSubMenu(exportmenu,exportmenu_DRAW,(Desk_menu_ptr)savedrawwin);
	Desk_Menu_AddSubMenu(exportmenu,exportmenu_GEDCOM,(Desk_menu_ptr)savegedcomwin);
/*	Desk_Menu_AddSubMenu(filemenu,filemenu_EXPORT,(Desk_menu_ptr)savedrawwin);*/
	Desk_Save_InitSaveWindowHandler(savedrawwin,Desk_TRUE,Desk_TRUE,Desk_FALSE,save_ICON,save_OK,save_CANCEL,save_FILENAME,Windows_SaveDraw,NULL,NULL/*Need a result handler?*/,1024*10/*Filesize estimate?*/,Desk_filetype_DRAWFILE,NULL);
	Desk_Save_InitSaveWindowHandler(savegedcomwin,Desk_TRUE,Desk_TRUE,Desk_FALSE,save_ICON,save_OK,save_CANCEL,save_FILENAME,File_SaveGEDCOM,NULL,NULL/*Need a result handler?*/,1024*10/*Filesize estimate?*/,Desk_filetype_TEXT,NULL);
	fileconfigwin=Desk_Window_Create("FileConfig",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,fileconfigwin,fileconfig_OK,Windows_FileConfigOk,NULL);
	Desk_Event_Claim(Desk_event_CLICK,fileconfigwin,fileconfig_CANCEL,Windows_Cancel,NULL);
	AJWLib_Window_KeyHandler(fileconfigwin,fileconfig_OK,Windows_FileConfigOk,fileconfig_CANCEL,Windows_Cancel,NULL);
	AJWLib_Window_RegisterDCS(unsavedwin,unsaved_DISCARD,unsaved_CANCEL,unsaved_SAVE,Windows_CloseAllWindows,Windows_OpenSaveWindow);
}

