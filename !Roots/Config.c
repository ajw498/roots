/*
	FT - Configuration
	© Alex Waugh 1999

	$Id: Config.c,v 1.18 2000/11/12 16:13:19 AJW Exp $

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
#include "AJWLib.Str.h"
#include "AJWLib.Window.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Menu.h"

#include <stdlib.h>
#include <string.h>

#include "Config.h"
#include "Modules.h"

#define config_IMPORTSTYLE 6
#define config_DISPLAYTITLE 9
#define config_AUTOINCREASE 7
#define config_AUTOINCREASEONLY 8
#define config_SNAP 11
#define config_SNAPDISTANCE 2
#define config_SNAPDISTANCETEXT 3
#define config_SCROLLSPEED 4
#define config_CANCEL 15
#define config_SAVE 14
#define config_OK 13
#define config_FONTBLEND 16

typedef struct configdata {
	Desk_bool separatemarriages;
	Desk_bool importgraphicsstyle;
	Desk_bool snap;
	Desk_bool title;
	int snapdistance;
	int scrollspeed;
	int scrolldistance;
	Desk_bool autoincreasesize;
	Desk_bool autoincreasealways;
	Desk_bool fontblend;
} configdata;

static configdata config;
static Desk_window_handle configwin;
char choicesread[256],choiceswrite[256];

static void Config_SetChoicesPath(void)
{
	/*Set up defaults*/
	strcpy(choicesread,"<Roots$Dir>.Defaults");
	strcpy(choiceswrite,"Null:Choices");
	Desk_Error2_Try {
		if (strcmp(getenv("Choices$Write"),"")) {
			/*We are running a new boot structure*/
			if (Desk_File_IsDirectory("<Roots$Dir>.Choices")) {
				/*We already have a directory inside !Roots*/
				if (Desk_File_IsDirectory("Choices:Roots")) {
					/*We also have a directory in Choices: so remove the one inside !Roots*/
					Desk_SWI(4,0,Desk_SWI_OS_FSControl,27,"<Roots$Dir>.Choices",NULL,0x03);
				} else {
					/*Move the directory from inside !Roots to Choices:*/
					Desk_Error2_CheckOS(Desk_SWI(4,0,Desk_SWI_OS_FSControl,26,"<Roots$Dir>.Choices","<Choices$Write>.Roots",0x83));
				}
			} else {
				if (!Desk_File_IsDirectory("Choices:Roots")) {
					/*We have no choices anywhere, so copy defaults to Choices:*/
					Desk_Error2_CheckOS(Desk_SWI(4,0,Desk_SWI_OS_FSControl,26,"<Roots$Dir>.Defaults","<Choices$Write>.Roots",0x03));
				}
			}
			strcpy(choicesread,"Choices:Roots");
			strcpy(choiceswrite,"<Choices$Write>.Roots");
		} else {
			/*We have no new boot structure*/
			if (!Desk_File_IsDirectory("<Roots$Dir>.Choices")) {
				/*We don't have a directory inside !Roots, so copy defaults*/
				Desk_Error2_CheckOS(Desk_SWI(4,0,Desk_SWI_OS_FSControl,26,"<Roots$Dir>.Defaults","<Roots$Dir>.Choices",0x03));
			}
			strcpy(choicesread,"<Roots$Dir>.Choices");
			strcpy(choiceswrite,"<Roots$Dir>.Choices");
		}
	} Desk_Error2_Catch {
		AJWLib_Error2_ReportMsgs("Error.Choices:%s");
	} Desk_Error2_EndCatch
}

static void Config_Default(void)
{
	config.importgraphicsstyle=Desk_TRUE;
	config.snap=Desk_TRUE;
	config.title=Desk_TRUE;
	config.snapdistance=30;
	config.scrollspeed=10;
	config.scrolldistance=50;
	config.autoincreasesize=Desk_TRUE;
	config.autoincreasealways=Desk_FALSE;
	config.fontblend=Desk_FALSE;
	config.separatemarriages=Desk_TRUE;
}

Desk_bool Config_SeparateMarriages(void)
{
	return config.separatemarriages;
}

Desk_bool Config_ImportGraphicsStyle(void)
{
	return config.importgraphicsstyle;
}

Desk_bool Config_Title(void)
{
	return config.title;
}

