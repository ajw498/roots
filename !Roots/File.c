/*
	FT - File loading and saving
	© Alex Waugh 1999

	$Log: File.c,v $
	Revision 1.9  2000/01/17 16:58:13  AJW
	Enabled calling of Windows_Load

	Revision 1.8  2000/01/14 19:42:48  AJW
	Added File_LoadFile

	Revision 1.7  2000/01/14 13:50:31  AJW
	Renamed Graphics.h to Windows.h etc

	Revision 1.6  2000/01/14 13:08:22  AJW
	Changed Graphics_ to Windows_

	Revision 1.5  2000/01/13 23:30:36  AJW
	Added Graphics_GetDate

	Revision 1.4  2000/01/13 22:58:36  AJW
	Added call to Graphics_Save

	Revision 1.3  2000/01/13 18:07:36  AJW
	Added handling of modified flag, and storing current filename

	Revision 1.2  2000/01/13 17:02:09  AJW
	Altered to be a SaveHandler for Desk_Save_* fns

	Revision 1.1  2000/01/11 17:10:49  AJW
	Initial revision


*/

/*	Includes  */

#include "Desk.Window.h"
#include "Desk.Error.h"
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
#include "Desk.GFX.h"
#include "Desk.Font2.h"
#include "Desk.ColourTran.h"

#include "AJWLib.Window.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Icon.h"
#include "AJWLib.File.h"
#include "AJWLib.Flex.h"
#include "AJWLib.Str.h"
#include "AJWLib.Draw.h"
#include "AJWLib.DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "Windows.h"
#include "Config.h"
#include "Layout.h"
#include "File.h"

#define FILEID "Root"
#define FILEVERSION 100

static char currentfilename[256],newfilename[256],filedate[256];
static Desk_bool modified=Desk_FALSE;

Desk_bool File_SaveFile(char *filename,void *ref)
{
	FILE *file;
	char fileid[]=FILEID;
	int fileversion=FILEVERSION;
	strcpy(newfilename,filename);
	file=AJWLib_File_fopen(filename,"w");
	AJWLib_File_fwrite(fileid,1,4,file);
	AJWLib_File_fwrite(&fileversion,4,1,file);
	Database_Save(file);
/*	Graphics_Save(file);*/
	Windows_Save(file);
	AJWLib_File_fclose(file);
	/*Error handling*/
	return Desk_TRUE;
}

void File_LoadFile(char *filename)
{
	FILE *file;
	char fileid[5];
	int fileversion;
	strcpy(newfilename,filename);
	file=AJWLib_File_fopen(filename,"r");
	AJWLib_File_fread(fileid,1,4,file);
	AJWLib_File_fread(&fileversion,4,1,file);
	fileid[4]='\0';
	if (strcmp(fileid,FILEID)) {
		AJWLib_File_fclose(file);
		Desk_Error2_HandleText("This is not a Roots file"); /*msgs*/
	}
	if (fileversion>FILEVERSION) {
		if (fileversion-99>FILEVERSION) {
			AJWLib_File_fclose(file);
			Desk_Error_Report(1,"Hello ,%d",fileversion); /*Give a choice? msgs*/
			Desk_Error2_HandleText("This was created in a later version of Roots and I cannot handle it"); /*msgs*/
		}
		Desk_Error_Report(1,"This was produced by a later version of Roots, but I will try to load it anyway"); /*Give a choice? msgs*/
	}
	Database_Load(file);
/*	Graphics_Load(file);*/
	Windows_Load(file);
	AJWLib_File_fclose(file);
	/*Error handling*/
}

void File_NewFile(void)
{
	modified=Desk_FALSE;
	strcpy(currentfilename,"Untitled");
	strcpy(filedate,"Tomorrow"); /**/
}

void File_Modified(void)
{
	modified=Desk_TRUE;
}

Desk_bool File_GetModified(void)
{
	return modified;
}

int File_GetSize(void)
{
	int size=4*sizeof(char)+sizeof(int);
	size+=Database_GetSize();
/*	size+=GraphicsConfig_GetSize();*/
	size+=Windows_GetSize();
	return size;
}

char *File_GetFilename(void)
{
	return currentfilename;
}

char *File_GetDate(void)
{
	return filedate;
}

void File_Result(Desk_save_result result,void *ref)
{
	switch (result) {
		case Desk_save_SAVEOK:
			strcpy(currentfilename,newfilename);
			modified=Desk_FALSE;
			/*Update window titles*/
			/*update date*/
			break;
		case Desk_save_RECEIVERFAILED:
			Desk_Error_Report(1,"Data transfer failed: reciever died"); /*msgs, Error2?*/
			break;
	}
}
