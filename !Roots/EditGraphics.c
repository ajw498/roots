/*
	Roots - EditGraphics, Graphically Edit graphics styles
	© Alex Waugh 2001

	$Id: EditGraphics.c,v 1.2 2001/06/11 23:10:13 AJW Exp $

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
#include "Desk/DeskMem.h"
#include "Desk/Pane2.h"

#include "AJWLib/Error2.h"
#include "AJWLib/Window.h"
#include "AJWLib/Menu.h"
#include "AJWLib/Assert.h"
#include "AJWLib/Msgs.h"
#include "AJWLib/Icon.h"
#include "AJWLib/Flex.h"
#include "AJWLib/Font.h"
#include "AJWLib/ColourPicker.h"
#include "AJWLib/Str.h"
#include "AJWLib/Draw.h"
#include "AJWLib/DrawFile.h"

#include "EditGraphics.h"
#include "Draw.h"
#include "Windows.h"


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define editsize_WIDTH 0
#define editsize_WIDTHUP 2
#define editsize_WIDTHDOWN 1
#define editsize_HEIGHT 4
#define editsize_HEIGHTUP 6
#define editsize_HEIGHTDOWN 5

#define toolbox_ADDSHAPE 0
#define toolbox_ADDTEXT 1
#define toolbox_EDITSIZE 2

#define editshape_TYPE 17
#define editshape_TYPEMENU 16
#define editshape_COLOUR 21
#define editshape_COLOURMENU 20
#define editshape_THICKNESS 25
#define editshape_THICKNESSUP 24
#define editshape_THICKNESSDOWN 23
#define editshape_X 0
#define editshape_XUP 3
#define editshape_XDOWN 2
#define editshape_Y 8
#define editshape_YUP 11
#define editshape_YDOWN 10
#define editshape_WIDTH 7
#define editshape_WIDTHUP 6
#define editshape_WIDTHDOWN 5
#define editshape_HEIGHT 12
#define editshape_HEIGHTUP 15
#define editshape_HEIGHTDOWN 14
#define editshape_EXPAND 26
#define editshape_BACK 27
#define editshape_DELETE 28


#define shapemenu_LINE 0
#define shapemenu_RECTANGLE 1
#define shapemenu_FILLEDRECTANGLE 2

#define textmenu_LABEL 0
#define textmenu_FIELD 1

#define justmenu_LEFT 0
#define justmenu_RIGHT 1
#define justmenu_CENTRED 2

#define edittext_TYPE 9
#define edittext_TYPEMENU 8
#define edittext_LABEL 11
#define edittext_JUST 24
#define edittext_JUSTMENU 23
#define edittext_FONT 21
#define edittext_FONTMENU 20
#define edittext_FONTSIZE 19
#define edittext_FONTSIZEUP 18
#define edittext_FONTSIZEDOWN 17
#define edittext_COLOUR 15
#define edittext_COLOURMENU 14
#define edittext_BACK 27
#define edittext_EXPAND 26
#define edittext_X 0
#define edittext_XUP 3
#define edittext_XDOWN 2
#define edittext_Y 4
#define edittext_YUP 7
#define edittext_YDOWN 6
#define edittext_DELETE 28

typedef struct shapedetails {
	enum {
		shape_LINE=shapemenu_LINE,
		shape_RECTANGLE=shapemenu_RECTANGLE,
		shape_FILLEDRECTANGLE=shapemenu_FILLEDRECTANGLE
	} type;
	int x,y,width,height;
	int thickness;
	unsigned int colour;
	Desk_bool expand;
} shapedetails;

typedef struct textdetails {
	enum {
		text_LABEL=textmenu_LABEL,
		text_FIELD=textmenu_FIELD
	} type;
	enum {
		just_LEFT=justmenu_LEFT,
		just_RIGHT=justmenu_RIGHT,
		just_CENTRED=justmenu_CENTRED
	} just;
	int x,y;
	Desk_wimp_rect bbox;
	int size;
	unsigned int colour;
	char label[256]; /*dynamic size?*/
	char font[256];
	Desk_bool expand;
} textdetails;

