/*
	Roots - Layout routines
	© Alex Waugh 1999

	$Id: Layout.c,v 1.58 2000/11/14 20:09:36 AJW Exp $

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

static layout *gedcomlayout=NULL;

void Layout_ChangeMarriageTypes(layout *layout,Desk_bool separate)
/*Change separate marriages into non, or vice versa*/
/*This should really be in TreeLayout.c, but I can't be bothered at the moment*/
{
	int i;
	flags flags;

	flags.editable=1;
	flags.moveable=1;
	flags.linkable=1;
	flags.snaptogrid=1;
	flags.selectable=1;

	if (separate) {
		for (i=0;i<layout->numtransients;i++) {
			if (Database_GetElementType(layout->transients[i].element)==element_MARRIAGE) {
				Layout_AddElement(layout,layout->transients[i].element,layout->transients[i].x,layout->transients[i].y,Graphics_MarriageWidth(),layout->transients[i].height,layout->transients[i].xgrid,layout->transients[i].ygrid,flags);
			}
		}
	} else {
		for (i=0;i<layout->numpeople;i++) {
			if (Database_GetElementType(layout->person[i].element)==element_MARRIAGE) {
				Layout_RemoveElement(layout,layout->person[i].element);
				i--;
			}
		}
	}
	Modules_ChangedStructure();
}

void Layout_Select(elementptr person)
{
	if (person>0) Database_SetFlag(person);
}

void Layout_DeSelect(elementptr person)
{
	if (person>0) Database_UnsetFlag(person);
}

void Layout_DeSelectAll(void)
{
	Database_UnsetAllFlags();
}

Desk_bool Layout_GetSelect(elementptr person)
{
	if (person>0) return Database_GetFlag(person);
	return Desk_FALSE;
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

void Layout_CalcAllGridFromPositions(layout *layout)
{
	int i;

	for (i=0;i<layout->numpeople;i++) {
		layout->person[i].xgrid=layout->person[i].x;
		layout->person[i].ygrid=layout->person[i].y/(Graphics_PersonHeight()+Graphics_GapHeightBelow()+Graphics_GapHeightAbove());
		if (layout->person[i].width==Graphics_PersonWidth()) layout->person[i].width=0;
		if (layout->person[i].height==Graphics_PersonHeight()) layout->person[i].height=0;
	}
}

void Layout_CalcAllPositionsFromGrid(layout *layout)
{
	int i;

	for (i=0;i<layout->numpeople;i++) {
		layout->person[i].x=layout->person[i].xgrid;
		layout->person[i].y=layout->person[i].ygrid*(Graphics_PersonHeight()+Graphics_GapHeightBelow()+Graphics_GapHeightAbove());
		if (layout->person[i].width==0) layout->person[i].width=Graphics_PersonWidth();
		if (layout->person[i].height==0) layout->person[i].height=Graphics_PersonHeight();
	}
}

void Layout_AddElement(layout *layout,elementptr person,int x,int y,int width,int height,int xgrid,int ygrid,flags flags)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	AJWLib_Flex_Extend((flex_ptr)&(layout->person),sizeof(elementlayout)*(layout->numpeople+1));
	layout->person[layout->numpeople].x=x;
	layout->person[layout->numpeople].y=y;
	layout->person[layout->numpeople].xgrid=xgrid;
	layout->person[layout->numpeople].ygrid=ygrid;
	layout->person[layout->numpeople].width=width;
	layout->person[layout->numpeople].height=height;
	layout->person[layout->numpeople].element=person;
	layout->person[layout->numpeople].flags=flags;
	layout->numpeople++;
	Layout_DeSelect(person);
	Modules_ChangedLayout();
}

void Layout_AddTransient(layout *layout,elementptr element,int x,int y,int width,int height,int xgrid,int ygrid,flags flags)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(element!=none);
	AJWLib_Flex_Extend((flex_ptr)&(layout->transients),sizeof(elementlayout)*(layout->numtransients+1));
	layout->transients[layout->numtransients].x=x;
	layout->transients[layout->numtransients].y=y;
	layout->transients[layout->numtransients].xgrid=xgrid;
	layout->transients[layout->numtransients].ygrid=ygrid;
	layout->transients[layout->numtransients].width=width;
	layout->transients[layout->numtransients].height=height;
	layout->transients[layout->numtransients].element=element;
	layout->transients[layout->numtransients].flags=flags;
	layout->numtransients++;
	/*Layout_DeSelect(element);*/
	/*Modules_ChangedLayout(); would cause an infinite loop?*/
}

void Layout_RemoveTransients(layout *layout)
{
	AJWLib_Assert(layout!=NULL);
	layout->numtransients=0;
	/*Flex block will be resized as soon as a transient is added*/
}

void Layout_DeleteSelected(layout *layout)
/*Delete all selected elements from the layout and database*/
{
	int i;
	for (i=0;i<layout->numpeople;i++) {
		if (Layout_GetSelect(layout->person[i].element)) {
			Database_RemoveElement(layout,layout->person[i].element);
			Modules_ChangedStructure();
			i--; /*This item in the layout has been removed, and so everyone above has moved down by one place*/
		}
	}
}

int Layout_FindXCoord(layout *layout,elementptr person)
{
	int i;

	AJWLib_Assert(layout!=NULL);

	for (i=0;i<layout->numpeople;i++) if (layout->person[i].element==person) return layout->person[i].x;
	for (i=0;i<layout->numtransients;i++) if (layout->transients[i].element==person) return layout->transients[i].x;
	AJWLib_AssertWarning(0);
	return 0;
}

