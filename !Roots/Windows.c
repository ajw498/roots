/*
	FT - Windows, menus and interface
	© Alex Waugh 1999

	$Id: Windows.c,v 1.55 2000/02/28 00:22:01 uid1 Exp $

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

#include "Database.h"
#include "Graphics.h"
#include "Modules.h"
#include "Windows.h"
#include "Config.h"
#include "Layout.h"
#include "Drawfile.h"
#include "Draw.h"
#include "File.h"

#define MAXWINDOWS 10
#define REDRAWOVERLAP 4

#define mainmenu_FILE 0
#define mainmenu_PERSON 1
#define mainmenu_NEWVIEW 3
#define mainmenu_SEARCH 4
#define mainmenu_REPORTS 5
#define mainmenu_SELECT 2
#define mainmenu_SCALE 6

#define filemenu_INFO 0
#define filemenu_SAVE 1
#define filemenu_EXPORT 2
#define filemenu_PRINT 3

#define exportmenu_GEDCOM 0
#define exportmenu_DRAW 1
#define exportmenu_TEXT 2

#define personmenu_ADD 0
#define personmenu_DELETE 1
#define personmenu_UNLINK 2

#define selectmenu_DESCENDENTS 0
#define selectmenu_ANCESTORS 1
#define selectmenu_SIBLINGS 2
#define selectmenu_SPOUSES 3

#define newview_UNLINKED 1
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

typedef struct dragdata {
	elementptr person;
	int persony;
	int personoffset;
	windowdata *windowdata;
	Desk_wimp_rect  coords;
	int origmousex,oldmousex,oldoffset,oldmousey,centered;
	Desk_bool plotted,marriage;
} dragdata;

#if DEBUG
extern layout *debuglayout;
#endif

static windowdata windows[MAXWINDOWS];
static int numwindows;
static Desk_window_handle addparentswin,newviewwin,fileinfowin,savewin,savedrawwin,scalewin;
static Desk_menu_ptr mainmenu,filemenu,exportmenu,personmenu,selectmenu;
static elementptr addparentsperson,addparentschild,newviewperson,menuoverperson;
static windowdata *menuoverwindow;

void Windows_RedrawPerson(windowdata *windowdata,personlayout *person)
{
	Desk_Window_ForceRedraw(windowdata->handle,(windowdata->scale*person->x)/100-REDRAWOVERLAP,(windowdata->scale*person->y)/100-REDRAWOVERLAP,(windowdata->scale*(person->x+Graphics_PersonWidth()))/100+REDRAWOVERLAP,(windowdata->scale*(person->y+Graphics_PersonHeight()))/100+REDRAWOVERLAP);
}

void Windows_RedrawMarriage(windowdata *windowdata,marriagelayout *marriage)
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
#if DEBUG
		Desk_ColourTrans_SetGCOL(0x00000000,0,0);
		AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+1000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
		AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-1000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
		Desk_ColourTrans_SetGCOL(0x0000FF00,0,0);
		AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+2000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
		AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-2000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
		Desk_ColourTrans_SetGCOL(0x00FF0000,0,0);
		AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+3000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
		AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-3000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
#endif
		Graphics_Redraw(windowdata->layout,windowdata->scale,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,&(blk.cliprect),Desk_TRUE,Draw_PlotLine,Draw_PlotRectangle,Draw_PlotRectangleFilled,Draw_PlotText);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
	return Desk_TRUE;
}

static Desk_bool Windows_RedrawAddParents(Desk_event_pollblock *block,void *ref)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=block->data.openblock.window;
	Desk_Wimp_RedrawWindow(&blk,&more);
	while (more) {
		Graphics_PlotPerson(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,addparentsperson,332,-16-Graphics_PersonHeight(),Desk_FALSE,Desk_FALSE);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
	return Desk_TRUE;
}

static Desk_bool Windows_AddParentsClick(Desk_event_pollblock *block,void *ref)
{
	int x=-INFINITY,i,y;
	elementptr person;
	layout *layout=NULL;
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	Desk_Window_Hide(addparentswin);
	Desk_Error2_TryCatch(Database_Marry(addparentschild,addparentsperson); , AJWLib_Error2_Report("%s"); return Desk_TRUE;)
	for (i=0;i<MAXWINDOWS;i++) if (windows[i].handle && windows[i].type==wintype_NORMAL) layout=windows[i].layout;
	if (layout==NULL) return Desk_FALSE;
	person=addparentschild;
	do {
		for (i=0;i<layout->numpeople;i++) {
			if (layout->person[i].person==person) {
				if (layout->person[i].x>x) x=layout->person[i].x;
				y=layout->person[i].y;
			}
		}
	} while ((person=Database_GetMarriageLtoR(person))!=none);
	person=addparentschild;
	while ((person=Database_GetMarriageRtoL(person))!=none) {
		for (i=0;i<layout->numpeople;i++) {
			if (layout->person[i].person==person) {
				if (layout->person[i].x>x) x=layout->person[i].x;
				y=layout->person[i].y;
			}
		}
	}
	AJWLib_Assert(x!=-INFINITY);
	x+=Graphics_PersonWidth()+Graphics_MarriageWidth()+Graphics_SecondMarriageGap();
	Desk_Error2_Try {
		Layout_AddPerson(layout,addparentsperson,x,y);
		Desk_Error2_Try {
			Layout_AddMarriage(layout,Database_GetMarriage(addparentsperson),x-Graphics_MarriageWidth(),y);
		} Desk_Error2_Catch {
			Layout_RemovePerson(layout,addparentsperson);
			Desk_Error2_ReThrow();
		} Desk_Error2_EndCatch
	} Desk_Error2_Catch {
		Database_RemoveSpouse(addparentsperson);
		AJWLib_Error2_Report("%s");
	} Desk_Error2_EndCatch
	return Desk_TRUE;
}

static void Windows_UnselectAll(windowdata *windowdata)
{
	int i;
	for (i=0;i<windowdata->layout->numpeople;i++) {
		if (windowdata->layout->person[i].selected) {
			windowdata->layout->person[i].selected=Desk_FALSE;
			Windows_RedrawPerson(windowdata,windowdata->layout->person+i);
		}
	}
	for (i=0;i<windowdata->layout->nummarriages;i++) {
		if (windowdata->layout->marriage[i].selected) {
			windowdata->layout->marriage[i].selected=Desk_FALSE;
			Windows_RedrawMarriage(windowdata,windowdata->layout->marriage+i);
		}
	}
}

static void Windows_AddSelected(layout *layout,int amount)
{
	int i;
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].selected) layout->person[i].x+=amount;
	}
	for (i=0;i<layout->nummarriages;i++) {
		if (layout->marriage[i].selected) layout->marriage[i].x+=amount;
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
		Draw_EORRectangle(dragdata->windowdata->scale,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,dragdata->oldmousex+dragdata->oldoffset-dragdata->personoffset,dragdata->persony,dragdata->marriage ? Graphics_MarriageWidth() : Graphics_PersonWidth(),Graphics_PersonHeight(),0,EORCOLOURRED);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
}

static void Windows_DragEnd(void *ref)
{
	/*User has finished a drag*/
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	elementptr initialperson=dragdata->person;
	int window=-1,mousex,mousey,i;
	Desk_Wimp_GetPointerInfo(&mouseblk);
	if (mouseblk.window==addparentswin) {
		if (initialperson==addparentsperson) return;
		if (Database_IsUnlinked(initialperson)) {
			/*Add parents*/
			int childx,childy;
			Desk_Window_Hide(addparentswin);
			Desk_Error2_TryCatch(Database_AddParents(addparentschild,addparentsperson,initialperson); , AJWLib_Error2_Report("%s"); return;)
			for (i=0;i<MAXWINDOWS;i++) if (windows[i].handle && windows[i].type==wintype_NORMAL) window=i;
			if (window==-1) return;
			childx=Layout_FindXCoord(windows[window].layout,addparentschild);
			childy=Layout_FindYCoord(windows[window].layout,addparentschild);
			Desk_Error2_Try {
				Layout_AddPerson(windows[window].layout,addparentsperson,childx-(Graphics_PersonWidth()+Graphics_MarriageWidth())/2,childy+Graphics_PersonHeight()+Graphics_GapHeightAbove()+Graphics_GapHeightBelow());
				Desk_Error2_Try {
					Layout_AddPerson(windows[window].layout,initialperson,childx+(Graphics_PersonWidth()+Graphics_MarriageWidth())/2,childy+Graphics_PersonHeight()+Graphics_GapHeightAbove()+Graphics_GapHeightBelow());
					Desk_Error2_Try {
						Layout_AddMarriage(windows[window].layout,Database_GetMarriage(initialperson),childx+(Graphics_PersonWidth()-Graphics_MarriageWidth())/2,childy+Graphics_PersonHeight()+Graphics_GapHeightAbove()+Graphics_GapHeightBelow());
					} Desk_Error2_Catch {
						Layout_RemovePerson(windows[window].layout,initialperson);
						Desk_Error2_ReThrow();
					} Desk_Error2_EndCatch
				} Desk_Error2_Catch {
					Layout_RemovePerson(windows[window].layout,addparentsperson);
					Desk_Error2_ReThrow();
				} Desk_Error2_EndCatch
				Layout_AlterChildline(windows[window].layout,addparentschild,Desk_TRUE);
			} Desk_Error2_Catch {
				AJWLib_Error2_Report("%s");
				Database_RemoveParents(addparentsperson);
				return;
			} Desk_Error2_EndCatch
			return;
		}
	}
	/*Find destination window*/
	for (i=0;i<MAXWINDOWS;i++) if (mouseblk.window==windows[i].handle) window=i;
	if (window==-1) return;
	if (dragdata->windowdata->type==wintype_NORMAL) {
		/*We are moving people/marriages*/
		int mousex;
		/*Act as if drag had ended on same win as started on*/
		if (dragdata->plotted) Windows_PlotDragBox(dragdata);
		/*Find mouse position relative to window origin and independant of current scale*/
		Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
		mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/dragdata->windowdata->scale;
		/*Move all people/marriages that were selected*/
		Windows_AddSelected(dragdata->windowdata->layout,mousex+dragdata->oldoffset-dragdata->origmousex);
		Layout_LayoutLines(dragdata->windowdata->layout);
		Windows_ResizeWindow(dragdata->windowdata);
		Desk_Window_ForceWholeRedraw(dragdata->windowdata->handle);
	} else if (dragdata->windowdata->type==wintype_UNLINKED) {
		if (windows[window].type==wintype_NORMAL) {
			/*We are adding a person to the layout*/
			AJWLib_Assert(Database_IsUnlinked(initialperson));
			if (Database_GetLinked()==none) {
				/*The layout is empty*/
				Database_LinkPerson(initialperson);
				Desk_Error2_TryCatch(Layout_AddPerson(windows[window].layout,initialperson,0,0);,AJWLib_Error2_Report("%s");)
				return;
			} else {
				/*There are already people on the layout*/
				Desk_Window_GetCoords(mouseblk.window,&blk);
				/*Find mouse position relative to window origin and independant of current scale*/
				mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/windows[window].scale;
				mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windows[window].scale;
				/*See if we were dropped on a person*/
				for (i=0;i<windows[window].layout->numpeople;i++) {
					if (mousex>=windows[window].layout->person[i].x && mousex<=windows[window].layout->person[i].x+Graphics_PersonWidth()) {
						if (mousey>=windows[window].layout->person[i].y && mousey<=windows[window].layout->person[i].y+Graphics_PersonHeight()) {
							elementptr finalperson=windows[window].layout->person[i].person;
							AJWLib_Assert(initialperson!=finalperson);
							if (Database_GetMarriage(finalperson) && Database_GetFather(finalperson)==none) {
								/*We could be adding parents or creating a second marriage, so give the user a choice*/
								addparentsperson=initialperson;
								addparentschild=finalperson;
								Desk_Window_SetExtent(addparentswin,0,304>Graphics_PersonHeight()+32 ? -304 : -Graphics_PersonHeight()-32,348+Graphics_PersonWidth(),0);
								Desk_Window_ForceWholeRedraw(addparentswin);
								Desk_Window_Show(addparentswin,Desk_open_CENTERED);
							} else {
								/*Must be a first marriage*/
								Desk_Error2_Try {
									Database_Marry(finalperson,initialperson);
									Desk_Error2_Try {
										Layout_AddPerson(windows[window].layout,initialperson,windows[window].layout->person[i].x+Graphics_PersonWidth()+Graphics_MarriageWidth(),windows[window].layout->person[i].y);
										Desk_Error2_Try {
											Layout_AddMarriage(windows[window].layout,Database_GetMarriage(finalperson),windows[window].layout->person[i].x+Graphics_PersonWidth(),windows[window].layout->person[i].y);
										} Desk_Error2_Catch {
											Layout_RemovePerson(windows[window].layout,initialperson);
											Desk_Error2_ReThrow();
										} Desk_Error2_EndCatch
									} Desk_Error2_Catch {
										Database_RemoveSpouse(initialperson);
										Desk_Error2_ReThrow();
									} Desk_Error2_EndCatch
								} Desk_Error2_Catch {
									AJWLib_Error2_Report("%s");
								} Desk_Error2_EndCatch
							}
							/*Nothing else to do*/
							return;
						}
					}
				}
				/*See if we were dropped on a marriage*/
				for (i=0;i<windows[window].layout->nummarriages;i++) {
					if (mousex>=windows[window].layout->marriage[i].x && mousex<=windows[window].layout->marriage[i].x+Graphics_MarriageWidth()) {
						if (mousey>=windows[window].layout->marriage[i].y && mousey<=windows[window].layout->marriage[i].y+Graphics_PersonHeight()) {
							/*Must be adding a child*/
							elementptr person;
							int x=-INFINITY;
							Desk_Error2_Try {
								Database_AddChild(windows[window].layout->marriage[i].marriage,initialperson);
								windows[window].layout->marriage[i].childline=Desk_TRUE;
								person=initialperson;
								do {
									int i;
									for (i=0;i<windows[window].layout->numpeople;i++) {
										if (windows[window].layout->person[i].person==person) {
											/*Find coords of furthest right hand sibling*/
											if (windows[window].layout->person[i].x>x) x=windows[window].layout->person[i].x;
										}
									}
								} while ((person=Database_GetSiblingLtoR(person))!=none);
								person=initialperson;
								while ((person=Database_GetSiblingRtoL(person))!=none) {
									int i;
									for (i=0;i<windows[window].layout->numpeople;i++) {
										if (windows[window].layout->person[i].person==person) {
											/*Find coords of furthest left hand sibling*/
											if (windows[window].layout->person[i].x>x) x=windows[window].layout->person[i].x;
										}
									}
								}
								if (x==-INFINITY) {
									/*There must have been no siblings, therefore centre under marriage*/
									x=windows[window].layout->marriage[i].x-Graphics_PersonWidth()-Graphics_GapWidth()+Graphics_MarriageWidth()/2-Graphics_PersonWidth()/2;
								}
								Desk_Error2_Try {
									Layout_AddPerson(windows[window].layout,initialperson,x+Graphics_PersonWidth()+Graphics_GapWidth(),windows[window].layout->marriage[i].y-Graphics_PersonHeight()-Graphics_GapHeightAbove()-Graphics_GapHeightBelow());
								} Desk_Error2_Catch {
									Database_RemoveChild(initialperson);
									Desk_Error2_ReThrow();
								} Desk_Error2_EndCatch
							} Desk_Error2_Catch {
								AJWLib_Error2_Report("%s");
							} Desk_Error2_EndCatch
							return;
						}
					}
				}
			}
		}
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
				dragdata->windowdata->layout->person[i].selected=(Desk_bool)!dragdata->windowdata->layout->person[i].selected;
				Windows_RedrawPerson(dragdata->windowdata,dragdata->windowdata->layout->person+i);
			}
		}
	}
	for (i=0;i<dragdata->windowdata->layout->nummarriages;i++) {
		if (mousex>dragdata->windowdata->layout->marriage[i].x && dragdata->oldmousex<dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth() || mousex<dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth() && dragdata->oldmousex>dragdata->windowdata->layout->marriage[i].x) {
			if (mousey>dragdata->windowdata->layout->marriage[i].y && dragdata->oldmousey<dragdata->windowdata->layout->marriage[i].y+Graphics_PersonHeight() || mousey<dragdata->windowdata->layout->marriage[i].y+Graphics_PersonHeight() && dragdata->oldmousey>dragdata->windowdata->layout->marriage[i].y) {
				dragdata->windowdata->layout->marriage[i].selected=(Desk_bool)!dragdata->windowdata->layout->marriage[i].selected;
				Windows_RedrawMarriage(dragdata->windowdata,dragdata->windowdata->layout->marriage+i);
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
		if (dragdata->windowdata->layout->person[i].y==dragdata->persony && !dragdata->windowdata->layout->person[i].selected) {
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
			if (dragdata->windowdata->layout->marriage[i].y==dragdata->persony && !dragdata->windowdata->layout->marriage[i].selected) {
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
	int mousex;
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
	if (mousex!=dragdata->oldmousex) {
		/*Unplot drag box if it has moved*/
		Windows_PlotDragBox(dragdata);
		dragdata->oldmousex=mousex;
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
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windowdata->scale;
	dragblk.type=Desk_drag_INVISIBLE;
	dragdata.coords=Layout_FindExtent(windowdata->layout,Desk_TRUE);
	dragdata.coords.min.x-=mousex;
	dragdata.coords.max.x-=mousex;
	dragdata.coords.min.y-=mousey;
	dragdata.coords.max.y-=mousey;
	if (blk.screenrect.min.x<0) dragblk.parent.min.x=0; else dragblk.parent.min.x=blk.screenrect.min.x;
	if (blk.screenrect.max.x>Desk_screen_size.x) dragblk.parent.max.x=Desk_screen_size.x; else dragblk.parent.max.x=blk.screenrect.max.x;
	dragblk.screenrect.min.x=0;
	dragblk.screenrect.max.x=0;
	dragblk.screenrect.min.y=0;
	dragblk.screenrect.max.y=0;
	dragblk.parent.min.y=mouseblk.pos.y;
	dragblk.parent.max.y=mouseblk.pos.y;
	dragdata.person=person;
	dragdata.persony=y;
	dragdata.personoffset=mousex-x;
	dragdata.windowdata=windowdata;
	dragdata.origmousex=mousex;
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
			if (!Layout_Selected(windowdata->layout,person1)) allsiblings=Desk_FALSE;
			if ((x=Layout_FindXCoord(windowdata->layout,person1))<leftx) leftx=x;
			if (x>rightx) rightx=x;
		}
		do {
			if (!Layout_Selected(windowdata->layout,person2)) allsiblings=Desk_FALSE;
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

static void Windows_StartDragUnlinked(elementptr person,int x,int y,windowdata *windowdata)
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
	dragblk.type=Desk_drag_FIXEDBOX;
	dragblk.screenrect.min.x=(windowdata->scale*x)/100+(blk.screenrect.min.x-blk.scroll.x);
	dragblk.screenrect.max.x=(windowdata->scale*(x+Graphics_PersonWidth()))/100+(blk.screenrect.min.x-blk.scroll.x);
	dragblk.screenrect.min.y=(windowdata->scale*y)/100+(blk.screenrect.max.y-blk.scroll.y);
	dragblk.screenrect.max.y=(windowdata->scale*(y+Graphics_PersonHeight()))/100+(blk.screenrect.max.y-blk.scroll.y);
	dragblk.parent.min.y=dragblk.screenrect.min.y-mouseblk.pos.y;
	dragblk.parent.max.y=Desk_screen_size.y+dragblk.screenrect.max.y-mouseblk.pos.y;
	dragblk.parent.min.x=dragblk.screenrect.min.x-mouseblk.pos.x;
	dragblk.parent.max.x=Desk_screen_size.x+dragblk.screenrect.max.x-mouseblk.pos.x;
	dragdata.person=person;
	dragdata.persony=y;
	dragdata.personoffset=0;
	dragdata.windowdata=windowdata;
	dragdata.origmousex=mousex;
	dragdata.oldmousex=mousex;
	dragdata.oldmousey=mousey;
	dragdata.oldoffset=0;
	dragdata.plotted=Desk_FALSE;
	Desk_Wimp_DragBox(&dragblk);
	Desk_Drag_SetHandlers(NULL,Windows_DragEnd,&dragdata);
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

static Desk_bool Windows_MouseClick(Desk_event_pollblock *block,void *ref)
{
	windowdata *windowdata=ref;
	int mousex,mousey,i;
	Desk_convert_block blk;
	Desk_Window_GetCoords(windowdata->handle,&blk);
	mousex=((block->data.mouse.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/windowdata->scale;
	mousey=((block->data.mouse.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windowdata->scale;
	menuoverperson=none;
	menuoverwindow=windowdata;
	AJWLib_Menu_Shade(personmenu,personmenu_ADD);
	AJWLib_Menu_Shade(personmenu,personmenu_DELETE);
	AJWLib_Menu_Shade(personmenu,personmenu_UNLINK);
	AJWLib_Menu_Shade(mainmenu,mainmenu_PERSON);
	AJWLib_Menu_Shade(mainmenu,mainmenu_SELECT);
	/*See if we clicked on a person*/
	for (i=windowdata->layout->numpeople-1;i>=0;i--) {
		if (mousex>=windowdata->layout->person[i].x && mousex<=windowdata->layout->person[i].x+Graphics_PersonWidth()) {
			if (mousey>=windowdata->layout->person[i].y && mousey<=windowdata->layout->person[i].y+Graphics_PersonHeight()) {
				if (block->data.mouse.button.data.clickselect) {
					if (windowdata->type==wintype_NORMAL) {
						if (!windowdata->layout->person[i].selected) {
							Windows_UnselectAll(windowdata);
							windowdata->layout->person[i].selected=Desk_TRUE;
							Windows_RedrawPerson(windowdata,windowdata->layout->person+i);
						}
						return Desk_TRUE;
					}
				} else if (block->data.mouse.button.data.clickadjust) {
					if (windowdata->type==wintype_NORMAL) {
						windowdata->layout->person[i].selected=(Desk_bool)!windowdata->layout->person[i].selected;
						Windows_RedrawPerson(windowdata,windowdata->layout->person+i);
						return Desk_TRUE;
					}
				} else if (block->data.mouse.button.data.menu) {
					elementptr marriage;
					menuoverperson=windowdata->layout->person[i].person;
					if (windowdata->type==wintype_NORMAL) {
						AJWLib_Menu_UnShade(mainmenu,mainmenu_PERSON);
						AJWLib_Menu_UnShade(mainmenu,mainmenu_SELECT);
						if ((marriage=Database_GetMarriage(menuoverperson))==none) {
							AJWLib_Menu_UnShade(personmenu,personmenu_UNLINK);
						} else if (Database_GetMother(menuoverperson)) {
							/*no*/
						} else if (Database_GetLeftChild(marriage)) {
							/*maybe*/
							elementptr principal,spouse;
							principal=Database_GetPrincipalFromMarriage(marriage);
							spouse=Database_GetSpouseFromMarriage(marriage);
							if (Database_GetMarriageLtoR(Database_GetMarriageLtoR(principal))==none) {
								if (Database_GetMother(principal)==none && Database_GetMother(spouse)==none) {
									if (Database_GetSiblingLtoR(Database_GetLeftChild(marriage))==none) {
										AJWLib_Menu_UnShade(personmenu,personmenu_UNLINK);
									}
								}
							}
						} else if (Database_GetPrincipalFromMarriage(marriage)!=menuoverperson) {
							AJWLib_Menu_UnShade(personmenu,personmenu_UNLINK);
						} else if (Database_GetMarriageLtoR(Database_GetMarriageLtoR(menuoverperson))==none) {
							AJWLib_Menu_UnShade(personmenu,personmenu_UNLINK);
						}
					} else if (windowdata->type==wintype_UNLINKED) {
						AJWLib_Menu_UnShade(personmenu,personmenu_DELETE);
					}
					if (windowdata->type==wintype_NORMAL) AJWLib_Menu_UnShade(mainmenu,mainmenu_SELECT);
				} else if (block->data.mouse.button.data.select) {
					Windows_UnselectAll(windowdata);
					Database_EditPerson(windowdata->layout->person[i].person);
					return Desk_TRUE;
				} else if (block->data.mouse.button.data.adjust) {
					if (windowdata->type==wintype_DESCENDENTS || windowdata->type==wintype_ANCESTORS) {
						windowdata->person=windowdata->layout->person[i].person;
						Windows_Relayout(); /*Rather inefficient*/
					}
					return Desk_TRUE;
				} else if (block->data.mouse.button.data.dragselect) {
					if (windowdata->type==wintype_NORMAL) {
						Windows_StartDragNormal(windowdata->layout->person[i].person,windowdata->layout->person[i].x,windowdata->layout->person[i].y,windowdata,Desk_FALSE);
						return Desk_TRUE;
					} else if (windowdata->type==wintype_UNLINKED) {
						Windows_StartDragUnlinked(windowdata->layout->person[i].person,windowdata->layout->person[i].x,windowdata->layout->person[i].y,windowdata);
						return Desk_TRUE;
					}
				}
			}
		}
	}
	/*Has the menu button been clicked?*/
	if (block->data.mouse.button.data.menu) {
		char buffer[20];
		if (windowdata->type==wintype_UNLINKED) {
			AJWLib_Menu_UnShade(mainmenu,mainmenu_PERSON);
			AJWLib_Menu_UnShade(personmenu,personmenu_ADD);
		}
		Desk_Icon_SetText(savewin,save_FILENAME,File_GetFilename());
		Desk_Icon_SetText(savedrawwin,save_FILENAME,AJWLib_Msgs_TempLookup("File.Draw:Drawfile"));
		sprintf(buffer,"%d",Database_GetNumPeople());
		Desk_Icon_SetText(fileinfowin,info_PEOPLE,buffer);
		Desk_Icon_SetText(fileinfowin,info_FILENAME,File_GetFilename());
		Desk_Icon_SetText(fileinfowin,info_MODIFIED,File_GetModified() ? AJWLib_Msgs_TempLookup("Mod.Yes:Yes") : AJWLib_Msgs_TempLookup("Mod.No:No"));
		Desk_Error2_CheckOS(Desk_SWI(3,0,Desk_SWI_OS_ConvertFileSize,File_GetSize(),buffer,sizeof(buffer)));
		Desk_Icon_SetText(fileinfowin,info_SIZE,buffer);
		Desk_Icon_SetText(fileinfowin,info_DATE,File_GetDate());
		Desk_Icon_SetInteger(scalewin,scale_TEXT,windowdata->scale);
		Desk_Menu_Show(mainmenu,block->data.mouse.pos.x,block->data.mouse.pos.y);
		return Desk_TRUE;
	}
	/*We are not on a person and Menu has not been clicked*/
	for (i=windowdata->layout->nummarriages-1;i>=0;i--) {
		if (mousex>=windowdata->layout->marriage[i].x && mousex<=windowdata->layout->marriage[i].x+Graphics_MarriageWidth()) {
			if (mousey>=windowdata->layout->marriage[i].y && mousey<=windowdata->layout->marriage[i].y+Graphics_PersonHeight()) {
				if (block->data.mouse.button.data.select) {
					Windows_UnselectAll(windowdata);
					Database_EditMarriage(windowdata->layout->marriage[i].marriage);
					return Desk_TRUE;
				} else if (block->data.mouse.button.data.clickselect) {
					if (windowdata->type==wintype_NORMAL) {
						if (!windowdata->layout->marriage[i].selected) {
							Windows_UnselectAll(windowdata);
							windowdata->layout->marriage[i].selected=Desk_TRUE;
							Windows_RedrawMarriage(windowdata,windowdata->layout->marriage+i);
						}
						return Desk_TRUE;
					}
				} else if (block->data.mouse.button.data.clickadjust) {
					if (windowdata->type==wintype_NORMAL) {
						windowdata->layout->marriage[i].selected=(Desk_bool)!windowdata->layout->marriage[i].selected;
						Windows_RedrawMarriage(windowdata,windowdata->layout->marriage+i);
						return Desk_TRUE;
					}
				} else if (block->data.mouse.button.data.dragselect) {
					if (windowdata->type==wintype_NORMAL) {
						Windows_StartDragNormal(windowdata->layout->marriage[i].marriage,windowdata->layout->marriage[i].x,windowdata->layout->marriage[i].y,windowdata,Desk_TRUE);
						return Desk_TRUE;
					}
				}
			}
		}
	}
	/*Menu hasn't been clicked, and we are not on a person or marriage*/
	if (windowdata->type==wintype_NORMAL) {
		if (block->data.mouse.button.data.clickselect) {
			Windows_UnselectAll(windowdata);
			return Desk_TRUE;
		} else if (block->data.mouse.button.data.dragselect) {
			Windows_UnselectAll(windowdata);
			Windows_StartDragSelect(windowdata);
			return Desk_TRUE;
		} else if (block->data.mouse.button.data.dragadjust) {
			Windows_StartDragSelect(windowdata);
			return Desk_TRUE;
		} else if (block->data.mouse.button.data.select) {
			if (mousey>windowdata->layout->title.y-Graphics_TitleHeight()/2) {
				Windows_UnselectAll(windowdata);
				Database_EditTitle();
			}
		}
	}
	return Desk_FALSE;
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
					case wintype_UNLINKED:
						Layout_Free(windows[i].layout);
						windows[i].layout=NULL;
						windows[i].layout=Layout_LayoutUnlinked();
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

void Windows_CloseAllWindows(void)
/*Tidy up after an error*/
{
	int i;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle) {
			Desk_Error2_TryCatch(if (windows[i].layout) Layout_Free(windows[i].layout);,)
			Desk_Error2_TryCatch(Desk_Window_Delete(windows[i].handle);,)
			windows[i].handle=NULL;
		}
	}
}

static Desk_bool Windows_CloseWindow(Desk_event_pollblock *block,windowdata *windowdata)
{
	int i,found=0;
	for (i=0;i<MAXWINDOWS;i++)
		if (windows[i].handle!=0)
			if (windows[i].type==wintype_NORMAL || windows[i].type==wintype_CLOSERELATIVES) found++;
	AJWLib_Assert(found>0);
	switch (windowdata->type) {
		case wintype_NORMAL:
			if (found<=1) {
				/*Warn - unsaved data*/
				/*Close all other windows*/
				for (i=0;i<MAXWINDOWS;i++) if (windows[i].handle!=0) {
					if (windows[i].layout!=NULL) Layout_Free(windows[i].layout);
					Desk_Window_Delete(windows[i].handle);
					windows[i].handle=0;
				}
				File_Remove();
			} else {
				Desk_Window_Delete(windowdata->handle);
				windowdata->handle=0;
			}
			break;
		case wintype_CLOSERELATIVES:
			/*d*/
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

void Windows_LayoutNormal(layout *layout)
{
	int i;
	if (layout==NULL) Desk_Error2_TryCatch(layout=Layout_LayoutNormal();,AJWLib_Error2_Report("%s");)
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle && windows[i].type==wintype_NORMAL) {
			AJWLib_Assert(windows[i].layout==NULL);
			windows[i].layout=layout;
			Windows_ResizeWindow(&windows[i]);
		}
	}
}

void Windows_OpenWindow(wintype type,elementptr person,int generations,int scale,Desk_convert_block *coords)
{
	/*Let any Error2s be habdled by the calling function*/
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
	if (coords==NULL) {
		Desk_Window_Show(windows[newwindow].handle,Desk_open_CENTERED);
	} else {
		Desk_window_openblock blk;
		blk.window=windows[newwindow].handle;
		blk.screenrect=coords->screenrect;
		blk.scroll=coords->scroll;
		blk.behind=-1;
		Desk_Wimp_OpenWindow(&blk);
	}
	switch (type) {
		case wintype_NORMAL:
			Desk_Window_SetTitle(windows[newwindow].handle,File_GetFilename());
#if DEBUG
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
#if DEBUG
			Desk_Event_Claim(Desk_event_REDRAW,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Windows_RedrawWindow,&windows[newwindow]);
			windows[newwindow].layout=debuglayout;
#endif
			windows[newwindow].layout=Layout_LayoutAncestors(person,generations);
			break;
		case wintype_UNLINKED:
			Desk_Msgs_Lookup("Win.Unlkd:Unlinked",str,255);
			Desk_Window_SetTitle(windows[newwindow].handle,str);
			windows[newwindow].layout=NULL;
			windows[newwindow].layout=Layout_LayoutUnlinked();
			break;
		default:
			windows[newwindow].layout=NULL;
			AJWLib_Assert(0);
	}
	if (type!=wintype_NORMAL) Windows_ResizeWindow(&windows[newwindow]);
	Desk_Event_Claim(Desk_event_CLICK,windows[newwindow].handle,Desk_event_ANY,Windows_MouseClick,&windows[newwindow]);
	Desk_Event_Claim(Desk_event_REDRAW,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Windows_RedrawWindow,&windows[newwindow]);
	Desk_Event_Claim(Desk_event_CLOSE,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Windows_CloseWindow,&windows[newwindow]);
}

static void Windows_MainMenuClick(int entry,void *ref)
{
	char buffer[10]="";
	switch (entry) {
		case mainmenu_NEWVIEW:
			if (menuoverperson==none) {
				Desk_Icon_Shade(newviewwin,newview_ANCESTOR);
				Desk_Icon_Shade(newviewwin,newview_ANCESTORPERSON);
				Desk_Icon_Shade(newviewwin,newview_DESCENDENT);
				Desk_Icon_Shade(newviewwin,newview_DESCENDENTPERSON);
				Desk_Icon_Shade(newviewwin,newview_CLOSERELATIVES);
				Desk_Icon_Shade(newviewwin,newview_CLOSERELATIVESPERSON);
			} else {
/*				Desk_Icon_Unshade(newviewwin,newview_ANCESTOR);
				Desk_Icon_Unshade(newviewwin,newview_ANCESTORPERSON);
*/				Desk_Icon_Unshade(newviewwin,newview_DESCENDENT);
				Desk_Icon_Unshade(newviewwin,newview_DESCENDENTPERSON);
/*				Desk_Icon_Unshade(newviewwin,newview_CLOSERELATIVES);
				Desk_Icon_Unshade(newviewwin,newview_CLOSERELATIVESPERSON);
*/				strcpy(buffer,Database_GetName(menuoverperson));
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
			newviewperson=menuoverperson;
			break;
		case mainmenu_SCALE:
			Desk_Window_Show(scalewin,Desk_open_CENTERED);
			Desk_Icon_SetCaret(scalewin,scale_TEXT);
			break;
	}
}

static Desk_bool Windows_Cancel(Desk_event_pollblock *block,void *ref)
{
	if (block->data.mouse.button.data.select) {
		Desk_Window_Hide(block->data.mouse.window);
		return Desk_TRUE;
	}
	return Desk_FALSE;
}

static Desk_bool Windows_NewViewOk(Desk_event_pollblock *block,void *ref)
{
	wintype wintype;
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	switch (Desk_Icon_WhichRadio(newviewwin,newview_NORMAL,newview_DESCENDENT)) {
		case newview_NORMAL:
			wintype=wintype_NORMAL;
			break;
		case newview_UNLINKED:
			wintype=wintype_UNLINKED;
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
	}
	if (block->data.mouse.button.data.select) Desk_Window_Hide(newviewwin);
	Desk_Error2_TryCatch(Windows_OpenWindow(wintype,newviewperson,atoi(Desk_Icon_GetTextPtr(newviewwin,newview_GENERATIONS)),100,NULL);,AJWLib_Error2_Report("%s");)
	return Desk_TRUE;
}

static Desk_bool Windows_SaveDraw(char *filename,void *ref)
{
	Drawfile_Save(filename,menuoverwindow->layout);
	return Desk_TRUE;
}

static void Windows_PersonMenuClick(int entry,void *ref)
{
	elementptr mother,marriage,mothermarriage;
	Desk_Error2_Try {
		switch (entry) {
			case personmenu_ADD:
				Database_Add();
				break;
			case personmenu_DELETE:
				Database_Delete(menuoverperson);
				AJWLib_Menu_Shade(personmenu,entry);
				break;
			case personmenu_UNLINK:
				AJWLib_Menu_Shade(personmenu,entry);
				mother=Database_GetMother(menuoverperson);
				marriage=Database_GetMarriage(menuoverperson);
				mothermarriage=Database_GetMarriage(mother);
				if (marriage==none) {
					Layout_RemovePerson(menuoverwindow->layout,menuoverperson);
					Database_RemoveChild(menuoverperson);
					if (Database_GetLeftChild(mothermarriage)==none) Layout_AlterMarriageChildline(menuoverwindow->layout,mothermarriage,Desk_FALSE);
				} else if (mother==none) {
					if (Database_GetLeftChild(marriage)) {
						Layout_RemovePerson(menuoverwindow->layout,Database_GetPrincipalFromMarriage(marriage));
						Layout_RemovePerson(menuoverwindow->layout,Database_GetSpouseFromMarriage(marriage));
						Layout_RemoveMarriage(menuoverwindow->layout,marriage);
						Layout_AlterChildline(menuoverwindow->layout,Database_GetLeftChild(marriage),Desk_FALSE);
						Database_RemoveParents(menuoverperson);
					} else {
						if (Database_GetMarriageRtoL(menuoverperson)!=none || Database_GetMarriageLtoR(Database_GetMarriageLtoR(menuoverperson))==none) {
							Layout_RemovePerson(menuoverwindow->layout,menuoverperson);
							Layout_RemoveMarriage(menuoverwindow->layout,marriage);
							Database_RemoveSpouse(menuoverperson);
						}
					}
				} else {
					/*cannot unlink*/
				}
				break;
		}
	} Desk_Error2_Catch {
		AJWLib_Error2_Report("%s");
	} Desk_Error2_EndCatch
}

static void Windows_SelectMenuClick(int entry,void *ref)
{
	switch (entry) {
		case selectmenu_DESCENDENTS:
			Layout_SelectDescendents(menuoverwindow->layout,menuoverperson);
			Desk_Window_ForceWholeRedraw(menuoverwindow->handle);
			break;
		case selectmenu_ANCESTORS:
			Layout_SelectAncestors(menuoverwindow->layout,menuoverperson);
			Desk_Window_ForceWholeRedraw(menuoverwindow->handle);
			break;
		case selectmenu_SIBLINGS:
			Layout_SelectSiblings(menuoverwindow->layout,menuoverperson);
			Desk_Window_ForceWholeRedraw(menuoverwindow->handle);
			break;
		case selectmenu_SPOUSES:
			Layout_SelectSpouses(menuoverwindow->layout,menuoverperson);
			Desk_Window_ForceWholeRedraw(menuoverwindow->handle);
			break;
	}
}

static Desk_bool Windows_NewViewClick(Desk_event_pollblock *block,void *ref)
{
	switch (block->data.mouse.icon) {
		case newview_UNLINKED:
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
	outlineblk.window=menuoverwindow->handle;
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
			bbox=Layout_FindExtent(menuoverwindow->layout,Desk_FALSE);
			Desk_Wimp_GetWindowState(menuoverwindow->handle,&stateblk);
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
			menuoverwindow->scale=scale;
			Windows_ResizeWindow(menuoverwindow);
			Desk_Window_ForceWholeRedraw(menuoverwindow->handle);
			if (block->data.mouse.button.data.select) Desk_Window_Hide(scalewin);
			break;
	}
	return Desk_FALSE;
}

void Windows_CloseAddParentsWindow(void)
{
	Desk_Window_Hide(addparentswin);
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
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_UNLINKED);
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_ANCESTOR);
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_DESCENDENT);
	Desk_Event_Claim(Desk_event_CLICK,newviewwin,Desk_event_ANY,Windows_NewViewClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,newviewwin,newview_OK,Windows_NewViewOk,NULL);
	Desk_Event_Claim(Desk_event_CLICK,newviewwin,newview_CANCEL,Windows_Cancel,NULL);
	addparentswin=Desk_Window_Create("AddParents",Desk_template_TITLEMIN);
	fileinfowin=Desk_Window_Create("FileInfo",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_REDRAW,addparentswin,Desk_event_ANY,Windows_RedrawAddParents,NULL);
	Desk_Event_Claim(Desk_event_CLICK,addparentswin,addparents_SECONDMARRIAGE,Windows_AddParentsClick,NULL);
	exportmenu=AJWLib_Menu_CreateFromMsgs("Title.Export:","Menu.Export:",NULL,NULL);
	filemenu=AJWLib_Menu_CreateFromMsgs("Title.File:","Menu.File:",NULL,NULL);
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
	scalewin=Desk_Window_Create("Scale",Desk_template_TITLEMIN);
	Desk_Icon_InitIncDecHandler(scalewin,scale_TEXT,scale_UP,scale_DOWN,Desk_FALSE,1,1,999,100);
	Desk_Event_Claim(Desk_event_CLICK,scalewin,Desk_event_ANY,Windows_ScaleClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,scalewin,scale_CANCEL,Windows_Cancel,NULL);
	savewin=Desk_Window_Create("Save",Desk_template_TITLEMIN);
	Desk_Menu_AddSubMenu(filemenu,filemenu_SAVE,(Desk_menu_ptr)savewin);
	Desk_Save_InitSaveWindowHandler(savewin,Desk_TRUE,Desk_TRUE,Desk_FALSE,save_ICON,save_OK,save_CANCEL,save_FILENAME,File_SaveFile,NULL,File_Result,1024*10/*Filesize estimate?*/,0x090/*Filetype*/,NULL);
	savedrawwin=Desk_Window_Create("Save",Desk_template_TITLEMIN);
	Desk_Menu_AddSubMenu(exportmenu,exportmenu_DRAW,(Desk_menu_ptr)savedrawwin);
	Desk_Save_InitSaveWindowHandler(savedrawwin,Desk_TRUE,Desk_TRUE,Desk_FALSE,save_ICON,save_OK,save_CANCEL,save_FILENAME,Windows_SaveDraw,NULL,NULL/*Need a result handler?*/,1024*10/*Filesize estimate?*/,Desk_filetype_DRAWFILE,NULL);
}

