/*
	FT - Graphics
	© Alex Waugh 1999

	$Log: Windows.c,v $
	Revision 1.2  1999/09/27 16:56:43  AJW
	Added centering siblings under marriage when dragging

	Revision 1.1  1999/09/27 15:33:01  AJW
	Initial revision


*/

/*	Includes  */

#include "DeskLib:Window.h"
#include "DeskLib:Error.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:Handler.h"
#include "DeskLib:Hourglass.h"
#include "DeskLib:Icon.h"
#include "DeskLib:Menu.h"
#include "DeskLib:Msgs.h"
#include "DeskLib:Drag.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Template.h"
#include "DeskLib:File.h"
#include "DeskLib:Filing.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:Screen.h"
#include "DeskLib:GFX.h"
#include "DeskLib:Font2.h"
#include "DeskLib:ColourTran.h"

#include "AJWLib:Window.h"
#include "AJWLib:Menu.h"
#include "AJWLib:Msgs.h"
#include "AJWLib:Misc.h"
#include "AJWLib:Handler.h"
#include "AJWLib:Icon.h"
#include "AJWLib:Error.h"
#include "AJWLib:Flex.h"
#include "AJWLib:Str.h"
#include "AJWLib:Save.h"
#include "AJWLib:Draw.h"
#include "AJWLib:DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "GConfig.h"
#include "Layout.h"

/*	Macros  */

#define MAXWINDOWS 10

#define SWI_ColourTrans_SetFontColours 0x4074F

#define EORCOLOUR 0xFFFFFF00
#define EORCOLOURRED 0xFFFF0000

#define SCROLLDISTANCE 50
#define SCROLLBASEAMOUNT 0
#define SCROLLMULTIPLIER 2

#define SNAPDISTANCE 30

#define mainmenu_FILE 0
#define mainmenu_PERSON 1
#define mainmenu_NEWVIEW 4
#define mainmenu_SEARCH 5
#define mainmenu_REPORTS 6
#define mainmenu_SELECT 2
#define mainmenu_LINEUP 3

#define lineupmenu_SIBLINGS 0
#define lineupmenu_PARENTS 1

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

typedef struct windowdata {
	window_handle handle;
	wintype type;
	elementptr person;
	int generations;
	layout *layout;
	/*Anything else?*/
} windowdata;

typedef struct dragdata {
	elementptr person;
	int persony;
	int personoffset;
	windowdata *windowdata;
	wimp_rect  coords;
	int origmousex,oldmousex,oldoffset,oldmousey,centeredunder;
	BOOL plotted,marriage;
} dragdata;

#if DEBUG
extern layout *layouts;
#endif

static windowdata windows[MAXWINDOWS];
static int numwindows;
static window_handle addparentswin,newviewwin,fileinfowin;
static menu_ptr mainmenu,filemenu,exportmenu,personmenu,selectmenu,lineupmenu;
static elementptr addparentsperson,addparentschild,menuoverperson,newviewperson;
static windowdata *menuoverwindow;
static os_trfm matrix;
extern graphics graphicsdata;

