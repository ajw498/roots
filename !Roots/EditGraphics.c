/*
	Roots - EditGraphics, Graphically Edit graphics styles
	© Alex Waugh 2001

	$Id: EditGraphics.c,v 1.1 2004/01/05 22:42:54 AJW Exp $

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
#include "AJWLib/File.h"
#include "AJWLib/Str.h"
#include "AJWLib/Draw.h"
#include "AJWLib/DrawFile.h"

#include "EditGraphics.h"
#include "Draw.h"
#include "Windows.h"


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define icon_WIDTH 3
#define icon_WIDTHUP 5
#define icon_WIDTHDOWN 4
#define icon_HEIGHT 0
#define icon_HEIGHTUP 2
#define icon_HEIGHTDOWN 1
#define icon_CANCEL 6
#define icon_OK 7

#define icon_ADDSHAPE 0
#define icon_ADDTEXT 1

#define editshape_TYPE 17
#define editshape_TYPEMENU 16
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

#define shapemenu_LINE 0
#define shapemenu_RECTANGLE 1
#define shapemenu_FILLEDRECTANGLE 2


typedef struct shapedetails {
	enum {
		shape_LINE,
		shape_RECTANGLE,
		shape_FILLEDRECTANGLE
	} shapetype;
	int x0,y0,x1,y1;
	int thickness;
	unsigned int colour;
	struct shapedetails *next;
} shapedetails;

typedef struct textdetails {
	enum {
		text_LABEL,
		text_FIELD
	} texttype;
	struct textdetails *next;
} textdetails;

typedef struct persondetails {
	int width;
	int height;
	shapedetails *shapes;
	textdetails *text;
} persondetails;


static persondetails person;
static shapedetails *editingshape;
static Desk_window_handle editwin,toolboxpane,shapewin/*,textwin*/;
static Desk_menu_ptr shapemenu;

void EditGraphics_Open(void)
{
	Desk_Pane2_Show(editwin,Desk_open_CENTERED);
}

static void EditGraphics_UpdateShapeWindow(void)
{
	Desk_Icon_SetInteger(shapewin,editshape_X,editingshape->x0);
	Desk_Icon_SetInteger(shapewin,editshape_Y,editingshape->y0);
	Desk_Icon_SetInteger(shapewin,editshape_WIDTH,editingshape->x1);
	Desk_Icon_SetInteger(shapewin,editshape_HEIGHT,editingshape->y1);
	Desk_Icon_SetInteger(shapewin,editshape_THICKNESS,editingshape->thickness);
}

static void EditGraphics_ShapeMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	Desk_Icon_SetText(shapewin,editshape_TYPE,Desk_Menu_GetText(shapemenu,entry));
	switch (entry) {
		case shapemenu_LINE:
			editingshape->shapetype=shape_LINE;
			break;
		case shapemenu_RECTANGLE:
			editingshape->shapetype=shape_RECTANGLE;
			break;
		case shapemenu_FILLEDRECTANGLE:
			editingshape->shapetype=shape_FILLEDRECTANGLE;
			break;
	}
	Desk_Window_ForceRedraw(editwin,-INFINITY,0,INFINITY,INFINITY);
}

static Desk_bool EditGraphics_AddShape(Desk_event_pollblock *block,void *ref)
{
	shapedetails *newshape;
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	newshape=Desk_DeskMem_Malloc(sizeof(shapedetails));
	newshape->next=person.shapes;
	person.shapes=newshape;
	newshape->shapetype=shape_FILLEDRECTANGLE;
	newshape->x0=0;
	newshape->y0=0;
	newshape->x1=100;
	newshape->y1=100;
	newshape->thickness=0;
	newshape->colour=0; /*black*/
	editingshape=newshape;
	Desk_Window_Show(shapewin,Desk_open_UNDERPOINTER);
	EditGraphics_UpdateShapeWindow();
	return Desk_TRUE;
}

static Desk_bool EditGraphics_UpdateWidthAndHeight(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	person.width=Desk_Icon_GetInteger(editwin,icon_WIDTH);
	person.height=Desk_Icon_GetInteger(editwin,icon_HEIGHT);
	if (editingshape) {
		editingshape->x0=Desk_Icon_GetInteger(shapewin,editshape_X);
		editingshape->y0=Desk_Icon_GetInteger(shapewin,editshape_Y);
		editingshape->x1=Desk_Icon_GetInteger(shapewin,editshape_WIDTH);
		editingshape->y1=Desk_Icon_GetInteger(shapewin,editshape_HEIGHT);
		editingshape->thickness=Desk_Icon_GetInteger(shapewin,editshape_THICKNESS);
	}
	Desk_Window_ForceRedraw(editwin,-INFINITY,0,INFINITY,INFINITY);
	return Desk_FALSE;
}

