/*
	FT - Graphics
	© Alex Waugh 1999

	$Log: Windows.c,v $
	Revision 1.15  1999/10/25 16:08:48  AJW
	Added centering of fields

	Revision 1.14  1999/10/24 23:16:38  AJW
	Added person to layout when adding a second marriage
	Added person to layout in a better position when adding a child

	Revision 1.13  1999/10/24 18:38:22  AJW
	Set title of windows

	Revision 1.12  1999/10/24 18:15:39  AJW
	Added extra field types

	Revision 1.11  1999/10/24 13:16:32  AJW
	Disabled dragging on all but normal windows

	Revision 1.10  1999/10/12 16:30:08  AJW
	Added Debug code for Ancestor layout

	Revision 1.9  1999/10/12 14:26:32  AJW
	Modified to use Config

	Revision 1.8  1999/10/11 22:15:36  AJW
	Modified to use Error2

	Revision 1.7  1999/10/10 20:54:56  AJW
	Modified to use Desk

	Revision 1.6  1999/10/02 18:46:15  AJW
	Added unlinking of people

	Revision 1.5  1999/09/29 17:31:05  AJW
	Add people to layout when linking/marrying etc

	Revision 1.4  1999/09/28 15:43:07  AJW
	Added Graphics_RedrawPerson and Graphics_RedrawMarriage
	Added dragging a box to select people

	Revision 1.3  1999/09/28 14:16:10  AJW
	Added centering marriage over children when dragging

	Revision 1.2  1999/09/27 16:56:43  AJW
	Added centering siblings under marriage when dragging

	Revision 1.1  1999/09/27 15:33:01  AJW
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
#include "AJWLib.Str.h"
#include "AJWLib.Draw.h"
#include "AJWLib.DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "GConfig.h"
#include "Config.h"
#include "Layout.h"

/*	Macros  */

#define MAXWINDOWS 10
#define REDRAWOVERLAP 4

#define SWI_ColourTrans_SetFontColours 0x4074F
#define SWI_Font_ScanString 0x400A1

#define EORCOLOUR 0xFFFFFF00
#define EORCOLOURRED 0xFFFF0000

#define mainmenu_FILE 0
#define mainmenu_PERSON 1
#define mainmenu_NEWVIEW 3
#define mainmenu_SEARCH 4
#define mainmenu_REPORTS 5
#define mainmenu_SELECT 2

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

#define addparents_SECONDMARRIAGE 1

typedef struct windowdata {
	Desk_window_handle handle;
	wintype type;
	elementptr person;
	int generations;
	layout *layout;
} windowdata;

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
extern layout *layouts;
#endif

static windowdata windows[MAXWINDOWS];
static int numwindows;
static Desk_window_handle addparentswin,newviewwin,fileinfowin;
static Desk_menu_ptr mainmenu,filemenu,exportmenu,personmenu,selectmenu;
static elementptr addparentsperson,addparentschild,menuoverperson,newviewperson;
static windowdata *menuoverwindow;
static os_trfm matrix;
extern graphics graphicsdata;
/*Make menuoverperson static local var to be passed as a reference?*/

void Graphics_RedrawPerson(Desk_window_handle win,personlayout *person)
{
	Desk_Window_ForceRedraw(win,person->x-REDRAWOVERLAP,person->y-REDRAWOVERLAP,person->x+Graphics_PersonWidth()+REDRAWOVERLAP,person->y+Graphics_PersonHeight()+REDRAWOVERLAP);
}

void Graphics_RedrawMarriage(Desk_window_handle win,marriagelayout *marriage)
{
	Desk_Window_ForceRedraw(win,marriage->x-REDRAWOVERLAP,marriage->y-REDRAWOVERLAP,marriage->x+Graphics_MarriageWidth()+REDRAWOVERLAP,marriage->y+Graphics_PersonHeight()+REDRAWOVERLAP);
}

int AJWLib_Font_GetWidth(Desk_font_handle handle,const char *str)
{
	int width,height;
	Desk_Error2_CheckOS(Desk_SWI(5,5,SWI_Font_ScanString,handle,str,1<<8,INFINITY,INFINITY,NULL,NULL,NULL,&width,&height));
	Desk_Font_ConverttoOS(width,height,&width,&height);
	return width;
}