void Graphics_PlotPerson(elementptr person,int x,int y,BOOL child,BOOL selected)
{
	int i;
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		switch (graphicsdata.person[i].type) {
			case graphictype_RECTANGLE:
/*Error*/		ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				Draw_PlotRectangle(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_FILLEDRECTANGLE:
/*Error*/		ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				Draw_PlotRectangleFilled(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,&matrix);
				break;
			case graphictype_CHILDLINE:
				if (!child) break;
				/*A line that is only plotted if there is a child and child==FALSE ?*/
			case graphictype_LINE:
/*Error*/		ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				Draw_PlotLine(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,x+graphicsdata.person[i].details.linebox.x1,y+graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_TEXTLABEL:
/*Errors*/		Font_SetFont(graphicsdata.person[i].details.textlabel.properties.font->handle);
				SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.person[i].details.textlabel.properties.bgcolour,graphicsdata.person[i].details.textlabel.properties.colour,14);
				Font_Paint(graphicsdata.person[i].details.textlabel.text,font_plot_OSCOORS,x+graphicsdata.person[i].details.textlabel.properties.x,y+graphicsdata.person[i].details.textlabel.properties.y);
				break;
		}
	}
	for (i=0;i<NUMPERSONFIELDS;i++) {
		if (graphicsdata.personfields[i].plot) {
			char fieldtext[256]=""; /*what is max field length?*/
			switch (i) {
			/*A centered field?*/
				case personfieldtype_SURNAME:
					strcpy(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_FORENAME:
					strcpy(fieldtext,Database_GetPersonData(person)->forename);
					break;
				case personfieldtype_MIDDLENAMES:
					strcpy(fieldtext,Database_GetPersonData(person)->middlenames);
					break;
				case personfieldtype_NAME:
					strcpy(fieldtext,Database_GetPersonData(person)->forename);
					strcat(fieldtext," ");
					strcat(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_FULLNAME:
					strcpy(fieldtext,Database_GetPersonData(person)->forename);
					strcat(fieldtext," ");
					strcat(fieldtext,Database_GetPersonData(person)->middlenames);
					strcat(fieldtext," ");
					strcat(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_TITLE:
					strcpy(fieldtext,Database_GetPersonData(person)->title);
					break;
			}
/*Errors*/	Font_SetFont(graphicsdata.personfields[i].textproperties.font->handle);
			SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.personfields[i].textproperties.bgcolour,graphicsdata.personfields[i].textproperties.colour,14);
			Font_Paint(fieldtext,font_plot_OSCOORS,x+graphicsdata.personfields[i].textproperties.x,y+graphicsdata.personfields[i].textproperties.y);
		}
	}
	if (selected) {
		ColourTrans_SetGCOL(EORCOLOUR,0,3);
		Draw_PlotRectangleFilled(x,y,Graphics_PersonWidth(),Graphics_PersonHeight(),&matrix);

	}
}

void Graphics_PlotMarriage(int x,int y,elementptr marriage,BOOL childline,BOOL selected)
{
	int i;
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		switch (graphicsdata.marriage[i].type) {
			case graphictype_RECTANGLE:
/*Error*/		ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				Draw_PlotRectangle(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_FILLEDRECTANGLE:
/*Error*/		ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				Draw_PlotRectangleFilled(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,&matrix);
				break;
			case graphictype_CHILDLINE:
				if (!childline) break;
			case graphictype_LINE:
/*Error*/		ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				Draw_PlotLine(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,x+graphicsdata.marriage[i].details.linebox.x1,y+graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_TEXTLABEL:
/*Errors*/		Font_SetFont(graphicsdata.marriage[i].details.textlabel.properties.font->handle);
				SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.marriage[i].details.textlabel.properties.bgcolour,graphicsdata.marriage[i].details.textlabel.properties.colour,14);
				Font_Paint(graphicsdata.marriage[i].details.textlabel.text,font_plot_OSCOORS,x+graphicsdata.marriage[i].details.textlabel.properties.x,y+graphicsdata.marriage[i].details.textlabel.properties.y);
				break;
		}
	}
	for (i=0;i<NUMMARRIAGEFIELDS;i++) {
		if (graphicsdata.marriagefields[i].plot) {
			char fieldtext[256]=""; /*what is max field length?*/
			switch (i) {
				case marriagefieldtype_PLACE:
					strcpy(fieldtext,Database_GetMarriageData(marriage)->place);
					break;
			}
/*Errors*/	Font_SetFont(graphicsdata.marriagefields[i].textproperties.font->handle);
			SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.marriagefields[i].textproperties.bgcolour,graphicsdata.marriagefields[i].textproperties.colour,14);
			Font_Paint(fieldtext,font_plot_OSCOORS,x+graphicsdata.marriagefields[i].textproperties.x,y+graphicsdata.marriagefields[i].textproperties.y);
		}
	}
	if (selected) {
		ColourTrans_SetGCOL(EORCOLOUR,0,3);
		Draw_PlotRectangleFilled(x,y,Graphics_MarriageWidth(),Graphics_PersonHeight(),&matrix);

	}
}

void Graphics_PlotChildren(int leftx,int rightx,int y)
{
	ColourTrans_SetGCOL(graphicsdata.siblinglinecolour,0/*or 1<<8 to use ECFs ?*/,0); /*Error?*/
	Draw_PlotLine(leftx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),rightx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),graphicsdata.siblinglinethickness,&matrix);
}

static BOOL Graphics_Redraw(event_pollblock *block,windowdata *windowdata)
{
	window_redrawblock blk;
	BOOL more=FALSE;
	blk.window=block->data.openblock.window;
	Wimp_RedrawWindow(&blk,&more);
	while (more) {
		int i=0;
#if DEBUG
ColourTrans_SetGCOL(0x00000000,0,0);
Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
ColourTrans_SetGCOL(0xFF000000,0,0);
Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+1000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-1000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
ColourTrans_SetGCOL(0x0000FF00,0,0);
Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+2000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-2000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
ColourTrans_SetGCOL(0x00FF0000,0,0);
Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+3000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-3000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
#endif
		for (i=0;i<windowdata->layout->numchildren;i++) {
			Graphics_PlotChildren(blk.rect.min.x-blk.scroll.x+windowdata->layout->children[i].leftx,blk.rect.min.x-blk.scroll.x+windowdata->layout->children[i].rightx,blk.rect.max.y-blk.scroll.y+windowdata->layout->children[i].y);
		}
		for (i=windowdata->layout->nummarriages-1;i>=0;i--) {
			Graphics_PlotMarriage(blk.rect.min.x-blk.scroll.x+windowdata->layout->marriage[i].x,blk.rect.max.y-blk.scroll.y+windowdata->layout->marriage[i].y,windowdata->layout->marriage[i].marriage,windowdata->layout->marriage[i].childline,windowdata->layout->marriage[i].selected);
		}
		for (i=windowdata->layout->numpeople-1;i>=0;i--) {
			Graphics_PlotPerson(windowdata->layout->person[i].person,blk.rect.min.x-blk.scroll.x+windowdata->layout->person[i].x,blk.rect.max.y-blk.scroll.y+windowdata->layout->person[i].y,windowdata->layout->person[i].child,windowdata->layout->person[i].selected);
		}
		Wimp_GetRectangle(&blk,&more);
		/*Use clip rectangle??*/
	}
	return TRUE;
}

static BOOL Graphics_RedrawAddParents(event_pollblock *block,void *ref)
{
	window_redrawblock blk;
	BOOL more=FALSE;
	blk.window=block->data.openblock.window;
	Wimp_RedrawWindow(&blk,&more);
	while (more) {
		Graphics_PlotPerson(addparentsperson,blk.rect.min.x-blk.scroll.x+332,blk.rect.max.y-blk.scroll.y-16-Graphics_PersonHeight(),FALSE,FALSE);
		Wimp_GetRectangle(&blk,&more);
	}
	return TRUE;
}

BOOL Graphics_AddParentsClick(event_pollblock *block,void *ref)
{
	if (!block->data.mouse.button.data.select) return FALSE;
	Database_Marry(addparentschild,addparentsperson);
	Window_Hide(addparentswin);
	return TRUE;
}

