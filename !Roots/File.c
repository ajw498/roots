/*
	FT - File loading and saving
	© Alex Waugh 1999

	$Id: File.c,v 1.28 2000/09/14 13:50:09 AJW Exp $

*/

#include "Desk.Error.h"
#include "Desk.Error2.h"
#include "Desk.Msgs.h"
#include "Desk.File.h"
#include "Desk.Icon.h"
#include "Desk.KernelSWIs.h"
#include "Desk.DeskMem.h"
#include "Desk.str.h"

#include "AJWLib.Msgs.h"
#include "AJWLib.Error2.h"
#include "AJWLib.File.h"
#include "AJWLib.Assert.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "Main.h"
#include "Database.h"
#include "Graphics.h"
#include "Windows.h"
#include "Config.h"
#include "Layout.h"
#include "File.h"

#define FILEID "Root"
#define FILEVERSION 200
#define ID_NONE (-1)
#define MAXLINELEN 256

#define SWI_Territory_ConvertStandardDateAndTime 0x4304C

static char currentfilename[256],oldfilename[256],filedate[256];
static Desk_bool modified=Desk_FALSE,oldmodified;
static int *idarray=NULL,idarraysize=0;

static void File_GetCurrentTime(void)
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
	Desk_UNUSED(ref);
	if (filename==NULL) filename=currentfilename;
	Desk_Error2_TryCatch(file=AJWLib_File_fopen(filename,"wb");,AJWLib_Error2_ReportMsgs("Error.Save:%s"); return Desk_TRUE;)
	/*file is guaranteed to be valid if we get here*/
	Desk_Error2_Try {
		AJWLib_File_fwrite(fileid,1,4,file);
		AJWLib_File_fwrite(&fileversion,4,1,file);
		Database_Save(file);
		Graphics_Save(file);
		while (nextwindow>=0) {
			if ((layout=Windows_Save(file,&nextwindow))!=NULL) normallayout=layout;
		}
		if (normallayout) Layout_Save(normallayout,file);
		AJWLib_File_fclose(file);
		strcpy(oldfilename,currentfilename);
		strcpy(currentfilename,filename);
		oldmodified=modified;
		modified=Desk_FALSE; /*This is assumed to have been ok unless File_Result told otherwise, as File_result does not get called if all ok when save button is clicked on */
		Windows_FileModified();
		File_GetCurrentTime();
	} Desk_Error2_Catch {
		if (file) fclose(file);
		AJWLib_Error2_ReportMsgs("Error.Save:%s");
	} Desk_Error2_EndCatch
    return Desk_TRUE;
}