void Graphics_PlotPerson(elementptr person,int x,int y,Desk_bool child,Desk_bool selected)
{
	int i;
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		int xcoord=0;
		switch (graphicsdata.person[i].type) {
			case graphictype_RECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotRectangle(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_FILLEDRECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotRectangleFilled(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,&matrix);
				break;
			case graphictype_CHILDLINE:
				if (!child) break;
				/*A line that is only plotted if there is a child and child==Desk_FALSE ?*/
			case graphictype_LINE:
				Desk_ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotLine(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,x+graphicsdata.person[i].details.linebox.x1,y+graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_CENTREDTEXTLABEL:
				xcoord=-AJWLib_Font_GetWidth(graphicsdata.person[i].details.textlabel.properties.font->handle,graphicsdata.person[i].details.textlabel.text)/2;
			case graphictype_TEXTLABEL:
				Desk_Font_SetFont(graphicsdata.person[i].details.textlabel.properties.font->handle);
				Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.person[i].details.textlabel.properties.bgcolour,graphicsdata.person[i].details.textlabel.properties.colour,14);
				Desk_Font_Paint(graphicsdata.person[i].details.textlabel.text,Desk_font_plot_OSCOORS,x+xcoord+graphicsdata.person[i].details.textlabel.properties.x,y+graphicsdata.person[i].details.textlabel.properties.y);
				break;
		}
	}
	for (i=0;i<NUMPERSONFIELDS;i++) {
		if (graphicsdata.personfields[i].plot) {
			char fieldtext[256]=""; /*what is max field length?*/
			int xcoord=0;
			switch (i) {
				case personfieldtype_SURNAME:
					strcat(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_FORENAME:
					strcat(fieldtext,Database_GetPersonData(person)->forename);
					break;
				case personfieldtype_MIDDLENAMES:
					strcat(fieldtext,Database_GetPersonData(person)->middlenames);
					break;
				case personfieldtype_TITLEDNAME:
					strcat(fieldtext,Database_GetPersonData(person)->title);
				case personfieldtype_NAME:
					strcat(fieldtext,Database_GetPersonData(person)->forename);
					strcat(fieldtext," ");
					strcat(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_TITLEDFULLNAME:
					strcat(fieldtext,Database_GetPersonData(person)->title);
				case personfieldtype_FULLNAME:
					strcat(fieldtext,Database_GetPersonData(person)->forename);
					strcat(fieldtext," ");
					strcat(fieldtext,Database_GetPersonData(person)->middlenames);
					strcat(fieldtext," ");
					strcat(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_TITLE:
					strcat(fieldtext,Database_GetPersonData(person)->title);
					break;
/*				case personfieldtype_SEX:
					strcat(fieldtext,Database_GetPersonData(person)->sex);
					break;
				case personfieldtype_DOB:
					strcat(fieldtext,Database_GetPersonData(person)->dob);
					break;
				case personfieldtype_DOD:
					strcat(fieldtext,Database_GetPersonData(person)->dod);
					break;
*/				case personfieldtype_BIRTHPLACE:
					strcat(fieldtext,Database_GetPersonData(person)->placeofbirth);
					break;
				case personfieldtype_USER1:
					strcat(fieldtext,Database_GetPersonData(person)->userdata[0]);
					break;
				case personfieldtype_USER2:
					strcat(fieldtext,Database_GetPersonData(person)->userdata[1]);
					break;
				case personfieldtype_USER3:
					strcat(fieldtext,Database_GetPersonData(person)->userdata[2]);
					break;
				default:
					strcat(fieldtext,"Unimplemented");
			}
			Desk_Font_SetFont(graphicsdata.personfields[i].textproperties.font->handle);
			Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.personfields[i].textproperties.bgcolour,graphicsdata.personfields[i].textproperties.colour,14));
			if (graphicsdata.personfields[i].type==graphictype_CENTREDFIELD) xcoord=-AJWLib_Font_GetWidth(graphicsdata.personfields[i].textproperties.font->handle,fieldtext)/2;
			Desk_Font_Paint(fieldtext,Desk_font_plot_OSCOORS,x+xcoord+graphicsdata.personfields[i].textproperties.x,y+graphicsdata.personfields[i].textproperties.y);
		}
	}
	if (selected) {
		Desk_ColourTrans_SetGCOL(EORCOLOUR,0,3);
		AJWLib_Draw_PlotRectangleFilled(x,y,Graphics_PersonWidth(),Graphics_PersonHeight(),&matrix);

	}
}

void Graphics_PlotMarriage(int x,int y,elementptr marriage,Desk_bool childline,Desk_bool selected)
{
	int i;
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		switch (graphicsdata.marriage[i].type) {
			case graphictype_RECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotRectangle(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_FILLEDRECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotRectangleFilled(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,&matrix);
				break;
			case graphictype_CHILDLINE:
				if (!childline) break;
			case graphictype_LINE:
				Desk_ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotLine(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,x+graphicsdata.marriage[i].details.linebox.x1,y+graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_TEXTLABEL:
				Desk_Font_SetFont(graphicsdata.marriage[i].details.textlabel.properties.font->handle);
				Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.marriage[i].details.textlabel.properties.bgcolour,graphicsdata.marriage[i].details.textlabel.properties.colour,14);
				Desk_Font_Paint(graphicsdata.marriage[i].details.textlabel.text,Desk_font_plot_OSCOORS,x+graphicsdata.marriage[i].details.textlabel.properties.x,y+graphicsdata.marriage[i].details.textlabel.properties.y);
				break;
		}
	}
	for (i=0;i<NUMMARRIAGEFIELDS;i++) {
		if (graphicsdata.marriagefields[i].plot) {
			char fieldtext[256]=""; /*what is max field length?*/
			switch (i) {
				case marriagefieldtype_PLACE:
					strcat(fieldtext,Database_GetMarriageData(marriage)->place);
					break;
/*				case marriagefieldtype_DATE:
					strcat(fieldtext,Database_GetMarriageData(marriage)->place);
					break;
*/				default:
					strcat(fieldtext,"Unimplemented");
			}
			Desk_Font_SetFont(graphicsdata.marriagefields[i].textproperties.font->handle);
			Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.marriagefields[i].textproperties.bgcolour,graphicsdata.marriagefields[i].textproperties.colour,14));
			Desk_Font_Paint(fieldtext,Desk_font_plot_OSCOORS,x+graphicsdata.marriagefields[i].textproperties.x,y+graphicsdata.marriagefields[i].textproperties.y);
		}
	}
	if (selected) {
		Desk_ColourTrans_SetGCOL(EORCOLOUR,0,3);
		AJWLib_Draw_PlotRectangleFilled(x,y,Graphics_MarriageWidth(),Graphics_PersonHeight(),&matrix);

	}
}