int Layout_FindYCoord(layout *layout,elementptr person)
{
	int i;

	AJWLib_Assert(layout!=NULL);

	for (i=0;i<layout->numpeople;i++) if (layout->person[i].element==person) return layout->person[i].y;
	for (i=0;i<layout->numtransients;i++) if (layout->transients[i].element==person) return layout->transients[i].y;
	AJWLib_AssertWarning(0);
	return 0;
}

int Layout_FindWidth(layout *layout,elementptr person)
{
	int i;

	AJWLib_Assert(layout!=NULL);

	for (i=0;i<layout->numpeople;i++) if (layout->person[i].element==person) return layout->person[i].width;
	for (i=0;i<layout->numtransients;i++) if (layout->transients[i].element==person) return layout->transients[i].width;
	AJWLib_AssertWarning(0);
	return 0;
}

int Layout_FindHeight(layout *layout,elementptr person)
{
	int i;

	AJWLib_Assert(layout!=NULL);

	for (i=0;i<layout->numpeople;i++) if (layout->person[i].element==person) return layout->person[i].height;
	for (i=0;i<layout->numtransients;i++) if (layout->transients[i].element==person) return layout->transients[i].height;
	AJWLib_AssertWarning(0);
	return 0;
}

int Layout_FindXGridCoord(layout *layout,elementptr person)
{
	int i;

	AJWLib_Assert(layout!=NULL);

	for (i=0;i<layout->numpeople;i++) if (layout->person[i].element==person) return layout->person[i].xgrid;
	for (i=0;i<layout->numtransients;i++) if (layout->transients[i].element==person) return layout->transients[i].xgrid;
	AJWLib_AssertWarning(0);
	return 0;
}

int Layout_FindYGridCoord(layout *layout,elementptr person)
{
	int i;

	AJWLib_Assert(layout!=NULL);

	for (i=0;i<layout->numpeople;i++) if (layout->person[i].element==person) return layout->person[i].ygrid;
	for (i=0;i<layout->numtransients;i++) if (layout->transients[i].element==person) return layout->transients[i].ygrid;
	AJWLib_AssertWarning(0);
	return 0;
}

void Layout_RemoveElement(layout *layout,elementptr person)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	if (person==none) return;
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].element==person) {
			AJWLib_Flex_MidExtend((flex_ptr)&(layout->person),sizeof(elementlayout)*(i+1),-sizeof(elementlayout));
			layout->numpeople--;
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
				if (layout->person[i].x+layout->person[i].width>box.max.x) box.max.x=layout->person[i].x+layout->person[i].width;
				if (layout->person[i].y<box.min.y) box.min.y=layout->person[i].y;
				if (layout->person[i].y+layout->person[i].height>box.max.y) box.max.y=layout->person[i].y+layout->person[i].height;
			}
		}
		for (i=0;i<layout->numtransients;i++) {
			if (Layout_GetSelect(layout->transients[i].element) || !selection) {
				if (layout->transients[i].x<box.min.x) box.min.x=layout->transients[i].x;
				if (layout->transients[i].x+layout->transients[i].width>box.max.x) box.max.x=layout->transients[i].x+layout->transients[i].width;
				if (layout->transients[i].y<box.min.y) box.min.y=layout->transients[i].y;
				if (layout->transients[i].y+layout->transients[i].height>box.max.y) box.max.y=layout->transients[i].y+layout->transients[i].height;
			}
		}
	}
	if (box.min.x==INFINITY) {
		box.min.x=0;
		box.min.y=0;
		box.max.x=0;
		box.max.y=0;
	}
	return box;
}

void Layout_Redraw(layout *layout,int scale,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection)
{
	int i;

	AJWLib_Assert(layout!=NULL);
	for (i=0;i<layout->numtransients;i++) {
		if ((originx+((layout->transients[i].x-Graphics_GapWidth())*scale)/100)<cliprect->max.x) {
			if ((originx+((layout->transients[i].x+layout->transients[i].width+Graphics_GapWidth())*scale)/100)>cliprect->min.x) {
				if ((originy+((layout->transients[i].y-Graphics_GapWidth())*scale)/100)<cliprect->max.y) {
					if ((originy+((layout->transients[i].y+layout->transients[i].height+Graphics_GapWidth())*scale)/100)>cliprect->min.y) {
						Graphics_PlotElement(layout,layout->transients[i].element,scale,originx,originy,layout->transients[i].x,layout->transients[i].y,layout->transients[i].width,layout->transients[i].height,plotselection);
					}
				}
			}
		}
	}
	for (i=0;i<layout->numpeople;i++) {
		if ((originx+((layout->person[i].x-Graphics_GapWidth())*scale)/100)<cliprect->max.x) {
			if ((originx+((layout->person[i].x+layout->person[i].width+Graphics_GapWidth())*scale)/100)>cliprect->min.x) {
				if ((originy+((layout->person[i].y-Graphics_GapWidth())*scale)/100)<cliprect->max.y) {
					if ((originy+((layout->person[i].y+layout->person[i].height+Graphics_GapWidth())*scale)/100)>cliprect->min.y) {
						Graphics_PlotElement(layout,layout->person[i].element,scale,originx,originy,layout->person[i].x,layout->person[i].y,layout->person[i].width,layout->person[i].height,plotselection);
					}
				}
			}
		}
	}
}