typedef struct object {
	enum {
		object_SHAPE,
		object_TEXT
	} type;
	union {
		shapedetails shape;
		textdetails text;
	} object;
	struct object *next;
} object;

typedef struct persondetails {
	int width;
	int height;
	object *objects;
} persondetails;


static persondetails person;
static shapedetails *editingshape=NULL;
static textdetails *editingtext=NULL;

static Desk_window_handle editwin,toolboxpane,shapewin,sizewin,textwin;
static Desk_menu_ptr shapemenu,textmenu,fontmenu,justmenu;

#define BORDER 50

static void EditGraphics_UpdateWindow(void)
{
	int x;
	Desk_wimp_rect bbox={0,0,0,0};
	object *object;

	bbox.max.x=person.width;
	bbox.max.y=person.height;
	object=person.objects;

	while (object!=NULL) {
		if (object->type==object_SHAPE) {
			if (object->object.shape.x<bbox.min.x) bbox.min.x=object->object.shape.x;
			if (object->object.shape.y<bbox.min.y) bbox.min.y=object->object.shape.y;
			if (object->object.shape.x+object->object.shape.width>bbox.max.x)  bbox.max.x=object->object.shape.x+object->object.shape.width;
			if (object->object.shape.y+object->object.shape.height>bbox.max.y) bbox.max.y=object->object.shape.y+object->object.shape.height;
		} else {
			x=object->object.text.x;
			if (object->object.text.just==just_RIGHT) x-=object->object.text.bbox.max.x;
			if (object->object.text.just==just_CENTRED) x-=object->object.text.bbox.max.x/2;
			if (x+object->object.text.bbox.min.x<bbox.min.x) bbox.min.x=x+object->object.text.bbox.min.x;
			if (object->object.text.y+object->object.text.bbox.min.y<bbox.min.y) bbox.min.y=object->object.text.y+object->object.text.bbox.min.y;
			if (x+object->object.text.bbox.max.x>bbox.max.x) bbox.max.x=x+object->object.text.bbox.max.x;
			if (object->object.text.y+object->object.text.bbox.max.y>bbox.max.y) bbox.max.y=object->object.text.y+object->object.text.bbox.max.y;
		}
		object=object->next;
	}
	Desk_Window_SetExtent(editwin,bbox.min.x-BORDER,bbox.min.y-BORDER,bbox.max.x+BORDER,bbox.max.y+BORDER);
	Desk_Window_ForceWholeRedraw(editwin);
}

void EditGraphics_Open(void)
{
	Desk_Pane2_Show(editwin,Desk_open_CENTERED);
	EditGraphics_UpdateWindow();
}

static void EditGraphics_UpdateShapeWindow(void)
{
	Desk_Icon_SetText(shapewin,editshape_TYPE,Desk_Menu_GetText(shapemenu,editingshape->type));
	Desk_Icon_SetInteger(shapewin,editshape_X,editingshape->x);
	Desk_Icon_SetInteger(shapewin,editshape_Y,editingshape->y);
	Desk_Icon_SetInteger(shapewin,editshape_WIDTH,editingshape->width);
	Desk_Icon_SetInteger(shapewin,editshape_HEIGHT,editingshape->height);
	Desk_Icon_SetInteger(shapewin,editshape_THICKNESS,editingshape->thickness);
	Desk_Icon_SetSelect(shapewin,editshape_EXPAND,editingshape->expand);
}

