/*
	FT - File loading and saving
	© Alex Waugh 1999

	$Id: File.c,v 1.11 2000/02/20 23:03:15 uid1 Exp $

*/

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
#include "AJWLib.Assert.h"
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
	FILE *file=NULL;
	char fileid[]=FILEID;
	int fileversion=FILEVERSION;
	layout *normallayout=NULL,*layout=NULL;
	int nextwindow=0;
	strcpy(newfilename,filename);
	file=AJWLib_File_fopen(filename,"wb");
	AJWLib_File_fwrite(fileid,1,4,file);
	AJWLib_File_fwrite(&fileversion,4,1,file);
	Database_Save(file);
/*	Graphics_Save(file);*/
	while (nextwindow>=0) {
		if ((layout=Windows_Save(file,&nextwindow))!=NULL) normallayout=layout;
	}
	if (normallayout) Layout_Save(normallayout,file);
	AJWLib_File_fclose(file);
	/*Error handling*/
	return Desk_TRUE;
}

void File_LoadFile(char *filename)
{
	FILE *file=NULL;
	char fileid[5];
	layout *layout=NULL;
	int fileversion;
	file=AJWLib_File_fopen(filename,"rb");
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
	while (!feof(file)) {
		int size;
		tag tag;
		long int pos=ftell(file);
		AJWLib_File_fread(&tag,sizeof(tag),1,file);
		AJWLib_File_fread(&size,sizeof(int),1,file);
		if (feof(file)) break;
		switch (tag) {
			case tag_WINDOW:
				Windows_Load(file);
				break;
			case tag_DATABASE:
				AJWLib_Assert(Database_GetSize()==0); /*Make this a proper error?*/
				Database_Load(file);
				break;
			case tag_LAYOUT:
				AJWLib_Assert(layout==NULL); /*Make this a proper error?*/
				layout=Layout_Load(file);
				break;
			default:
				Desk_Error_Report(1,"Tag is %d",tag);
				AJWLib_Assert(0);
				fseek(file,pos+size,SEEK_SET);
		}
		if ((pos+size)!=ftell(file)) Desk_Error2_HandleText("Corrupt file or something");
	}
	AJWLib_File_fclose(file);
	Windows_LayoutNormal(layout);
	strcpy(currentfilename,filename);
	modified=Desk_FALSE;
	Windows_FilenameChanged(currentfilename);
	/*update date*/
	/*Error handling*/
}

void File_New(void)
{
	modified=Desk_FALSE;
	strcpy(currentfilename,"Untitled");
	strcpy(filedate,"Tomorrow"); /**/
	Database_New();
}

void File_Modified(void)
{
	modified=Desk_TRUE;
	Windows_FileModified();
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
			Windows_FilenameChanged(currentfilename);
			/*update date*/
			break;
		case Desk_save_RECEIVERFAILED:
			Desk_Error_Report(1,"Data transfer failed: reciever died"); /*msgs, Error2?*/
			break;
	}
}
