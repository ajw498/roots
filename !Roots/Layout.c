/*
	Roots - Layout routines
	© Alex Waugh 1999

	$Id: Layout.c,v 1.44 2000/10/14 16:32:08 AJW Exp $

*/

#include "Desk.Core.h"
#include "Desk.Error2.h"
#include "Desk.Window.h"
#include "Desk.Event.h"
#include "Desk.DeskMem.h"

#include "AJWLib.Flex.h"
#include "AJWLib.File.h"
#include "AJWLib.Assert.h"

#include <stdlib.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "Modules.h"
#include "Windows.h"
#include "Layout.h"
#include "File.h"
#include "Config.h"

void Layout_Select(elementptr person)
{
	Database_SetFlag(person);
}

void Layout_DeSelect(elementptr person)
{
	Database_UnsetFlag(person);
}

void Layout_DeSelectAll(void)
{
	Database_UnsetAllFlags();
}

Desk_bool Layout_GetSelect(elementptr person)
{
	return Database_GetFlag(person);
}

elementtype Layout_AnyoneSelected(void)
{
	return Database_AnyoneFlagged();
}

int Layout_NearestGeneration(int y)
{
	int neg;
	int h=(Graphics_PersonHeight()+Graphics_GapHeightBelow()+Graphics_GapHeightAbove());
	neg=y<0 ? -1 : 1;
	y*=neg;
	if (neg>0) {
		if (y%h>Graphics_GapHeightAbove()+Graphics_PersonHeight()) y+=h;
	} else {
		if (y%h>Graphics_GapHeightBelow()) y+=h;
	}
	y-=y%h;
	return y*neg;
}

void Layout_AddPerson(layout *layout,elementptr person,int x,int y)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	AJWLib_Flex_Extend((flex_ptr)&(layout->person),sizeof(elementlayout)*(layout->numpeople+1));
	layout->person[layout->numpeople].x=x;
	layout->person[layout->numpeople].y=y;
	layout->person[layout->numpeople].element=person;
	layout->numpeople++;
	Layout_DeSelect(person);
	Modules_ChangedLayout();
}

void Layout_AddMarriage(layout *layout,elementptr marriage,int x,int y)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(marriage!=none);
	AJWLib_Flex_Extend((flex_ptr)&(layout->marriage),sizeof(elementlayout)*(layout->nummarriages+1));
	layout->marriage[layout->nummarriages].x=x;
	layout->marriage[layout->nummarriages].y=y;
	layout->marriage[layout->nummarriages].element=marriage;
	Layout_DeSelect(marriage);
	layout->nummarriages++;
	Modules_ChangedLayout();
}

int Layout_FindXCoord(layout *layout,elementptr person)
{
	int i;

	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);

	for (i=0;i<layout->numpeople;i++) if (layout->person[i].element==person) return layout->person[i].x;
	AJWLib_AssertWarning(0);
	return 0;
}

int Layout_FindYCoord(layout *layout,elementptr person)
{
	int i;

	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);

	for (i=0;i<layout->numpeople;i++) if (layout->person[i].element==person) return layout->person[i].y;
	AJWLib_AssertWarning(0);
	return 0;
}

int Layout_FindMarriageXCoord(layout *layout,elementptr marriage)
{
	int i;

	AJWLib_Assert(layout!=NULL);

	for (i=0;i<layout->nummarriages;i++) if (layout->marriage[i].element==marriage) return layout->marriage[i].x;
	return 0;
}

int Layout_FindMarriageYCoord(layout *layout,elementptr marriage)
{
	int i;

	AJWLib_Assert(layout!=NULL);

	for (i=0;i<layout->nummarriages;i++) if (layout->marriage[i].element==marriage) return layout->marriage[i].y;
	AJWLib_AssertWarning(0);
	return 0;
}

void Layout_RemovePerson(layout *layout,elementptr person)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].element==person) {
			AJWLib_Flex_MidExtend((flex_ptr)&(layout->person),sizeof(elementlayout)*(i+1),-sizeof(elementlayout));
			layout->numpeople--;
			Modules_ChangedLayout();
			return;
		}
	}
}

