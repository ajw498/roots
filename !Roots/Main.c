/*
	FT - Main program
	© Alex Waugh 1999
	Started on 01-Apr-99 (Honest!)

	$Id: Main.c,v 1.14 2000/02/28 00:21:50 uid1 Exp $
	
*/


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
#include "config.h"


#define VERSION "0.50 (27-Oct-99)"
#define DIRPREFIX "Roots"

#define iconbarmenu_INFO 0
#define iconbarmenu_CHOICES 1
#define iconbarmenu_QUIT 2


Desk_window_handle info;
Desk_menu_ptr iconbarmenu;
char *taskname=NULL,*errbad=NULL;

#if DEBUG
extern Desk_bool halt;
#endif

static Desk_bool ReceiveDrag(Desk_event_pollblock *block, void *r)
{
	if (Database_GetSize()) {
		Desk_Msgs_Report(1,"Error.NoLoad:Another file already loaded");
	} else {
		File_LoadFile(block->data.message.data.dataload.filename);
	}
	return Desk_TRUE;
}

static Desk_bool IconBarClick(Desk_event_pollblock *block, void *r)
{
	if (block->data.mouse.button.data.select==1) {
		if (!Windows_BringToFront()) File_New();
		return Desk_TRUE;
	}
#if DEBUG
	if (block->data.mouse.button.data.adjust==1) halt=Desk_FALSE;
#endif
	return Desk_FALSE;
}

static void IconBarMenuClick(int entry, void *r)
{
	switch (entry) {
		case iconbarmenu_CHOICES:
			Config_Open();
			break;
		case iconbarmenu_QUIT:
			Desk_Event_CloseDown();
			break;
	}
}

int main(void)
{
	Desk_Error2_Init_JumpSig();
	Desk_Error2_Try {
		Desk_Resource_Initialise(DIRPREFIX);
		Desk_Msgs_LoadFile("Messages");
		Desk_Event_Initialise(taskname=AJWLib_Msgs_Lookup("Task.Name:"));
		errbad=AJWLib_Msgs_Lookup("Error.Bad:%s Click Ok to continue, Cancel to quit.");
		Desk_EventMsg_Initialise();
		Desk_Screen_CacheModeInfo();
		Desk_Template_Initialise();
		Desk_EventMsg_Claim(Desk_message_MODECHANGE,Desk_event_ANY,Desk_Handler_ModeChange,NULL);
		Desk_Event_Claim(Desk_event_CLOSE,Desk_event_ANY,Desk_event_ANY,Desk_Handler_CloseWindow,NULL);
		Desk_Event_Claim(Desk_event_OPEN,Desk_event_ANY,Desk_event_ANY,Desk_Handler_OpenWindow,NULL);
		Desk_Event_Claim(Desk_event_KEY,Desk_event_ANY,Desk_event_ANY,Desk_Handler_Key,NULL);
		Desk_Event_Claim(Desk_event_REDRAW,Desk_event_ANY,Desk_event_ANY,Desk_Handler_HatchRedraw,NULL);
		Desk_Icon_BarIcon(AJWLib_Msgs_TempLookup("Task.Icon:"),Desk_iconbar_RIGHT);
		info=AJWLib_Window_CreateInfoWindowFromMsgs("Task.Name:","Task.Purpose:","© Alex Waugh 1999",VERSION);
		Desk_Template_LoadFile("Templates");
		iconbarmenu=AJWLib_Menu_CreateFromMsgs("Title.IconBar:","Menu.IconBar:Info,Quit",IconBarMenuClick,NULL);
		Desk_Menu_AddSubMenu(iconbarmenu,iconbarmenu_INFO,(Desk_menu_ptr)info);
		AJWLib_Menu_Attach(Desk_window_ICONBAR,Desk_event_ANY,iconbarmenu,Desk_button_MENU);
		Desk_Event_Claim(Desk_event_CLICK,Desk_window_ICONBAR,Desk_event_ANY,IconBarClick,NULL);
		Desk_EventMsg_Claim(Desk_message_DATALOAD,Desk_event_ANY,ReceiveDrag,NULL);
		AJWLib_Flex_InitDA("Task.Name:","DA.MaxSize:16");
		Modules_Init();
	} Desk_Error2_Catch {
		AJWLib_Error2_Report("Fatal error while initialising (%s)");
		return EXIT_FAILURE;
	} Desk_Error2_EndCatch
	while (Desk_TRUE) {
		Desk_Error2_Try {
			Modules_ReflectChanges();
			Desk_Event_Poll();
		} Desk_Error2_Catch {
			Desk_os_error errblk={1,""};
			sprintf(errblk.errmess,errbad,AJWLib_Error2_Describe(&Desk_Error2_globalblock));
			if (Desk_Wimp_ReportErrorR(&errblk,3,taskname)==Desk_wimp_reporterror_button_CANCEL) return EXIT_FAILURE;
		} Desk_Error2_EndCatch
	}
	return EXIT_SUCCESS;
}