static void EditGraphics_UpdateTextWindow(void)
{
	Desk_Icon_SetText(textwin,edittext_TYPE,Desk_Menu_GetText(textmenu,editingtext->type));
	Desk_Icon_SetText(textwin,edittext_JUST,Desk_Menu_GetText(justmenu,editingtext->just));
	Desk_Icon_SetInteger(textwin,edittext_X,editingtext->x);
	Desk_Icon_SetInteger(textwin,edittext_Y,editingtext->y);
	Desk_Icon_SetInteger(textwin,edittext_FONTSIZE,editingtext->size);
	Desk_Icon_SetText(textwin,edittext_LABEL,editingtext->label);
	Desk_Icon_SetText(textwin,edittext_FONT,editingtext->font);
	Desk_Icon_SetSelect(textwin,edittext_EXPAND,editingtext->expand);
}

static void EditGraphics_ShapeMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	Desk_Icon_SetText(shapewin,editshape_TYPE,Desk_Menu_GetText(shapemenu,entry));
	switch (entry) {
		case shapemenu_LINE:
			editingshape->type=shape_LINE;
			break;
		case shapemenu_RECTANGLE:
			editingshape->type=shape_RECTANGLE;
			break;
		case shapemenu_FILLEDRECTANGLE:
			editingshape->type=shape_FILLEDRECTANGLE;
			break;
	}
	EditGraphics_UpdateWindow();
}

#define SWI_Font_FindField 0x400A6

static Desk_bool EditGraphics_FontMenuClick(Desk_event_pollblock *block,void *ref)
{
	char *font, *d;

	Desk_UNUSED(ref);

	if (Desk_menu_currentopen!=fontmenu) return Desk_FALSE;

	AJWLib_Assert(editingtext!=NULL);
	font=Desk_Menu_FontMenuDecode3(block->data.selection);

	Desk_Error2_CheckOS(Desk_SWI(3,2,SWI_Font_FindField,0,font,'F',NULL,&font));
	d=editingtext->font;
	while (*font>' ' && *font!='\\') *d++=*font++; /*Copy ctrl or space terminated string*/
	*d='\0';

	Desk_Icon_SetText(textwin,edittext_FONT,editingtext->font);
	EditGraphics_UpdateWindow();
	AJWLib_Menu_CheckAdjust();
	return Desk_TRUE;
}

static void EditGraphics_TextMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(editingtext!=NULL);
	Desk_Icon_SetText(textwin,edittext_TYPE,Desk_Menu_GetText(textmenu,entry));
	switch (entry) {
		case textmenu_LABEL:
			editingtext->type=text_LABEL;
			break;
		case textmenu_FIELD:
			editingtext->type=text_FIELD;
			break;
	}
	EditGraphics_UpdateWindow();
}

static void EditGraphics_JustMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(editingtext!=NULL);
	Desk_Icon_SetText(textwin,edittext_JUST,Desk_Menu_GetText(justmenu,entry));
	switch (entry) {
		case justmenu_LEFT:
			editingtext->just=just_LEFT;
			break;
		case justmenu_RIGHT:
			editingtext->just=just_RIGHT;
			break;
		case justmenu_CENTRED:
			editingtext->just=just_CENTRED;
			break;
	}
	EditGraphics_UpdateWindow();
}

static Desk_bool EditGraphics_SendToBack(Desk_event_pollblock *block,void *ref)
{
	void *editingobject;
    object *object, *nextobject;

	AJWLib_Assert(editingshape!=NULL || editingtext!=NULL);
	
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	if (ref) editingobject=editingtext; else editingobject=editingshape;

	object=person.objects;
	while (object!=NULL && &(object->next->object.shape)!=editingobject) object=object->next;
	if (object==NULL) return Desk_TRUE;
	nextobject=object->next;
	object->next=nextobject->next;
	nextobject->next=person.objects;
	person.objects=nextobject;
	EditGraphics_UpdateWindow();
	return Desk_TRUE;
}

static Desk_bool EditGraphics_EditSize(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	Desk_Window_Show(sizewin,Desk_open_NEARLAST);
	return Desk_TRUE;
}

