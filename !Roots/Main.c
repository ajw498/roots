/*
	FT - Main program
	© Alex Waugh 1999
	Started on 01-Apr-99 (Honest!)

	$Log: Main.c,v $
	Revision 1.9  2000/01/17 16:47:43  AJW
	Altered Windows_OpenWindow calls

	Revision 1.8  2000/01/14 19:43:04  AJW
	Enabled file loading

	Revision 1.7  2000/01/14 13:45:57  AJW
	Renamed Graphics.h to Windows.h

	Revision 1.6  2000/01/14 13:02:49  AJW
	Changed Graphics_ to Windows_

	Revision 1.5  2000/01/11 17:13:24  AJW
	Removed Database_Load until it works

	Revision 1.4  1999/10/11 22:30:06  AJW
	Changed to use Error2

	Revision 1.3  1999/10/10 20:55:20  AJW
	Modified to use Desk

	Revision 1.2  1999/10/02 18:46:37  AJW
	I'm not sure what has changed

	Revision 1.1  1999/09/27 15:25:18  AJW
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
#include "Desk.Resource.h"
#include "Desk.Screen.h"
#include "Desk.Template.h"
#include "Desk.File.h"
#include "Desk.Filing.h"
#include "Desk.Sprite.h"
#include "Desk.GFX.h"
#include "Desk.ColourTran.h"

#include "AJWLib.Window.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Error2.h"
#include "AJWLib.Flex.h"
#include "AJWLib.DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Modules.h"
#include "Windows.h"
#include "File.h"

/*	Macros  */

#define VERSION "0.50 (27-Oct-99)"
#define DIRPREFIX "Roots"

#define iconbarmenu_INFO 0
#define iconbarmenu_CHOICES 1
#define iconbarmenu_QUIT 2

/*	Variables  */

Desk_window_handle info;
Desk_menu_ptr iconbarmenu;

#if DEBUG
extern Desk_bool halt;
#endif
/*	Functions  */



Desk_bool ReceiveDrag(Desk_event_pollblock *block, void *r)
{
	File_LoadFile(block->data.message.data.dataload.filename);
	return Desk_TRUE;
}

Desk_bool IconBarClick(Desk_event_pollblock *block, void *r)
{
	if (block->data.mouse.button.data.select==1) {
		Windows_OpenWindow(wintype_UNLINKED,none,0,NULL);
		Windows_OpenWindow(wintype_NORMAL,none,0,NULL);
		return Desk_TRUE;
	}
#if DEBUG
	if (block->data.mouse.button.data.adjust==1) halt=Desk_FALSE;
#endif
	return Desk_FALSE;
}

void IconBarMenuClick(int entry, void *r)
{
	switch (entry) {
		case iconbarmenu_QUIT:
			Desk_Event_CloseDown();
			break;
	}
}

int main(void)
{
	Desk_Debug_SetLevel(99);
	Desk_Error2_Init_JumpSig(); /*Just call handleallsignals here?*/
	Desk_Error2_SetHandler(AJWLib_Error2_ReportFatal);
	Desk_Resource_Initialise(DIRPREFIX);
	Desk_Msgs_LoadFile("Messages");
	Desk_Event_Initialise(AJWLib_Msgs_Lookup("Task.Name:"));
	Desk_EventMsg_Initialise();
	Desk_Screen_CacheModeInfo();
	Desk_Template_Initialise();
	Desk_EventMsg_Claim(Desk_message_MODECHANGE,Desk_event_ANY,Desk_Handler_ModeChange,NULL);
	Desk_Event_Claim(Desk_event_CLOSE,Desk_event_ANY,Desk_event_ANY,Desk_Handler_CloseWindow,NULL);
	Desk_Event_Claim(Desk_event_OPEN,Desk_event_ANY,Desk_event_ANY,Desk_Handler_OpenWindow,NULL);
	Desk_Event_Claim(Desk_event_KEY,Desk_event_ANY,Desk_event_ANY,Desk_Handler_Key,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,Desk_event_ANY,Desk_event_ANY,Desk_Handler_HatchRedraw,NULL);
	Desk_Icon_BarIcon(AJWLib_Msgs_TempLookup("Task.Icon:"),Desk_iconbar_RIGHT);
	info=AJWLib_Window_CreateInfoWindowFromMsgs("Task.Name:","Task.Purpose:","©Alex Waugh 1999",VERSION);
	Desk_Template_LoadFile("Templates");
	iconbarmenu=AJWLib_Menu_CreateFromMsgs("Title.IconBar:","Menu.IconBar:Info,Quit",IconBarMenuClick,NULL);
	Desk_Menu_AddSubMenu(iconbarmenu,iconbarmenu_INFO,(Desk_menu_ptr)info);
	AJWLib_Menu_Attach(Desk_window_ICONBAR,Desk_event_ANY,iconbarmenu,Desk_button_MENU);
	Desk_Event_Claim(Desk_event_CLICK,Desk_window_ICONBAR,Desk_event_ANY,IconBarClick,NULL);
	Desk_EventMsg_Claim(Desk_message_DATALOAD,Desk_event_ANY,ReceiveDrag,NULL);
	AJWLib_Flex_InitDA("Task.Name:","DA.MaxSize:16");
	Modules_Init();
	while (Desk_TRUE) {
		/*put in a try catch type error handling here?*/
		Modules_ReflectChanges();
		Desk_Event_Poll();
	}
	return 0;
}