void Layout_RemoveMarriage(layout *layout,elementptr marriage)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(marriage!=none);
	for (i=0;i<layout->nummarriages;i++) {
		if (layout->marriage[i].element==marriage) {
			AJWLib_Flex_MidExtend((flex_ptr)&(layout->marriage),sizeof(elementlayout)*(i+1),-sizeof(elementlayout));
			layout->nummarriages--;
			Modules_ChangedLayout();
			return;
		}
	}
}

Desk_wimp_rect Layout_FindExtent(layout *layout,Desk_bool selection)
{
	Desk_wimp_rect box;
	int i;
	box.min.x=INFINITY;
	box.min.y=INFINITY;
	box.max.x=-INFINITY;
	box.max.y=-INFINITY;
	if (layout) {
		for (i=0;i<layout->numpeople;i++) {
			if (Layout_GetSelect(layout->person[i].element) || !selection) {
				if (layout->person[i].x<box.min.x) box.min.x=layout->person[i].x;
				if (layout->person[i].x+Graphics_PersonWidth()>box.max.x) box.max.x=layout->person[i].x+Graphics_PersonWidth();
				if (layout->person[i].y<box.min.y) box.min.y=layout->person[i].y;
				if (layout->person[i].y>box.max.y) box.max.y=layout->person[i].y;
			}
		}
		for (i=0;i<layout->nummarriages;i++) {
			if (Layout_GetSelect(layout->marriage[i].element) || !selection) {
				if (layout->marriage[i].x<box.min.x) box.min.x=layout->marriage[i].x;
				if (layout->marriage[i].x+Graphics_MarriageWidth()>box.max.x) box.max.x=layout->marriage[i].x+Graphics_MarriageWidth();
				if (layout->marriage[i].y<box.min.y) box.min.y=layout->marriage[i].y;
				if (layout->marriage[i].y>box.max.y) box.max.y=layout->marriage[i].y;
			}
		}
	}
	if (box.min.x==INFINITY) {
		box.min.x=0;
		box.min.y=0;
		box.max.x=Graphics_PersonWidth();
		box.max.y=Graphics_PersonHeight();
	} else {
		box.max.y+=Graphics_PersonHeight();
	}
	return box;
}

layout *Layout_New(void)
/* Create a new empty layout*/
{
	layout *layout;

	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
	layout->numpeople=0;
	layout->nummarriages=0;
	layout->numchildren=0;
	AJWLib_Flex_Alloc((flex_ptr)&(layout->person),1);
	AJWLib_Flex_Alloc((flex_ptr)&(layout->marriage),1);
	AJWLib_Flex_Alloc((flex_ptr)&(layout->children),1);
	return layout;
}

void Layout_Free(layout *layout)
{
	AJWLib_AssertWarning(layout!=NULL);
	if (layout==NULL) return;
	AJWLib_Flex_Free((flex_ptr)&(layout->person));
	AJWLib_Flex_Free((flex_ptr)&(layout->marriage));
	AJWLib_Flex_Free((flex_ptr)&(layout->children));
	Desk_DeskMem_Free(layout);
}
/*
	Roots - Layout related windows
	© Alex Waugh 1999

	$Id: Layout.c,v 1.44 2000/10/14 16:32:08 AJW Exp $

*/

#include "Desk.Window.h"
#include "Desk.Error.h"
#include "Desk.Error2.h"
#include "Desk.SWI.h"
#include "Desk.WimpSWIs.h"
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
#include "Desk.Kbd.h"
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

#include "Main.h"
#include "Graphics.h"
#include "Modules.h"
#include "Windows.h"
#include "Config.h"
#include "Layout.h"
#include "TreeLayout.h"
#include "Database.h"
#include "Drawfile.h"
#include "Draw.h"
#include "File.h"
#include "Print.h"

typedef enum dragtype {
	drag_MOVE,
	drag_UNLINK,
	drag_LINK
} dragtype;

typedef enum ptrtype {
	ptr_DEFAULT,
	ptr_LINK,
	ptr_NOLINK,
	ptr_UNLINK
} ptrtype;

#define REDRAWOVERLAP 4