static void EditGraphics_OpenTextWindow(textdetails *newtext)
{
	if (editingtext==NULL) {
		Desk_Window_Show(textwin,Desk_open_NEARLAST);
		fontmenu=Desk_Menu_FontMenu3(Desk_FALSE,Desk_Menu_FontMenu_NOTICK); /*Old fontmenu will get automatically freed*/
		Desk_Event_Claim(Desk_event_MENU,Desk_event_ANY,Desk_event_ANY,EditGraphics_FontMenuClick,NULL);
		AJWLib_Menu_AttachPopup(textwin,edittext_FONTMENU,edittext_FONT,fontmenu,Desk_button_MENU | Desk_button_SELECT);
	}
	editingtext=newtext;
	Desk_Icon_ForceRedraw(textwin,edittext_COLOUR);
	EditGraphics_UpdateTextWindow();
}

static Desk_bool EditGraphics_CloseTextWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	if (editingtext!=NULL) {
		Desk_Window_Hide(textwin);
		editingtext=NULL;
		Desk_Event_Release(Desk_event_MENU,Desk_event_ANY,Desk_event_ANY,EditGraphics_FontMenuClick,NULL);
		AJWLib_Menu_Release(fontmenu);
	}
	return Desk_TRUE;
}

static Desk_bool EditGraphics_AddText(Desk_event_pollblock *block,void *ref)
{
	object *newtext, *oldtext;
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	oldtext=person.objects;
	while (oldtext!=NULL && oldtext->next!=NULL) oldtext=oldtext->next; /*Find end of text list*/
	newtext=Desk_DeskMem_Malloc(sizeof(object));
	if (oldtext==NULL) { /*Add new text to end of list*/
		newtext->next=person.objects;
		person.objects=newtext;
	} else {
		newtext->next=NULL;
		oldtext->next=newtext;
	}
	newtext->type=object_TEXT;
	newtext->object.text.type=text_LABEL;
	newtext->object.text.just=just_LEFT;
	newtext->object.text.x=0;
	newtext->object.text.y=0;
	newtext->object.text.size=12;
	strcpy(newtext->object.text.label,"Wibble");
	strcpy(newtext->object.text.font,"Trinity.Medium");
	newtext->object.text.colour=0; /*black*/
	newtext->object.text.expand=Desk_FALSE;
	newtext->object.text.bbox=*AJWLib_Font_GetBBoxGiven(newtext->object.text.font,newtext->object.text.size*16,newtext->object.text.label);
	EditGraphics_UpdateWindow();
	EditGraphics_OpenTextWindow(&(newtext->object.text));
	return Desk_TRUE;
}

static void EditGraphics_OpenShapeWindow(shapedetails *newshape)
{
	if (editingshape==NULL) Desk_Window_Show(shapewin,Desk_open_NEARLAST);
	editingshape=newshape;
	Desk_Icon_ForceRedraw(shapewin,editshape_COLOUR);
	EditGraphics_UpdateShapeWindow();
}

static Desk_bool EditGraphics_CloseShapeWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	if (editingshape!=NULL) {
		Desk_Window_Hide(shapewin);
		editingshape=NULL;
	}
	return Desk_TRUE;
}

static Desk_bool EditGraphics_AddShape(Desk_event_pollblock *block,void *ref)
{
	object *newshape, *oldshape;
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	oldshape=person.objects;
	while (oldshape!=NULL && oldshape->next!=NULL) oldshape=oldshape->next; /*Find end of shape list*/
	newshape=Desk_DeskMem_Malloc(sizeof(object));
	if (oldshape==NULL) { /*Add new shape to end of list*/
		newshape->next=person.objects;
		person.objects=newshape;
	} else {
		newshape->next=NULL;
		oldshape->next=newshape;
	}
	newshape->type=object_SHAPE;
	newshape->object.shape.type=shape_FILLEDRECTANGLE;
	newshape->object.shape.x=0;
	newshape->object.shape.y=0;
	newshape->object.shape.width=100;
	newshape->object.shape.height=100;
	newshape->object.shape.thickness=0;
	newshape->object.shape.colour=0; /*black*/
	newshape->object.shape.expand=Desk_FALSE;
	EditGraphics_UpdateWindow();
	EditGraphics_OpenShapeWindow(&(newshape->object.shape));
	return Desk_TRUE;
}