layout *Layout_New(void)
/* Create a new empty layout*/
{
	layout *layout;

	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
	layout->numpeople=0;
	layout->numtransients=0;
	AJWLib_Flex_Alloc((flex_ptr)&(layout->person),1);
	AJWLib_Flex_Alloc((flex_ptr)&(layout->transients),1);
	return layout;
}

void Layout_Free(layout *layout)
{
	AJWLib_AssertWarning(layout!=NULL);
	if (layout==NULL) return;
	AJWLib_Flex_Free((flex_ptr)&(layout->person));
	AJWLib_Flex_Free((flex_ptr)&(layout->transients));
	Desk_DeskMem_Free(layout);
}


/*
	Roots - Layout related windows
	© Alex Waugh 1999

	$Id: Layout.c,v 1.58 2000/11/14 20:09:36 AJW Exp $

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

#define REDRAWOVERLAP 4
#define MINWINDOW_SIZE_X 500
#define MINWINDOW_SIZE_Y 256

#define SWI_OS_SpriteOp 0x2E
#define SWI_Wimp_SpriteOp 0x400E9
#define SWI_Wimp_DragBox 0x400D0

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

typedef struct dragdata {
	elementptr person;
	int personwidth,personheight;
	int origmousey;
	int personoffset;
	windowdata *windowdata;
	Desk_wimp_rect  coords;
	int origmousex,oldmousex,oldoffset,oldmousey,centered;
	Desk_bool plotted;
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

static void Layout_RedrawElement(windowdata *windowdata,elementlayout *person)
{
	Desk_Window_ForceRedraw(windowdata->handle,(windowdata->scale*person->x)/100-REDRAWOVERLAP,(windowdata->scale*person->y)/100-REDRAWOVERLAP,(windowdata->scale*(person->x+person->width))/100+REDRAWOVERLAP,(windowdata->scale*(person->y+person->height))/100+REDRAWOVERLAP);
}

Desk_bool Layout_RedrawWindow(Desk_event_pollblock *block,windowdata *windowdata)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=block->data.openblock.window;
	Desk_Wimp_RedrawWindow(&blk,&more);
	while (more) {
		Graphics_SetFunctions(Draw_PlotLine,Draw_PlotRectangle,Draw_PlotRectangleFilled,Draw_PlotText);
		Layout_Redraw(windowdata->layout,windowdata->scale,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,&(blk.cliprect),Desk_TRUE);
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
			Layout_RedrawElement(windowdata,windowdata->layout->person+i);
		}
	}
	for (i=0;i<windowdata->layout->numtransients;i++) {
		if (Layout_GetSelect(windowdata->layout->transients[i].element)) {
			Layout_DeSelect(windowdata->layout->transients[i].element);
			Layout_RedrawElement(windowdata,windowdata->layout->transients+i);
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
	Modules_ChangedLayout();
}

void Layout_ResizeWindow(windowdata *windowdata)
{
	Desk_wimp_rect box;
	Desk_window_info infoblk;
	Desk_window_outline outlineblk;
	Desk_bool maxxextent=Desk_FALSE,maxyextent=Desk_FALSE;
	int size;
	
	box=Layout_FindExtent(windowdata->layout,Desk_FALSE);
	size=MINWINDOW_SIZE_X-(box.max.x-box.min.x);
	if (size>0) {
		box.min.x-=size/2;
		box.max.x+=size/2;
	}
	size=MINWINDOW_SIZE_Y-(box.max.y-box.min.y);
	if (size>0) box.min.y-=size;
	Desk_Window_GetInfo3(windowdata->handle,&infoblk);
	outlineblk.window=windowdata->handle;
	if (infoblk.block.workarearect.max.x-infoblk.block.workarearect.min.x==infoblk.block.screenrect.max.x-infoblk.block.screenrect.min.x) {
		/*We are currently open at maximum x extent*/
		maxxextent=Desk_TRUE;
	}
	if (infoblk.block.workarearect.max.y-infoblk.block.workarearect.min.y==infoblk.block.screenrect.max.y-infoblk.block.screenrect.min.y) {
		/*We are currently open at maximum y extent*/
		maxyextent=Desk_TRUE;
	}
	/*Set window extent to fit layout size*/
	Desk_Window_SetExtent(windowdata->handle,(windowdata->scale*(box.min.x-Graphics_WindowBorder()))/100,(windowdata->scale*(box.min.y-Graphics_WindowBorder()))/100,(windowdata->scale*(box.max.x+Graphics_WindowBorder()))/100,(windowdata->scale*(box.max.y+Graphics_WindowBorder()))/100);
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
		/*Find amount we need to increase by*/
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

static void Layout_PlotDragBox(dragdata *dragdata)
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
		Draw_EORRectangle(dragdata->windowdata->scale,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,dragdata->oldmousex+dragdata->oldoffset-dragdata->personoffset,dragdata->oldmousey,dragdata->personwidth,dragdata->personheight,0,EORCOLOURRED);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
}

