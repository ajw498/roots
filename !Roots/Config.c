/*
	FT - Configuration
	© Alex Waugh 1999

	$Id: Config.c,v 1.11 2000/03/04 23:52:29 uid1 Exp $

*/

#include "Desk.Core.h"
#include "Desk.Window.h"
#include "Desk.Icon.h"
#include "Desk.Error.h"
#include "Desk.Error2.h"
#include "Desk.File.h"
#include "Desk.Filing.h"
#include "Desk.Msgs.h"
#include "Desk.Menu.h"
#include "Desk.Event.h"
#include "Desk.Template.h"

#include "AJWLib.Assert.h"
#include "AJWLib.Error2.h"
#include "AJWLib.File.h"
#include "AJWLib.Window.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Menu.h"

#include <stdlib.h>

#include "Config.h"
#include "Modules.h"

#define GRAPHICSDIR "<Roots$Dir>.Graphic"

#define config_DEFAULTSTYLE 20
#define config_DEFAULTSTYLEMENU 18
#define config_IMPORTSTYLE 8
#define config_USER1 11
#define config_USER2 12
#define config_USER3 13
#define config_DISPLAYTITLE 17
#define config_AUTOINCREASE 9
#define config_AUTOINCREASEONLY 10
#define config_SNAP 22
#define config_SNAPDISTANCE 4
#define config_SNAPDISTANCETEXT 5
#define config_SCROLLSPEED 6
#define config_CANCEL 26
#define config_SAVE 25
#define config_OK 24

#define CONFIGFILEWRITE "<Roots$Dir>.Config" /*Use Choices:*/
#define CONFIGFILEREAD "<Roots$Dir>.Config" /*Use Choices:*/

typedef struct configdata {
	char graphicsstyle[256];
	Desk_bool importgraphicsstyle;
	Desk_bool snap;
	Desk_bool title;
	int snapdistance;
	int scrollspeed;
	int scrolldistance;
	Desk_bool autoincreasesize;
	Desk_bool autoincreasealways;
	char userdesc[3][20];
} configdata;

configdata config={"Default",Desk_TRUE,Desk_TRUE,Desk_TRUE,30,10,30,Desk_TRUE,Desk_FALSE,{"Profession","Status","Other"}};
static Desk_window_handle configwin;
static Desk_menu_ptr configmenu;

char *Config_GraphicsStyle(void)
{
	return config.graphicsstyle;
}

Desk_bool Config_ImportGraphicsStyle(void)
{
	return config.importgraphicsstyle;
}

Desk_bool Config_Title(void)
{
	return config.title;
}

Desk_bool Config_Snap(void)
{
	return config.snap;
}

int Config_SnapDistance(void)
{
	return config.snapdistance;
}

int Config_ScrollSpeed(void)
{
	return config.scrollspeed;
}

int Config_ScrollDistance(void)
{
	return config.scrolldistance;
}

Desk_bool Config_AutoIncreaseSize(void)
{
	return config.autoincreasesize;
}

Desk_bool Config_AutoIncreaseAlways(void)
{
	return config.autoincreasealways;
}

char *Config_UserDesc(int num)
{
	AJWLib_Assert(num>0);
	AJWLib_Assert(num<4);
	return config.userdesc[num-1];
}

static Desk_bool Config_Ok(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	Desk_Icon_GetText(configwin,config_DEFAULTSTYLE,config.graphicsstyle);
	Desk_Icon_GetText(configwin,config_USER1,config.userdesc[0]);
	Desk_Icon_GetText(configwin,config_USER2,config.userdesc[1]);
	Desk_Icon_GetText(configwin,config_USER3,config.userdesc[2]);
	config.snapdistance=Desk_Icon_GetInteger(configwin,config_SNAPDISTANCE);
	config.scrollspeed=Desk_Icon_GetInteger(configwin,config_SCROLLSPEED);
	config.importgraphicsstyle=Desk_Icon_GetSelect(configwin,config_IMPORTSTYLE);
	config.snap=Desk_Icon_GetSelect(configwin,config_SNAP);
	config.title=Desk_Icon_GetSelect(configwin,config_DISPLAYTITLE);
	config.autoincreasesize=Desk_Icon_GetSelect(configwin,config_AUTOINCREASE);
	config.autoincreasealways=(Desk_bool)!Desk_Icon_GetSelect(configwin,config_AUTOINCREASEONLY);
	if (block->data.mouse.button.data.select) {
		Desk_Window_Hide(configwin);
		if (configmenu) {
			AJWLib_Menu_FullDispose(configmenu);
			configmenu=NULL;
		}
	}
	Modules_ChangedLayout();
	return Desk_TRUE;
}

static Desk_bool Config_Save(Desk_event_pollblock *block,void *ref)
{
	FILE *file=NULL;
	if (!Config_Ok(block,ref)) return Desk_FALSE;
	Desk_Error2_Try {
		file=AJWLib_File_fopen(CONFIGFILEWRITE,"wb");
		AJWLib_File_fwrite(&config,sizeof(config),1,file);
		AJWLib_File_fclose(file);
	} Desk_Error2_Catch {
		AJWLib_Error2_ReportMsgs("Error.ConfigS:%s");
	} Desk_Error2_EndCatch
	return Desk_TRUE;
}