Desk_bool Config_FontBlend(void)
{
	return config.fontblend;
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

static Desk_bool Config_Ok(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	config.snapdistance=Desk_Icon_GetInteger(configwin,config_SNAPDISTANCE);
	config.scrollspeed=Desk_Icon_GetInteger(configwin,config_SCROLLSPEED);
	config.importgraphicsstyle=Desk_Icon_GetSelect(configwin,config_IMPORTSTYLE);
	config.snap=Desk_Icon_GetSelect(configwin,config_SNAP);
	config.title=Desk_Icon_GetSelect(configwin,config_DISPLAYTITLE);
	config.fontblend=Desk_Icon_GetSelect(configwin,config_FONTBLEND);
	config.autoincreasesize=Desk_Icon_GetSelect(configwin,config_AUTOINCREASE);
	config.autoincreasealways=(Desk_bool)!Desk_Icon_GetSelect(configwin,config_AUTOINCREASEONLY);
	if (block->data.mouse.button.data.select) Desk_Window_Hide(configwin);
	Modules_ChangedStructure();
	return Desk_TRUE;
}

static Desk_bool Config_Save(Desk_event_pollblock *block,void *ref)
{
	FILE *file=NULL;
	char filename[256];
	if (!Config_Ok(block,ref)) return Desk_FALSE;
	Desk_Error2_Try {
		if (!Desk_File_IsDirectory(choiceswrite)) Desk_File_EnsureDirectory(choiceswrite);
		sprintf(filename,"%s.%s",choiceswrite,CONFIGFILE);
		file=AJWLib_File_fopen(filename,"w");
		fprintf(file,"Choices file for Roots\n\n");
		fprintf(file,"ImportGraphicsStyle:%d\n",config.importgraphicsstyle);
		fprintf(file,"SnapToPlace:%d\n",config.snap);
		fprintf(file,"SnapDistance:%d\n",config.snapdistance);
		fprintf(file,"DisplayTitle:%d\n",config.title);
		fprintf(file,"ScrollSpeed:%d\n",config.scrollspeed);
		fprintf(file,"ScrollDistance:%d\n",config.scrolldistance);
        fprintf(file,"AutoIncreaseWindowSize:%d\n",config.autoincreasesize);
        fprintf(file,"AlwaysAutoIncrease:%d\n",config.autoincreasealways);
        fprintf(file,"FontBackgroundBlending:%d\n",config.fontblend);
		AJWLib_File_fclose(file);
	} Desk_Error2_Catch {
		AJWLib_Error2_ReportMsgs("Error.ConfigS:%s");
	} Desk_Error2_EndCatch
	return Desk_TRUE;
}

static void Config_Load(void)
{
	FILE *file=NULL;
	char filename[256];
	Config_Default();
	sprintf(filename,"%s.%s",choicesread,CONFIGFILE);
	Desk_Error2_Try {
		if (Desk_File_Exists(filename)) {
			file=AJWLib_File_fopen(filename,"r");
			while (!feof(file)) {
				char line[256]="";
				char *brk;
				fscanf(file,"%255[^\n]\n",line);
				brk=strchr(line,':');
				if (brk) {
					int val;
					char *end;
					*(brk++)='\0';
					val=(int)strtol(brk,&end,10);
					AJWLib_Str_LowerCase(line);
					if (!strcmp(line,"importgraphicsstyle")) config.importgraphicsstyle=(Desk_bool)val;
					else if (!strcmp(line,"snaptoplace")) config.snap=(Desk_bool)val;
					else if (!strcmp(line,"snapdistance")) config.snapdistance=val;
					else if (!strcmp(line,"displaytitle")) config.title=(Desk_bool)val;
					else if (!strcmp(line,"scrollspeed")) config.scrollspeed=val;
					else if (!strcmp(line,"scrolldistance")) config.scrolldistance=val;
			        else if (!strcmp(line,"autoincreasewindowsize")) config.autoincreasesize=(Desk_bool)val;
			        else if (!strcmp(line,"alwaysautoincrease")) config.autoincreasealways=(Desk_bool)val;
			        else if (!strcmp(line,"fontbackgroundblending")) config.fontblend=(Desk_bool)val;
				}
			}
			AJWLib_File_fclose(file);
		}
	} Desk_Error2_Catch {
		AJWLib_Error2_ReportMsgs("Error.ConfigL:%s");
		Config_Default();
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

void Config_Open(void)
{
	Desk_Icon_SetInteger(configwin,config_SNAPDISTANCE,config.snapdistance);
	Desk_Icon_SetInteger(configwin,config_SCROLLSPEED,config.scrollspeed);
	Desk_Icon_SetSelect(configwin,config_IMPORTSTYLE,config.importgraphicsstyle);
	Desk_Icon_SetSelect(configwin,config_SNAP,config.snap);
	Desk_Icon_SetSelect(configwin,config_DISPLAYTITLE,config.title);
	Desk_Icon_SetSelect(configwin,config_FONTBLEND,config.fontblend);
	Desk_Icon_SetSelect(configwin,config_AUTOINCREASE,config.autoincreasesize);
	Desk_Icon_SetSelect(configwin,config_AUTOINCREASEONLY,!config.autoincreasealways);
	Config_SnapClick(NULL,NULL);
	Config_AutoIncreaseClick(NULL,NULL);
	Desk_Window_Show(configwin,Desk_open_CENTERED);
	Desk_Icon_SetCaret(configwin,-1);
}

static Desk_bool Config_Cancel(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.select) {
		Desk_Window_Hide(configwin);
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
	Config_SetChoicesPath();
	Config_Load();
}