void Graphics_UnselectAll(windowdata *windowdata)
{
	int i;
	for (i=0;i<windowdata->layout->numpeople;i++) {
		if (windowdata->layout->person[i].selected) {
			windowdata->layout->person[i].selected=FALSE;
			Window_ForceRedraw(windowdata->handle,windowdata->layout->person[i].x,windowdata->layout->person[i].y,windowdata->layout->person[i].x+Graphics_PersonWidth(),windowdata->layout->person[i].y+Graphics_PersonHeight());
		}
	}
	for (i=0;i<windowdata->layout->nummarriages;i++) {
		if (windowdata->layout->marriage[i].selected) {
			windowdata->layout->marriage[i].selected=FALSE;
			Window_ForceRedraw(windowdata->handle,windowdata->layout->marriage[i].x,windowdata->layout->marriage[i].y,windowdata->layout->marriage[i].x+Graphics_MarriageWidth(),windowdata->layout->marriage[i].y+Graphics_PersonHeight());
		}
	}
}

void Graphics_AddSelected(layout *layout,int amount)
{
	int i;
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].selected) layout->person[i].x+=amount;
	}
	for (i=0;i<layout->nummarriages;i++) {
		if (layout->marriage[i].selected) layout->marriage[i].x+=amount;
	}
}

void Graphics_ResizeWindow(windowdata *windowdata)
{
	wimp_rect box;
	box=Layout_FindExtent(windowdata->layout,FALSE);
	Window_SetExtent(windowdata->handle,box.min.x-Graphics_WindowBorder(),box.min.y-Graphics_WindowBorder(),box.max.x+Graphics_WindowBorder(),box.max.y+Graphics_WindowBorder());
}

void Graphics_PlotDragBox(dragdata *dragdata)
{
	window_redrawblock blk;
	BOOL more=FALSE;
	blk.window=dragdata->windowdata->handle;
	blk.rect.min.x=-INFINITY;
	blk.rect.max.x=INFINITY;
	blk.rect.min.y=-INFINITY;
	blk.rect.max.y=INFINITY;
	Wimp_UpdateWindow(&blk,&more);
	while (more) {
		ColourTrans_SetGCOL(EORCOLOUR,0,3);
		Draw_PlotRectangle(blk.rect.min.x-blk.scroll.x+dragdata->oldmousex+dragdata->coords.min.x+dragdata->oldoffset,blk.rect.max.y-blk.scroll.y+dragdata->oldmousey+dragdata->coords.min.y,dragdata->coords.max.x-dragdata->coords.min.x,dragdata->coords.max.y-dragdata->coords.min.y,0,&matrix);
		ColourTrans_SetGCOL(EORCOLOURRED,0,3);
		Draw_PlotRectangle(blk.rect.min.x-blk.scroll.x+dragdata->oldmousex+dragdata->oldoffset-dragdata->personoffset,blk.rect.max.y-blk.scroll.y+dragdata->persony,dragdata->marriage ? Graphics_MarriageWidth() : Graphics_PersonWidth(),Graphics_PersonHeight(),0,&matrix);
		Wimp_GetRectangle(&blk,&more);
	}
}

void Graphics_DragEnd(void *ref)
{
	mouse_block mouseblk;
	convert_block blk;
	dragdata *dragdata=ref;
	elementptr initialperson=dragdata->person;
	int window=-1,mousex,mousey,i;
	Wimp_GetPointerInfo(&mouseblk); /*errors*/
	if (mouseblk.window==addparentswin) {
		if (initialperson==addparentsperson) return;
		if (Database_IsUnlinked(initialperson)) {
			Database_AddParents(addparentschild,addparentsperson,initialperson);
			Window_Hide(addparentswin);
			return;
		}
	}
	for (i=0;i<MAXWINDOWS;i++) if (mouseblk.window==windows[i].handle) window=i;
	if (window==-1) return;
	if (dragdata->windowdata->type==wintype_NORMAL) {
		int mousex;
		/*act as if drag had ended on same win as started on*/
		if (dragdata->plotted) Graphics_PlotDragBox(dragdata);
		Window_GetCoords(dragdata->windowdata->handle,&blk);
		mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
		Graphics_AddSelected(dragdata->windowdata->layout,mousex+dragdata->oldoffset-dragdata->origmousex);
		Layout_LayoutLines(dragdata->windowdata->layout);
		Graphics_ResizeWindow(dragdata->windowdata);
		Window_ForceRedraw(dragdata->windowdata->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
	} else if (dragdata->windowdata->type==wintype_UNLINKED) {
		if (windows[window].type==wintype_NORMAL) {
			if (!Database_IsUnlinked(initialperson)) return; /*This should never happen*/
			if (Database_GetLinked()==none) {
				Database_LinkPerson(initialperson);
				return;
			}
			Window_GetCoords(mouseblk.window,&blk);
			mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
			mousey=mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y);
			for (i=0;i<windows[window].layout->numpeople;i++) {
				if (mousex>=windows[window].layout->person[i].x && mousex<=windows[window].layout->person[i].x+Graphics_PersonWidth()) {
					if (mousey>=windows[window].layout->person[i].y && mousey<=windows[window].layout->person[i].y+Graphics_PersonHeight()) {
						elementptr finalperson=windows[window].layout->person[i].person;
						if (initialperson==finalperson) return; /*This should never happen*/
						if (Database_GetMarriage(finalperson) && Database_GetFather(finalperson)==none) {
							addparentsperson=initialperson;
							addparentschild=finalperson;
							Window_SetExtent(addparentswin,0,304>Graphics_PersonHeight()+32 ? -304 : -Graphics_PersonHeight()-32,348+Graphics_PersonWidth(),0);
							Window_ForceRedraw(addparentswin,-INFINITY,-INFINITY,INFINITY,INFINITY);
							Window_Show(addparentswin,open_CENTERED);
						} else {
							Database_Marry(finalperson,initialperson);
						}
						return;
					}
				}
			}
			for (i=0;i<windows[window].layout->nummarriages;i++) {
				if (mousex>=windows[window].layout->marriage[i].x && mousex<=windows[window].layout->marriage[i].x+Graphics_MarriageWidth()) {
					if (mousey>=windows[window].layout->marriage[i].y && mousey<=windows[window].layout->marriage[i].y+Graphics_PersonHeight()) {
						Database_AddChild(windows[window].layout->marriage[i].marriage,initialperson);
						return;
					}
				}
			}
		}
	}
}

