/*
	Roots - Windows, menus and interface
	© Alex Waugh 1999

	$Id: Windows.c,v 1.113 2002/08/01 14:56:58 ajw Exp $

*/

#include "Desk/Window.h"
#include "Desk/Error.h"
#include "Desk/Error2.h"
#include "Desk/SWI.h"
#include "Desk/WimpSWIs.h"
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
#include "Desk/GFX.h"
#include "Desk/Save.h"
#include "Desk/Kbd.h"
#include "Desk/Keycodes.h"
#include "Desk/Str.h"
#include "Desk/Font2.h"
#include "Desk/ColourTran.h"

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

#define MAXWINDOWS 10
#define REDRAWOVERLAP 4

#define mainmenu_FILE 0
#define mainmenu_PERSON 1
#define mainmenu_ADDPERSON 2
#define mainmenu_SELECT 9 /*3*/
#define mainmenu_GRAPHICSSTYLE 3 /*4*/
#define mainmenu_NEWVIEW 10 /*5*/
#define mainmenu_SCALE 4 /*6*/
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
#define personmenu_UNLINK 2

#define selectmenu_DESCENDENTS 0
#define selectmenu_ANCESTORS 1
#define selectmenu_SIBLINGS 2
#define selectmenu_SPOUSES 3

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
#define info_PEOPLE 3
#define info_DATE 8
#define info_ICON 4

#define save_ICON 3
#define save_OK 0
#define save_CANCEL 1
#define save_FILENAME 2

#define unsaved_DISCARD 2
#define unsaved_CANCEL 3
#define unsaved_SAVE 0

#define SWI_OS_SpriteOp 0x2E
#define SWI_Wimp_SpriteOp 0x400E9
#define SWI_Wimp_DragBox 0x400D0

typedef struct savedata {
	wintype type;
	elementptr person;
	int generations;
	int scale;
	Desk_convert_block coords;
} savedata;

static windowdata windows[MAXWINDOWS];
static int numwindows;
static Desk_window_handle newviewwin,fileinfowin,savewin,savedrawwin,savegedcomwin,scalewin,unsavedwin;
static Desk_menu_ptr mainmenu,filemenu,exportmenu,personmenu/*,selectmenu*/,fileconfigmenu=NULL;
static elementptr newviewperson;
static savedata nextloadwindowdata;

void Windows_ForceRedraw(void)
{
	int i;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle) Desk_Window_ForceWholeRedraw(windows[i].handle);
		/*Make this more efficient*/
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
			Layout_ResizeWindow(&(windows[i]));
			/*Make this more efficient*/
		}
	}
}

static void Windows_StyleMenuClick(int entry,void *ref)
{
	Desk_menu_item *item;
	int i;

	Desk_UNUSED(ref);

	/*This is inefficient*/
	for (i=0;i<MAXWINDOWS;i++) if (windows[i].handle) Layout_CalcAllGridFromPositions(windows[i].layout);

	Graphics_RemoveStyle();
	/*Untick all entries*/
	item=Desk_Menu_FirstItem(fileconfigmenu);
	while (!item->menuflags.data.last) {
		item->menuflags.data.ticked=0;
		item++;
	}
	item->menuflags.data.ticked=0;
	/*Tick the entry clicked on*/
	Desk_Menu_SetFlags(fileconfigmenu,entry,1,0);
	Graphics_LoadStyle(Desk_Menu_GetText(fileconfigmenu,entry));

	/*This is inefficient*/
	for (i=0;i<MAXWINDOWS;i++) if (windows[i].handle) Layout_CalcAllPositionsFromGrid(windows[i].layout);
	for (i=0;i<MAXWINDOWS;i++) if (windows[i].handle) Layout_ResizeAllWidths(windows[i].layout);

	Modules_ChangedStructure();
}