#define SWI_OS_SpriteOp 0x2E
#define SWI_Wimp_SpriteOp 0x400E9
#define SWI_Wimp_DragBox 0x400D0

typedef struct dragdata {
	elementptr person;
	int origmousey;
	int personoffset;
	windowdata *windowdata;
	Desk_wimp_rect  coords;
	int origmousex,oldmousex,oldoffset,oldmousey,centered;
	Desk_bool plotted,marriage;
	dragtype type;
	ptrtype ptr;
} dragdata;

mouseclickdata mousedata;

static Desk_sprite_area ptrsprites;

static void Layout_SetPointerShape(char *name,int num)
/* Set the pointer shape*/
{
	if (num==1) {
		Desk_Error2_CheckOS(Desk_SWI(8,0,SWI_Wimp_SpriteOp,36,NULL,name,num | 0x20,0,0,0,NULL));
	} else {
		Desk_Error2_CheckOS(Desk_SWI(8,0,SWI_OS_SpriteOp,36+256,ptrsprites,name,num | 0x20,8,8,0,NULL));
	}
}

static void Layout_RedrawPerson(windowdata *windowdata,elementlayout *person)
{
	Desk_Window_ForceRedraw(windowdata->handle,(windowdata->scale*person->x)/100-REDRAWOVERLAP,(windowdata->scale*person->y)/100-REDRAWOVERLAP,(windowdata->scale*(person->x+Graphics_PersonWidth()))/100+REDRAWOVERLAP,(windowdata->scale*(person->y+Graphics_PersonHeight()))/100+REDRAWOVERLAP);
}

static void Layout_RedrawMarriage(windowdata *windowdata,elementlayout *marriage)
{
	Desk_Window_ForceRedraw(windowdata->handle,(windowdata->scale*marriage->x)/100-REDRAWOVERLAP,(windowdata->scale*marriage->y)/100-REDRAWOVERLAP,(windowdata->scale*(marriage->x+Graphics_MarriageWidth()))/100+REDRAWOVERLAP,(windowdata->scale*(marriage->y+Graphics_PersonHeight()))/100+REDRAWOVERLAP);
}

Desk_bool Layout_RedrawWindow(Desk_event_pollblock *block,windowdata *windowdata)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=block->data.openblock.window;
	Desk_Wimp_RedrawWindow(&blk,&more);
	while (more) {
		Graphics_Redraw(windowdata->layout,windowdata->scale,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,&(blk.cliprect),Desk_TRUE,Draw_PlotLine,Draw_PlotRectangle,Draw_PlotRectangleFilled,Draw_PlotText);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
	return Desk_TRUE;
}

void Windows_UnselectAll(windowdata *windowdata)
{
	int i;
	AJWLib_Assert(windowdata!=NULL);
	for (i=0;i<windowdata->layout->numpeople;i++) {
		if (Layout_GetSelect(windowdata->layout->person[i].element)) {
			Layout_DeSelect(windowdata->layout->person[i].element);
			Layout_RedrawPerson(windowdata,windowdata->layout->person+i);
		}
	}
	for (i=0;i<windowdata->layout->nummarriages;i++) {
		if (Layout_GetSelect(windowdata->layout->marriage[i].element)) {
			Layout_DeSelect(windowdata->layout->marriage[i].element);
			Layout_RedrawMarriage(windowdata,windowdata->layout->marriage+i);
		}
	}
}

static void Windows_AddSelected(layout *layout,int amountx,int amounty)
{
	int i;
	for (i=0;i<layout->numpeople;i++) {
		if (Layout_GetSelect(layout->person[i].element)) {
			layout->person[i].x+=amountx;
			layout->person[i].y+=amounty;
		}
	}
	for (i=0;i<layout->nummarriages;i++) {
		if (Layout_GetSelect(layout->marriage[i].element)) {
			layout->marriage[i].x+=amountx;
			layout->marriage[i].y+=amounty;
		}
	}
	Modules_ChangedLayout();
}