static Desk_bool EditGraphics_Delete(Desk_event_pollblock *block,void *ref)
{
	void *editingobject;
    object *object, *nextobject;

	AJWLib_Assert(editingshape!=NULL || editingtext!=NULL);
	
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	if (ref) editingobject=editingtext; else editingobject=editingshape;

	object=person.objects;
	while (object!=NULL && &(object->next->object.shape)!=editingobject) object=object->next;
	if (object==NULL) return Desk_TRUE;
	nextobject=object->next;
	object->next=nextobject->next;
	Desk_DeskMem_Free(nextobject);
	if (ref) EditGraphics_CloseTextWindow(NULL,NULL); else EditGraphics_CloseShapeWindow(NULL,NULL);
	EditGraphics_UpdateWindow();
	return Desk_TRUE;
}

static Desk_bool EditGraphics_UpdateWidthAndHeight(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	person.width=Desk_Icon_GetInteger(sizewin,editsize_WIDTH);
	person.height=Desk_Icon_GetInteger(sizewin,editsize_HEIGHT);
	if (editingshape) {
		editingshape->x=Desk_Icon_GetInteger(shapewin,editshape_X);
		editingshape->y=Desk_Icon_GetInteger(shapewin,editshape_Y);
		editingshape->width=Desk_Icon_GetInteger(shapewin,editshape_WIDTH);
		editingshape->height=Desk_Icon_GetInteger(shapewin,editshape_HEIGHT);
		editingshape->thickness=Desk_Icon_GetInteger(shapewin,editshape_THICKNESS);
		editingshape->expand=Desk_Icon_GetSelect(shapewin,editshape_EXPAND);
	}
	if (editingtext) {
		editingtext->x=Desk_Icon_GetInteger(textwin,edittext_X);
		editingtext->y=Desk_Icon_GetInteger(textwin,edittext_Y);
		editingtext->size=Desk_Icon_GetInteger(textwin,edittext_FONTSIZE);
		if (editingtext->size<1) editingtext->size=1;
		Desk_Icon_GetText(textwin,edittext_LABEL,editingtext->label);
		Desk_Icon_GetText(textwin,edittext_FONT,editingtext->font);
		editingtext->expand=Desk_Icon_GetSelect(textwin,edittext_EXPAND);
		editingtext->bbox=*AJWLib_Font_GetBBoxGiven(editingtext->font,editingtext->size*16,editingtext->label);
	}
	EditGraphics_UpdateWindow();
	return Desk_FALSE;
}

static Desk_bool EditGraphics_CloseWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	Desk_Pane2_Hide(editwin);
	EditGraphics_CloseTextWindow(NULL,NULL);
	EditGraphics_CloseShapeWindow(NULL,NULL);
	return Desk_TRUE;
}

static Desk_bool EditGraphics_ShapeColourChoice(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);

	editingshape->colour=block->data.words[7];
	Desk_Icon_ForceRedraw(shapewin,editshape_COLOUR);
	EditGraphics_UpdateWindow();
	return Desk_TRUE;
}

static Desk_bool EditGraphics_ShapePickColour(Desk_event_pollblock *block,void *ref)
{
	Desk_wimp_rect pos;

	Desk_UNUSED(ref);
	Desk_UNUSED(block);

	Desk_Icon_ScreenPos(shapewin,editshape_COLOURMENU,&pos);
	AJWLib_ColourPicker_Open(colourpicker_MENU,pos.max.x,pos.max.y,editingshape->colour,Desk_FALSE,NULL,EditGraphics_ShapeColourChoice,NULL);
	return Desk_TRUE;
}