void Graphics_PlotChildren(int leftx,int rightx,int y)
{
	Desk_ColourTrans_SetGCOL(graphicsdata.siblinglinecolour,0/*or 1<<8 to use ECFs ?*/,0);
	AJWLib_Draw_PlotLine(leftx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),rightx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),graphicsdata.siblinglinethickness,&matrix);
}

static Desk_bool Graphics_Redraw(Desk_event_pollblock *block,windowdata *windowdata)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=block->data.openblock.window;
	Desk_Wimp_RedrawWindow(&blk,&more);
	while (more) {
		int i=0;
#if DEBUG
Desk_ColourTrans_SetGCOL(0x00000000,0,0);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
Desk_ColourTrans_SetGCOL(0xFF000000,0,0);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+1000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-1000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
Desk_ColourTrans_SetGCOL(0x0000FF00,0,0);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+2000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-2000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
Desk_ColourTrans_SetGCOL(0x00FF0000,0,0);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+3000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-3000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
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
		Desk_Wimp_GetRectangle(&blk,&more);
		/*Use clip rectangle??*/
	}
	return Desk_TRUE;
}

static Desk_bool Graphics_RedrawAddParents(Desk_event_pollblock *block,void *ref)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=block->data.openblock.window;
	Desk_Wimp_RedrawWindow(&blk,&more);
	while (more) {
		Graphics_PlotPerson(addparentsperson,blk.rect.min.x-blk.scroll.x+332,blk.rect.max.y-blk.scroll.y-16-Graphics_PersonHeight(),Desk_FALSE,Desk_FALSE);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
	return Desk_TRUE;
}

Desk_bool Graphics_AddParentsClick(Desk_event_pollblock *block,void *ref)
{
	int x=-INFINITY,window=-1,i,y;
	elementptr person;
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	Desk_Window_Hide(addparentswin);
	Database_Marry(addparentschild,addparentsperson);
	for (i=0;i<MAXWINDOWS;i++) if (windows[i].handle && windows[i].type==wintype_NORMAL) window=i;
	if (window==-1) return Desk_FALSE;
	person=addparentschild;
	do {
		for (i=0;i<windows[window].layout->numpeople;i++) {
			if (windows[window].layout->person[i].person==person) {
				if (windows[window].layout->person[i].x>x) x=windows[window].layout->person[i].x;
				y=windows[window].layout->person[i].y;
			}
		}
	} while ((person=Database_GetMarriageLtoR(person))!=none);
	person=addparentschild;
	while ((person=Database_GetMarriageRtoL(person))!=none) {
		for (i=0;i<windows[window].layout->numpeople;i++) {
			if (windows[window].layout->person[i].person==person) {
				if (windows[window].layout->person[i].x>x) x=windows[window].layout->person[i].x;
				y=windows[window].layout->person[i].y;
			}
		}
	}
	AJWLib_Assert(x!=-INFINITY);
	x+=Graphics_PersonWidth()+Graphics_MarriageWidth()+Graphics_SecondMarriageGap();
	Layout_AddPerson(windows[window].layout,addparentsperson,x,y);
	Layout_AddMarriage(windows[window].layout,Database_GetMarriage(addparentsperson),x-Graphics_MarriageWidth(),y);
	/*What happens if a addparentschild has since been unlinked?  hide addparents win whenever structure changes?*/
	return Desk_TRUE;
}