void Graphics_GetOffset(dragdata *dragdata)
{
	int i,distance;
	dragdata->oldoffset=0;
	for (i=0;i<dragdata->windowdata->layout->numpeople;i++) {
		if (dragdata->windowdata->layout->person[i].y==dragdata->persony && !dragdata->windowdata->layout->person[i].selected) {
			/*Look for right hand edge of person or marriage*/
			distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth());
			if (!dragdata->marriage) distance-=Graphics_GapWidth();
			if (abs(distance)<SNAPDISTANCE) dragdata->oldoffset=-distance;
			/*Look for second marriage on right*/
			if (dragdata->marriage) {
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth()+Graphics_SecondMarriageGap());
				if (abs(distance)<SNAPDISTANCE) dragdata->oldoffset=-distance;
			}
			/*Look for left hand edge of person or marriage*/
			distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x);
			if (!dragdata->marriage) distance+=Graphics_GapWidth()+Graphics_PersonWidth(); else distance+=Graphics_MarriageWidth();
			if (abs(distance)<SNAPDISTANCE) dragdata->oldoffset=-distance;
			/*Look for second marriage on left*/
			if (dragdata->marriage) {
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->person[i].x-Graphics_MarriageWidth()-Graphics_SecondMarriageGap());
				if (abs(distance)<SNAPDISTANCE) dragdata->oldoffset=-distance;
			}
		}
	}
	if (!dragdata->marriage) {
		for (i=0;i<dragdata->windowdata->layout->nummarriages;i++) {
			if (dragdata->windowdata->layout->marriage[i].y==dragdata->persony && !dragdata->windowdata->layout->marriage[i].selected) {
				/*Look for right hand edge of marriage*/
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth());
				if (abs(distance)<SNAPDISTANCE) dragdata->oldoffset=-distance;
				/*Look for left hand edge of marriage*/
				distance=(dragdata->oldmousex-dragdata->personoffset)-(dragdata->windowdata->layout->marriage[i].x-Graphics_PersonWidth());
				if (abs(distance)<SNAPDISTANCE) dragdata->oldoffset=-distance;
			}
		}
	}
	/*Look for siblings centered under marriage*/
	distance=dragdata->oldmousex-dragdata->centeredunder;
	if (abs(distance)<SNAPDISTANCE) dragdata->oldoffset=-distance;
}

void Graphics_DragFn(void *ref)
{
	dragdata *dragdata=ref;
	mouse_block mouseblk;
	window_state blk;
	int mousex;
	if (dragdata->windowdata->type!=wintype_NORMAL) return;
	if (!dragdata->plotted) {
		Graphics_GetOffset(dragdata);
		Graphics_PlotDragBox(dragdata);
		dragdata->plotted=TRUE;
	}
	Wimp_GetPointerInfo(&mouseblk);
	Wimp_GetWindowState(dragdata->windowdata->handle,&blk);
	mousex=mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x);
	if (mousex!=dragdata->oldmousex) {
		Graphics_PlotDragBox(dragdata);
		dragdata->oldmousex=mousex;
		Graphics_GetOffset(dragdata);
		Graphics_PlotDragBox(dragdata);
	}
	if (mouseblk.pos.x-blk.openblock.screenrect.min.x<SCROLLDISTANCE) {
		Graphics_PlotDragBox(dragdata);
		dragdata->plotted=FALSE;
		blk.openblock.scroll.x-=SCROLLBASEAMOUNT+(SCROLLMULTIPLIER*(SCROLLDISTANCE-(mouseblk.pos.x-blk.openblock.screenrect.min.x)));
		Wimp_OpenWindow(&blk.openblock);
		mousex=mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x);
		dragdata->oldmousex=mousex;
	} else if (blk.openblock.screenrect.max.x-mouseblk.pos.x<SCROLLDISTANCE) {
		Graphics_PlotDragBox(dragdata);
		dragdata->plotted=FALSE;
		blk.openblock.scroll.x+=SCROLLBASEAMOUNT+(SCROLLMULTIPLIER*(SCROLLDISTANCE-(blk.openblock.screenrect.max.x-mouseblk.pos.x)));
		Wimp_OpenWindow(&blk.openblock);
		mousex=mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x);
		dragdata->oldmousex=mousex;
	}
}

