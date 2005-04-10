/*
	Roots - File loading and saving
	© Alex Waugh 1999

	$Id: File.c,v 1.51 2004/04/28 21:21:04 ajw Exp $

*/

#include "Desk/Error.h"
#include "Desk/Error2.h"
#include "Desk/Msgs.h"
#include "Desk/File.h"
#include "Desk/Icon.h"
#include "Desk/KernelSWIs.h"
#include "Desk/DeskMem.h"
#include "Desk/Window.h"
#include "Desk/str.h"
#include "Desk/Hourglass.h"

#include "AJWLib/Msgs.h"
#include "AJWLib/Error2.h"
#include "AJWLib/File.h"
#include "AJWLib/Assert.h"

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
#include "TreeLayout.h"
#include "File.h"

#define MAXLINELEN 10240
#define ID_NONE (-1)

#define SWI_Territory_ConvertStandardDateAndTime 0x4304C

static char currentfilename[256],oldfilename[256],filedate[256];
static Desk_bool modified=Desk_FALSE,oldmodified;
static char **idarray=NULL;
static int idarraysize=0;

static void File_GetCurrentTime(void)
{
	char *buffer[5];
	*((int *)buffer)=3;
	Desk_OS_Word(Desk_osword_READCMOSCLOCK,buffer);
	Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_Territory_ConvertStandardDateAndTime,-1,buffer,filedate,sizeof(filedate)));
}

Desk_bool File_SaveGEDCOM(char *filename,void *ref)
{
	FILE *file=NULL;
	layout *normallayout=NULL,*layout=NULL;
	int nextwindow=0;
	Desk_bool plain;

	AJWLib_Assert(filename!=NULL);
	if (ref==NULL) plain=Desk_FALSE; else plain=Desk_TRUE;
	Desk_Error2_TryCatch(file=AJWLib_File_fopen(filename,"w");,AJWLib_Error2_ReportMsgs("Error.Save:%s"); return Desk_TRUE;)
	/*file is guaranteed to be valid if we get here*/
	Desk_Error2_Try {
		fprintf(file,"0 HEAD\n1 SOUR Roots\n2 VERS %s\n2 CORP Alex Waugh\n3 ADDR http://www.alexwaugh.com/\n",ROOTS_VERSION);
		if (plain) {
			fprintf(file,"1 CHAR ASCII\n1 GEDC\n2 VERS 5.5\n2 FORM LINEAGE-LINKED\n");
		} else {
			fprintf(file,"0 _MARRTYPE\n1 _SEP %d\n1 _JOIN %d\n",Config_SeparateMarriages(),Config_JoinMarriages());
		}
		Database_SaveGEDCOM(file,plain);
		if (!plain) {
			Graphics_SaveGEDCOM(file);
			while (nextwindow>=0) {
				if ((layout=Windows_SaveGEDCOM(file,&nextwindow))!=NULL) normallayout=layout;
			}
			if (normallayout) Layout_SaveGEDCOM(normallayout,file);
		}
		fprintf(file,"0 TRLR\n");
		if (!plain) {
			strcpy(oldfilename,currentfilename);
			strcpy(currentfilename,filename);
			oldmodified=modified;
			modified=Desk_FALSE; /*This is assumed to have been ok unless File_Result told otherwise, as File_result does not get called if all ok when save button is clicked on */
			Windows_FileModified();
			File_GetCurrentTime();
		}
	} Desk_Error2_Catch {
		AJWLib_Error2_ReportMsgs("Error.Save:%s");
	} Desk_Error2_EndCatch
	if (file) fclose(file);
    return Desk_TRUE;
}