void Graphics_UnselectAll(windowdata *windowdata)
{
	int i;
	for (i=0;i<windowdata->layout->numpeople;i++) {
		if (windowdata->layout->person[i].selected) {
			windowdata->layout->person[i].selected=Desk_FALSE;
			Graphics_RedrawPerson(windowdata->handle,windowdata->layout->person+i);
		}
	}
	for (i=0;i<windowdata->layout->nummarriages;i++) {
		if (windowdata->layout->marriage[i].selected) {
			windowdata->layout->marriage[i].selected=Desk_FALSE;
			Graphics_RedrawMarriage(windowdata->handle,windowdata->layout->marriage+i);
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
	Desk_wimp_rect box;
	box=Layout_FindExtent(windowdata->layout,Desk_FALSE);
	Desk_Window_SetExtent(windowdata->handle,box.min.x-Graphics_WindowBorder(),box.min.y-Graphics_WindowBorder(),box.max.x+Graphics_WindowBorder(),box.max.y+Graphics_WindowBorder());
}

void Graphics_PlotDragBox(dragdata *dragdata)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=dragdata->windowdata->handle;
	blk.rect.min.x=-INFINITY;
	blk.rect.max.x=INFINITY;
	blk.rect.min.y=-INFINITY;
	blk.rect.max.y=INFINITY;
	Desk_Wimp_UpdateWindow(&blk,&more);
	while (more) {
		Desk_ColourTrans_SetGCOL(EORCOLOUR,0,3);
		AJWLib_Draw_PlotRectangle(blk.rect.min.x-blk.scroll.x+dragdata->oldmousex+dragdata->coords.min.x+dragdata->oldoffset,blk.rect.max.y-blk.scroll.y+dragdata->oldmousey+dragdata->coords.min.y,dragdata->coords.max.x-dragdata->coords.min.x,dragdata->coords.max.y-dragdata->coords.min.y,0,&matrix);
		Desk_ColourTrans_SetGCOL(EORCOLOURRED,0,3);
		AJWLib_Draw_PlotRectangle(blk.rect.min.x-blk.scroll.x+dragdata->oldmousex+dragdata->oldoffset-dragdata->personoffset,blk.rect.max.y-blk.scroll.y+dragdata->persony,dragdata->marriage ? Graphics_MarriageWidth() : Graphics_PersonWidth(),Graphics_PersonHeight(),0,&matrix);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
}

void Graphics_DragEnd(void *ref)
{
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	elementptr initialperson=dragdata->person;
	int window=-1,mousex,mousey,i;
	Desk_Wimp_GetPointerInfo(&mouseblk);
	if (mouseblk.window==addparentswin) {
		if (initialperson==addparentsperson) return;
		if (Database_IsUnlinked(initialperson)) {
			int childx,childy;
			Desk_Window_Hide(addparentswin);
			Database_AddParents(addparentschild,addparentsperson,initialperson);
			for (i=0;i<MAXWINDOWS;i++) if (windows[i].handle && windows[i].type==wintype_NORMAL) window=i;
			if (window==-1) return;
			childx=Layout_FindXCoord(windows[window].layout,addparentschild);
			childy=Layout_FindYCoord(windows[window].layout,addparentschild);
			/*what if an error occours - some are added,some not?*/
			Layout_AddPerson(windows[window].layout,addparentsperson,childx-(Graphics_PersonWidth()+Graphics_MarriageWidth())/2,childy+Graphics_PersonHeight()+Graphics_GapHeightAbove()+Graphics_GapHeightBelow());
			Layout_AddPerson(windows[window].layout,initialperson,childx+(Graphics_PersonWidth()+Graphics_MarriageWidth())/2,childy+Graphics_PersonHeight()+Graphics_GapHeightAbove()+Graphics_GapHeightBelow());
			Layout_AddMarriage(windows[window].layout,Database_GetMarriage(initialperson),childx+(Graphics_PersonWidth()-Graphics_MarriageWidth())/2,childy+Graphics_PersonHeight()+Graphics_GapHeightAbove()+Graphics_GapHeightBelow());
			Layout_AlterChildline(windows[window].layout,addparentschild,Desk_TRUE);
			return;
		}
	}
	for (i=0;i<MAXWINDOWS;i++) if (mouseblk.window==windows[i].handle) window=i;
	if (window==-1) return;
	if (dragdata->windowdata->type==wintype_NORMAL) {
		int mousex;
		/*act as if drag had ended on same win as started on*/
		if (dragdata->plotted) Graphics_PlotDragBox(dragdata);
		Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
		mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
		Graphics_AddSelected(dragdata->windowdata->layout,mousex+dragdata->oldoffset-dragdata->origmousex);
		Layout_LayoutLines(dragdata->windowdata->layout);
		Graphics_ResizeWindow(dragdata->windowdata);
		Desk_Window_ForceRedraw(dragdata->windowdata->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
	} else if (dragdata->windowdata->type==wintype_UNLINKED) {
		if (windows[window].type==wintype_NORMAL) {
			AJWLib_Assert(Database_IsUnlinked(initialperson));
			if (Database_GetLinked()==none) {
				Database_LinkPerson(initialperson);
				Layout_AddPerson(windows[window].layout,initialperson,0,0);
				return;
			}
			Desk_Window_GetCoords(mouseblk.window,&blk);
			mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
			mousey=mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y);
			for (i=0;i<windows[window].layout->numpeople;i++) {
				if (mousex>=windows[window].layout->person[i].x && mousex<=windows[window].layout->person[i].x+Graphics_PersonWidth()) {
					if (mousey>=windows[window].layout->person[i].y && mousey<=windows[window].layout->person[i].y+Graphics_PersonHeight()) {
						elementptr finalperson=windows[window].layout->person[i].person;
						AJWLib_Assert(initialperson!=finalperson);
						if (Database_GetMarriage(finalperson) && Database_GetFather(finalperson)==none) {
							addparentsperson=initialperson;
							addparentschild=finalperson;
							Desk_Window_SetExtent(addparentswin,0,304>Graphics_PersonHeight()+32 ? -304 : -Graphics_PersonHeight()-32,348+Graphics_PersonWidth(),0);
							Desk_Window_ForceRedraw(addparentswin,-INFINITY,-INFINITY,INFINITY,INFINITY);
							Desk_Window_Show(addparentswin,Desk_open_CENTERED);
						} else {
							Database_Marry(finalperson,initialperson);
							Layout_AddPerson(windows[window].layout,initialperson,windows[window].layout->person[i].x+Graphics_PersonWidth()+Graphics_MarriageWidth(),windows[window].layout->person[i].y);
							Layout_AddMarriage(windows[window].layout,Database_GetMarriage(finalperson),windows[window].layout->person[i].x+Graphics_PersonWidth(),windows[window].layout->person[i].y);
						}
						return;
					}
				}
			}
			for (i=0;i<windows[window].layout->nummarriages;i++) {
				if (mousex>=windows[window].layout->marriage[i].x && mousex<=windows[window].layout->marriage[i].x+Graphics_MarriageWidth()) {
					if (mousey>=windows[window].layout->marriage[i].y && mousey<=windows[window].layout->marriage[i].y+Graphics_PersonHeight()) {
						elementptr person;
						int x=-INFINITY;
						Database_AddChild(windows[window].layout->marriage[i].marriage,initialperson);
						windows[window].layout->marriage[i].childline=Desk_TRUE;
						person=initialperson;
						do {
							int i;
							for (i=0;i<windows[window].layout->numpeople;i++) {
								if (windows[window].layout->person[i].person==person) {
									if (windows[window].layout->person[i].x>x) x=windows[window].layout->person[i].x;
								}
							}
						} while ((person=Database_GetSiblingLtoR(person))!=none);
						person=initialperson;
						while ((person=Database_GetSiblingRtoL(person))!=none) {
							int i;
							for (i=0;i<windows[window].layout->numpeople;i++) {
								if (windows[window].layout->person[i].person==person) {
									if (windows[window].layout->person[i].x>x) x=windows[window].layout->person[i].x;
								}
							}
						}
						if (x==-INFINITY) x=windows[window].layout->marriage[i].x-Graphics_PersonWidth()-Graphics_GapWidth()+Graphics_MarriageWidth()/2-Graphics_PersonWidth()/2;
						Layout_AddPerson(windows[window].layout,initialperson,x+Graphics_PersonWidth()+Graphics_GapWidth(),windows[window].layout->marriage[i].y-Graphics_PersonHeight()-Graphics_GapHeightAbove()-Graphics_GapHeightBelow());
						return;
					}
				}
			}
		}
	}
}

void Graphics_SelectDragEnd(void *ref)
{
	Desk_mouse_block mouseblk;
	Desk_convert_block blk;
	dragdata *dragdata=ref;
	int mousex,mousey,i;
	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Window_GetCoords(dragdata->windowdata->handle,&blk);
	mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
	mousey=mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y);
	for (i=0;i<dragdata->windowdata->layout->numpeople;i++) {
		if (mousex>dragdata->windowdata->layout->person[i].x && dragdata->oldmousex<dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth() || mousex<dragdata->windowdata->layout->person[i].x+Graphics_PersonWidth() && dragdata->oldmousex>dragdata->windowdata->layout->person[i].x) {
			if (mousey>dragdata->windowdata->layout->person[i].y && dragdata->oldmousey<dragdata->windowdata->layout->person[i].y+Graphics_PersonHeight() || mousey<dragdata->windowdata->layout->person[i].y+Graphics_PersonHeight() && dragdata->oldmousey>dragdata->windowdata->layout->person[i].y) {
				dragdata->windowdata->layout->person[i].selected=(Desk_bool)!dragdata->windowdata->layout->person[i].selected;
				Graphics_RedrawPerson(dragdata->windowdata->handle,dragdata->windowdata->layout->person+i);
			}
		}
	}
	for (i=0;i<dragdata->windowdata->layout->nummarriages;i++) {
		if (mousex>dragdata->windowdata->layout->marriage[i].x && dragdata->oldmousex<dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth() || mousex<dragdata->windowdata->layout->marriage[i].x+Graphics_MarriageWidth() && dragdata->oldmousex>dragdata->windowdata->layout->marriage[i].x) {
			if (mousey>dragdata->windowdata->layout->marriage[i].y && dragdata->oldmousey<dragdata->windowdata->layout->marriage[i].y+Graphics_PersonHeight() || mousey<dragdata->windowdata->layout->marriage[i].y+Graphics_PersonHeight() && dragdata->oldmousey>dragdata->windowdata->layout->marriage[i].y) {
				dragdata->windowdata->layout->marriage[i].selected=(Desk_bool)!dragdata->windowdata->layout->marriage[i].selected;
				Graphics_RedrawMarriage(dragdata->windowdata->handle,dragdata->windowdata->layout->marriage+i);
			}
		}
	}
}

void Graphics_GetOffset(dragdata *dragdata)
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

void Graphics_DragFn(void *ref)
{
	dragdata *dragdata=ref;
	Desk_mouse_block mouseblk;
	Desk_window_state blk;
	int mousex;
	if (dragdata->windowdata->type!=wintype_NORMAL) return;
	if (!dragdata->plotted) {
		Graphics_GetOffset(dragdata);
		Graphics_PlotDragBox(dragdata);
		dragdata->plotted=Desk_TRUE;
	}
	Desk_Wimp_GetPointerInfo(&mouseblk);
	Desk_Wimp_GetWindowState(dragdata->windowdata->handle,&blk);
	mousex=mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x);
	if (mousex!=dragdata->oldmousex) {
		Graphics_PlotDragBox(dragdata);
		dragdata->oldmousex=mousex;
		Graphics_GetOffset(dragdata);
		Graphics_PlotDragBox(dragdata);
	}
	if (mouseblk.pos.x-blk.openblock.screenrect.min.x<Config_ScrollDistance()) {
		Graphics_PlotDragBox(dragdata);
		dragdata->plotted=Desk_FALSE;
		blk.openblock.scroll.x-=(Config_ScrollSpeed()*(Config_ScrollDistance()-(mouseblk.pos.x-blk.openblock.screenrect.min.x)))/20;
		Desk_Wimp_OpenWindow(&blk.openblock);
		mousex=mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x);
		dragdata->oldmousex=mousex;
	} else if (blk.openblock.screenrect.max.x-mouseblk.pos.x<Config_ScrollDistance()) {
		Graphics_PlotDragBox(dragdata);
		dragdata->plotted=Desk_FALSE;
		blk.openblock.scroll.x+=(Config_ScrollSpeed()*(Config_ScrollDistance()-(blk.openblock.screenrect.max.x-mouseblk.pos.x)))/20;
		Desk_Wimp_OpenWindow(&blk.openblock);
		mousex=mouseblk.pos.x-(blk.openblock.screenrect.min.x-blk.openblock.scroll.x);
		dragdata->oldmousex=mousex;
	}
}