void Windows_ResizeWindow(windowdata *windowdata)
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

	Layout_SetPointerShape("ptr_DEFAULT",1);
	Desk_Wimp_GetPointerInfo(&mouseblk);
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
		if ((mousex>dragdata->windowdata->layout->person[i].x && dragdata->origmousex<dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth()) || (mousex<dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth() && dragdata->origmousex>dragdata->windowdata->layout->person[i].x)) {
			if ((mousey>dragdata->windowdata->layout->person[i].y && dragdata->origmousey<dragdata->windowdata->layout->person[i].y+Graphics_PersonHeight()) || (mousey<dragdata->windowdata->layout->person[i].y+Graphics_PersonHeight() && dragdata->origmousey>dragdata->windowdata->layout->person[i].y)) {
				if (Layout_GetSelect(dragdata->windowdata->layout->person[i].element)) {
					Layout_DeSelect(dragdata->windowdata->layout->person[i].element);
				} else {
					Layout_Select(dragdata->windowdata->layout->person[i].element);
				}
				Layout_RedrawPerson(dragdata->windowdata,dragdata->windowdata->layout->person+i);
			}
		}
	}
	for (i=0;i<dragdata->windowdata->layout->nummarriages;i++) {
		if ((mousex>dragdata->windowdata->layout->marriage[i].x && dragdata->origmousex<dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth()) || (mousex<dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth() && dragdata->origmousex>dragdata->windowdata->layout->marriage[i].x)) {
			if ((mousey>dragdata->windowdata->layout->marriage[i].y && dragdata->origmousey<dragdata->windowdata->layout->marriage[i].y+Graphics_PersonHeight()) || (mousey<dragdata->windowdata->layout->marriage[i].y+Graphics_PersonHeight() && dragdata->origmousey>dragdata->windowdata->layout->marriage[i].y)) {
				if (Layout_GetSelect(dragdata->windowdata->layout->marriage[i].element)) {
					Layout_DeSelect(dragdata->windowdata->layout->marriage[i].element);
				} else {
					Layout_Select(dragdata->windowdata->layout->marriage[i].element);
				}
				Layout_RedrawMarriage(dragdata->windowdata,dragdata->windowdata->layout->marriage+i);
			}
		}
	}
}

static void Windows_LinkValid(void *ref,elementptr *person,elementptr *marriage)
/* Check if the link drag would produce a valid link*/
{
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	int mousex,mousey,i;

	*person=none;
	*marriage=none;
	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/dragdata->windowdata->scale;
	/*See who we were dropped on*/
	for (i=dragdata->windowdata->layout->numpeople-1;i>=0;i--) {
		if (mousex>dragdata->windowdata->layout->person[i].x && mousex<dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth()) {
			if (mousey>dragdata->windowdata->layout->person[i].y && mousey<dragdata->windowdata->layout->person[i].y+Graphics_PersonHeight()) {
				/*Check that the dragged person is not the same as the destination person*/
				if (dragdata->person==dragdata->windowdata->layout->person[i].element) return;
				/*Check that both people are in the same generation*/
				if (dragdata->origmousey!=Layout_NearestGeneration(mousey)) return;
				*person=dragdata->windowdata->layout->person[i].element;
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
				*marriage=dragdata->windowdata->layout->marriage[i].element;
				return;
			}
		}
	}
}

