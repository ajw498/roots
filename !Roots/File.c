/*
	FT - File loading and saving
	© Alex Waugh 1999

	$Id: File.c,v 1.15 2000/02/26 17:47:24 uid1 Exp $

*/

#include "Desk.Error.h"
#include "Desk.Msgs.h"
#include "Desk.File.h"
#include "Desk.Icon.h"
#include "Desk.KernelSWIs.h"

#include "AJWLib.Msgs.h"
#include "AJWLib.File.h"
#include "AJWLib.Assert.h"

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

#define SWI_Territory_ConvertStandardDateAndTime 0x4304C

static char currentfilename[256],oldfilename[256],filedate[256];
static Desk_bool modified=Desk_FALSE,oldmodified;

void File_GetCurrentTime(void)
{
	char *buffer[5];
	*((int *)buffer)=3;
	Desk_OS_Word(Desk_osword_READCMOSCLOCK,buffer);
	Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_Territory_ConvertStandardDateAndTime,-1,buffer,filedate,sizeof(filedate)));
}

Desk_bool File_SaveFile(char *filename,void *ref)
{
	FILE *file=NULL;
	char fileid[]=FILEID;
	int fileversion=FILEVERSION;
	layout *normallayout=NULL,*layout=NULL;
	int nextwindow=0;
	file=AJWLib_File_fopen(filename,"wb");
	AJWLib_File_fwrite(fileid,1,4,file);
	AJWLib_File_fwrite(&fileversion,4,1,file);
	Database_Save(file);
	Graphics_Save(file);
	while (nextwindow>=0) {
		if ((layout=Windows_Save(file,&nextwindow))!=NULL) normallayout=layout;
	}
	if (normallayout) Layout_Save(normallayout,file);
	AJWLib_File_fclose(file);
	/*Error handling*/
	strcpy(oldfilename,currentfilename);
	strcpy(currentfilename,filename);
	oldmodified=modified;
	modified=Desk_FALSE; /*This is assumed to have been ok unless File_Result told otherwise, as File_result does not get called if all ok when save button is clicked on */
	Windows_FilenameChanged(currentfilename);
	File_GetCurrentTime();
    return Desk_TRUE;
}

void File_LoadFile(char *filename)
{
	FILE *file=NULL;
	char fileid[5];
	char fivebytedate[5];
	layout *layout=NULL;
	Desk_bool graphicsloaded=Desk_FALSE;
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
			case tag_GRAPHICS:
				AJWLib_Assert(graphicsloaded==Desk_FALSE);
				graphicsloaded=Desk_TRUE;
				Graphics_Load(file);
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
	if (!graphicsloaded) Graphics_LoadStyle(Config_GraphicsStyle());
	strcpy(currentfilename,filename);
	modified=Desk_FALSE;
	Windows_FilenameChanged(currentfilename);
	Desk_File_Date(filename,fivebytedate);
	Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_Territory_ConvertStandardDateAndTime,-1,fivebytedate,filedate,sizeof(filedate)));
	/*Error handling*/
}

void File_New(void)
{
	modified=Desk_FALSE;
	strcpy(currentfilename,"Untitled");
	Windows_FilenameChanged(currentfilename);
	Database_New();
	Graphics_LoadStyle(Config_GraphicsStyle());
	Windows_OpenWindow(wintype_UNLINKED,none,0,100,NULL);
	Windows_OpenWindow(wintype_NORMAL,none,0,100,NULL);
	Windows_LayoutNormal(NULL);
	File_GetCurrentTime();
}

void File_Remove(void)
{
	Database_Remove();
	Graphics_RemoveStyle();
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
	size+=Graphics_GetSize();
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
			break;
		case Desk_save_RECEIVERFAILED:
			strcpy(currentfilename,oldfilename);
			modified=oldmodified;
			/*date=olddate;*/
			Windows_FilenameChanged(currentfilename);
			Desk_Error_Report(1,"Data transfer failed: reciever died"); /*msgs, Error2?*/
			break;
	}
}