void Graphics_StartDragNormal(elementptr person,int x,int y,windowdata *windowdata,Desk_bool marriage)
{
	static dragdata dragdata;
	Desk_drag_block dragblk;
	Desk_convert_block blk;
	Desk_mouse_block mouseblk;
	int mousex,mousey;
	Desk_Window_GetCoords(windowdata->handle,&blk);
	Desk_Wimp_GetPointerInfo(&mouseblk);
	mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
	mousey=mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y);
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
	Desk_Drag_SetHandlers(Graphics_DragFn,Graphics_DragEnd,&dragdata);
}

void Graphics_StartDragUnlinked(elementptr person,int x,int y,windowdata *windowdata)
{
	static dragdata dragdata;
	Desk_drag_block dragblk;
	Desk_convert_block blk;
	Desk_mouse_block mouseblk;
	int mousex,mousey;
	Desk_Window_GetCoords(windowdata->handle,&blk);
	Desk_Wimp_GetPointerInfo(&mouseblk);
	mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
	mousey=mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y);
	dragblk.type=Desk_drag_FIXEDBOX;
	dragblk.screenrect.min.x=x+(blk.screenrect.min.x-blk.scroll.x);
	dragblk.screenrect.max.x=x+Graphics_PersonWidth()+(blk.screenrect.min.x-blk.scroll.x);
	dragblk.screenrect.min.y=y+(blk.screenrect.max.y-blk.scroll.y);
	dragblk.screenrect.max.y=y+Graphics_PersonHeight()+(blk.screenrect.max.y-blk.scroll.y);
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
	Desk_Drag_SetHandlers(NULL,Graphics_DragEnd,&dragdata);
}

