/*
	Roots - Main program
	� Alex Waugh 1999
	Started on 01-Apr-99 (Honest!)

	$Id: Main.c,v 1.39 2002/08/01 15:36:22 ajw Exp $
	
*/

#ifdef MemCheck_MEMCHECK
#include "MemCheck:MemCheck.h"
#endif

#include "Desk/Window.h"
#include "Desk/Error2.h"
#include "Desk/Event.h"
#include "Desk/EventMsg.h"
#include "Desk/Handler.h"
#include "Desk/Hourglass.h"
#include "Desk/Icon.h"
#include "Desk/Menu.h"
#include "Desk/Msgs.h"
#include "Desk/Resource.h"
#include "Desk/Screen.h"
#include "Desk/Template.h"
#include "Desk/Str.h"
#include "Desk/File.h"
#include "Desk/Filing.h"
#include "Desk/Sprite.h"
#include "Desk/GFX.h"
#include "Desk/Hourglass.h"
#include "Desk/SWI.h"

#include "AJWLib/Window.h"
#include "AJWLib/Menu.h"
#include "AJWLib/Msgs.h"
#include "AJWLib/Error2.h"
#include "AJWLib/Flex.h"
#include "AJWLib/DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "Main.h"
#include "Modules.h"
#include "Windows.h"
#include "File.h"
#include "EditGraphics.h"
#include "Config.h"
#include "Shareware.h"

#define COPYRIGHT "� Alex Waugh 2000"

#define quiticon_DISCARD 0
#define quiticon_CANCEL 2

#define proginfo_AUTHOR 2
#define proginfo_VERSION 3
#define proginfo_LICENCE 9

#define iconbarmenu_INFO 0
#define iconbarmenu_CHOICES 1
#define iconbarmenu_GRAPHICSSTYLES 2
#define iconbarmenu_QUIT 3


static Desk_window_handle proginfowin,quitwin;
static Desk_menu_ptr iconbarmenu;
static char *taskname=NULL,*errbad=NULL;

static Desk_bool ReceiveDrag(Desk_event_pollblock *block, void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.message.data.dataload.window == Desk_window_ICONBAR) {
		if (block->data.message.data.dataload.filetype!=ROOTS_FILETYPE && block->data.message.data.dataload.size<100 && Desk_stricmp(Desk_Str_LeafName(block->data.message.data.dataload.filename),"User")==0) {
			/* It must be a user details file, so copy it then load it*/
			char name[256];
	
			sprintf(name,"%s.User",choiceswrite);
			Desk_Error2_CheckOS(Desk_SWI(4,0,Desk_SWI_OS_FSControl,26,block->data.message.data.dataload.filename,name,0x03));
			Desk_Icon_SetText(proginfowin,proginfo_LICENCE,Shareware_GetUser());
		} else {
			if (Database_Loaded()) {
				Desk_Msgs_Report(1,"Error.NoLoad:Another file already loaded");
			} else {
				File_LoadGEDCOM(block->data.message.data.dataload.filename,block->data.message.data.dataload.filetype==ROOTS_FILETYPE ? Desk_FALSE : Desk_TRUE);
			}
		}
	} /*else {
		if (block->data.message.data.dataload.filetype==Desk_filetype_DRAWFILE) {
			Windows_AddDrawfile(block->data.message.data.dataload.window,&(block->data.message.data.dataload.pos),block->data.message.data.dataload.filename);
		}
	}*/
	return Desk_TRUE;
}

static Desk_bool ReceiveDataOpen(Desk_event_pollblock *block, void *ref)
{
	Desk_UNUSED(ref);
	/*Ignore if we already have a file loaded*/
	if (Database_Loaded()) return Desk_FALSE;
	/*Ignore if it isn't a treefile*/
	if (block->data.message.data.dataopen.filetype!=ROOTS_FILETYPE) return Desk_FALSE;
	/*Return message to say we'll load the file*/
	block->data.message.header.action=Desk_message_DATAOPEN;
	block->data.message.header.yourref=block->data.message.header.myref;
	Desk_Wimp_SendMessage(Desk_event_USERMESSAGEACK,&(block->data.message),block->data.message.header.sender,0);
	File_LoadGEDCOM(block->data.message.data.dataopen.filename,Desk_FALSE);
	return Desk_TRUE;
}