static void Layout_MoveDragEnd(void *ref)
/*User has finished a move drag*/
{
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	int mousex,mousey;

	Layout_SetPointerShape("ptr_DEFAULT",1);
	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Icon_SetCaret(mouseblk.window,-1);
	/*Remove dragbox*/
	if (dragdata->plotted) Layout_PlotDragBox(dragdata);
	/*Find mouse position relative to window origin and independant of current scale*/
	Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=Layout_NearestGeneration(((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/dragdata->windowdata->scale);
	/*Move all people/marriages that were selected*/
	Windows_AddSelected(dragdata->windowdata->layout,mousex+dragdata->oldoffset-dragdata->origmousex,mousey-dragdata->origmousey);
	/*Check to see if any people might need unlinking*/
	if (mousey!=dragdata->origmousey) {
		int i;

		for (i=0;i<dragdata->windowdata->layout->numpeople;i++) {
			if (Layout_GetSelect(dragdata->windowdata->layout->person[i].element)) TreeLayout_CheckForUnlink(dragdata->windowdata->layout,dragdata->windowdata->layout->person[i].element);
		}
	}
	Layout_LayoutLines(dragdata->windowdata->layout);
	Layout_LayoutTitle(dragdata->windowdata->layout);
	Layout_ResizeWindow(dragdata->windowdata);
	Desk_Window_ForceWholeRedraw(dragdata->windowdata->handle);
}

static void Layout_SelectDragEnd(void *ref)
/*A select drag box has ended, so select everything enclosed by it*/
{
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	int mousex,mousey,i;

	/*Get info*/
	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/dragdata->windowdata->scale;
	/*See who the drag enclosed*/
	for (i=0;i<dragdata->windowdata->layout->numpeople;i++) {
		if ((mousex>dragdata->windowdata->layout->person[i].x && dragdata->origmousex<dragdata->windowdata->layout->person[i].x+dragdata->windowdata->layout->person[i].width) || (mousex<dragdata->windowdata->layout->person[i].x+dragdata->windowdata->layout->person[i].width && dragdata->origmousex>dragdata->windowdata->layout->person[i].x)) {
			if ((mousey>dragdata->windowdata->layout->person[i].y && dragdata->origmousey<dragdata->windowdata->layout->person[i].y+dragdata->windowdata->layout->person[i].height) || (mousey<dragdata->windowdata->layout->person[i].y+dragdata->windowdata->layout->person[i].height && dragdata->origmousey>dragdata->windowdata->layout->person[i].y)) {
				/*Element was enclosed so toggle its selection*/
				if (dragdata->windowdata->layout->person[i].flags.selectable) {
					if (Layout_GetSelect(dragdata->windowdata->layout->person[i].element)) {
						Layout_DeSelect(dragdata->windowdata->layout->person[i].element);
					} else {
						Layout_Select(dragdata->windowdata->layout->person[i].element);
					}
					Layout_RedrawElement(dragdata->windowdata,dragdata->windowdata->layout->person+i);
				}
			}
		}
	}
	for (i=0;i<dragdata->windowdata->layout->numtransients;i++) {
		if ((mousex>dragdata->windowdata->layout->transients[i].x && dragdata->origmousex<dragdata->windowdata->layout->transients[i].x+dragdata->windowdata->layout->transients[i].width) || (mousex<dragdata->windowdata->layout->transients[i].x+dragdata->windowdata->layout->transients[i].width && dragdata->origmousex>dragdata->windowdata->layout->transients[i].x)) {
			if ((mousey>dragdata->windowdata->layout->transients[i].y && dragdata->origmousey<dragdata->windowdata->layout->transients[i].y+dragdata->windowdata->layout->transients[i].height) || (mousey<dragdata->windowdata->layout->transients[i].y+dragdata->windowdata->layout->transients[i].height && dragdata->origmousey>dragdata->windowdata->layout->transients[i].y)) {
				/*Element was enclosed so toggle its selection*/
				if (dragdata->windowdata->layout->transients[i].flags.selectable) {
					if (Layout_GetSelect(dragdata->windowdata->layout->transients[i].element)) {
						Layout_DeSelect(dragdata->windowdata->layout->transients[i].element);
					} else {
						Layout_Select(dragdata->windowdata->layout->transients[i].element);
					}
					Layout_RedrawElement(dragdata->windowdata,dragdata->windowdata->layout->transients+i);
				}
			}
		}
	}
}

static elementlayout *Layout_FindElementAtCoords(layout *layout,int x,int y,Desk_bool *transient)
{
	int i;

	AJWLib_Assert(layout!=NULL);
	if (transient) *transient=Desk_FALSE;
	/*See if we clicked on a transient*/
	/*Transients take priority over people, as they are generally smaller*/
	for (i=layout->numtransients-1;i>=0;i--) {
		if (x>=layout->transients[i].x && x<=layout->transients[i].x+layout->transients[i].width) {
			if (y>=layout->transients[i].y && y<=layout->transients[i].y+layout->transients[i].height) {
				if (transient) *transient=Desk_TRUE;
				return layout->transients+i;
			}
		}
	}
	/*See if we clicked on a person*/
	for (i=layout->numpeople-1;i>=0;i--) {
		if (x>=layout->person[i].x && x<=layout->person[i].x+layout->person[i].width) {
			if (y>=layout->person[i].y && y<=layout->person[i].y+layout->person[i].height) {
				return layout->person+i;
			}
		}
	}
	return NULL;
}

static void Windows_LinkDragEnd(void *ref)
/*A link drag has ended, so link the person if possible*/
{
	dragdata *dragdata=ref;
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	elementlayout *elementlayout;
	int mousex,mousey;

	Layout_SetPointerShape("ptr_default",1);

	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/dragdata->windowdata->scale;
	elementlayout=Layout_FindElementAtCoords(dragdata->windowdata->layout,mousex,mousey,NULL);

	Windows_UnselectAll(dragdata->windowdata);
	if (elementlayout) {
		Desk_Error2_Try {
			elementptr marriage;

			marriage=Database_Link(dragdata->windowdata->layout,dragdata->person,elementlayout->element);
			if (marriage) TreeLayout_AddMarriage(dragdata->windowdata->layout,marriage);
		} Desk_Error2_Catch {
			AJWLib_Error2_Report("%s");
		} Desk_Error2_EndCatch
	}
}

static void Layout_AutoScroll(dragdata *dragdata,Desk_bool increasesize,Desk_bool plotbox)
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
			Layout_PlotDragBox(dragdata);
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
			Layout_PlotDragBox(dragdata);
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
			Layout_PlotDragBox(dragdata);
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
			Layout_PlotDragBox(dragdata);
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

static void Layout_LinkDragPoll(void *ref)
/*A link drag is in progress, so set pointer shape*/
{
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	elementlayout *elementlayout;
	int mousex,mousey;

	Layout_AutoScroll(dragdata,Desk_FALSE,Desk_FALSE);

	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/dragdata->windowdata->scale;
	elementlayout=Layout_FindElementAtCoords(dragdata->windowdata->layout,mousex,mousey,NULL);

	if (elementlayout) {
		if (Database_LinkValid(dragdata->windowdata->layout,dragdata->person,elementlayout->element)) {
			if (dragdata->ptr!=ptr_LINK) Layout_SetPointerShape("ptr_link",2);
			dragdata->ptr=ptr_LINK;
		} else {
			if (dragdata->ptr!=ptr_NOLINK) Layout_SetPointerShape("ptr_nolink",2);
			dragdata->ptr=ptr_NOLINK;
		}
	} else {
		if (dragdata->ptr!=ptr_NOLINK) Layout_SetPointerShape("ptr_nolink",2);
		dragdata->ptr=ptr_NOLINK;
	}
}

static void Layout_SelectDragPoll(void *ref)
/*A select drag is in progress, so scroll if needed*/
{
	dragdata *dragdata=ref;

	Layout_AutoScroll(dragdata,Desk_FALSE,Desk_FALSE);
}

static void Windows_GetOffset(dragdata *dragdata)
{
	int i,distance;
	dragdata->oldoffset=0;

	if (!Config_Snap()) return;
	for (i=0;i<dragdata->windowdata->layout->numpeople;i++) {
		if (dragdata->windowdata->layout->person[i].y==dragdata->oldmousey && !Layout_GetSelect(dragdata->windowdata->layout->person[i].element)) {
			/*Look for right hand edge of person or marriage*/
			distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x+dragdata->windowdata->layout->person[i].width);
			if (Database_GetElementType(dragdata->person)==element_PERSON) {
				if (Database_GetElementType(dragdata->windowdata->layout->person[i].element)==element_PERSON) {
					if (Database_Married(dragdata->person,dragdata->windowdata->layout->person[i].element)) distance-=Graphics_MarriageWidth(); else distance-=Graphics_GapWidth();
				}
			} else {
				if (Database_GetElementType(dragdata->windowdata->layout->person[i].element)==element_MARRIAGE) distance=INFINITY;
			}
			if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
			/*Look for second marriage on right*/
			if (Database_GetElementType(dragdata->person)==element_MARRIAGE && Database_GetElementType(dragdata->windowdata->layout->person[i].element)==element_PERSON) {
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x+dragdata->windowdata->layout->person[i].width+Graphics_SecondMarriageGap());
				if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
			}
			/*Look for left hand edge of person or marriage*/
			distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x);
			if (Database_GetElementType(dragdata->windowdata->layout->person[i].element)==element_PERSON) {
				if (Database_GetElementType(dragdata->person)==element_PERSON) {
					distance+=dragdata->windowdata->layout->person[i].width;
					if (Database_Married(dragdata->person,dragdata->windowdata->layout->person[i].element)) distance+=Graphics_MarriageWidth(); else distance+=Graphics_GapWidth();
				} else {
					distance+=Graphics_MarriageWidth();
				}
			} else {
				if (Database_GetElementType(dragdata->person)==element_PERSON) distance+=dragdata->personwidth; else distance=INFINITY;
			}
			if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
			/*Look for second marriage on left*/
			if (Database_GetElementType(dragdata->person)==element_MARRIAGE && Database_GetElementType(dragdata->windowdata->layout->person[i].element)==element_PERSON) {
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x-Graphics_MarriageWidth()-Graphics_SecondMarriageGap());
				if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
			}
		}
	}

	/*Look for siblings centered under marriage and marriages centered over siblings*/
	distance=dragdata->oldmousex-dragdata->centered;
	if (abs(distance)<Config_SnapDistance()) dragdata->oldoffset=-distance;
}