void Graphics_StartDragSelect(windowdata *windowdata)
{
	static dragdata dragdata;
	Desk_drag_block dragblk;
	Desk_convert_block blk;
	Desk_mouse_block mouseblk;
	int mousex,mousey;
	Desk_Window_GetCoords(windowdata->handle,&blk);
	Desk_Wimp_GetPointerInfo(&mouseblk);
	mousex=mouseblk.pos.x-(blk.screenrect.min.x-blk.scroll.x);
	mousey=mouseblk.pos.y-(blk.screenrect.max.y-blk.scroll.y);
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
	Desk_Drag_SetHandlers(NULL,Graphics_SelectDragEnd,&dragdata);
}

Desk_bool Graphics_MouseClick(Desk_event_pollblock *block,void *ref)
{
	windowdata *windowdata=ref;
	int mousex,mousey,i;
	Desk_convert_block blk;
	Desk_Window_GetCoords(windowdata->handle,&blk);
	mousex=block->data.mouse.pos.x-(blk.screenrect.min.x-blk.scroll.x);
	mousey=block->data.mouse.pos.y-(blk.screenrect.max.y-blk.scroll.y);
	menuoverperson=none;
	AJWLib_Menu_Shade(personmenu,personmenu_ADD);
	AJWLib_Menu_Shade(personmenu,personmenu_DELETE);
	AJWLib_Menu_Shade(personmenu,personmenu_UNLINK);
	AJWLib_Menu_Shade(mainmenu,mainmenu_PERSON);
	AJWLib_Menu_Shade(mainmenu,mainmenu_SELECT);
	for (i=0;i<windowdata->layout->numpeople;i++) {
		if (mousex>=windowdata->layout->person[i].x && mousex<=windowdata->layout->person[i].x+Graphics_PersonWidth()) {
			if (mousey>=windowdata->layout->person[i].y && mousey<=windowdata->layout->person[i].y+Graphics_PersonHeight()) {
				if (block->data.mouse.button.data.clickselect) {
					if (windowdata->type==wintype_NORMAL) {
						if (!windowdata->layout->person[i].selected) {
							Graphics_UnselectAll(windowdata);
							windowdata->layout->person[i].selected=Desk_TRUE;
							Graphics_RedrawPerson(windowdata->handle,windowdata->layout->person+i);
						}
						return Desk_TRUE;
					}
				} else if (block->data.mouse.button.data.clickadjust) {
					if (windowdata->type==wintype_NORMAL) {
						windowdata->layout->person[i].selected=(Desk_bool)!windowdata->layout->person[i].selected;
						Graphics_RedrawPerson(windowdata->handle,windowdata->layout->person+i);
						return Desk_TRUE;
					}
				} else if (block->data.mouse.button.data.menu) {
					elementptr marriage;
					menuoverperson=windowdata->layout->person[i].person;
					menuoverwindow=windowdata;
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
					Database_EditPerson(windowdata->layout->person[i].person);
					return Desk_TRUE;
				} else if (block->data.mouse.button.data.adjust) {
					if (windowdata->type==wintype_DESCENDENTS || windowdata->type==wintype_ANCESTORS) {
						windowdata->person=windowdata->layout->person[i].person;
						Graphics_Relayout(); /*Rather inefficient*/
					}
					return Desk_TRUE;
				} else if (block->data.mouse.button.data.dragselect) {
					if (windowdata->type==wintype_NORMAL) {
						Graphics_StartDragNormal(windowdata->layout->person[i].person,windowdata->layout->person[i].x,windowdata->layout->person[i].y,windowdata,Desk_FALSE);
						return Desk_TRUE;
					} else if (windowdata->type==wintype_UNLINKED) {
						Graphics_StartDragUnlinked(windowdata->layout->person[i].person,windowdata->layout->person[i].x,windowdata->layout->person[i].y,windowdata);
						return Desk_TRUE;
					}
				}
			}
		}
	}
	if (block->data.mouse.button.data.menu) {
		if (windowdata->type==wintype_UNLINKED) {
			AJWLib_Menu_UnShade(mainmenu,mainmenu_PERSON);
			AJWLib_Menu_UnShade(personmenu,personmenu_ADD);
		}
		Desk_Menu_Show(mainmenu,block->data.mouse.pos.x,block->data.mouse.pos.y);
		return Desk_TRUE;
	}
	for (i=0;i<windowdata->layout->nummarriages;i++) {
		if (mousex>=windowdata->layout->marriage[i].x && mousex<=windowdata->layout->marriage[i].x+Graphics_MarriageWidth()) {
			if (mousey>=windowdata->layout->marriage[i].y && mousey<=windowdata->layout->marriage[i].y+Graphics_PersonHeight()) {
				if (block->data.mouse.button.data.select) {
					Database_EditMarriage(windowdata->layout->marriage[i].marriage);
					return Desk_TRUE;
				} else if (block->data.mouse.button.data.clickselect) {
					if (windowdata->type==wintype_NORMAL) {
						if (!windowdata->layout->marriage[i].selected) {
							Graphics_UnselectAll(windowdata);
							windowdata->layout->marriage[i].selected=Desk_TRUE;
							Graphics_RedrawMarriage(windowdata->handle,windowdata->layout->marriage+i);
						}
						return Desk_TRUE;
					}
				} else if (block->data.mouse.button.data.clickadjust) {
					if (windowdata->type==wintype_NORMAL) {
						windowdata->layout->marriage[i].selected=(Desk_bool)!windowdata->layout->marriage[i].selected;
						Graphics_RedrawMarriage(windowdata->handle,windowdata->layout->marriage+i);
						return Desk_TRUE;
					}
				} else if (block->data.mouse.button.data.dragselect) {
					if (windowdata->type==wintype_NORMAL) {
						Graphics_StartDragNormal(windowdata->layout->marriage[i].marriage,windowdata->layout->marriage[i].x,windowdata->layout->marriage[i].y,windowdata,Desk_TRUE);
						return Desk_TRUE;
					}
				}
			}
		}
	}
	if (windowdata->type==wintype_NORMAL) {
		if (block->data.mouse.button.data.clickselect) {
			Graphics_UnselectAll(windowdata);
			return Desk_TRUE;
		} else if (block->data.mouse.button.data.dragselect) {
			Graphics_UnselectAll(windowdata);
			Graphics_StartDragSelect(windowdata);
			return Desk_TRUE;
		} else if (block->data.mouse.button.data.dragadjust) {
			Graphics_StartDragSelect(windowdata);
			return Desk_TRUE;
		}
	}
	return Desk_FALSE;
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
				case wintype_ANCESTORS:
					windows[i].layout=Layout_LayoutAncestors(windows[i].person,windows[i].generations);
					break;
				case wintype_UNLINKED:
					windows[i].layout=Layout_LayoutUnlinked();
					break;
			}
			Graphics_ResizeWindow(&windows[i]);
			Desk_Window_ForceRedraw(windows[i].handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
		}
	}
}