static Desk_bool EditGraphics_TextColourChoice(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);

	editingtext->colour=block->data.words[7];
	Desk_Icon_ForceRedraw(textwin,edittext_COLOUR);
	Desk_Window_ForceRedraw(editwin,-INFINITY,-INFINITY,INFINITY,INFINITY);
	return Desk_TRUE;
}

static Desk_bool EditGraphics_TextPickColour(Desk_event_pollblock *block,void *ref)
{
	Desk_wimp_rect pos;

	Desk_UNUSED(ref);
	Desk_UNUSED(block);

	Desk_Icon_ScreenPos(textwin,edittext_COLOURMENU,&pos);
	AJWLib_ColourPicker_Open(colourpicker_MENU,pos.max.x,pos.max.y,editingtext->colour,Desk_FALSE,NULL,EditGraphics_TextColourChoice,NULL);
	return Desk_TRUE;
}

static Desk_bool EditGraphics_MouseClick(Desk_event_pollblock *block,void *ref)
{
	int mousex,mousey;
	Desk_convert_block blk;

	Desk_UNUSED(ref);
	/*Set window focus if select or adjust clicked*/
	if (!block->data.mouse.button.data.menu) Desk_Icon_SetCaret(block->data.mouse.window,-1);

	/*Find out what was clicked on*/
	Desk_Window_GetCoords(block->data.mouse.window,&blk);
	mousex=(block->data.mouse.pos.x-(blk.screenrect.min.x-blk.scroll.x));
	mousey=(block->data.mouse.pos.y-(blk.screenrect.max.y-blk.scroll.y));

	if (block->data.mouse.button.data.select) {
		object *object, *foundobject;

		object=person.objects;
		foundobject=NULL;
		while (object!=NULL) {
			if (object->type==object_SHAPE) {
				if (mousex>object->object.shape.x && mousex<(object->object.shape.x+object->object.shape.width) && mousey>object->object.shape.y && mousey<(object->object.shape.y+object->object.shape.height)) foundobject=object;
			} else {
				int x;

				x=object->object.text.x;
				if (object->object.text.just==just_RIGHT) x-=object->object.text.bbox.max.x;
				if (object->object.text.just==just_CENTRED) x-=object->object.text.bbox.max.x/2;
				if (mousex>x+object->object.text.bbox.min.x && mousex<x+object->object.text.bbox.max.x && mousey>object->object.text.y+object->object.text.bbox.min.y && mousey<(object->object.text.y+object->object.text.bbox.max.y)) foundobject=object;
			}
			object=object->next;
		}
		if (foundobject) {
			if (foundobject->type==object_SHAPE) {
				EditGraphics_OpenShapeWindow(&(foundobject->object.shape));
			} else {
				EditGraphics_OpenTextWindow(&(foundobject->object.text));
			}
			return Desk_TRUE;
		} else {
			EditGraphics_CloseShapeWindow(NULL,NULL);
			EditGraphics_CloseTextWindow(NULL,NULL);
		}
	}
	return Desk_FALSE;
}