static void Windows_LinkDragEnd(void *ref)
/*A link drag has ended, so link the person if possible*/
{
	dragdata *dragdata=ref;
	elementptr person,marriage;

	Layout_SetPointerShape("ptr_default",1);
	Windows_LinkValid(ref,&person,&marriage);
	if (person) {
		Desk_Error2_Try {
			volatile elementptr marriage;
			marriage=Database_Marry(person,dragdata->person);
			Desk_Error2_Try {
				int startx,finishx,marriagex;
				/*Marriage is put next to the person that the drag was started on*/
				/*Find out if we need to put it to the left or right of the person*/
				startx=Layout_FindXCoord(dragdata->windowdata->layout,dragdata->person);
				finishx=Layout_FindXCoord(dragdata->windowdata->layout,person);
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
	} else if (marriage) {
		Database_AddChild(marriage,dragdata->person);
		Windows_UnselectAll(dragdata->windowdata);
	}
}

static void Windows_AutoScroll(dragdata *dragdata,Desk_bool increasesize,Desk_bool plotbox)
/* Auto scroll the window when dragging, increaing the window size if asked*/
{
	Desk_mouse_block mouseblk;
	Desk_window_state blk;
	Desk_window_info infoblk;
	int mousex,mousey;

	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Wimp_GetWindowState(dragdata->windowdata->handle,&blk);
	Desk_Window_GetInfo3(dragdata->windowdata->handle,&infoblk);
	mousex=((mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.openblock.screenrect.max.y-blk.openblock.scroll.y))*100)/dragdata->windowdata->scale;

	if (mouseblk.pos.x-blk.openblock.screenrect.min.x<Config_ScrollDistance()) {
		/*We are near left edge of window*/
		if (plotbox) {
			Windows_PlotDragBox(dragdata);
			dragdata->plotted=Desk_FALSE;
		}
		if (increasesize && infoblk.block.scroll.x<=infoblk.block.workarearect.min.x) {
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
		if (plotbox) {
			Windows_PlotDragBox(dragdata);
			dragdata->plotted=Desk_FALSE;
		}
		if (increasesize && infoblk.block.scroll.x-infoblk.block.workarearect.min.x+(infoblk.block.screenrect.max.x-infoblk.block.screenrect.min.x)>=infoblk.block.workarearect.max.x-infoblk.block.workarearect.min.x) {
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
		if (plotbox) {
			Windows_PlotDragBox(dragdata);
			dragdata->plotted=Desk_FALSE;
		}
		if (increasesize && -(infoblk.block.scroll.y-infoblk.block.workarearect.max.y-(infoblk.block.screenrect.max.y-infoblk.block.screenrect.min.y))>=infoblk.block.workarearect.max.y-infoblk.block.workarearect.min.y) {
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
		if (plotbox) {
			Windows_PlotDragBox(dragdata);
			dragdata->plotted=Desk_FALSE;
		}
		if (increasesize && infoblk.block.scroll.y>=infoblk.block.workarearect.max.y) {
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
		dragdata->oldmousey=Layout_NearestGeneration(mousey);
	}
}

static void Windows_LinkDragFn(void *ref)
/*A link drag is in progress, so set pointer shape*/
{
	dragdata *dragdata=ref;
	elementptr person,marriage;

	Windows_AutoScroll(dragdata,Desk_FALSE,Desk_FALSE);
	Windows_LinkValid(ref,&person,&marriage);
	if (person || marriage) {
		if (dragdata->ptr!=ptr_LINK) Layout_SetPointerShape("ptr_link",2);
		dragdata->ptr=ptr_LINK;
	} else {
		if (dragdata->ptr!=ptr_NOLINK) Layout_SetPointerShape("ptr_nolink",2);
		dragdata->ptr=ptr_NOLINK;
	}
}

static void Windows_SelectDragFn(void *ref)
/*A select drag is in progress, so scroll if needed*/
{
	dragdata *dragdata=ref;

	Windows_AutoScroll(dragdata,Desk_FALSE,Desk_FALSE);
}

static void Windows_GetOffset(dragdata *dragdata)
{
	int i,distance;
	dragdata->oldoffset=0;
	if (!Config_Snap()) return;
	for (i=0;i<dragdata->windowdata->layout->numpeople;i++) {
		if (dragdata->windowdata->layout->person[i].y==dragdata->oldmousey && !Layout_GetSelect(dragdata->windowdata->layout->person[i].element)) {
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
			if (dragdata->windowdata->layout->marriage[i].y==dragdata->oldmousey && !Layout_GetSelect(dragdata->windowdata->layout->marriage[i].element)) {
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
	if (Layout_NearestGeneration(mousey)!=dragdata->origmousey) {
		if (dragdata->ptr!=ptr_UNLINK) Layout_SetPointerShape("ptr_unlink",2);
		dragdata->ptr=ptr_UNLINK;
	} else {
		if (dragdata->ptr!=ptr_DEFAULT) Layout_SetPointerShape("ptr_default",1);
		dragdata->ptr=ptr_DEFAULT;
	}
	if (mousex!=dragdata->oldmousex || Layout_NearestGeneration(mousey)!=dragdata->oldmousey) {
		/*Unplot drag box if it has moved*/
		Windows_PlotDragBox(dragdata);
		dragdata->oldmousex=mousex;
		dragdata->oldmousey=Layout_NearestGeneration(mousey);
		Windows_GetOffset(dragdata);
		/*Replot it in new position*/
		Windows_PlotDragBox(dragdata);
	}
	Windows_AutoScroll(dragdata,Desk_TRUE,Desk_TRUE);
}

static void Windows_StartDragNormal(elementptr person,int x,windowdata *windowdata,Desk_bool marriage)
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
	dragdata.ptr=ptr_DEFAULT;
	if (!marriage) {
		Desk_bool allsiblings=Desk_TRUE;
		elementptr person1=person,person2=person;
		int x,rightx=-INFINITY,leftx=INFINITY;
		while ((person1=Database_GetSiblingLtoR(person1))!=none) {
			if (!Layout_GetSelect(person1)) allsiblings=Desk_FALSE;
			if ((x=Layout_FindXCoord(windowdata->layout,person1))<leftx) leftx=x;
			if (x>rightx) rightx=x;
		}
		do {
			if (!Layout_GetSelect(person2)) allsiblings=Desk_FALSE;
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
/* Start a dragbox*/
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
	dragblk.window=windowdata->handle;
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
	dragdata.origmousey=mousey;
	dragdata.oldmousex=mousex;
	dragdata.oldmousey=mousey;
	Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_Wimp_DragBox,NULL,&dragblk,0x4B534154,0x3)); /*using RO4 features if present*/
	Desk_Drag_SetHandlers(Windows_SelectDragFn,Windows_SelectDragEnd,&dragdata);
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
	dragdata.ptr=ptr_DEFAULT;
	dragdata.windowdata=windowdata;
	dragdata.person=person;
	dragdata.origmousex=mousex;
	dragdata.origmousey=Layout_NearestGeneration(mousey);
	dragdata.oldmousex=mousex;
	dragdata.oldmousey=mousey;
	Windows_UnselectAll(windowdata);
	Layout_Select(person);
	for (i=0;i<windowdata->layout->numpeople;i++) {
		if (windowdata->layout->person[i].element==person) Layout_RedrawPerson(windowdata,windowdata->layout->person+i);
	}
	Desk_Wimp_DragBox(&dragblk);
	Desk_Drag_SetHandlers(Windows_LinkDragFn,Windows_LinkDragEnd,&dragdata);
}

static Desk_bool Windows_MenusDeleted(Desk_event_pollblock *block,void *ref)
{
	windowdata *windowdata=ref;
	Desk_UNUSED(block);
	if (windowdata->handle) Windows_UnselectAll(windowdata);
	Desk_EventMsg_Release(Desk_message_MENUSDELETED,Desk_event_ANY,Windows_MenusDeleted);
	return Desk_TRUE;
}

Desk_bool Layout_MouseClick(Desk_event_pollblock *block,void *ref)
{
	windowdata *windowdata=ref;
	int mousex,mousey,i;
	Desk_convert_block blk;
	elementtype selected;
	int layoutptr=0;
	
	if (!block->data.mouse.button.data.menu) Desk_Icon_SetCaret(block->data.mouse.window,-1);
	Desk_Window_GetCoords(windowdata->handle,&blk);
	mousex=((block->data.mouse.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/windowdata->scale;
	mousey=((block->data.mouse.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windowdata->scale;
	mousedata.element=none;
	mousedata.type=element_NONE;
	mousedata.window=windowdata;
	mousedata.pos.x=mousex;
	mousedata.pos.y=mousey;
	/*See if we clicked on a person*/
	for (i=windowdata->layout->numpeople-1;i>=0;i--) {
		if (mousex>=windowdata->layout->person[i].x && mousex<=windowdata->layout->person[i].x+Graphics_PersonWidth()) {
			if (mousey>=windowdata->layout->person[i].y && mousey<=windowdata->layout->person[i].y+Graphics_PersonHeight()) {
				mousedata.type=element_PERSON;
				layoutptr=i;
				mousedata.element=windowdata->layout->person[i].element;
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
				layoutptr=i;
				mousedata.element=windowdata->layout->marriage[i].element;
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
			default:
				break;
		}
		return Desk_TRUE;
	}
    switch (windowdata->type) {
		case wintype_NORMAL:
			switch (block->data.mouse.button.value) {
				case Desk_button_MENU:
					selected=Layout_AnyoneSelected();
					if (selected==element_NONE) {
						switch (mousedata.type) {
							case element_PERSON:
								Layout_Select(mousedata.element);
								Layout_RedrawPerson(windowdata,windowdata->layout->person+layoutptr);
								selected=element_PERSON;
								Desk_EventMsg_Claim(Desk_message_MENUSDELETED,Desk_event_ANY,Windows_MenusDeleted,windowdata);
								break;
							case element_MARRIAGE:
								Layout_Select(mousedata.element);
								Layout_RedrawMarriage(windowdata,windowdata->layout->marriage+layoutptr);
								selected=element_MARRIAGE;
								Desk_EventMsg_Claim(Desk_message_MENUSDELETED,Desk_event_ANY,Windows_MenusDeleted,windowdata);
								break;
							default:
								break;
						}
					}
					Windows_SetUpMenu(windowdata,selected,block->data.mouse.pos.x,block->data.mouse.pos.y);
					break;
				case Desk_button_CLICKSELECT:
					switch (mousedata.type) {
						case element_PERSON:
							if (!Layout_GetSelect(windowdata->layout->person[layoutptr].element)) {
								Windows_UnselectAll(windowdata);
								Layout_Select(windowdata->layout->person[layoutptr].element);
								Layout_RedrawPerson(windowdata,windowdata->layout->person+layoutptr);
							}
							break;
						case element_MARRIAGE:
							if (!Layout_GetSelect(windowdata->layout->marriage[layoutptr].element)) {
								Windows_UnselectAll(windowdata);
								Layout_Select(windowdata->layout->marriage[layoutptr].element);
								Layout_RedrawMarriage(windowdata,windowdata->layout->marriage+layoutptr);
							}
							break;
						default:
							Windows_UnselectAll(windowdata);
					}
					break;
				case Desk_button_CLICKADJUST:
					switch (mousedata.type) {
						case element_PERSON:
							if (Layout_GetSelect(windowdata->layout->person[layoutptr].element)) {
								Layout_DeSelect(windowdata->layout->person[layoutptr].element);
							} else {
								Layout_Select(windowdata->layout->person[layoutptr].element);
							}
							Layout_RedrawPerson(windowdata,windowdata->layout->person+layoutptr);
							break;
						case element_MARRIAGE:
							if (Layout_GetSelect(windowdata->layout->marriage[layoutptr].element)) {
								Layout_DeSelect(windowdata->layout->marriage[layoutptr].element);
							} else {
								Layout_Select(windowdata->layout->marriage[layoutptr].element);
							}
							Layout_RedrawMarriage(windowdata,windowdata->layout->marriage+layoutptr);
							break;
						default:
							break;
					}
					break;
				case Desk_button_DRAGSELECT:
					switch (mousedata.type) {
						case element_PERSON:
							if (Desk_Kbd_KeyDown(Desk_inkey_SHIFT)) {
								Windows_StartDragLink(windowdata,mousedata.element);
							} else {
								Windows_StartDragNormal(windowdata->layout->person[layoutptr].element,windowdata->layout->person[layoutptr].x,windowdata,Desk_FALSE);
							}
							break;
						case element_MARRIAGE:
							Windows_StartDragNormal(windowdata->layout->marriage[layoutptr].element,windowdata->layout->marriage[layoutptr].x,windowdata,Desk_TRUE);
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
						default:
							Windows_StartDragSelect(windowdata);
					}
			}
		default:
			break;
	}
	return Desk_TRUE;
}

void Layout_Init(void)
{
	ptrsprites=Desk_Sprite_LoadFile(ROOTSDIR".Sprites");
}