void Graphics_OpenWindow(wintype type,elementptr person,int generations)
{
	int newwindow;
	char str[256]="";
	if (numwindows>MAXWINDOWS) {
		Desk_Error2_HandleText("Too many windows");
		return;
	}
	for (newwindow=0;windows[newwindow].handle!=0 && newwindow<MAXWINDOWS;newwindow++);
	if (newwindow==MAXWINDOWS) {
		Desk_Error2_HandleText("Too many windows");
		return;
	}
	windows[newwindow].handle=Desk_Window_CreateAndShow("Main",Desk_template_TITLEMIN,Desk_open_CENTERED);
	windows[newwindow].type=type;
	windows[newwindow].person=person;
	windows[newwindow].generations=generations;
	switch (type) {
		case wintype_NORMAL:
/*			Desk_Window_SetTitle(windows[newwindow].handle,Database_GetFilename());
*/#if DEBUG
Desk_Event_Claim(Desk_event_CLICK,windows[newwindow].handle,Desk_event_ANY,Graphics_MouseClick,&windows[newwindow]);
Desk_Event_Claim(Desk_event_REDRAW,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Graphics_Redraw,&windows[newwindow]);
windows[newwindow].layout=layouts;
Layout_LayoutNormal();
#else
			windows[newwindow].layout=Layout_LayoutNormal();
#endif
		break;
		case wintype_DESCENDENTS:
			Desk_Msgs_Lookup("Win.Desc:",str,255);
			strcat(str," ");
			strcat(str,Database_GetPersonData(person)->forename);
			strcat(str," ");
			strcat(str,Database_GetPersonData(person)->surname);
			Desk_Window_SetTitle(windows[newwindow].handle,str);
			windows[newwindow].layout=Layout_LayoutDescendents(person,generations);
		break;
		case wintype_ANCESTORS:
			Desk_Msgs_Lookup("Win.Anc:",str,255);
			strcat(str," ");
			strcat(str,Database_GetPersonData(person)->forename);
			strcat(str," ");
			strcat(str,Database_GetPersonData(person)->surname);
			Desk_Window_SetTitle(windows[newwindow].handle,str);
#if DEBUG
Desk_Event_Claim(Desk_event_CLICK,windows[newwindow].handle,Desk_event_ANY,Graphics_MouseClick,&windows[newwindow]);
Desk_Event_Claim(Desk_event_REDRAW,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Graphics_Redraw,&windows[newwindow]);
windows[newwindow].layout=layouts;
Layout_LayoutAncestors(person,generations);
#else
			windows[newwindow].layout=Layout_LayoutAncestors(person,generations);
#endif
		break;
		case wintype_UNLINKED:
			Desk_Msgs_Lookup("Win.Unlkd:Unlinked",str,255);
			Desk_Window_SetTitle(windows[newwindow].handle,str);
			windows[newwindow].layout=Layout_LayoutUnlinked();
		break;
	}
	Graphics_ResizeWindow(&windows[newwindow]);
	/*Register handlers for this window*/
	Desk_Event_Claim(Desk_event_CLICK,windows[newwindow].handle,Desk_event_ANY,Graphics_MouseClick,&windows[newwindow]);
	Desk_Event_Claim(Desk_event_REDRAW,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Graphics_Redraw,&windows[newwindow]);
}