void Graphics_StartDragNormal(elementptr person,int x,int y,windowdata *windowdata,BOOL marriage)
{
	static dragdata dragdata;
	drag_block dragblk;
	convert_block blk;
	mouse_block mouseblk;
	int mousex,mousey;
	Window_GetCoords(windowdata->handle,&blk);
	Wimp_GetPointerInfo(&mouseblk);
	mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
	mousey=mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y);
	dragblk.type=drag_INVISIBLE;
	dragdata.coords=Layout_FindExtent(windowdata->layout,TRUE);
	dragdata.coords.min.x-=mousex;
	dragdata.coords.max.x-=mousex;
	dragdata.coords.min.y-=mousey;
	dragdata.coords.max.y-=mousey;
	if (blk.screenrect.min.x<0) dragblk.parent.min.x=0; else dragblk.parent.min.x=blk.screenrect.min.x;
	if (blk.screenrect.max.x>screen_size.x) dragblk.parent.max.x=screen_size.x; else dragblk.parent.max.x=blk.screenrect.max.x;
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
	dragdata.plotted=FALSE;
	if (!marriage) {
		BOOL allsiblings=TRUE;
		elementptr person1=person,person2=person;
		int x,rightx=-INFINITY,leftx=INFINITY;
		while ((person1=Database_GetSiblingLtoR(person1))!=none) {
			if (!Layout_Selected(windowdata->layout,person1)) allsiblings=FALSE;
			if ((x=Layout_FindXCoord(windowdata->layout,person1))<leftx) leftx=x;
			if (x>rightx) rightx=x;
		}
		do {
			if (!Layout_Selected(windowdata->layout,person2)) allsiblings=FALSE;
			if ((x=Layout_FindXCoord(windowdata->layout,person2))<leftx) leftx=x;
			if (x>rightx) rightx=x;
		} while ((person2=Database_GetSiblingRtoL(person2))!=none);
		if (allsiblings) {
			int centre=(rightx+leftx+Graphics_PersonWidth())/2;
			int marriagepos=Layout_FindMarriageXCoord(windowdata->layout,Database_GetMarriage(Database_GetMother(person)))+Graphics_MarriageWidth()/2;
			dragdata.centeredunder=marriagepos+(mousex-centre);
		} else {
			dragdata.centeredunder=INFINITY;
		}
	} else {
		dragdata.centeredunder=INFINITY;
	}
	Wimp_DragBox(&dragblk);
	Drag_SetHandlers(Graphics_DragFn,Graphics_DragEnd,&dragdata);
}

void Graphics_StartDragUnlinked(elementptr person,int x,int y,windowdata *windowdata)
{
	static dragdata dragdata;
	drag_block dragblk;
	convert_block blk;
	mouse_block mouseblk;
	int mousex,mousey;
	Window_GetCoords(windowdata->handle,&blk);
	Wimp_GetPointerInfo(&mouseblk);
	mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
	mousey=mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y);
	dragblk.type=drag_FIXEDBOX;
	dragblk.screenrect.min.x=x+(blk.screenrect.min.x-blk.scroll.x);
	dragblk.screenrect.max.x=x+Graphics_PersonWidth()+(blk.screenrect.min.x-blk.scroll.x);
	dragblk.screenrect.min.y=y+(blk.screenrect.max.y-blk.scroll.y);
	dragblk.screenrect.max.y=y+Graphics_PersonHeight()+(blk.screenrect.max.y-blk.scroll.y);
	dragblk.parent.min.y=dragblk.screenrect.min.y-mouseblk.pos.y;
	dragblk.parent.max.y=screen_size.y+dragblk.screenrect.max.y-mouseblk.pos.y;
	dragblk.parent.min.x=dragblk.screenrect.min.x-mouseblk.pos.x;
	dragblk.parent.max.x=screen_size.x+dragblk.screenrect.max.x-mouseblk.pos.x;
	dragdata.person=person;
	dragdata.persony=y;
	dragdata.personoffset=0;
	dragdata.windowdata=windowdata;
	dragdata.origmousex=mousex;
	dragdata.oldmousex=mousex;
	dragdata.oldmousey=mousey;
	dragdata.oldoffset=0;
	dragdata.plotted=FALSE;
	Wimp_DragBox(&dragblk);
	Drag_SetHandlers(NULL,Graphics_DragEnd,&dragdata);
}