Desk_bool File_SaveGEDCOM(char *filename,void *ref)
{
	FILE *file=NULL;
	layout *normallayout=NULL,*layout=NULL;
	int nextwindow=0;

	AJWLib_Assert(filename!=NULL);
	Desk_UNUSED(ref);
	Desk_Error2_TryCatch(file=AJWLib_File_fopen(filename,"w");,AJWLib_Error2_ReportMsgs("Error.Save:%s"); return Desk_TRUE;)
	/*file is guaranteed to be valid if we get here*/
	Desk_Error2_Try {
		fprintf(file,"0 HEAD\n1 SOUR ROOTS\n2 VERS %s\n",ROOTS_VERSION);
		Database_SaveGEDCOM(file);
		Graphics_SaveGEDCOM(file);
		while (nextwindow>=0) {
			if ((layout=Windows_SaveGEDCOM(file,&nextwindow))!=NULL) normallayout=layout;
		}
		if (normallayout) Layout_SaveGEDCOM(normallayout,file);
		AJWLib_File_fclose(file);
	} Desk_Error2_Catch {
		if (file) fclose(file);
		AJWLib_Error2_ReportMsgs("Error.Save:%s");
	} Desk_Error2_EndCatch
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
	Desk_Error2_TryCatch(file=AJWLib_File_fopen(filename,"rb");,AJWLib_Error2_ReportMsgs("Error.Load:%s"); return;)
	/*file is guaranteed to be valid if we get here*/
	Desk_Error2_Try {
		AJWLib_File_fread(fileid,1,4,file);
		AJWLib_File_fread(&fileversion,4,1,file);
		fileid[4]='\0';
		if (strcmp(fileid,FILEID)) AJWLib_Error2_HandleMsgs("Error.NotRoot:This is not a Roots file");
		if (fileversion>FILEVERSION) {
			if (fileversion-99>FILEVERSION) AJWLib_Error2_HandleMsgs("Error.TooNew:This was created in a later version of Roots and I cannot handle it");
			Desk_Msgs_Report(1,"Error.New:This was produced by a later version of Roots, but I will try to load it anyway");
		}
		while (!feof(file)) {
			int size;
			tag tag;
			long int pos=ftell(file);
			fread(&tag,sizeof(tag),1,file);
			fread(&size,sizeof(int),1,file);
			if (feof(file)) break;
			switch (tag) {
				case tag_WINDOW:
					Windows_Load(file);
					break;
				case tag_DATABASE:
					if (Database_GetSize()) {
						Desk_Msgs_Report(1,"Error.TwoData:Two databases");
					} else {
						Database_Load(file);
					}
					break;
				case tag_LAYOUT:
					if (layout) {
						Desk_Msgs_Report(1,"Error.TwoLay:Two layouts");
					} else {
						layout=Layout_Load(file);
					}
					break;
				case tag_GRAPHICS:
					if (graphicsloaded) {
						Desk_Msgs_Report(1,"Error.TwoGra:Two graphics styles");
					} else {
						graphicsloaded=Desk_TRUE;
						Graphics_Load(file);
					}
					break;
				default:
					Desk_Msgs_Report(1,"Error.BadTag:Unknown tag (%d)",tag);
					fseek(file,pos+size,SEEK_SET);
			}
			if ((pos+size)!=ftell(file)) AJWLib_Error2_HandleMsgs("Error.Corrupt:Corrupt file");
		}
		AJWLib_File_fclose(file);
		Windows_LayoutNormal(layout,Desk_FALSE);
		if (!graphicsloaded) Graphics_LoadStyle(AJWLib_Msgs_TempLookup("Style.Default:Default"));
		strcpy(currentfilename,filename);
		modified=Desk_FALSE;
		Windows_FileModified();
		Desk_File_Date(filename,fivebytedate);
		Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_Territory_ConvertStandardDateAndTime,-1,fivebytedate,filedate,sizeof(filedate)));
	} Desk_Error2_Catch {
		AJWLib_Error2_ReportMsgs("Error.Load:%s");
		if (file) fclose(file);
		Graphics_RemoveStyle();
		Windows_CloseAllWindows();
		Database_Remove();
	} Desk_Error2_EndCatch
}

static elementptr File_GetElementFromID(int id,elementtype type)
/* Translate from ID to elementptr, creating a new element if nessecery*/
{
	int i;
	elementptr element=none;

	for (i=0;i<idarraysize;i++) {
		if (idarray[i]==id) element=i;
	}
	if (element==none) {
		/* The element does not exist, so create it*/
		switch (type) {
			case element_PERSON:
				element=Database_Add();
				break;
			case element_MARRIAGE:
				element=Database_AddMarriage();
				break;
			default:
				AJWLib_Assert(0);
		}
		/* Increase array size if nessecery, in blocks of 20 to avoid reallocing to often*/
		if (idarraysize<=element) {
			idarray=Desk_DeskMem_Realloc(idarray,(element+20)*sizeof(int));
			for (i=idarraysize;i<element+20;i++) idarray[i]=ID_NONE;
			idarraysize=element+20;
		}
		/* Add entry to array*/
		idarray[element]=id;
		return element;
	}
	return element;
}