static elementptr File_GetElementFromID(char *id,elementtype type)
/* Translate from ID to elementptr, creating a new element if necessary*/
{
	int i;
	elementptr element=none;

	for (i=0;i<idarraysize;i++) {
		if (idarray[i]!=NULL) {
			if (strcmp(idarray[i],id)==0) {
				element=i;
				break;
			}
		}
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
		/* Increase array size if necessary, in blocks of 20 to avoid reallocing to often*/
		if (idarraysize<=element) {
			int i;

			idarray=Desk_DeskMem_Realloc(idarray,(element+20)*sizeof(char*));
			for (i=idarraysize;i<(element+20);i++) idarray[i]=NULL;
			idarraysize=element+20;
		}
		/* Add entry to array*/
		idarray[element]=Desk_DeskMem_Malloc(strlen(id)+1);
		strcpy(idarray[element],id);
		return element;
	}
	return element;
}

static void File_HandleData(char *id,char *tag,char *data,Desk_bool plain,Desk_bool prescan)
/* Handle a line of GEDCOM*/
{
	static int user=0;

	if (tag==NULL) return;
	if (!Desk_stricmp(tag,"HEAD.SOUR")) {
		if (strcmp(data,"Roots") && !plain) AJWLib_Error2_HandleMsgs("Error.NotRoot:");

	} else if (!Desk_stricmp(tag,"HEAD.SOUR.VERS")) {
		if (!plain && atof(data)>atof(ROOTS_VERSION_NUMBER)) Desk_Msgs_Report(1,"Error.TooNew:");
		
	} else if (!Desk_stricmp(tag,"HEAD.SOUR.CORP")) {
	} else if (!Desk_stricmp(tag,"HEAD.SOUR.CORP.ADDR")) {
	} else if (!Desk_stricmp(tag,"HEAD.CHAR")) {
	} else if (!Desk_stricmp(tag,"HEAD.GEDC.VERS")) {
	} else if (!Desk_stricmp(tag,"HEAD.GEDC.FORM")) {
	} else if (!Desk_stricmp(tag,"_MARRTYPE._SEP")) {
		Config_SetSeparateMarriages(NULL,(Desk_bool)atoi(data));

	} else if (!Desk_stricmp(tag,"_MARRTYPE._JOIN")) {
		Config_SetJoinMarriages((Desk_bool)atoi(data));

	} else if (!Desk_stricmp(tag,"_FILEINFO._TITLE")) {
		if (prescan) return;
		Database_SetTitle(data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._NEXTNEWPERSON")) {
		if (prescan) return;
		Database_SetNextNewPerson(atoi(data));

	} else if (!Desk_stricmp(tag,"_FILEINFO._PERSONUSER")) {
		user=atoi(data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._PERSONUSER._DESC")) {
		if (prescan) return;
		Database_SetPersonUserDesc(user,data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._PERSONUSER._GEDCOM")) {
		if (prescan) return;
		Database_SetPersonGEDCOMDesc(user,data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._PERSONUSER._TYPE")) {
		if (prescan) return;
		Database_SetPersonFieldType(user,atoi(data));

	} else if (!Desk_stricmp(tag,"_FILEINFO._PERSONUSER._LIST")) {
		if (prescan) return;
		Database_SetPersonFieldList(user,data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._MARRIAGEUSER")) {
		user=atoi(data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._MARRIAGEUSER._DESC")) {
		if (prescan) return;
		Database_SetMarriageUserDesc(user,data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._MARRIAGEUSER._GEDCOM")) {
		if (prescan) return;
		Database_SetMarriageGEDCOMDesc(user,data);

	} else if (!Desk_stricmp(tag,"_FILEINFO._MARRIAGEUSER._TYPE")) {
		if (prescan) return;
		Database_SetMarriageFieldType(user,atoi(data));

	} else if (!Desk_stricmp(tag,"_FILEINFO._MARRIAGEUSER._LIST")) {
		if (prescan) return;
		Database_SetMarriageFieldList(user,data);

	} else if (!Desk_stricmp(tag,"INDI")) {
		if (prescan) return;
		File_GetElementFromID(id,element_PERSON);

	} else if (!Desk_stricmp(tag,"INDI.NAME")) {
		char *ptr;
		elementptr person;

		if (prescan) return;
		person=File_GetElementFromID(id,element_PERSON);
		/* Skip leading whitespace*/
		while (isspace(*data)) data++;
		if ((ptr=strchr(data,'/'))!=NULL) {
			char *seg;

			/* Terminate end of forename/middlename*/
			*ptr++='\0';
			/* Find start of surname*/
			while (isspace(*ptr)) ptr++;
			seg=ptr;
			/* Find end of surname*/
			while (*ptr!='\0' && *ptr!='/') ptr++;
			if (*ptr!='\0') {
				*ptr++='\0';
				/*Treat any remaining non whitespace as the middlename*/
				while (isspace(*ptr)) ptr++;
			}
			Database_SetSurname(person,seg);
			if (*ptr!='\0') {
				/* The middlename is after the surname*/
				Database_SetMiddleNames(person,ptr);
			} else {
				ptr=data;
				/* Skip forname*/
				while (*ptr!='\0' && !isspace(*ptr)) ptr++;
				if (*ptr!='\0') *ptr++='\0';
				while (isspace(*ptr)) ptr++;
				Database_SetMiddleNames(person,ptr);
			}
			Database_SetForename(person,data);
		} else {
			/* Guess at the split points*/
			/* Skip leading whitespace*/
			while (isspace(*data)) data++;
			ptr=data;
			/* Find end of first word*/
			while (*ptr!='\0' && !isspace(*ptr)) ptr++;
			if (*ptr!='\0') *ptr++='\0';
			while (isspace(*ptr)) ptr++;
			if (*ptr!='\0') {
				Database_SetForename(person,data);
				data=ptr;
				ptr=data+strlen(data);
				/* Remove trailing whitespace*/
				while (ptr>data && isspace(*(ptr-1))) ptr--;
				*ptr='\0';
				/* Find the beggining of the surname*/
				while (ptr>data && !isspace(*(ptr-1))) ptr--;
				Database_SetSurname(person,ptr);
				*ptr='\0';
				/* Anything remaining must be the middlenames*/
				/* Remove trailing whitespace*/
				while (ptr>data && isspace(*(ptr-1))) ptr--;
				*ptr='\0';
				Database_SetMiddleNames(person,data);
			}
		}

	} else if (!Desk_stricmp(tag,"INDI.SEX")) {
		elementptr person;

		if (prescan) return;
		person=File_GetElementFromID(id,element_PERSON);
		switch (data[0]) {
			case 'm':
			case 'M':
				Database_SetSex(person,sex_MALE);
				break;
			case 'f':
			case 'F':
				Database_SetSex(person,sex_FEMALE);
				break;
		}

	} else if (!Desk_stricmp(tag,"INDI.FAMS")) {
		elementptr marriage,person;

		if (prescan) return;
		person=File_GetElementFromID(id,element_PERSON);
		marriage=File_GetElementFromID(data,element_MARRIAGE);
		Database_SetMarriage(person,marriage);
		
	} else if (!Desk_stricmp(tag,"INDI.FAMC")) {
		elementptr marriage,person;

		if (prescan) return;
		person=File_GetElementFromID(id,element_PERSON);
		marriage=File_GetElementFromID(data,element_MARRIAGE);
		Database_SetParentsMarriage(person,marriage);

	} else if (!Desk_stricmp(tag,"FAM")) {
		if (prescan) return;
		File_GetElementFromID(id,element_MARRIAGE);

	} else if (!Desk_stricmp(tag,"FAM.HUSB")) {
		elementptr marriage,person;

		if (prescan) return;
		person=File_GetElementFromID(data,element_PERSON);
		marriage=File_GetElementFromID(id,element_MARRIAGE);
		Database_SetPrincipal(marriage,person);
		
	} else if (!Desk_stricmp(tag,"FAM.WIFE")) {
		elementptr marriage,person;

		if (prescan) return;
		person=File_GetElementFromID(data,element_PERSON);
		marriage=File_GetElementFromID(id,element_MARRIAGE);
		Database_SetSpouse(marriage,person);
		
	} else if (!Desk_stricmp(tag,"FAM.CHIL")) {
		/* Do nothing, as database will sort this out after everyone has been loaded*/

	} else if (!Desk_stricmp(tag,"_GRAPHICS._LUAFILE._LINE")) {
		if (prescan) return;
		Graphics_LoadLuaFileLine(data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._PERSONFILE._LINE")) {
		if (prescan) return;
		Graphics_LoadPersonFileLine(data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._MARRIAGEFILE._LINE")) {
		if (prescan) return;
		Graphics_LoadMarriageFileLine(data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._TITLEFILE._LINE")) {
		if (prescan) return;
		Graphics_LoadTitleFileLine(data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._DIMENSIONSFILE._LINE")) {
		if (prescan) return;
		Graphics_LoadDimensionsFileLine(data);

	} else if (!Desk_stricmp(tag,"_GRAPHICS._CURRENTSTYLE")) {
		if (prescan) return;
		Graphics_SetGraphicsStyle(data);

	} else if (!Desk_stricmp(tag,"_WINDOWS._TYPE")) {
		if (prescan) return;
		Windows_CreateWindow((wintype)atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCREENRECT._MIN._X")) {
		if (prescan) return;
		Windows_SetMinX(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCREENRECT._MIN._Y")) {
		if (prescan) return;
		Windows_SetMinY(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCREENRECT._MAX._X")) {
		if (prescan) return;
		Windows_SetMaxX(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCREENRECT._MAX._Y")) {
		if (prescan) return;
		Windows_SetMaxY(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCROLL._X")) {
		if (prescan) return;
		Windows_SetScrollX(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._COORDS._SCROLL._Y")) {
		if (prescan) return;
		Windows_SetScrollY(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._PERSON")) {
		elementptr person;

		if (prescan) return;
		if (atoi(data)>0) {
			person=File_GetElementFromID(data,element_PERSON);
			Windows_SetPerson(person);
		}

	} else if (!Desk_stricmp(tag,"_WINDOWS._GENERATIONS")) {
		if (prescan) return;
		Windows_SetGenerations(atoi(data));

	} else if (!Desk_stricmp(tag,"_WINDOWS._SCALE")) {
		if (prescan) return;
		Windows_SetScale(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._PERSON")) {
		elementptr person;

		if (prescan) return;
		person=File_GetElementFromID(data,element_PERSON);
		if (Database_GetElementType(person)==element_MARRIAGE) Config_SetSeparateMarriages(NULL,Desk_TRUE);
		Layout_GEDCOMNewPerson(person);

	} else if (!Desk_stricmp(tag,"_LAYOUT._PERSON._X")) {
		if (prescan) return;
		Layout_GEDCOMNewPersonX(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._PERSON._Y")) {
		if (prescan) return;
		Layout_GEDCOMNewPersonY(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._PERSON._W")) {
		if (prescan) return;
		Layout_GEDCOMNewPersonWidth(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._PERSON._H")) {
		if (prescan) return;
		Layout_GEDCOMNewPersonHeight(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._PERSON._F")) {
		int temp1;
		flags *temp2;

		if (prescan) return;
		temp1=atoi(data);
		temp2=(flags *)&temp1;
		Layout_GEDCOMNewPersonFlags(*temp2);

	} else if (!Desk_stricmp(tag,"_LAYOUT._MARRIAGE")) {
		elementptr person;

		if (prescan) return;
		Config_SetSeparateMarriages(NULL,Desk_TRUE);
		person=File_GetElementFromID(data,element_MARRIAGE);
		Layout_GEDCOMNewPerson(person);

	} else if (!Desk_stricmp(tag,"_LAYOUT._MARRIAGE._X")) {
		if (prescan) return;
		Layout_GEDCOMNewPersonX(atoi(data));

	} else if (!Desk_stricmp(tag,"_LAYOUT._MARRIAGE._Y")) {
		if (prescan) return;
		Layout_GEDCOMNewPersonY(atoi(data));

	} else if (!Desk_stricmp(tag,"")) {
	} else {
		/* Check though all the user defined tags*/
		int i;
		Desk_bool found=Desk_FALSE;

		if (prescan) {
			if (!Desk_strnicmp(tag,"INDI.",5)) {
				for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
					if (!Desk_stricmp(Desk_Icon_GetTextPtr(fieldconfigwin,fieldconfigpersonicons[i].gedcom),tag)) return;
					if (!Desk_stricmp(Desk_Icon_GetTextPtr(fieldconfigwin,fieldconfigpersonicons[i].gedcom),"")) {
						Desk_Icon_SetText(fieldconfigwin,fieldconfigpersonicons[i].gedcom,tag);
						return;
					}
				}
			} else if (!Desk_strnicmp(tag,"FAM.",4)) {
				for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
					if (!Desk_stricmp(Desk_Icon_GetTextPtr(fieldconfigwin,fieldconfigmarriageicons[i].gedcom),tag)) return;
					if (!Desk_stricmp(Desk_Icon_GetTextPtr(fieldconfigwin,fieldconfigmarriageicons[i].gedcom),"")) {
						Desk_Icon_SetText(fieldconfigwin,fieldconfigmarriageicons[i].gedcom,tag);
						return;
					}
				}
			}
		} else {
			for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
				if (!Desk_stricmp(tag,Database_GetPersonGEDCOMDesc(i))) {
					elementptr person;
	
					person=File_GetElementFromID(id,element_PERSON);
					Database_SetPersonUser(i,person,data);
					found=Desk_TRUE;
					break;
				}
			}
			if (!found) {
				for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
					if (!Desk_stricmp(tag,Database_GetMarriageGEDCOMDesc(i))) {
						elementptr marriage;
	
						marriage=File_GetElementFromID(id,element_MARRIAGE);
						Database_SetMarriageUser(i,marriage,data);
						found=Desk_TRUE;
						break;
					}
				}
	
			}
			if (!found && !plain) AJWLib_AssertWarning(0);
		}
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
			while (*line!='\0' && *line!='\n' && *line!='\r') line++;
			*line='\0';
	}
}

static char *File_GEDCOMLine(FILE *file,char *line,char *existingtag,char *existingid,Desk_bool plain,Desk_bool prescan)
/* Recursively read though the GEDCOM file building up the whole tag for each line*/
{
	int level,newlevel,concat=0;
	char wholetag[MAXLINELEN],idbuffer[MAXLINELEN]="";
	char newlinebuffer[MAXLINELEN], *newline=NULL;
	char *id,*tag,*data;
	char *newid,*newtag,*newdata;
	
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
		if (strlen(existingtag)+strlen(tag)+2>MAXLINELEN) AJWLib_Error2_HandleMsgs("Error.TooLong:");
		/* Create the tag including all tags from higher levels*/
		if (existingtag) sprintf(wholetag,"%s.%s",existingtag,tag); else strcpy(wholetag,tag);

		do {
			/* Get the next line*/
			newline=NULL;
			concat=0;
			if (feof(file)==0) {
				newline=fgets(newlinebuffer,MAXLINELEN,file);
			}
			if (newline) {
				int len;
				char buffer[MAXLINELEN];

				strcpy(buffer,newline);
				File_ParseLine(buffer,&newlevel,&newid,&newtag,&newdata);
				len=strlen(newtag);
				if (len>=4) {
					if (!Desk_stricmp(newtag+len-4,"CONT")) concat=1;
					if (!Desk_stricmp(newtag+len-4,"CONC")) concat=2;
				}
				if (concat && data) {
					if (strlen(data)+strlen(newdata)+2>MAXLINELEN) AJWLib_Error2_HandleMsgs("Error.TooLong:");
					if (concat==1) strcat(data," ");
					strcat(data,newdata);
				}
			}
		} while (concat);

		/* Do something if there is data associated with the line*/
		if (data) File_HandleData(id,wholetag,data,plain,prescan);

		if (newline==NULL) return NULL;

		strcpy(line,newline);

		/* Recurse if the new level is lower, return to previous recursion if level is higher*/
		
		while (newlevel>level) {
			line=File_GEDCOMLine(file,line,wholetag,id,plain,prescan);
			newlevel=atoi(line);
		}
	} while (newlevel==level);
	return line;
}

void File_LoadGEDCOM(char *filename,Desk_bool plain)
/* Load a GEDCOM file*/
{
	FILE *file=NULL;
	char fivebytedate[5];
	char line[MAXLINELEN];
	static char oldfilename[256];
	layout* gedcomlayout=NULL;
	volatile Desk_bool prescan;

	AJWLib_Assert(idarray==NULL);
	AJWLib_Assert(idarraysize==0);
	if (filename==NULL) {
		prescan=Desk_FALSE;
		filename=oldfilename;
	} else {
		strcpy(oldfilename,filename);
		prescan=plain;
	}
	Desk_Error2_TryCatch(file=AJWLib_File_fopen(filename,"r");,AJWLib_Error2_ReportMsgs("Error.Load:%s"); return;)
	/* file is guaranteed to be valid if we get here*/
	Desk_Error2_Try {
		Desk_Hourglass_On();
		if (prescan || !plain) {
			Database_New();
			Config_LoadFileConfig();
		}
		if (fgets(line,MAXLINELEN,file)!=NULL) {
			/* Get the first line, File_GEDCOMLine will get and process the rest*/
			if (prescan) {
				Database_OpenFileConfig();
				Desk_Icon_SetText(fieldconfigwin,fieldconfig_OK,AJWLib_Msgs_TempLookup("Icon.Imp:"));
			} else {
				File_GEDCOMLine(file,line,NULL,NULL,plain,prescan);
			}
			if (!prescan) {
				Database_LinkAllChildren();
				Database_LinkAllMarriages();
				Database_CheckConsistency();
				if (plain) {
					Graphics_LoadStyle(AJWLib_Msgs_TempLookup("Style.Default:Default"));
					gedcomlayout=Layout_LayoutNormal();
					Windows_OpenWindow(wintype_NORMAL,none,0,100,NULL);
				} else {
					gedcomlayout=Layout_GetGEDCOMLayout();
					if (gedcomlayout==NULL) gedcomlayout=Layout_LayoutNormal();
					Layout_LayoutLines(gedcomlayout,Desk_FALSE);
				}
				Windows_LayoutNormal(gedcomlayout,Desk_FALSE);
				if (!plain) strcpy(currentfilename,filename); else strcpy(currentfilename,"GEDCOM");
				modified=Desk_FALSE;
				Windows_FileModified();
				Desk_File_Date(filename,fivebytedate);
				Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_Territory_ConvertStandardDateAndTime,-1,fivebytedate,filedate,sizeof(filedate)));
			}
		}
		Desk_Hourglass_Off();
		AJWLib_File_fclose(file);
	} Desk_Error2_Catch {
		Desk_Hourglass_Off();
		AJWLib_Error2_ReportMsgs("Error.Load:%s");
		if (file) fclose(file);
		Graphics_RemoveStyle();
		Windows_CloseAllWindows();
		Database_Remove();
	} Desk_Error2_EndCatch
	if (idarray) {
		int i;

		for (i=0;i<idarraysize;i++) {
			if (idarray[i]) free(idarray[i]);
		}
		free(idarray);
	}
	idarray=NULL;
	idarraysize=0;
}

void File_New(void)
{
	Desk_Error2_Try {
		Database_New();
		Config_LoadFileConfig();
		Desk_Error2_Try {
			char buffer[256];
			strcpy(buffer,AJWLib_Msgs_TempLookup("Style.Default:Default"));
			Graphics_LoadStyle(buffer);
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
	if (Database_Loaded()) modified=Desk_TRUE;
	Windows_FileModified();
}

Desk_bool File_GetModified(void)
{
	return modified;
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
		default:
			break;
	}
}