static Desk_bool EditGraphics_RedrawWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=block->data.openblock.window;
	Desk_UNUSED(ref);
	Desk_Wimp_RedrawWindow(&blk,&more);
	while (more) {
		object *object=person.objects;

		Draw_PlotRectangleFilled(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,0,0,person.width,person.height,0,0xFFFFFF00);
		while (object) {
			shapedetails *shape=&(object->object.shape);
			textdetails *text=&(object->object.text);
			int x;

			switch (object->type) {
				case object_SHAPE:
					switch (shape->type) {
						case shape_LINE:
							Draw_PlotLine(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,shape->x,shape->y,shape->x+shape->width,shape->y+shape->height,shape->thickness,shape->colour);
							break;
						case shape_RECTANGLE:
							Draw_PlotRectangle(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,shape->x,shape->y,shape->width,shape->height,shape->thickness,shape->colour);
							break;
						case shape_FILLEDRECTANGLE:
							Draw_PlotRectangleFilled(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,shape->x,shape->y,shape->width,shape->height,shape->thickness,shape->colour);
							break;
					}
					break;
				case object_TEXT:
					x=text->x;
					if (text->just==just_RIGHT) x-=text->bbox.max.x;
					if (text->just==just_CENTRED) x-=text->bbox.max.x/2;
					Draw_PlotText(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,x,text->y,0,text->font,text->size,0xFFFFFF00,text->colour,text->label);
					break;
			}
			object=object->next;
		}
		Desk_Wimp_GetRectangle(&blk,&more);
	}
	return Desk_TRUE;
}

static Desk_bool EditGraphics_RedrawShapeWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	Desk_wimp_rect pos;

	blk.window=block->data.openblock.window;

	Desk_UNUSED(ref);
	AJWLib_Assert(editingshape!=NULL);

	Desk_Wimp_RedrawWindow(&blk,&more);

	Desk_Icon_ScreenPos(shapewin,editshape_COLOUR,&pos);

	while (more) {
		Draw_PlotRectangleFilled(100,0,0,pos.min.x,pos.min.y,pos.max.x-pos.min.x,pos.max.y-pos.min.y,0,editingshape->colour);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
	return Desk_TRUE;
}

static Desk_bool EditGraphics_RedrawTextWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	Desk_wimp_rect pos;

	blk.window=block->data.openblock.window;

	Desk_UNUSED(ref);
	AJWLib_Assert(editingtext!=NULL);

	Desk_Wimp_RedrawWindow(&blk,&more);

	Desk_Icon_ScreenPos(textwin,edittext_COLOUR,&pos);

	while (more) {
		Draw_PlotRectangleFilled(100,0,0,pos.min.x,pos.min.y,pos.max.x-pos.min.x,pos.max.y-pos.min.y,0,editingtext->colour);
		Desk_Wimp_GetRectangle(&blk,&more);
	}
	return Desk_TRUE;
}