void Windows_GraphicsStylesMenu(Desk_menu_ptr *menuptr,char *dirname)
{
	static int i=0;
	Desk_filing_dirdata dir;
	char *name=NULL;

	if (Desk_File_IsDirectory(dirname)) {
		Desk_Filing_OpenDir(dirname,&dir,256,Desk_readdirtype_NAMEONLY); /*Does this cope with long filenames?*/
		do {
			name=Desk_Filing_ReadDir(&dir);
			if (name && strcmp(name,"CVS")!=0) {
				Desk_bool found=Desk_FALSE;

				if (*menuptr) {
					/*Check that the style is not already in the menu*/
					int j=-1;
					Desk_menu_item *menu;

					menu=Desk_Menu_FirstItem(*menuptr);
					do {
						j++;
						if (menu[j].iconflags.data.indirected) {
							if (strcmp(menu[j].icondata.indirecttext.buffer,name)==0) found=Desk_TRUE;
						} else {
							if (strncmp(menu[j].icondata.text,name,Desk_wimp_MAXNAME)==0) found=Desk_TRUE;
						}
					}
					while (!found && !menu[j].menuflags.data.last);
					if (!found) {
						*menuptr=Desk_Menu_Extend(*menuptr,name);
						i++;
					}
				} else {
					*menuptr=Desk_Menu_New(AJWLib_Msgs_TempLookup("Title.Config:"),name);
					i=0;
				}
				if (!found) {
					if (Desk_stricmp(name,Graphics_GetCurrentStyle())) {
						Desk_Menu_SetFlags(*menuptr,i,0,0);
					} else {
						Desk_Menu_SetFlags(*menuptr,i,1,0);
					}
				}
			}
		} while (name);
		Desk_Filing_CloseDir(&dir);
	}
}

void Windows_SetUpMenu(windowdata *windowdata,elementtype selected,int x,int y)
{
	char buffer[20];
	char dirname[256];

	Desk_UNUSED(windowdata);
	AJWLib_Menu_Shade(mainmenu,mainmenu_ADDPERSON);
	AJWLib_Menu_Shade(personmenu,personmenu_EDIT);
	AJWLib_Menu_Shade(personmenu,personmenu_DELETE);
	AJWLib_Menu_Shade(personmenu,personmenu_UNLINK);
	AJWLib_Menu_Shade(mainmenu,mainmenu_SELECT);
	AJWLib_Menu_Shade(mainmenu,mainmenu_PERSON);
	Desk_Menu_SetText(mainmenu,mainmenu_PERSON,AJWLib_Msgs_TempLookup("Item.Person:Person"));

	AJWLib_Menu_UnShade(mainmenu,mainmenu_ADDPERSON);
	/*AJWLib_Menu_UnShade(mainmenu,mainmenu_SELECT);*/ /*Temporary, until selected descendents etc. works again*/

	switch (selected) {
		case element_PERSON:
			AJWLib_Menu_UnShade(personmenu,personmenu_EDIT);
			AJWLib_Menu_UnShade(personmenu,personmenu_DELETE);
			AJWLib_Menu_UnShade(personmenu,personmenu_UNLINK);
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
			AJWLib_Menu_UnShade(personmenu,personmenu_UNLINK);
			AJWLib_Menu_UnShade(mainmenu,mainmenu_PERSON);
			Desk_Menu_SetText(mainmenu,mainmenu_PERSON,AJWLib_Msgs_TempLookup("Item.Select:Selection"));
			break;
		case element_NONE:
			AJWLib_Menu_Shade(mainmenu,mainmenu_SELECT);
			break;
		default:
			break;
	}

	Desk_Icon_SetText(savewin,save_FILENAME,File_GetFilename());
	Desk_Icon_SetText(savedrawwin,save_FILENAME,AJWLib_Msgs_TempLookup("File.Draw:Drawfile"));
	Desk_Icon_SetText(savegedcomwin,save_FILENAME,AJWLib_Msgs_TempLookup("File.GED:GEDCOM"));
	sprintf(buffer,"%d",Database_GetNumPeople());
	Desk_Icon_SetText(fileinfowin,info_PEOPLE,buffer);
	Desk_Icon_SetText(fileinfowin,info_FILENAME,File_GetFilename());
	Desk_Icon_SetText(fileinfowin,info_MODIFIED,File_GetModified() ? AJWLib_Msgs_TempLookup("Mod.Yes:Yes") : AJWLib_Msgs_TempLookup("Mod.No:No"));
	Desk_Icon_SetText(fileinfowin,info_DATE,File_GetDate());
	Desk_Icon_SetInteger(scalewin,scale_TEXT,mousedata.window->scale);
	if (fileconfigmenu) {
		AJWLib_Menu_FullDispose(fileconfigmenu);
		fileconfigmenu=NULL;
	}
	sprintf(dirname,"%s.%s",choicesread,GRAPHICSDIR);
	Windows_GraphicsStylesMenu(&fileconfigmenu,dirname);
	sprintf(dirname,"%s.%s",DEFAULTS,GRAPHICSDIR);
	Windows_GraphicsStylesMenu(&fileconfigmenu,dirname);
	AJWLib_Menu_Register(fileconfigmenu,Windows_StyleMenuClick,NULL);
	Desk_Menu_AddSubMenu(mainmenu,mainmenu_GRAPHICSSTYLE,fileconfigmenu);

	Desk_Menu_Show(mainmenu,x,y);
}