static Desk_bool EditGraphics_CloseWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(block);
	Desk_UNUSED(ref);

	Desk_Pane2_Hide(editwin);
	return Desk_TRUE;
}
#define SWI_Wimp_AddMessages 0x400F6

#define SWI_ColourPicker_OpenDialogue 0x47702
#define SWI_ColourPicker_CloseDialogue 0x47703
#define SWI_ColourPicker_UpdateDialogue 0x47704
#define AJWLib_colourpicker_COLOURCHOICE 0x47700
#define AJWLib_colourpicker_CLOSEDIALOGUEREQUEST 0x47702
#define AJWLib_colourpicker_RESETCOLOURREQUEST 0x47704

typedef enum {
	colourpicker_NORMAL,
	colourpicker_MENU,
	colourpicker_TOOLBOX,
	colourpicker_SUBMENU
} AJWLib_colourpicker_type;

typedef int AJWLib_colourpicker_handle;

static lastinitcolour=0;

static Desk_bool AJWLib_ColourPicker_CloseRequest(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);

	Desk_Error2_CheckOS(Desk_SWI(2,0,SWI_ColourPicker_CloseDialogue,0,block->data.words[4]));
	return Desk_TRUE;
}

static Desk_bool AJWLib_ColourPicker_ResetColourRequest(Desk_event_pollblock *block,void *ref)
{
	int blk[10]={0,0,0,0x80000000,0x7FFFFFFF,0,0,0,0,0};

	Desk_UNUSED(ref);

	blk[8]=lastinitcolour;
	Desk_Error2_CheckOS(Desk_SWI(3,0,SWI_ColourPicker_UpdateDialogue,1<<6,block->data.words[5],blk));
	return Desk_TRUE;
}

AJWLib_colourpicker_handle AJWLib_ColourPicker_Open(AJWLib_colourpicker_type type,int x,int y,unsigned int initcolour,Desk_bool colournone,char *title,Desk_event_handler handler, void *ref)
{
	int blk[10];
	AJWLib_colourpicker_handle handle;
	Desk_window_handle window;
	static Desk_bool registered=Desk_FALSE;

	blk[0]=colournone & 1;
	blk[1]=(int)title; /*Evil cast*/
	blk[2]=x;
	blk[3]=0x80000000;
	blk[4]=0x7FFFFFFF;
	blk[5]=y;
	blk[6]=0;
	blk[7]=0;
	blk[8]=initcolour;
	blk[9]=0;
	Desk_Error2_CheckOS(Desk_SWI(2,2,SWI_ColourPicker_OpenDialogue,type,blk,&handle,&window));
	lastinitcolour=initcolour;
	if (!registered) {
		int msglist[2]={0x400C9,0};
		Desk_EventMsg_Claim((Desk_message_action)AJWLib_colourpicker_COLOURCHOICE,Desk_event_ANY,handler,ref);
		Desk_EventMsg_Claim((Desk_message_action)AJWLib_colourpicker_CLOSEDIALOGUEREQUEST,Desk_event_ANY,AJWLib_ColourPicker_CloseRequest,NULL);
		Desk_EventMsg_Claim((Desk_message_action)AJWLib_colourpicker_RESETCOLOURREQUEST,Desk_event_ANY,AJWLib_ColourPicker_ResetColourRequest,NULL);
		Desk_Error2_CheckOS(Desk_SWI(1,0,SWI_Wimp_AddMessages,msglist));
		registered=Desk_TRUE;
	}
	return handle;
}

static Desk_bool EditGraphics_ColourChoice(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);

	editingshape->colour=block->data.words[7];
	Desk_Window_ForceRedraw(editwin,-INFINITY,0,INFINITY,INFINITY);
	return Desk_TRUE;
}