BOOL Graphics_MouseClick(event_pollblock *block,void *ref)
{
	windowdata *windowdata=ref;
	int mousex,mousey,i;
	convert_block blk;
	Window_GetCoords(windowdata->handle,&blk);
	mousex=block->data.mouse.pos.x-(blk.screenrect.min.x-blk.scroll.x);
	mousey=block->data.mouse.pos.y-(blk.screenrect.max.y-blk.scroll.y);
	menuoverperson=none;
	Menu_Shade(personmenu,personmenu_ADD);
	Menu_Shade(personmenu,personmenu_DELETE);
	Menu_Shade(personmenu,personmenu_UNLINK);
	Menu_Shade(lineupmenu,lineupmenu_SIBLINGS);
	Menu_Shade(lineupmenu,lineupmenu_PARENTS);
	Menu_Shade(mainmenu,mainmenu_PERSON);
	Menu_Shade(mainmenu,mainmenu_SELECT);
	Menu_Shade(mainmenu,mainmenu_LINEUP);
	for (i=0;i<windowdata->layout->numpeople;i++) {
		if (mousex>=windowdata->layout->person[i].x && mousex<=windowdata->layout->person[i].x+Graphics_PersonWidth()) {
			if (mousey>=windowdata->layout->person[i].y && mousey<=windowdata->layout->person[i].y+Graphics_PersonHeight()) {
				if (block->data.mouse.button.data.clickselect) {
					if (windowdata->type==wintype_NORMAL) {
						if (!windowdata->layout->person[i].selected) {
							Graphics_UnselectAll(windowdata);
							windowdata->layout->person[i].selected=TRUE;
							Window_ForceRedraw(windowdata->handle,windowdata->layout->person[i].x,windowdata->layout->person[i].y,windowdata->layout->person[i].x+Graphics_PersonWidth(),windowdata->layout->person[i].y+Graphics_PersonHeight());
						}
						return TRUE;
					}
				} else if (block->data.mouse.button.data.clickadjust) {
					if (windowdata->type==wintype_NORMAL) {
						windowdata->layout->person[i].selected=!windowdata->layout->person[i].selected;
						Window_ForceRedraw(windowdata->handle,windowdata->layout->person[i].x,windowdata->layout->person[i].y,windowdata->layout->person[i].x+Graphics_PersonWidth(),windowdata->layout->person[i].y+Graphics_PersonHeight());
						return TRUE;
					}
				} else if (block->data.mouse.button.data.menu) {
					menuoverperson=windowdata->layout->person[i].person;
					menuoverwindow=windowdata;
					if (windowdata->type==wintype_NORMAL) {
						Menu_UnShade(mainmenu,mainmenu_PERSON);
						Menu_UnShade(mainmenu,mainmenu_SELECT);
						Menu_UnShade(mainmenu,mainmenu_LINEUP);
						Menu_UnShade(personmenu,personmenu_UNLINK);
					} else if (windowdata->type==wintype_UNLINKED) {
						Menu_UnShade(personmenu,personmenu_DELETE);
					}
					if (windowdata->type==wintype_NORMAL) Menu_UnShade(mainmenu,mainmenu_SELECT);
				} else if (block->data.mouse.button.data.select) {
					Database_EditPerson(windowdata->layout->person[i].person);
					return TRUE;
				} else if (block->data.mouse.button.data.adjust) {
					if (windowdata->type==wintype_DESCENDENTS || windowdata->type==wintype_ANCESTORS) {
						windowdata->person=windowdata->layout->person[i].person;
						Graphics_Relayout(); /*Rather inefficient*/
					}
					return TRUE;
				} else if (block->data.mouse.button.data.dragselect) {
					if (windowdata->type==wintype_NORMAL) {
						Graphics_StartDragNormal(windowdata->layout->person[i].person,windowdata->layout->person[i].x,windowdata->layout->person[i].y,windowdata,FALSE);
						return TRUE;
					} else if (windowdata->type==wintype_UNLINKED) {
						Graphics_StartDragUnlinked(windowdata->layout->person[i].person,windowdata->layout->person[i].x,windowdata->layout->person[i].y,windowdata);
						return TRUE;
					}
				}
			}
		}
	}
	if (block->data.mouse.button.data.menu) {
		if (windowdata->type==wintype_UNLINKED) {
			Menu_UnShade(mainmenu,mainmenu_PERSON);
			Menu_UnShade(personmenu,personmenu_ADD);
		}
		Menu_Show(mainmenu,block->data.mouse.pos.x,block->data.mouse.pos.y);
		return TRUE;
	}
	for (i=0;i<windowdata->layout->nummarriages;i++) {
		if (mousex>=windowdata->layout->marriage[i].x && mousex<=windowdata->layout->marriage[i].x+Graphics_MarriageWidth()) {
			if (mousey>=windowdata->layout->marriage[i].y && mousey<=windowdata->layout->marriage[i].y+Graphics_PersonHeight()) {
				if (block->data.mouse.button.data.select) {
					Database_EditMarriage(windowdata->layout->marriage[i].marriage);
					return TRUE;
				} else if (block->data.mouse.button.data.clickselect) {
					if (windowdata->type==wintype_NORMAL) {
						if (!windowdata->layout->marriage[i].selected) {
							Graphics_UnselectAll(windowdata);
							windowdata->layout->marriage[i].selected=TRUE;
							Window_ForceRedraw(windowdata->handle,windowdata->layout->marriage[i].x,windowdata->layout->marriage[i].y,windowdata->layout->marriage[i].x+Graphics_MarriageWidth(),windowdata->layout->marriage[i].y+Graphics_PersonHeight());
						}
						return TRUE;
					}
				} else if (block->data.mouse.button.data.clickadjust) {
					if (windowdata->type==wintype_NORMAL) {
						windowdata->layout->marriage[i].selected=!windowdata->layout->marriage[i].selected;
						Window_ForceRedraw(windowdata->handle,windowdata->layout->marriage[i].x,windowdata->layout->marriage[i].y,windowdata->layout->marriage[i].x+Graphics_MarriageWidth(),windowdata->layout->marriage[i].y+Graphics_PersonHeight());
						return TRUE;
					}
				} else if (block->data.mouse.button.data.dragselect) {
					if (windowdata->type==wintype_NORMAL) {
						Graphics_StartDragNormal(windowdata->layout->marriage[i].marriage,windowdata->layout->marriage[i].x,windowdata->layout->marriage[i].y,windowdata,TRUE);
						return TRUE;
					}
				}
			}
		}
	}
	if (block->data.mouse.button.data.clickselect) {
		Graphics_UnselectAll(windowdata);
		return TRUE;
	}
	return FALSE;
}

void Graphics_Relayout(void)
{
	int i;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle!=0) {
			/*Free layout mem first*/
			switch (windows[i].type) {
				case wintype_NORMAL:
					/*windows[i].layout=Layout_LayoutNormal();*/
					Layout_LayoutLines(windows[i].layout);
					break;
				case wintype_DESCENDENTS:
					windows[i].layout=Layout_LayoutDescendents(windows[i].person,windows[i].generations);
					break;
				case wintype_UNLINKED:
					windows[i].layout=Layout_LayoutUnlinked();
					break;
			}
			Graphics_ResizeWindow(&windows[i]);
			Window_ForceRedraw(windows[i].handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
		}
	}
}