static void Layout_MoveDragPoll(void *ref)
/*Called every null poll when dragging*/
{
	dragdata *dragdata=ref;
	Desk_mouse_block mouseblk;
	Desk_window_state blk;
	Desk_window_info infoblk;
	int mousex,mousey;

	/*Plot drag box if not already plotted*/
	if (!dragdata->plotted) {
		Windows_GetOffset(dragdata);
		Layout_PlotDragBox(dragdata);
		dragdata->plotted=Desk_TRUE;
	}

	/*Get info about mouse position*/
	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Wimp_GetWindowState(dragdata->windowdata->handle,&blk);
	Desk_Window_GetInfo3(dragdata->windowdata->handle,&infoblk);
	mousex=((mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x))*100)/dragdata->windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.openblock.screenrect.max.y-blk.openblock.scroll.y))*100)/dragdata->windowdata->scale;

	/*Change pointer shape if unlinking might happen*/
	if (Layout_NearestGeneration(mousey)!=dragdata->origmousey) {
		if (dragdata->ptr!=ptr_UNLINK) Layout_SetPointerShape("ptr_unlink",2);
		dragdata->ptr=ptr_UNLINK;
	} else {
		if (dragdata->ptr!=ptr_DEFAULT) Layout_SetPointerShape("ptr_default",1);
		dragdata->ptr=ptr_DEFAULT;
	}

	/*Check if the mouse has moved*/
	if (mousex!=dragdata->oldmousex || Layout_NearestGeneration(mousey)!=dragdata->oldmousey) {
		/*Unplot drag box if it has moved*/
		Layout_PlotDragBox(dragdata);
		dragdata->oldmousex=mousex;
		dragdata->oldmousey=Layout_NearestGeneration(mousey);
		Windows_GetOffset(dragdata);
		/*Replot it in new position*/
		Layout_PlotDragBox(dragdata);
	}

	/*Scroll the window if we are near the edge*/
	Layout_AutoScroll(dragdata,Desk_TRUE,Desk_TRUE);
}