void EditGraphics_Init(void)
{
	Desk_wimp_point paneoffset={-182,0};

	editwin=Desk_Pane2_CreateAndAddMain("EditStyle",Desk_template_TITLEMIN);
	toolboxpane=Desk_Pane2_CreateAndAddPane("EditToolbox",Desk_template_TITLEMIN,editwin,&paneoffset,NULL,Desk_pane2_PANETOP | Desk_pane2_MAINTOP | Desk_pane2_FIXED);

	shapewin=Desk_Window_Create("EditObject",Desk_template_TITLEMIN);
	textwin=Desk_Window_Create("EditText",Desk_template_TITLEMIN);
	sizewin=Desk_Window_Create("EditSize",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,toolboxpane,toolbox_ADDSHAPE,EditGraphics_AddShape,NULL);
	Desk_Event_Claim(Desk_event_CLICK,toolboxpane,toolbox_ADDTEXT,EditGraphics_AddText,NULL);
	Desk_Event_Claim(Desk_event_CLICK,toolboxpane,toolbox_EDITSIZE,EditGraphics_EditSize,NULL);
	Desk_Event_Claim(Desk_event_CLICK,shapewin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Event_Claim(Desk_event_KEY,shapewin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_WIDTH,editshape_WIDTHUP,editshape_WIDTHDOWN,Desk_FALSE,5,0,9995,100);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_HEIGHT,editshape_HEIGHTUP,editshape_HEIGHTDOWN,Desk_FALSE,5,0,9995,100);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_THICKNESS,editshape_THICKNESSUP,editshape_THICKNESSDOWN,Desk_FALSE,1,0,9995,0);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_X,editshape_XUP,editshape_XDOWN,Desk_FALSE,5,-995,9995,0);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_Y,editshape_YUP,editshape_YDOWN,Desk_FALSE,5,-995,9995,0);
	Desk_Event_Claim(Desk_event_CLICK,shapewin,editshape_COLOURMENU,EditGraphics_ShapePickColour,NULL);
	Desk_Event_Claim(Desk_event_CLICK,shapewin,editshape_BACK,EditGraphics_SendToBack,NULL);
	Desk_Event_Claim(Desk_event_CLICK,shapewin,editshape_DELETE,EditGraphics_Delete,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,shapewin,Desk_event_ANY,EditGraphics_RedrawShapeWindow,NULL);
	Desk_Event_Claim(Desk_event_CLOSE,shapewin,Desk_event_ANY,EditGraphics_CloseShapeWindow,NULL);

	Desk_Event_Claim(Desk_event_CLOSE,textwin,Desk_event_ANY,EditGraphics_CloseTextWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,textwin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Event_Claim(Desk_event_KEY,textwin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Icon_InitIncDecHandler(textwin,edittext_FONTSIZE,edittext_FONTSIZEUP,edittext_FONTSIZEDOWN,Desk_FALSE,1,1,999,12);
	Desk_Icon_InitIncDecHandler(textwin,edittext_X,edittext_XUP,edittext_XDOWN,Desk_FALSE,5,-995,9995,0);
	Desk_Icon_InitIncDecHandler(textwin,edittext_Y,edittext_YUP,edittext_YDOWN,Desk_FALSE,5,-995,9995,0);
	Desk_Event_Claim(Desk_event_CLICK,textwin,edittext_COLOURMENU,EditGraphics_TextPickColour,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,textwin,Desk_event_ANY,EditGraphics_RedrawTextWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,textwin,edittext_BACK,EditGraphics_SendToBack,(void *)1);
	Desk_Event_Claim(Desk_event_CLICK,textwin,edittext_DELETE,EditGraphics_Delete,(void *)1);

	Desk_Event_Claim(Desk_event_CLICK,editwin,Desk_event_ANY,EditGraphics_MouseClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,sizewin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,editwin,Desk_event_ANY,EditGraphics_RedrawWindow,NULL);
	Desk_Event_Claim(Desk_event_CLOSE,editwin,Desk_event_ANY,EditGraphics_CloseWindow,NULL);
	Desk_Event_Claim(Desk_event_KEY,sizewin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Icon_InitIncDecHandler(sizewin,editsize_WIDTH,editsize_WIDTHUP,editsize_WIDTHDOWN,Desk_FALSE,5,0,9995,400);
	Desk_Icon_InitIncDecHandler(sizewin,editsize_HEIGHT,editsize_HEIGHTUP,editsize_HEIGHTDOWN,Desk_FALSE,5,0,9995,200);
	person.width=400;
	person.height=200;
	person.objects=NULL;
	editingshape=NULL;
	editingtext=NULL;

	shapemenu=AJWLib_Menu_CreateFromMsgs("Title.Shape:","Menu.Shape:",EditGraphics_ShapeMenuClick,NULL);
	AJWLib_Menu_AttachPopup(shapewin,editshape_TYPEMENU,editshape_TYPE,shapemenu,Desk_button_MENU | Desk_button_SELECT);

	textmenu=AJWLib_Menu_CreateFromMsgs("Title.Text:","Menu.Text:",EditGraphics_TextMenuClick,NULL);
	AJWLib_Menu_AttachPopup(textwin,edittext_TYPEMENU,edittext_TYPE,textmenu,Desk_button_MENU | Desk_button_SELECT);

	justmenu=AJWLib_Menu_CreateFromMsgs("Title.Just:","Menu.Just:",EditGraphics_JustMenuClick,NULL);
	AJWLib_Menu_AttachPopup(textwin,edittext_JUSTMENU,edittext_JUST,justmenu,Desk_button_MENU | Desk_button_SELECT);

}