static void Config_Load(void)
{
	FILE *file=NULL;
	Desk_Error2_Try {
		if (Desk_File_Exists(CONFIGFILEREAD)) {
			file=AJWLib_File_fopen(CONFIGFILEREAD,"rb");
			AJWLib_File_fread(&config,sizeof(config),1,file);
			AJWLib_File_fclose(file);
		}
	} Desk_Error2_Catch {
		AJWLib_Error2_ReportMsgs("Error.ConfigL:%s");
	} Desk_Error2_EndCatch
}

static Desk_bool Config_SnapClick(Desk_event_pollblock *block,void *ref)
{
	Desk_caret_block caretblk;
	Desk_bool shade=Desk_Icon_GetSelect(configwin,config_SNAP);
	Desk_UNUSED(ref);
	Desk_UNUSED(block);
	Desk_Wimp_GetCaretPosition(&caretblk);
	if (!shade && caretblk.window==configwin && caretblk.icon==config_SNAPDISTANCE) Desk_Icon_SetCaret(configwin,config_SCROLLSPEED);
	Desk_Icon_SetShade(configwin,config_SNAPDISTANCE,!shade);
	Desk_Icon_SetShade(configwin,config_SNAPDISTANCETEXT,!shade);
	return Desk_TRUE;
}

static Desk_bool Config_AutoIncreaseClick(Desk_event_pollblock *block,void *ref)
{
	Desk_bool shade=Desk_Icon_GetSelect(configwin,config_AUTOINCREASE);
	Desk_UNUSED(ref);
	Desk_UNUSED(block);
	Desk_Icon_SetShade(configwin,config_AUTOINCREASEONLY,!shade);
	return Desk_TRUE;
}

static void Config_MenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	Desk_Icon_SetText(configwin,config_DEFAULTSTYLE,Desk_Menu_GetText(configmenu,entry));
}

void Config_Open(void)
{
	Desk_filing_dirdata dir;
	char *name=NULL;
	Desk_Icon_SetText(configwin,config_DEFAULTSTYLE,config.graphicsstyle);
	Desk_Icon_SetText(configwin,config_USER1,config.userdesc[0]);
	Desk_Icon_SetText(configwin,config_USER2,config.userdesc[1]);
	Desk_Icon_SetText(configwin,config_USER3,config.userdesc[2]);
	Desk_Icon_SetInteger(configwin,config_SNAPDISTANCE,config.snapdistance);
	Desk_Icon_SetInteger(configwin,config_SCROLLSPEED,config.scrollspeed);
	Desk_Icon_SetSelect(configwin,config_IMPORTSTYLE,config.importgraphicsstyle);
	Desk_Icon_SetSelect(configwin,config_SNAP,config.snap);
	Desk_Icon_SetSelect(configwin,config_DISPLAYTITLE,config.title);
	Desk_Icon_SetSelect(configwin,config_AUTOINCREASE,config.autoincreasesize);
	Desk_Icon_SetSelect(configwin,config_AUTOINCREASEONLY,!config.autoincreasealways);
	Config_SnapClick(NULL,NULL);
	Config_AutoIncreaseClick(NULL,NULL);
	Desk_Filing_OpenDir(GRAPHICSDIR,&dir,256,Desk_readdirtype_NAMEONLY);
	do {
		name=Desk_Filing_ReadDir(&dir);
		if (name) {
			if (configmenu) {
				configmenu=Desk_Menu_Extend(configmenu,name);
			} else {
				configmenu=Desk_Menu_New(AJWLib_Msgs_TempLookup("Title.Config:"),name);
			}
		}
	} while (name);
	Desk_Filing_CloseDir(&dir);
	Desk_Window_Show(configwin,Desk_open_CENTERED);
	Desk_Icon_SetCaret(configwin,config_USER1);
	AJWLib_Menu_Register(configmenu,Config_MenuClick,NULL);
	AJWLib_Menu_AttachPopup(configwin,config_DEFAULTSTYLEMENU,config_DEFAULTSTYLE,configmenu,Desk_button_MENU | Desk_button_SELECT);
}

static Desk_bool Config_Cancel(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.select) {
		Desk_Window_Hide(configwin);
		if (configmenu) {
			AJWLib_Menu_FullDispose(configmenu);
			configmenu=NULL;
		}
		return Desk_TRUE;
	}
	return Desk_FALSE;
}

void Config_Init(void)
{
	configwin=Desk_Window_Create("Config",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,configwin,config_OK,Config_Ok,NULL);
	Desk_Event_Claim(Desk_event_CLICK,configwin,config_CANCEL,Config_Cancel,NULL);
	Desk_Event_Claim(Desk_event_CLICK,configwin,config_SAVE,Config_Save,NULL);
	Desk_Event_Claim(Desk_event_CLICK,configwin,config_SNAP,Config_SnapClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,configwin,config_AUTOINCREASE,Config_AutoIncreaseClick,NULL);
	AJWLib_Window_KeyHandler(configwin,config_OK,Config_Ok,config_CANCEL,Config_Cancel,NULL);
	Config_Load();
}