void Graphics_OpenWindow(wintype type,elementptr person,int generations)
{
	int newwindow;
	if (numwindows>MAXWINDOWS) {
		/*Report Error?*/
		return;
	}
	for (newwindow=0;windows[newwindow].handle!=0 && newwindow<MAXWINDOWS;newwindow++);
	if (newwindow==MAXWINDOWS) /*Error?*/ return;
	windows[newwindow].handle=Window_CreateAndShow("Main",template_TITLEMIN,open_CENTERED);
	windows[newwindow].type=type;
	windows[newwindow].person=person;
	windows[newwindow].generations=generations;
	switch (type) {
		case wintype_NORMAL:
#if DEBUG
Event_Claim(event_CLICK,windows[newwindow].handle,event_ANY,Graphics_MouseClick,&windows[newwindow]);
Event_Claim(event_REDRAW,windows[newwindow].handle,event_ANY,(event_handler)Graphics_Redraw,&windows[newwindow]);
windows[newwindow].layout=layouts;
Layout_LayoutNormal();
#else
			windows[newwindow].layout=Layout_LayoutNormal();
#endif
		break;
		case wintype_DESCENDENTS:
			windows[newwindow].layout=Layout_LayoutDescendents(person,generations);
		break;
		case wintype_UNLINKED:
			windows[newwindow].layout=Layout_LayoutUnlinked();
		break;
	}
	Graphics_ResizeWindow(&windows[newwindow]);
	/*Register handlers for this window*/
	Event_Claim(event_CLICK,windows[newwindow].handle,event_ANY,Graphics_MouseClick,&windows[newwindow]);
	Event_Claim(event_REDRAW,windows[newwindow].handle,event_ANY,(event_handler)Graphics_Redraw,&windows[newwindow]);
	/*Error handling*/
}

void Graphics_MainMenuClick(int entry,void *ref)
{
	char buffer[10]="";
	switch (entry) {
		case mainmenu_NEWVIEW:
			if (menuoverperson==none) {
				Icon_Shade(newviewwin,newview_ANCESTOR);
				Icon_Shade(newviewwin,newview_ANCESTORPERSON);
				Icon_Shade(newviewwin,newview_DESCENDENT);
				Icon_Shade(newviewwin,newview_DESCENDENTPERSON);
				Icon_Shade(newviewwin,newview_CLOSERELATIVES);
				Icon_Shade(newviewwin,newview_CLOSERELATIVESPERSON);
			} else {
				Icon_Unshade(newviewwin,newview_ANCESTOR);
				Icon_Unshade(newviewwin,newview_ANCESTORPERSON);
				Icon_Unshade(newviewwin,newview_DESCENDENT);
				Icon_Unshade(newviewwin,newview_DESCENDENTPERSON);
				Icon_Unshade(newviewwin,newview_CLOSERELATIVES);
				Icon_Unshade(newviewwin,newview_CLOSERELATIVESPERSON);
				strcpy(buffer,Database_GetPersonData(menuoverperson)->forename);
				strcat(buffer," ");
				strcat(buffer,Database_GetPersonData(menuoverperson)->surname);
			}
			Icon_Shade(newviewwin,newview_UPTO);
			Icon_Shade(newviewwin,newview_GENERATIONS);
			Icon_Shade(newviewwin,newview_UP);
			Icon_Shade(newviewwin,newview_DOWN);
			Icon_Shade(newviewwin,newview_GENERATIONSTEXT);
			Icon_SetText(newviewwin,newview_ANCESTORPERSON,buffer);
			Icon_SetText(newviewwin,newview_DESCENDENTPERSON,buffer);
			Icon_SetText(newviewwin,newview_CLOSERELATIVESPERSON,buffer);
			Window_Show(newviewwin,open_CENTERED);
			Icon_SetCaret(newviewwin,-1);
			Icon_SetRadios(newviewwin,newview_NORMAL,newview_CLOSERELATIVES,newview_NORMAL);
			newviewperson=menuoverperson;
			break;
	}
}

BOOL Graphics_NewViewCancel(event_pollblock *block,void *ref)
{
	if (block->data.mouse.button.data.select) {
		Window_Hide(newviewwin);
		return TRUE;
	}
	return FALSE;
}