static void Layout_MoveDragStart(elementptr person,int x,windowdata *windowdata)
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
	/*Get initial position of drag*/
	dragdata.coords=Layout_FindExtent(windowdata->layout,Desk_TRUE);
	dragdata.coords.min.x-=mousex;
	dragdata.coords.max.x-=mousex;
	dragdata.coords.min.y-=mousey;
	dragdata.coords.max.y-=mousey;
	/*Set limits of drag to within the window*/
	if (blk.screenrect.min.x<0) dragblk.parent.min.x=0; else dragblk.parent.min.x=blk.screenrect.min.x;
	if (blk.screenrect.max.x>Desk_screen_size.x) dragblk.parent.max.x=Desk_screen_size.x; else dragblk.parent.max.x=blk.screenrect.max.x;
	if (blk.screenrect.min.y<0) dragblk.parent.min.y=0; else dragblk.parent.min.y=blk.screenrect.min.y;
	if (blk.screenrect.max.y>Desk_screen_size.y) dragblk.parent.max.y=Desk_screen_size.y; else dragblk.parent.max.y=blk.screenrect.max.y;
    /*Not used, but set anyway*/
	dragblk.screenrect.min.x=0;
	dragblk.screenrect.max.x=0;
	dragblk.screenrect.min.y=0;
	dragblk.screenrect.max.y=0;
	/*Set up structure used for updating drag*/
	dragdata.person=person;
	dragdata.personwidth=Layout_FindWidth(windowdata->layout,person);
	dragdata.personheight=Layout_FindHeight(windowdata->layout,person);
	dragdata.personoffset=mousex-x;
	dragdata.windowdata=windowdata;
	dragdata.origmousex=mousex;
	dragdata.origmousey=mousey;
	dragdata.oldmousex=mousex;
	dragdata.oldmousey=mousey;
	dragdata.oldoffset=0;
	dragdata.plotted=Desk_FALSE;
	dragdata.ptr=ptr_DEFAULT;
	/*Set up the centered position*/
	dragdata.centered=INFINITY;

	if (Database_GetElementType(person)==element_PERSON) {
		Desk_bool allsiblings=Desk_TRUE;
		elementptr person1=person,person2=person;
		int x,rightx=-INFINITY,leftx=INFINITY;
		elementptr leftperson=person;

		while ((person1=Database_GetSiblingLtoR(person1))!=none) {
			if (!Layout_GetSelect(person1)) allsiblings=Desk_FALSE;
			if ((x=Layout_FindXCoord(windowdata->layout,person1))<leftx) {
				leftx=x;
				leftperson=person1;
			}
			if (x>rightx) rightx=x;
		}
		do {
			if (!Layout_GetSelect(person2)) allsiblings=Desk_FALSE;
			if ((x=Layout_FindXCoord(windowdata->layout,person2))<leftx) {
				leftx=x;
				leftperson=person2;
			}
			if (x>rightx) rightx=x;
		} while ((person2=Database_GetSiblingRtoL(person2))!=none);
		if (allsiblings) {
			int centre=(rightx+leftx+Layout_FindWidth(windowdata->layout,leftperson))/2;
			elementptr marriage=Database_GetMarriage(Database_GetMother(person));
			int marriagepos=INFINITY;
			if (marriage) marriagepos=Layout_FindXCoord(windowdata->layout,marriage)+Layout_FindWidth(windowdata->layout,marriage)/2;
			dragdata.centered=marriagepos+(mousex-centre);
		} else {
			dragdata.centered=INFINITY;
		}
	} else {
		if (Database_GetRightChild(person)) {
			int tempx,rightx=-INFINITY,leftx=INFINITY,centre,marriagepos;
			elementptr person1=Database_GetRightChild(person);
			elementptr leftperson=person1;
			do {
				if ((tempx=Layout_FindXCoord(windowdata->layout,person1))<leftx) {
					leftx=tempx;
					leftperson=person1;
				}
				if (tempx>rightx) rightx=tempx;
			} while ((person1=Database_GetSiblingRtoL(person1))!=none);
			centre=(rightx+leftx+Layout_FindWidth(windowdata->layout,leftperson))/2;
			marriagepos=Layout_FindXCoord(windowdata->layout,person)+Layout_FindWidth(windowdata->layout,person)/2;
			dragdata.centered=centre+(mousex-marriagepos);
		} else {
			dragdata.centered=INFINITY;
		}
	}

	Desk_Wimp_DragBox(&dragblk);
	Desk_Drag_SetHandlers(Layout_MoveDragPoll,Layout_MoveDragEnd,&dragdata);
}