void Windows_Relayout(void)
{
	volatile int i;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle!=0) {
			Desk_Error2_Try {
				switch (windows[i].type) {
					case wintype_NORMAL:
						Layout_LayoutLines(windows[i].layout,Desk_FALSE);
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
			Layout_ResizeWindow(&windows[i]);
			Desk_Window_ForceWholeRedraw(windows[i].handle);
		}
	}
}

void Windows_CreateWindow(wintype type)
{
	/* Create the window using details stored in nextloadwindowdata*/
	Windows_OpenWindow(type,nextloadwindowdata.person,nextloadwindowdata.generations,nextloadwindowdata.scale,&(nextloadwindowdata.coords));
}

void Windows_SetMinX(int val)
{
	nextloadwindowdata.coords.screenrect.min.x=val;
}

void Windows_SetMinY(int val)
{
	nextloadwindowdata.coords.screenrect.min.y=val;
}

void Windows_SetMaxX(int val)
{
	nextloadwindowdata.coords.screenrect.max.x=val;
}

void Windows_SetMaxY(int val)
{
	nextloadwindowdata.coords.screenrect.max.y=val;
}

void Windows_SetScrollX(int val)
{
	nextloadwindowdata.coords.scroll.x=val;
}

void Windows_SetScrollY(int val)
{
	nextloadwindowdata.coords.scroll.y=val;
}

void Windows_SetPerson(elementptr person)
{
	nextloadwindowdata.person=person;
}

void Windows_SetGenerations(int val)
{
	nextloadwindowdata.generations=val;
}

void Windows_SetScale(int val)
{
	nextloadwindowdata.scale=val;
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
		Desk_Window_GetCoords(windows[i].handle,&(coords));
		fprintf(file,"1 _COORDS\n2 _SCREENRECT\n3 _MIN\n4 _X %d\n4 _Y %d\n3 _MAX\n4 _X %d\n4 _Y %d\n2 _SCROLL\n3 _X %d\n3 _Y %d\n",coords.screenrect.min.x,coords.screenrect.min.y,coords.screenrect.max.x,coords.screenrect.max.y,coords.scroll.x,coords.scroll.y);
		if (windows[i].person) fprintf(file,"1 _PERSON @%d@\n",windows[i].person);
		fprintf(file,"1 _GENERATIONS %d\n",windows[i].generations);
		fprintf(file,"1 _SCALE %d\n",windows[i].scale);
		fprintf(file,"1 _TYPE %d\n",windows[i].type);

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
		File_SaveGEDCOM(File_GetFilename(),NULL);
		Windows_CloseAllWindows();
	}
}