static void File_HandleData(char *id,char *tag,char *data)
/* Handle a line of GEDCOM*/
{
	if (tag==NULL) return;
	if (!Desk_stricmp(tag,"HEAD.SOUR")) {
		if (strcmp(data,"ROOTS")) AJWLib_Error2_HandleMsgs("Error.NotRoot:");

	} else if (!Desk_stricmp(tag,"HEAD.SOUR.VERS")) {
		/*Check version?*/
	} else if (!Desk_stricmp(tag,"_FILEINFO._TITLE")) {
		Database_SetTitle(data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._NEXTNEWPERSON")) {
		Database_SetNextNewPerson(atoi(data));

	} else if (!Desk_stricmp(tag,"_FILEINFO._USER1")) {
		Database_SetUserDesc(0,data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._USER2")) {
		Database_SetUserDesc(1,data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._USER3")) {
		Database_SetUserDesc(2,data);

	} else if (!Desk_stricmp(tag,"INDI")) {
		/* Entry will be created for this person when the first data associated with them is processed*/
	} else if (!Desk_stricmp(tag,"INDI.NAME")) {
		char *ptr=data;
		elementptr person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		/*Find end of forename*/
		while (*ptr!='\0' && !isspace(*ptr)) ptr++;
		*(ptr++)='\0';
		Database_SetForename(person,data);
		data=ptr;
		/*Find end of middle names*/
		while (*ptr!='\0' && *ptr!='/') ptr++;
		*(ptr++)='\0';
		Database_SetMiddleNames(person,data);
		data=ptr;
		/*Find end of surname*/
		while (*ptr!='\0' && *ptr!='/') ptr++;
		*(ptr++)='\0';
		Database_SetSurname(person,data);

	} else if (!Desk_stricmp(tag,"INDI.SEX")) {
		elementptr person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		Database_SetSex(person,data[0]);

	} else if (!Desk_stricmp(tag,"INDI.BIRT.PLAC")) {
		elementptr person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		Database_SetPlaceOfBirth(person,data);

	} else if (!Desk_stricmp(tag,"INDI.BIRT.DATE")) {
		elementptr person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		Database_SetDOB(person,data);

	} else if (!Desk_stricmp(tag,"INDI.DEAT.DATE")) {
		elementptr person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		Database_SetDOD(person,data);

	} else if (!Desk_stricmp(tag,"INDI._USER1")) {
		elementptr person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		Database_SetUser(0,person,data);

	} else if (!Desk_stricmp(tag,"INDI._USER2")) {
		elementptr person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		Database_SetUser(1,person,data);

	} else if (!Desk_stricmp(tag,"INDI._USER3")) {
		elementptr person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		Database_SetUser(2,person,data);

	} else if (!Desk_stricmp(tag,"INDI.FAMS")) {
		elementptr marriage,person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		marriage=File_GetElementFromID(atoi(data),element_MARRIAGE);
		Database_SetMarriage(person,marriage);
		
	} else if (!Desk_stricmp(tag,"INDI.FAMC")) {
		elementptr marriage,person;

		person=File_GetElementFromID(atoi(id),element_PERSON);
		marriage=File_GetElementFromID(atoi(data),element_MARRIAGE);
		Database_SetParentsMarriage(person,marriage);

	} else if (!Desk_stricmp(tag,"FAM")) {
		/* Entry will be created for this marrriage when the first data associated with it is processed*/
	} else if (!Desk_stricmp(tag,"FAM.HUSB")) {
		elementptr marriage,person;

		person=File_GetElementFromID(atoi(data),element_PERSON);
		marriage=File_GetElementFromID(atoi(id),element_MARRIAGE);
		Database_SetPrincipal(marriage,person);
		
	} else if (!Desk_stricmp(tag,"FAM.WIFE")) {
		elementptr marriage,person;

		person=File_GetElementFromID(atoi(data),element_PERSON);
		marriage=File_GetElementFromID(atoi(id),element_MARRIAGE);
		Database_SetSpouse(marriage,person);
		
	} else if (!Desk_stricmp(tag,"FAM.CHIL")) {
		/* Do nothing, as database will sort this out after everyone has been loaded*/
	} else if (!Desk_stricmp(tag,"FAM._LINKN")) {
		elementptr marriage,nextmarriage;

		marriage=File_GetElementFromID(atoi(id),element_MARRIAGE);
		nextmarriage=File_GetElementFromID(atoi(data),element_MARRIAGE);
		Database_SetNextMarriage(marriage,nextmarriage);

	} else if (!Desk_stricmp(tag,"FAM._LINKP")) {
		elementptr marriage,previousmarriage;

		marriage=File_GetElementFromID(atoi(id),element_MARRIAGE);
		previousmarriage=File_GetElementFromID(atoi(data),element_MARRIAGE);
		Database_SetPreviousMarriage(marriage,previousmarriage);

	} else if (!Desk_stricmp(tag,"FAM.MARR.DATE")) {
		elementptr marriage;

		marriage=File_GetElementFromID(atoi(id),element_MARRIAGE);
		Database_SetMarriageDate(marriage,data);

	} else if (!Desk_stricmp(tag,"FAM.MARR.PLAC")) {
		elementptr marriage;

		marriage=File_GetElementFromID(atoi(id),element_MARRIAGE);
		Database_SetMarriagePlace(marriage,data);

	} else if (!Desk_stricmp(tag,"FAM.DIV")) {
		elementptr marriage;

		marriage=File_GetElementFromID(atoi(id),element_MARRIAGE);
		Database_SetDivorceDate(marriage,data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._PERSONFILE._LINE")) {
		Graphics_LoadPersonFileLine(data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._MARRIAGEFILE._LINE")) {
		Graphics_LoadMarriageFileLine(data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._TITLEFILE._LINE")) {
		Graphics_LoadTitleFileLine(data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._DIMENSIONSFILE._LINE")) {
		Graphics_LoadDimensionsFileLine(data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._CURRENTSTYLE")) {
		Graphics_SetGraphicsStyle(data);

	} else if (!Desk_stricmp(tag,"_WINDOWS._TYPE")) {
		Windows_CreateWindow((wintype)atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCREENRECT._MIN._X")) {
		Windows_SetMinX(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCREENRECT._MIN._Y")) {
		Windows_SetMinY(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCREENRECT._MAX._X")) {
		Windows_SetMaxX(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCREENRECT._MAX._Y")) {
		Windows_SetMaxY(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCROLL._X")) {
		Windows_SetScrollX(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCROLL._Y")) {
		Windows_SetScrollY(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._PERSON")) {
		elementptr person;

		person=File_GetElementFromID(atoi(data),element_PERSON);
		Windows_SetPerson(person);

	} else if (!Desk_stricmp(tag,"_WINDOWS._GENERATIONS")) {
		Windows_SetGenerations(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._SCALE")) {
		Windows_SetScale(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._PERSON")) {
		elementptr person;

		person=File_GetElementFromID(atoi(data),element_PERSON);
		Layout_GEDCOMNewPerson(person);

	} else if (!Desk_stricmp(tag,"_LAYOUT._PERSON._X")) {
		Layout_GEDCOMNewPersonX(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._PERSON._Y")) {
		Layout_GEDCOMNewPersonY(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._MARRIAGE")) {
		elementptr marriage;

		marriage=File_GetElementFromID(atoi(data),element_MARRIAGE);
		Layout_GEDCOMNewMarriage(marriage);

	} else if (!Desk_stricmp(tag,"_LAYOUT._MARRIAGE._X")) {
		Layout_GEDCOMNewMarriageX(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._MARRIAGE._Y")) {
		Layout_GEDCOMNewMarriageY(atoi(data));

	} else if (!Desk_stricmp(tag,"")) {
	} else {
		char temp[256]="Unknown Tag: ";
		strcat(temp,tag);
		Desk_Error2_HandleText(temp);
	}
}

static void File_ParseLine(char *line,int *level,char **id,char **tag,char **data)
/* Parse a line of GEDCOM into the level, id (if any), tag and any data*/
{
	/* Remove initial whitespace*/
	while (isspace(*line)) line++;
	/* Get tag level*/
	*level=atoi(line);
	/* Skip over tag level*/
	while (*line!='\0' && !isspace(*line)) line++;
	/* Skip whitespace*/
	while(isspace(*line)) line++;
	/* Get id if it exists*/
	if (*line=='@') {
		*id=++line;
		while (*line!='@' && *line!='\0') line++;
		*line='\0';
		line++;
	} else {
		*id=NULL;
	}
	/* Skip whitespace*/
	while(isspace(*line)) line++;
	/* Get tag*/
	*tag=line;
	while (*line!='\0' && !isspace(*line)) line++;
	*(line++)='\0';
	/* Remaining text must be the data (if any)*/
	switch (*line) {
		case '\0':
			*data=NULL;
			break;
		case '@':
			/* Remove @ at begging and end of data*/
			*data=++line;
			while (*line!='\0' && *line!='@') line++;
			*line='\0';
			break;
		default:
			*data=line;
			/* Remove the terminating '\n'*/
			while (*line!='\0' && *line!='\n') line++;
			*line='\0';
	}
}

static char *File_GEDCOMLine(FILE *file,char *line,char *existingtag,char *existingid)
/* Recursively read though the GEDCOM file building up the whole tag for each line*/
{
	int level,newlevel;
	char wholetag[MAXLINELEN],idbuffer[MAXLINELEN]="";
	char *id,*tag,*data;
	
	do {
		/* Unwind if error or EOF reached*/
		if (line==NULL) return NULL;
		/* Split line into parts*/
		File_ParseLine(line,&level,&id,&tag,&data);
		/* Use last known id*/
		if (id==NULL) {
			if (existingid!=NULL) strcpy(idbuffer,existingid);
		} else {
			strcpy(idbuffer,id);
		}
		id=idbuffer;
		/* Check for overlong tags*/
		if (strlen(wholetag)+strlen(tag)+2>MAXLINELEN) AJWLib_Error2_HandleMsgs("Error.TooLong:");
		/* Create the tag including all tags from higher levels*/
		if (existingtag) sprintf(wholetag,"%s.%s",existingtag,tag); else strcpy(wholetag,tag);
		/* Do something if there is data associated with the line*/
		if (data) File_HandleData(id,wholetag,data);

		/* Get the next line*/
		if (feof(file)) return NULL;
		if (fgets(line,MAXLINELEN,file)==NULL) return NULL;
        newlevel=atoi(line);

		/* Recurse if the new level is lower, return to previous recursion if level is higher*/
		
		while (newlevel>level) {
			line=File_GEDCOMLine(file,line,wholetag,id);
			newlevel=atoi(line);
		}
	} while (newlevel==level);
	return line;
}

void File_LoadGEDCOM(char *filename)
/* Load a GEDCOM file*/
{
	FILE *file=NULL;
	char fivebytedate[5];
	char line[MAXLINELEN];
	layout* gedcomlayout=NULL;

	AJWLib_Assert(idarray==NULL);
	AJWLib_Assert(idarraysize==0);
	Desk_Error2_TryCatch(file=AJWLib_File_fopen(filename,"r");,AJWLib_Error2_ReportMsgs("Error.Load:%s"); return;)
	/* file is guaranteed to be valid if we get here*/
	Desk_Error2_Try {
		Database_New();
		if (fgets(line,MAXLINELEN,file)==NULL) AJWLib_Error2_HandleMsgs("Error.TooLong:");
		/* Get the first line, File_GEDCOMLine will get and process the rest*/
		File_GEDCOMLine(file,line,NULL,NULL);
		AJWLib_File_fclose(file);
		Database_LinkAllChildren();
		Database_CheckConsistency();
		gedcomlayout=Layout_GetGEDCOMLayout();
		Layout_LayoutLines(gedcomlayout);
		Windows_LayoutNormal(gedcomlayout,Desk_FALSE);
		strcpy(currentfilename,filename);
		modified=Desk_FALSE;
		Windows_FileModified();
		Desk_File_Date(filename,fivebytedate);
		Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_Territory_ConvertStandardDateAndTime,-1,fivebytedate,filedate,sizeof(filedate)));
	} Desk_Error2_Catch {
		AJWLib_Error2_ReportMsgs("Error.Load:%s");
		if (file) fclose(file);
		Graphics_RemoveStyle();
		Windows_CloseAllWindows();
		Database_Remove();
	} Desk_Error2_EndCatch
	free(idarray);
	idarray=NULL;
	idarraysize=0;
}

void File_New(void)
{
	Desk_Error2_Try {
		Database_New();
		Desk_Error2_Try {
			Graphics_LoadStyle(AJWLib_Msgs_TempLookup("Style.Default:Default"));
			Desk_Error2_Try {
				Windows_OpenWindow(wintype_NORMAL,none,0,100,NULL);
				Windows_LayoutNormal(NULL,Desk_TRUE);
				File_GetCurrentTime();
			} Desk_Error2_Catch {
				Graphics_RemoveStyle();
				Desk_Error2_ReThrow();
			} Desk_Error2_EndCatch
		} Desk_Error2_Catch {
			Database_Remove();
			Windows_CloseAllWindows();
			Desk_Error2_ReThrow();
		} Desk_Error2_EndCatch
	} Desk_Error2_Catch {
		AJWLib_Error2_Report("%s");
	} Desk_Error2_EndCatch
	modified=Desk_FALSE;
	strcpy(currentfilename,AJWLib_Msgs_TempLookup("File.Tree:Untitled"));
	Windows_FileModified();
}

void File_Remove(void)
{
	modified=Desk_FALSE;
	Database_Remove();
	Graphics_RemoveStyle();
}

void File_Modified(void)
{
	if (Database_GetSize()) modified=Desk_TRUE;
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
	Desk_UNUSED(ref);
	switch (result) {
		case Desk_save_SAVEOK:
			break;
		case Desk_save_RECEIVERFAILED:
			strcpy(currentfilename,oldfilename);
			modified=oldmodified;
			/*date=olddate;*/
			Windows_FileModified();
			Desk_Msgs_Report(1,"Error.BadDrag:Data transer failed");
			break;
	}
}