static Desk_bool EditGraphics_PickColour(Desk_event_pollblock *block,void *ref)
{
	Desk_wimp_rect pos;

	Desk_UNUSED(ref);
	Desk_UNUSED(block);

	Desk_Icon_ScreenPos(shapewin,editshape_COLOURMENU,&pos);
	AJWLib_ColourPicker_Open(colourpicker_MENU,pos.max.x,pos.max.y,editingshape->colour,Desk_FALSE,/*"Fred's colour"*/NULL,EditGraphics_ColourChoice,NULL);
	return Desk_TRUE;
}

static Desk_bool EditGraphics_RedrawWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=block->data.openblock.window;
	Desk_UNUSED(ref);
	Desk_Wimp_RedrawWindow(&blk,&more);
	while (more) {
		shapedetails *shape=person.shapes;
		Draw_PlotRectangleFilled(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,0,0,person.width,person.height,0,0xFFFFFF00);
		while (shape) {
			switch (shape->shapetype) {
				case shape_LINE:
					Draw_PlotLine(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,shape->x0,shape->y0,shape->x1,shape->y1,shape->thickness,shape->colour);
					break;
				case shape_RECTANGLE:
					Draw_PlotRectangle(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,shape->x0,shape->y0,shape->x1,shape->y1,shape->thickness,shape->colour);
					break;
				case shape_FILLEDRECTANGLE:
					Draw_PlotRectangleFilled(100,blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y,shape->x0,shape->y0,shape->x1,shape->y1,shape->thickness,shape->colour);
					break;
			}
			shape=shape->next;
		}
		Desk_Wimp_GetRectangle(&blk,&more);
	}
	return Desk_TRUE;
}

void EditGraphics_Init(void)
{
	Desk_wimp_point paneoffset={-200,0};

	editwin=Desk_Pane2_CreateAndAddMain("EditStyle",Desk_template_TITLEMIN);
	toolboxpane=Desk_Pane2_CreateAndAddPane("EditToolbox",Desk_template_TITLEMIN,editwin,&paneoffset,NULL,Desk_pane2_PANETOP | Desk_pane2_MAINTOP | Desk_pane2_FIXED);

	shapewin=Desk_Window_Create("EditObject",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK, toolboxpane,icon_ADDSHAPE,EditGraphics_AddShape,NULL);
	Desk_Event_Claim(Desk_event_CLICK, shapewin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Event_Claim(Desk_event_KEY,   shapewin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_WIDTH,editshape_WIDTHUP,editshape_WIDTHDOWN,Desk_FALSE,5,0,9995,100);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_HEIGHT,editshape_HEIGHTUP,editshape_HEIGHTDOWN,Desk_FALSE,5,0,9995,100);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_THICKNESS,editshape_THICKNESSUP,editshape_THICKNESSDOWN,Desk_FALSE,1,0,9995,0);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_X,editshape_XUP,editshape_XDOWN,Desk_FALSE,5,-995,9995,0);
	Desk_Icon_InitIncDecHandler(shapewin,editshape_Y,editshape_YUP,editshape_YDOWN,Desk_FALSE,5,-995,9995,0);
	Desk_Event_Claim(Desk_event_CLICK, shapewin,editshape_COLOURMENU,EditGraphics_PickColour,NULL);

	Desk_Event_Claim(Desk_event_CLICK, editwin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,editwin,Desk_event_ANY,EditGraphics_RedrawWindow,NULL);
	Desk_Event_Claim(Desk_event_CLOSE, editwin,Desk_event_ANY,EditGraphics_CloseWindow,NULL);
	Desk_Event_Claim(Desk_event_KEY,   editwin,Desk_event_ANY,EditGraphics_UpdateWidthAndHeight,NULL);
	Desk_Icon_InitIncDecHandler(editwin,icon_WIDTH,icon_WIDTHUP,icon_WIDTHDOWN,Desk_FALSE,5,0,9995,400);
	Desk_Icon_InitIncDecHandler(editwin,icon_HEIGHT,icon_HEIGHTUP,icon_HEIGHTDOWN,Desk_FALSE,5,0,9995,200);
	person.width=400;
	person.height=200;
	person.text=NULL;
	person.shapes=NULL;
	editingshape=NULL;

	shapemenu=AJWLib_Menu_CreateFromMsgs("Title.Shape:","Menu.Shape:",EditGraphics_ShapeMenuClick,NULL);
	AJWLib_Menu_AttachPopup(shapewin,editshape_TYPEMENU,editshape_TYPE,shapemenu,Desk_button_MENU | Desk_button_SELECT);

}