void Windows_CloseAllWindows(void)
/*Tidy up quietly*/
{
	volatile int i;
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle) {
			Desk_Error2_TryCatch(if (windows[i].layout) Layout_Free(windows[i].layout);,)
			Desk_Error2_TryCatch(Desk_Window_Delete(windows[i].handle);,)
			windows[i].handle=NULL;
		}
	}
	Desk_Error2_TryCatch(Desk_Window_Hide(fieldconfigwin);,)
	Desk_Error2_TryCatch(Print_CloseWindow();,)
	Desk_Error2_TryCatch(File_Remove();,)
}

static Desk_bool Windows_CloseWindow(Desk_event_pollblock *block,windowdata *windowdata)
{
	int i,found=0;
	Desk_UNUSED(block);
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

static Desk_bool Windows_KeyPress(Desk_event_pollblock *block,windowdata *windowdata)
{
	switch (block->data.key.code) {
		case Desk_keycode_ESCAPE:
			Windows_UnselectAll(windowdata);
			break;
		case Desk_keycode_DELETE:
			Layout_DeleteSelected(windowdata->layout);
			break;
		case Desk_keycode_PRINT:
			Print_OpenWindow(windowdata->layout);
			break;
		case Desk_keycode_F3:
			Desk_Icon_SetText(savewin,save_FILENAME,File_GetFilename());
			AJWLib_Window_OpenTransient(savewin);
			break;
		default:
			/*Let the default handler deal with it*/
			return Desk_FALSE;
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
			if (windows[i].type==wintype_NORMAL && windows[i].layout!=NULL) Desk_Window_SetTitle(windows[i].handle,title);
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
	Desk_Window_GainCaret(windowdata->handle);
}

void Windows_LayoutNormal(layout *layout,Desk_bool opencentred)
{
	int i;
	if (layout==NULL) Desk_Error2_TryCatch(layout=Layout_LayoutNormal();,AJWLib_Error2_Report("%s");)
	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle && windows[i].type==wintype_NORMAL && windows[i].layout==NULL) {
			windows[i].layout=layout;
			Layout_ResizeWindow(&windows[i]);
			if (opencentred) Windows_OpenWindowCentered(&windows[i],NULL);
		}
	}
}

void Windows_AddDrawfile(Desk_window_handle window, Desk_wimp_point *pos, char *filename)
{
	int i;

	for (i=0;i<MAXWINDOWS;i++) {
		if (windows[i].handle == window) {
			int x,y;
			layout *layout;
			Desk_convert_block blk;
			drawfileholder *picture;
			flags flags;

			layout = windows[i].layout;

			/*Find mouse position relative to window origin and independant of current scale*/
			Desk_Window_GetCoords(windows[i].handle,&blk);
			x=((pos->x-(blk.screenrect.min.x-blk.scroll.x))*100)/windows[i].scale;
			y=((pos->y-(blk.screenrect.max.y-blk.scroll.y))*100)/windows[i].scale;

			Layout_AddDrawfile(windows[i].layout, x, y, filename);

			Layout_ResizeWindow(&windows[i]);
			Desk_Window_ForceWholeRedraw(windows[i].handle);
			break;
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
			windows[newwindow].layout=Layout_LayoutAncestors(person,generations);
			break;
		default:
			windows[newwindow].layout=NULL;
			AJWLib_Assert(0);
	}
	if (type!=wintype_NORMAL || layoutnormal!=NULL) Layout_ResizeWindow(&windows[newwindow]);
	Desk_Event_Claim(Desk_event_CLICK,windows[newwindow].handle,Desk_event_ANY,Layout_MouseClick,&windows[newwindow]);
	Desk_Event_Claim(Desk_event_REDRAW,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Layout_RedrawWindow,&windows[newwindow]);
	Desk_Event_Claim(Desk_event_CLOSE,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Windows_CloseWindow,&windows[newwindow]);
	Desk_Event_Claim(Desk_event_KEY,windows[newwindow].handle,Desk_event_ANY,(Desk_event_handler)Windows_KeyPress,&windows[newwindow]);
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
				flags flags;
				flags.editable=1;
				flags.moveable=1;
				flags.linkable=1;
				flags.snaptogrid=1;
				flags.selectable=1;
				Layout_AddElement(mousedata.window->layout,newperson,mousedata.pos.x,Layout_NearestGeneration(mousedata.pos.y),Graphics_PersonWidth(none),Graphics_PersonHeight(),0,0,flags);
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

static void Windows_FileMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	switch (entry) {
		case filemenu_SAVE:
			AJWLib_Window_OpenTransient(savewin);
			break;
		case filemenu_CHOICES:
			Database_OpenFileConfig();
			break;
		case filemenu_PRINT:
			Print_OpenWindow(mousedata.window->layout);
	}
}

static Desk_bool Windows_NewViewOk(Desk_event_pollblock *block,void *ref)
{
	volatile wintype wintype;
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
			Layout_DeleteSelected(mousedata.window->layout);
			AJWLib_Menu_Shade(personmenu,personmenu_EDIT);
			AJWLib_Menu_Shade(personmenu,personmenu_DELETE);
			AJWLib_Menu_Shade(personmenu,personmenu_UNLINK);
			break;
		case personmenu_UNLINK:
			Layout_UnlinkSelected(mousedata.window->layout);
			AJWLib_Menu_Shade(personmenu,entry);
			break;
	}
}