void Graphics_MainMenuClick(int entry,void *ref)
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
*/				strcpy(buffer,Database_GetPersonData(menuoverperson)->forename);
				strcat(buffer," ");
				strcat(buffer,Database_GetPersonData(menuoverperson)->surname);
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
	}
}

Desk_bool Graphics_NewViewCancel(Desk_event_pollblock *block,void *ref)
{
	if (block->data.mouse.button.data.select) {
		Desk_Window_Hide(newviewwin);
		return Desk_TRUE;
	}
	return Desk_FALSE;
}

Desk_bool Graphics_NewViewOk(Desk_event_pollblock *block,void *ref)
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
	Graphics_OpenWindow(wintype,newviewperson,atoi(Desk_Icon_GetTextPtr(newviewwin,newview_GENERATIONS)));
	if (block->data.mouse.button.data.select) Desk_Window_Hide(newviewwin);
	return Desk_TRUE;
}

void Graphics_PersonMenuClick(int entry,void *ref)
{
	elementptr mother,marriage,mothermarriage;
	switch (entry) {
		case personmenu_ADD:
			Database_Add();
			break;
		case personmenu_DELETE:
			Database_Delete(menuoverperson);
			break;
		case personmenu_UNLINK:
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
}

void Graphics_SelectMenuClick(int entry,void *ref)
{
	switch (entry) {
		case selectmenu_DESCENDENTS:
			Layout_SelectDescendents(menuoverwindow->layout,menuoverperson);
			Desk_Window_ForceRedraw(menuoverwindow->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
			break;
		case selectmenu_ANCESTORS:
			Layout_SelectAncestors(menuoverwindow->layout,menuoverperson);
			Desk_Window_ForceRedraw(menuoverwindow->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
			break;
		case selectmenu_SIBLINGS:
			Layout_SelectSiblings(menuoverwindow->layout,menuoverperson);
			Desk_Window_ForceRedraw(menuoverwindow->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
			break;
		case selectmenu_SPOUSES:
			Layout_SelectSpouses(menuoverwindow->layout,menuoverperson);
			Desk_Window_ForceRedraw(menuoverwindow->handle,-INFINITY,-INFINITY,INFINITY,INFINITY);
			break;
	}
}

Desk_bool Graphics_NewViewClick(Desk_event_pollblock *block,void *ref)
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

Desk_bool SaveHandler(char *filename,Desk_bool safe,Desk_bool selection,void *ref)
{
	Database_Save(filename);
	return Desk_TRUE;
}

void Graphics_Init(void)
{
	int i;
	numwindows=0;
	for (i=0;i<MAXWINDOWS;i++) windows[i].handle=0;
	Desk_Drag_Initialise(Desk_TRUE);
	matrix.entries[0][0]=1<<24;
	matrix.entries[0][1]=0;
	matrix.entries[1][0]=0;
	matrix.entries[1][1]=1<<24;
	matrix.entries[2][0]=0;
	matrix.entries[2][1]=0;
	newviewwin=Desk_Window_Create("NewView",Desk_template_TITLEMIN);
	Desk_Icon_InitIncDecHandler(newviewwin,newview_GENERATIONS,newview_UP,newview_DOWN,Desk_FALSE,1,1,999,10);
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_NORMAL);
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_UNLINKED);
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_ANCESTOR);
	AJWLib_Icon_RegisterCheckAdjust(newviewwin,newview_DESCENDENT);
	Desk_Event_Claim(Desk_event_CLICK,newviewwin,Desk_event_ANY,Graphics_NewViewClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,newviewwin,newview_OK,Graphics_NewViewOk,NULL);
	Desk_Event_Claim(Desk_event_CLICK,newviewwin,newview_CANCEL,Graphics_NewViewCancel,NULL);
	addparentswin=Desk_Window_Create("AddParents",Desk_template_TITLEMIN);
	fileinfowin=Desk_Window_Create("FileInfo",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_REDRAW,addparentswin,Desk_event_ANY,Graphics_RedrawAddParents,NULL);
	Desk_Event_Claim(Desk_event_CLICK,addparentswin,addparents_SECONDMARRIAGE,Graphics_AddParentsClick,NULL);
	exportmenu=AJWLib_Menu_CreateFromMsgs("Title.Export:","Menu.Export:",NULL,NULL);
	filemenu=AJWLib_Menu_CreateFromMsgs("Title.File:","Menu.File:",NULL,NULL);
	personmenu=AJWLib_Menu_CreateFromMsgs("Title.Person:","Menu.Person:",Graphics_PersonMenuClick,NULL);
	mainmenu=AJWLib_Menu_CreateFromMsgs("Title.Main:","Menu.Main:",Graphics_MainMenuClick,NULL);
	selectmenu=AJWLib_Menu_CreateFromMsgs("Title.Select:","Menu.Select:",Graphics_SelectMenuClick,NULL);
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
/*	Desk_Menu_Warn(filemenu,filemenu_INFO,Desk_TRUE,Database_GetInfo,...);
*/	{
/*		Desk_window_handle savewin=Save_CreateWindow(0xFFF,Desk_FALSE,1024,Desk_TRUE,SaveHandler,NULL,NULL);
		Desk_Menu_AddSubMenu(filemenu,filemenu_SAVE,(Desk_menu_ptr)savewin);
*/	}
}

