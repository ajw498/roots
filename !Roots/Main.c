/*
	FT - Main program
	© Alex Waugh 1999

	$Log: Main.c,v $
	Revision 1.1  1999/09/27 15:25:18  AJW
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
#include "DeskLib:Resource.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Template.h"
#include "DeskLib:File.h"
#include "DeskLib:Filing.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:GFX.h"
#include "DeskLib:ColourTran.h"

#include "AJWLib:Window.h"
#include "AJWLib:Menu.h"
#include "AJWLib:Msgs.h"
#include "AJWLib:Misc.h"
#include "AJWLib:Handler.h"
#include "AJWLib:Error.h"
#include "AJWLib:Flex.h"
#include "AJWLib:DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Modules.h"
#include "Graphics.h"
#include "Layout.h"

/*	Macros  */

#define VERSION "0.01 (01-Apr-99)"
#define DIRPREFIX "FT"

#define iconbarmenu_INFO 0
#define iconbarmenu_CHOICES 1
#define iconbarmenu_QUIT 2

/*	Variables  */

window_handle info;
menu_ptr iconbarmenu;

#if DEBUG
extern BOOL halt;
#endif
/*	Functions  */



BOOL ReceiveDrag(event_pollblock *block, void *r)
{
	Database_Load(block->data.message.data.dataload.filename);
	return TRUE;
}

BOOL IconBarClick(event_pollblock *block, void *r)
{
	if (block->data.mouse.button.data.select==1) {
		Graphics_OpenWindow(wintype_UNLINKED,none,0);
		Graphics_OpenWindow(wintype_NORMAL,none,0);
		return TRUE;
	}
#if DEBUG
if (block->data.mouse.button.data.adjust==1) halt=FALSE;
#endif
	return FALSE;
}

void IconBarMenuClick(int entry, void *r)
{
	switch (entry) {
		case iconbarmenu_QUIT:
			Event_CloseDown();
			break;
	}
}

int main(void)
{

	Error_RegisterSignals();
	Resource_Initialise(DIRPREFIX);
	Msgs_LoadFile("Messages");
	Event_Initialise(Msgs_TempLookup("Task.Name:"));
	EventMsg_Initialise();
	Screen_CacheModeInfo();      /*Errors*/
	Template_Initialise();
	EventMsg_Claim(message_MODECHANGE,event_ANY,Handler_ModeChange,NULL);
	Event_Claim(event_CLOSE,event_ANY,event_ANY,Handler_CloseWindow,NULL);
	Event_Claim(event_OPEN,event_ANY,event_ANY,Handler_OpenWindow,NULL);
	Event_Claim(event_KEY,event_ANY,event_ANY,Handler_KeyPress,NULL);
	Event_Claim(event_REDRAW,event_ANY,event_ANY,Handler_HatchRedraw,NULL);
	Icon_BarIcon(Msgs_TempLookup("Task.Icon:"),iconbar_RIGHT);
	info=Window_CreateInfoWindowFromMsgs("Task.Name:","Task.Purpose:","©Alex Waugh 1999",VERSION);
	Template_LoadFile("Templates");
	iconbarmenu=Menu_CreateFromMsgs("Title.IconBar:","Menu.IconBar:Info,Quit",IconBarMenuClick,NULL);
	Menu_AddSubMenu(iconbarmenu,iconbarmenu_INFO,(menu_ptr)info);
	Menu_Attach(window_ICONBAR,event_ANY,iconbarmenu,button_MENU);
	Event_Claim(event_CLICK,window_ICONBAR,event_ANY,IconBarClick,NULL);
	EventMsg_Claim(message_DATALOAD,event_ANY,ReceiveDrag,NULL);
	Flex_InitDA("Task.Name:","DA.MaxSize:16");
	Modules_Init();
	while (TRUE) {
		Modules_ReflectChanges();
		Event_Poll();
	}
	return 0;
}