BOOL Graphics_NewViewOk(event_pollblock *block,void *ref)
{
	wintype wintype;
	if (block->data.mouse.button.data.menu) return FALSE;
	switch (Icon_WhichRadio(newviewwin,newview_NORMAL,newview_DESCENDENT)) {
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
	Graphics_OpenWindow(wintype,newviewperson,atoi(Icon_GetTextPtr(newviewwin,newview_GENERATIONS)));
	if (block->data.mouse.button.data.select) Window_Hide(newviewwin);
	return TRUE;
}

void Graphics_PersonMenuClick(int entry,void *ref)
{
	switch (entry) {
		case personmenu_ADD:
			Database_Add();
			break;
		case personmenu_DELETE:
			Database_Delete(menuoverperson);
			break;
		case personmenu_UNLINK:
			if (Database_GetMarriage(menuoverperson)==none) {
				Database_RemoveChild(menuoverperson);
				Layout_RemovePerson(menuoverwindow->layout,menuoverperson);
			}
			break;
	}
}

void Graphics_LineUpMenuClick(int entry,void *ref)
{
	switch (entry) {
		case lineupmenu_SIBLINGS:
			break;
		case lineupmenu_PARENTS:
			break;
	}
}

void Graphics_SelectMenuClick(int entry,void *ref)
{
	switch (entry) {
		case selectmenu_DESCENDENTS:
			Layout_SelectDescendents(menuoverwindow->layout,menuoverperson);
			Window_ForceRedraw(menuoverwindow->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
			break;
		case selectmenu_ANCESTORS:
			Layout_SelectAncestors(menuoverwindow->layout,menuoverperson);
			Window_ForceRedraw(menuoverwindow->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
			break;
		case selectmenu_SIBLINGS:
			Layout_SelectSiblings(menuoverwindow->layout,menuoverperson);
			Window_ForceRedraw(menuoverwindow->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
			break;
		case selectmenu_SPOUSES:
			Layout_SelectSpouses(menuoverwindow->layout,menuoverperson);
			Window_ForceRedraw(menuoverwindow->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
			break;
	}
}

BOOL Graphics_NewViewClick(event_pollblock *block,void *ref)
{
	switch (block->data.mouse.icon) {
		case newview_UNLINKED:
		case newview_NORMAL:
			Icon_Shade(newviewwin,newview_UPTO);
			Icon_Shade(newviewwin,newview_GENERATIONS);
			Icon_Shade(newviewwin,newview_UP);
			Icon_Shade(newviewwin,newview_DOWN);
			Icon_Shade(newviewwin,newview_GENERATIONSTEXT);
			Icon_SetCaret(newviewwin,-1);
			break;
		case newview_ANCESTOR:
		case newview_DESCENDENT:
		case newview_CLOSERELATIVES:
			Icon_Unshade(newviewwin,newview_UPTO);
			Icon_Unshade(newviewwin,newview_GENERATIONS);
			Icon_Unshade(newviewwin,newview_UP);
			Icon_Unshade(newviewwin,newview_DOWN);
			Icon_Unshade(newviewwin,newview_GENERATIONSTEXT);
			Icon_SetCaret(newviewwin,newview_GENERATIONS);
			break;
	}
	return FALSE;
}

BOOL SaveHandler(char *filename,BOOL safe,BOOL selection,void *ref)
{
	Database_Save(filename);
	return TRUE;
}

void Graphics_Init(void)
{
	int i;
	numwindows=0;
	for (i=0;i<MAXWINDOWS;i++) windows[i].handle=0;
	Drag_Initialise(TRUE);
	matrix.entries[0][0]=1<<24;
	matrix.entries[0][1]=0;
	matrix.entries[1][0]=0;
	matrix.entries[1][1]=1<<24;
	matrix.entries[2][0]=0;
	matrix.entries[2][1]=0;
	newviewwin=Window_Create("NewView",template_TITLEMIN);
	Icon_InitIncDecHandler(newviewwin,newview_GENERATIONS,newview_UP,newview_DOWN,FALSE,1,1,999,10);
	Icon_RegisterCheckAdjust(newviewwin,newview_NORMAL);
	Icon_RegisterCheckAdjust(newviewwin,newview_UNLINKED);
	Icon_RegisterCheckAdjust(newviewwin,newview_ANCESTOR);
	Icon_RegisterCheckAdjust(newviewwin,newview_DESCENDENT);
	Event_Claim(event_CLICK,newviewwin,event_ANY,Graphics_NewViewClick,NULL);
	Event_Claim(event_CLICK,newviewwin,newview_OK,Graphics_NewViewOk,NULL);
	Event_Claim(event_CLICK,newviewwin,newview_CANCEL,Graphics_NewViewCancel,NULL);
	addparentswin=Window_Create("AddParents",template_TITLEMIN);
	fileinfowin=Window_Create("FileInfo",template_TITLEMIN);
	Event_Claim(event_REDRAW,addparentswin,event_ANY,Graphics_RedrawAddParents,NULL);
	Event_Claim(event_CLICK,addparentswin,1/*Icon Name macro?*/,Graphics_AddParentsClick,NULL);
	exportmenu=Menu_CreateFromMsgs("Title.Export:","Menu.Export:",NULL,NULL);
	filemenu=Menu_CreateFromMsgs("Title.File:","Menu.File:",NULL,NULL);
	personmenu=Menu_CreateFromMsgs("Title.Person:","Menu.Person:",Graphics_PersonMenuClick,NULL);
	mainmenu=Menu_CreateFromMsgs("Title.Main:","Menu.Main:",Graphics_MainMenuClick,NULL);
	selectmenu=Menu_CreateFromMsgs("Title.Select:","Menu.Select:",Graphics_SelectMenuClick,NULL);
	lineupmenu=Menu_CreateFromMsgs("Title.LineUp:","Menu.LineUp:",Graphics_LineUpMenuClick,NULL);
	Menu_AddSubMenu(mainmenu,mainmenu_PERSON,personmenu);
	Menu_AddSubMenu(mainmenu,mainmenu_FILE,filemenu);
	Menu_AddSubMenu(mainmenu,mainmenu_LINEUP,lineupmenu);
	Menu_AddSubMenu(filemenu,filemenu_EXPORT,exportmenu);
	Menu_AddSubMenu(filemenu,filemenu_INFO,(menu_ptr)fileinfowin);
	Menu_AddSubMenu(mainmenu,mainmenu_SELECT,selectmenu);
/*	Menu_Warn(filemenu,filemenu_INFO,TRUE,Database_GetInfo,...);
*/	{
/*		window_handle savewin=Save_CreateWindow(0xFFF,FALSE,1024,TRUE,SaveHandler,NULL,NULL);
		Menu_AddSubMenu(filemenu,filemenu_SAVE,(menu_ptr)savewin);
*/	}
}