/*static void Windows_SelectMenuClick(int entry,void *ref)
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
}*/

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
			Layout_ResizeWindow(mousedata.window);
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
	Desk_Event_mask.data.null=1; /* Disable Null polls*/
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
/*	selectmenu=AJWLib_Menu_CreateFromMsgs("Title.Select:","Menu.Select:",Windows_SelectMenuClick,NULL);
*/	Desk_Menu_AddSubMenu(mainmenu,mainmenu_PERSON,personmenu);
	Desk_Menu_AddSubMenu(mainmenu,mainmenu_FILE,filemenu);
	Desk_Menu_AddSubMenu(filemenu,filemenu_EXPORT,exportmenu);
	Desk_Menu_AddSubMenu(filemenu,filemenu_INFO,(Desk_menu_ptr)fileinfowin);
/*	Desk_Menu_AddSubMenu(mainmenu,mainmenu_SELECT,selectmenu);
*/	Desk_Icon_Shade(newviewwin,newview_ANCESTOR);
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
	Desk_Save_InitSaveWindowHandler(savewin,Desk_TRUE,Desk_TRUE,Desk_FALSE,save_ICON,save_OK,save_CANCEL,save_FILENAME,File_SaveGEDCOM,NULL,File_Result,1024*10/*Filesize estimate?*/,ROOTS_FILETYPE,NULL);
	savedrawwin=Desk_Window_Create("Save",Desk_template_TITLEMIN);
	savegedcomwin=Desk_Window_Create("Save",Desk_template_TITLEMIN);
	Desk_Menu_AddSubMenu(exportmenu,exportmenu_DRAW,(Desk_menu_ptr)savedrawwin);
	Desk_Menu_AddSubMenu(exportmenu,exportmenu_GEDCOM,(Desk_menu_ptr)savegedcomwin);
	Desk_Save_InitSaveWindowHandler(savedrawwin,Desk_TRUE,Desk_TRUE,Desk_FALSE,save_ICON,save_OK,save_CANCEL,save_FILENAME,Windows_SaveDraw,NULL,NULL/*Need a result handler?*/,1024*10/*Filesize estimate?*/,Desk_filetype_DRAWFILE,NULL);
	Desk_Save_InitSaveWindowHandler(savegedcomwin,Desk_TRUE,Desk_TRUE,Desk_FALSE,save_ICON,save_OK,save_CANCEL,save_FILENAME,File_SaveGEDCOM,NULL,NULL/*Need a result handler?*/,1024*10/*Filesize estimate?*/,Desk_filetype_TEXT,(void *)1);
	AJWLib_Window_RegisterDCS(unsavedwin,unsaved_DISCARD,unsaved_CANCEL,unsaved_SAVE,Windows_CloseAllWindows,Windows_OpenSaveWindow);
}