static Desk_bool IconBarClick(Desk_event_pollblock *block, void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.select==1) {
		if (!Windows_BringToFront()) File_New();
		return Desk_TRUE;
	}
	return Desk_FALSE;
}

static void IconBarMenuClick(int entry, void *ref)
{
	Desk_UNUSED(ref);
	switch (entry) {
		case iconbarmenu_CHOICES:
			Config_Open();
			break;
		case iconbarmenu_QUIT:
			if (File_GetModified()) AJWLib_Window_OpenTransient(quitwin); else Desk_Event_CloseDown();
			break;
	}
}

int main(int argc,char *argv[])
{
	Desk_Hourglass_On();
#ifdef MemCheck_MEMCHECK
	MemCheck_Init();
	MemCheck_RegisterArgs(argc,argv);
	MemCheck_InterceptSCLStringFunctions();
	MemCheck_SetStoreMallocFunctions(1);
	MemCheck_SetAutoOutputBlocksInfo(0);
/*	MemCheck_SetQuitting(1,1);*/
#endif
	Desk_Error2_Init_JumpSig();
	signal(SIGABRT,SIG_DFL);
	Desk_Error2_Try {
#ifdef MemCheck_MEMCHECK
		MemCheck_SetChecking(0,0);
#endif
		Desk_Resource_Initialise(DIRPREFIX);
#ifdef MemCheck_MEMCHECK
		MemCheck_SetChecking(1,1);
#endif
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
		Desk_Template_LoadFile("Templates");
		quitwin=Desk_Window_Create("Quit",Desk_template_TITLEMIN);
		proginfowin=Desk_Window_Create("ProgInfo",Desk_template_TITLEMIN);
		Desk_Icon_SetText(proginfowin,proginfo_AUTHOR,COPYRIGHT);
		Desk_Icon_SetText(proginfowin,proginfo_VERSION,ROOTS_VERSION);
		AJWLib_Window_RegisterDCS(quitwin,quiticon_DISCARD,quiticon_CANCEL,-1,NULL,NULL);
		iconbarmenu=AJWLib_Menu_CreateFromMsgs("Title.IconBar:","Menu.IconBar:Info,Quit",IconBarMenuClick,NULL);

		Desk_Menu_AddSubMenu(iconbarmenu,iconbarmenu_INFO,(Desk_menu_ptr)proginfowin);
		AJWLib_Menu_Attach(Desk_window_ICONBAR,Desk_event_ANY,iconbarmenu,Desk_button_MENU);
		Desk_Event_Claim(Desk_event_CLICK,Desk_window_ICONBAR,Desk_event_ANY,IconBarClick,NULL);
		Desk_EventMsg_Claim(Desk_message_DATALOAD,Desk_event_ANY,ReceiveDrag,NULL);
		Desk_EventMsg_Claim(Desk_message_DATAOPEN,Desk_event_ANY,ReceiveDataOpen,NULL);
		AJWLib_Flex_InitDA("Task.Name:","DA.MaxSize:16");
		Modules_Init();
		EditGraphics_IconBarMenu(iconbarmenu,iconbarmenu_GRAPHICSSTYLES);
		Desk_Icon_SetText(proginfowin,proginfo_LICENCE,Shareware_GetUser());
	} Desk_Error2_Catch {
		Desk_Hourglass_Off();
		AJWLib_Error2_Report("Fatal error while initialising (%s)");
		return EXIT_FAILURE;
	} Desk_Error2_EndCatch
	if (argc>2) {
		Desk_Msgs_Report(1,"Error.CmdLine:Too many arguments");
	} else if (argc==2) {
		File_LoadGEDCOM(argv[1],Desk_FALSE);
	}
	Desk_Hourglass_Off();
	while (Desk_TRUE) {
		Desk_Error2_Try {
			Modules_ReflectChanges();
			Desk_Event_Poll();
		} Desk_Error2_Catch {
			Desk_os_error errblk={1,""};
			if (Shareware_GetErrorStatus()) {
				AJWLib_Error2_Report("%s");
			} else {
				sprintf(errblk.errmess,errbad,AJWLib_Error2_Describe(&Desk_Error2_globalblock));
				if (Desk_Wimp_ReportErrorR(&errblk,3,taskname)==Desk_wimp_reporterror_button_CANCEL) return EXIT_FAILURE;
			}
		} Desk_Error2_EndCatch
	}
	return EXIT_SUCCESS;
}