static void Layout_SelectDragStart(windowdata *windowdata)
/*Start a dragbox*/
{
	static dragdata dragdata;
	Desk_drag_block dragblk;
	Desk_convert_block blk;
	Desk_mouse_block mouseblk;
	int mousex,mousey;

	/*Get info*/
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
	/*Restrict drag to current window*/
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
	Desk_Drag_SetHandlers(Layout_SelectDragPoll,Layout_SelectDragEnd,&dragdata);
}

static void Layout_LinkDragStart(windowdata *windowdata,elementptr person)
/*Start a drag to link two elements*/
{
	static dragdata dragdata;
	Desk_drag_block dragblk;
	Desk_convert_block blk;
	Desk_mouse_block mouseblk;
	int mousex,mousey,i;

	/*Get info*/
	Desk_Window_GetCoords(windowdata->handle,&blk);
	Desk_Wimp_GetPointerInfo(&mouseblk);
	mousex=((mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/windowdata->scale;
	mousey=((mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windowdata->scale;
	dragblk.type=Desk_drag_INVISIBLE;
	dragblk.screenrect.min.x=mouseblk.pos.x;
	dragblk.screenrect.max.x=mouseblk.pos.x;
	dragblk.screenrect.min.y=mouseblk.pos.y;
	dragblk.screenrect.max.y=mouseblk.pos.y;
	/*Restrict drag to current window*/
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
		if (windowdata->layout->person[i].element==person) Layout_RedrawElement(windowdata,windowdata->layout->person+i);
	}
	Desk_Wimp_DragBox(&dragblk);
	Desk_Drag_SetHandlers(Layout_LinkDragPoll,Windows_LinkDragEnd,&dragdata);
}

static Desk_bool Layout_MenusDeleted(Desk_event_pollblock *block,void *ref)
{
	windowdata *windowdata=ref;
	Desk_UNUSED(block);
	if (windowdata->handle) Windows_UnselectAll(windowdata);
	Desk_EventMsg_Release(Desk_message_MENUSDELETED,Desk_event_ANY,Layout_MenusDeleted);
	return Desk_TRUE;
}

Desk_bool Layout_MouseClick(Desk_event_pollblock *block,void *ref)
{
	windowdata *windowdata=ref;
	int mousex,mousey;
	Desk_convert_block blk;
	elementtype selected;
	Desk_bool transient;
	elementlayout *elementlayout;

	/*Set window focus if select or adjust clicked*/
	if (!block->data.mouse.button.data.menu) Desk_Icon_SetCaret(block->data.mouse.window,-1);

	/*Find out what was clicked on*/
	Desk_Window_GetCoords(windowdata->handle,&blk);
	mousex=((block->data.mouse.pos.x-(blk.screenrect.min.x-blk.scroll.x))*100)/windowdata->scale;
	mousey=((block->data.mouse.pos.y-(blk.screenrect.max.y-blk.scroll.y))*100)/windowdata->scale;
	mousedata.element=none;
	mousedata.window=windowdata;
	mousedata.pos.x=mousex;
	mousedata.pos.y=mousey;
	elementlayout=Layout_FindElementAtCoords(windowdata->layout,mousex,mousey,&transient);
	mousedata.transient=transient;
	if (elementlayout) mousedata.element=elementlayout->element; else mousedata.element=element_NONE;
	
	switch (block->data.mouse.button.value) {
		case Desk_button_SELECT: /*Double click select*/
			Windows_UnselectAll(windowdata);
			if (elementlayout) {
				if (elementlayout->flags.editable) Database_Edit(mousedata.element);
			}
			break;

		case Desk_button_MENU:
			selected=Layout_AnyoneSelected();
			if (selected==element_NONE && elementlayout) {
				/*If noone selected, then select the element under the pointer*/
				if (elementlayout->flags.selectable) {
					Layout_Select(mousedata.element);
					Layout_RedrawElement(windowdata,elementlayout);
					selected=Database_GetElementType(mousedata.element);
					Desk_EventMsg_Claim(Desk_message_MENUSDELETED,Desk_event_ANY,Layout_MenusDeleted,windowdata);
				}
			}
			Windows_SetUpMenu(windowdata,selected,block->data.mouse.pos.x,block->data.mouse.pos.y);
			break;

		case Desk_button_CLICKSELECT:
			/*Deselect everybody, then select the element clicked on*/
			if (!Layout_GetSelect(mousedata.element)) {
				Windows_UnselectAll(windowdata);
				if (elementlayout) {
					if (elementlayout->flags.selectable) {
						Layout_Select(mousedata.element);
						Layout_RedrawElement(windowdata,elementlayout);
					}
				}
			}
			break;

		case Desk_button_CLICKADJUST:
			/*Toggle the selection state of the element clicked on*/
			if (elementlayout) {
				if (elementlayout->flags.selectable) {
					if (Layout_GetSelect(mousedata.element)) {
						Layout_DeSelect(mousedata.element);
					} else {
						Layout_Select(mousedata.element);
					}
					Layout_RedrawElement(windowdata,elementlayout);
				}
			}
			break;
			
		case Desk_button_DRAGSELECT:
			if (elementlayout) {
				if (Desk_Kbd_KeyDown(Desk_inkey_SHIFT)) {
					if (elementlayout->flags.linkable) Layout_LinkDragStart(windowdata,mousedata.element);
				} else {
					if (elementlayout->flags.moveable) Layout_MoveDragStart(mousedata.element,elementlayout->x,windowdata);
				}
			} else {
				Windows_UnselectAll(windowdata);
				Layout_SelectDragStart(windowdata);
			}
			break;

		case Desk_button_DRAGADJUST:
			if (elementlayout) {
				if (elementlayout->flags.linkable) Layout_LinkDragStart(windowdata,mousedata.element);
			} else {
				Layout_SelectDragStart(windowdata);
			}
			break;
			
		default:
			break;
	}
	return Desk_TRUE;
}

void Layout_SaveGEDCOM(layout *layout,FILE *file)
/*Save a GEDCOM layout to the given file ptr*/
{
	int i;
	int *temp;
	
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(file!=NULL);

	fprintf(file,"0 @L1@ _LAYOUT\n");
	for (i=0;i<layout->numpeople;i++) {
		fprintf(file,"1 _PERSON @%d@\n",layout->person[i].element);
		fprintf(file,"2 _X %d\n",layout->person[i].x);
		fprintf(file,"2 _Y %d\n",layout->person[i].y);
		fprintf(file,"2 _W %d\n",layout->person[i].width);
		fprintf(file,"2 _H %d\n",layout->person[i].height);
		temp=(int *)&(layout->person[i].flags);
		fprintf(file,"2 _F %d\n",*temp);
	}
}

layout *Layout_GetGEDCOMLayout(void)
/* Return the GEDCOM layout used while loading, then reset it ready for the next load*/
{
	layout *returnvalue=gedcomlayout;
	int i;

	for (i=0;i<gedcomlayout->numpeople;i++) {
		if (gedcomlayout->person[i].width==0 && gedcomlayout->person[i].height==0) {
			gedcomlayout->person[i].width=Database_GetElementType(gedcomlayout->person[i].element)==element_MARRIAGE ? Graphics_MarriageWidth() : Graphics_PersonWidth();
			gedcomlayout->person[i].height=Graphics_PersonHeight();
		}
	}
	gedcomlayout=NULL;
	return returnvalue;
}

void Layout_GEDCOMNewPerson(elementptr person)
/* Add a new person to the GEDCOM layout*/
{
	flags defaultflags;

	if (gedcomlayout==NULL) gedcomlayout=Layout_New();
	AJWLib_Flex_Extend((flex_ptr)&(gedcomlayout->person),(gedcomlayout->numpeople+1)*sizeof(elementlayout));
	gedcomlayout->person[gedcomlayout->numpeople].element=person;
	gedcomlayout->person[gedcomlayout->numpeople].x=0;
	gedcomlayout->person[gedcomlayout->numpeople].y=0;
	gedcomlayout->person[gedcomlayout->numpeople].width=0;
	gedcomlayout->person[gedcomlayout->numpeople].height=0;
	defaultflags.editable=1;
	defaultflags.moveable=1;
	defaultflags.linkable=1;
	defaultflags.snaptogrid=1;
	defaultflags.selectable=1;
	gedcomlayout->person[gedcomlayout->numpeople].flags=defaultflags;
	gedcomlayout->person[gedcomlayout->numpeople].xgrid=0;
	gedcomlayout->person[gedcomlayout->numpeople].ygrid=0;
	gedcomlayout->numpeople++;
}

void Layout_GEDCOMNewPersonX(int pos)
/* Add the x coord to a new person to the GEDCOM layout*/
{
	AJWLib_Assert(gedcomlayout!=NULL);
	gedcomlayout->person[gedcomlayout->numpeople-1].x=pos;
}

void Layout_GEDCOMNewPersonY(int pos)
/* Add the y coord to a new person to the GEDCOM layout*/
{
	AJWLib_Assert(gedcomlayout!=NULL);
	gedcomlayout->person[gedcomlayout->numpeople-1].y=Layout_NearestGeneration(pos);
}

void Layout_GEDCOMNewPersonWidth(int width)
/* Add the width coord to a new person to the GEDCOM layout*/
{
	AJWLib_Assert(gedcomlayout!=NULL);
	gedcomlayout->person[gedcomlayout->numpeople-1].width=width;
}
void Layout_GEDCOMNewPersonHeight(int height)
/* Add the height coord to a new person to the GEDCOM layout*/
{
	AJWLib_Assert(gedcomlayout!=NULL);
	gedcomlayout->person[gedcomlayout->numpeople-1].height=height;
}

void Layout_GEDCOMNewPersonFlags(flags flags)
/* Add the flags to a new person to the GEDCOM layout*/
{
	AJWLib_Assert(gedcomlayout!=NULL);
	gedcomlayout->person[gedcomlayout->numpeople-1].flags=flags;
}

void Layout_Init(void)
{
	ptrsprites=Desk_Sprite_LoadFile(ROOTSDIR".Sprites");
}

