/*
	FT - Graphics Configuration
	© Alex Waugh 1999

	$Id: Graphics.c,v 1.24 2000/02/29 19:52:19 uid1 Exp $

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
#include "AJWLib.File.h"
#include "AJWLib.Font.h"
#include "AJWLib.Draw.h"
#include "AJWLib.DrawFile.h"
#include "AJWLib.Assert.h"
#include "AJWLib.Error2.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "Windows.h"
#include "Config.h"
#include "Layout.h"
#include "Draw.h"
#include "File.h"


#define GRAPHICSDIR "<Roots$Dir>.Graphic"

#define Graphics_ConvertToOS(x) ((int)(strtol(x,NULL,10)*7.087))


#define NUMPERSONFIELDS 15
#define NUMMARRIAGEFIELDS 3

typedef enum personfieldtype {
	personfieldtype_UNKNOWN=0,
	personfieldtype_SURNAME=0,
	personfieldtype_FORENAME,
	personfieldtype_MIDDLENAMES,
	personfieldtype_TITLE,
	personfieldtype_FULLNAME,
	personfieldtype_NAME,
	personfieldtype_TITLEDNAME,
	personfieldtype_TITLEDFULLNAME,
	personfieldtype_SEX,
	personfieldtype_DOB,
	personfieldtype_DOD,
	personfieldtype_BIRTHPLACE,
	personfieldtype_USER1,
	personfieldtype_USER2,
	personfieldtype_USER3
} personfieldtype;

typedef enum marriagefieldtype {
	marriagefieldtype_UNKNOWN=0,
	marriagefieldtype_PLACE=0,
	marriagefieldtype_DATE,
	marriagefieldtype_DIVORCE
} marriagefieldtype;

typedef struct textproperties {
	unsigned int colour;
	unsigned int bgcolour;
	int x;
	int y;
	char fontname[256];
	int size;
	Desk_font2_block *font;
} textproperties;

typedef struct lineboxproperties {
	int x0;
	int y0;
	int x1;
	int y1;
	int colour;
	int thickness;
} lineboxproperties;

typedef enum graphictype {
	graphictype_LINE,
	graphictype_CHILDLINE,
	graphictype_SIBLINGLINE,
	graphictype_RECTANGLE,
	graphictype_FILLEDRECTANGLE,
	graphictype_TEXTLABEL,
	graphictype_CENTREDTEXTLABEL,
	graphictype_FIELD,
	graphictype_CENTREDFIELD,
	graphictype_INVALID
} graphictype;

typedef struct textlabel {
	textproperties properties;
	char text[256];
} textlabel;

typedef union details {
	textlabel textlabel;
	lineboxproperties linebox;
} details;

typedef struct object {
	graphictype type;
	details details;
} object;

typedef struct fieldproperties {
	Desk_bool plot;
    graphictype type;
	textproperties textproperties;
} fieldproperties;

typedef struct graphics {
	int personwidth;
	int personheight;
	int gapheightabove;
	int gapheightbelow;
	int gapwidth;
	int marriagewidth;
	int secondmarriagegap;
	int windowborder;
	int gapheightunlinked;
	int siblinglinethickness;
	int siblinglinecolour;
	int titleheight;
	textproperties title;
	int numpersonobjects;
	object *person;
	int nummarriageobjects;
	object *marriage;
	fieldproperties personfields[NUMPERSONFIELDS];
	fieldproperties marriagefields[NUMMARRIAGEFIELDS];
} graphics;

static graphics graphicsdata;
static char currentstyle[256]="";
static plotfn Graphics_PlotLine=NULL,Graphics_PlotRectangle=NULL,Graphics_PlotRectangleFilled=NULL;
static plottextfn Graphics_PlotText=NULL;

static unsigned int Graphics_RGBToPalette(char *str)
{
	unsigned int colour=(unsigned int)strtol(str,NULL,16);
	return (((colour & 0x00FF0000)>>8) | ((colour & 0x0000FF00)<<8) | ((colour & 0x000000FF)<<24));
}

static void Graphics_StoreDimensionDetails(char *values[],int numvalues,int linenum)
{
	if (numvalues!=2) Desk_Msgs_Report(1,"Error.SynD:Syntax error %d",linenum);
	if (!strcmp(values[0],"personwidth")) graphicsdata.personwidth=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],"personheight")) graphicsdata.personheight=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],"gapheightabove")) graphicsdata.gapheightabove=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],"gapheightbelow")) graphicsdata.gapheightbelow=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],"gapwidth")) graphicsdata.gapwidth=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],"marriagewidth")) graphicsdata.marriagewidth=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],"secondmarriagegap")) graphicsdata.secondmarriagegap=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],"windowborder")) graphicsdata.windowborder=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],"gapheightunlinked")) graphicsdata.gapheightunlinked=Graphics_ConvertToOS(values[1]);
	else Desk_Msgs_Report(1,"Error.SynD:Syntax error %d",linenum);
}

static void Graphics_StoreTitleDetails(char *values[],int numvalues,int linenum)
{
	if (!strcmp(values[0],"filetitle")) {
		if (numvalues!=6) {
			Desk_Msgs_Report(1,"Error.SynT:Syntax error %d",linenum);
		} else {
			graphicsdata.titleheight=Graphics_ConvertToOS(values[1]);
			graphicsdata.title.size=(int)strtol(values[2],NULL,10);
			graphicsdata.title.colour=Graphics_RGBToPalette(values[3]);
			graphicsdata.title.bgcolour=Graphics_RGBToPalette(values[4]);
			strcpy(graphicsdata.title.fontname,values[5]);
		}
	} else Desk_Msgs_Report(1,"Error.SynT:Syntax error %d",linenum);
}

static void Graphics_StorePersonDetails(char *values[],int numvalues,int linenum)
{
	graphictype graphictype=graphictype_INVALID;
	AJWLib_Assert(graphicsdata.person!=NULL);
	AJWLib_Assert(graphicsdata.marriage!=NULL);
	if (!strcmp(values[0],"line")) graphictype=graphictype_LINE;
	else if (!strcmp(values[0],"childline")) graphictype=graphictype_CHILDLINE;
	else if (!strcmp(values[0],"box")) graphictype=graphictype_RECTANGLE;
	else if (!strcmp(values[0],"filledbox")) graphictype=graphictype_FILLEDRECTANGLE;
	else if (!strcmp(values[0],"text")) graphictype=graphictype_TEXTLABEL;
	else if (!strcmp(values[0],"field")) graphictype=graphictype_FIELD;
	else if (!strcmp(values[0],"centredtext")) graphictype=graphictype_CENTREDTEXTLABEL;
	else if (!strcmp(values[0],"centredfield")) graphictype=graphictype_CENTREDFIELD;
	else Desk_Msgs_Report(1,"Error.SynP:Syntax error %d",linenum);
	switch (graphictype) {
		case graphictype_LINE:
		case graphictype_CHILDLINE:
			if (numvalues!=7) {
				Desk_Msgs_Report(1,"Error.SynP:Syntax error %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.person),(++graphicsdata.numpersonobjects)*sizeof(object));
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x0=Graphics_ConvertToOS(values[1]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y0=Graphics_ConvertToOS(values[2]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x1=Graphics_ConvertToOS(values[3]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y1=Graphics_ConvertToOS(values[4]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.thickness=Graphics_ConvertToOS(values[5]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
			}
			break;
		case graphictype_RECTANGLE:
			if (numvalues!=7) {
				Desk_Msgs_Report(1,"Error.SynP:Syntax error %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.person),(++graphicsdata.numpersonobjects)*sizeof(object));
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype_RECTANGLE;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x0=Graphics_ConvertToOS(values[1]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y0=Graphics_ConvertToOS(values[2]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x1=Graphics_ConvertToOS(values[3]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y1=Graphics_ConvertToOS(values[4]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.thickness=Graphics_ConvertToOS(values[5]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
			}
			break;
		case graphictype_FILLEDRECTANGLE:
			if (numvalues!=6) {
				Desk_Msgs_Report(1,"Error.SynP:Syntax error %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.person),(++graphicsdata.numpersonobjects)*sizeof(object));
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype_FILLEDRECTANGLE;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x0=Graphics_ConvertToOS(values[1]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y0=Graphics_ConvertToOS(values[2]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x1=Graphics_ConvertToOS(values[3]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y1=Graphics_ConvertToOS(values[4]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[5]);
			}
			break;
		case graphictype_CENTREDTEXTLABEL:
		case graphictype_TEXTLABEL:
			if (numvalues!=8) {
				Desk_Msgs_Report(1,"Error.SynP:Syntax error %d",linenum);
			} else {
				int size;
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.person),(++graphicsdata.numpersonobjects)*sizeof(object));
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.x=Graphics_ConvertToOS(values[1]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.y=Graphics_ConvertToOS(values[2]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.bgcolour=Graphics_RGBToPalette(values[5]);
				strcpy(graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.text,values[7]);
				size=(int)strtol(values[3],NULL,10);
				strcpy(graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.fontname,values[6]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.size=size;
			}
			break;
		case graphictype_CENTREDFIELD:
		case graphictype_FIELD:
			if (numvalues!=8) {
				Desk_Msgs_Report(1,"Error.SynP:Syntax error %d",linenum);
			} else {
				personfieldtype field=personfieldtype_UNKNOWN;
				int size;
				AJWLib_Str_LowerCase(values[7]);
				if (!strcmp(values[7],"surname")) field=personfieldtype_SURNAME;
				else if (!strcmp(values[7],"forename")) field=personfieldtype_FORENAME;
				else if (!strcmp(values[7],"name")) field=personfieldtype_NAME;
				else if (!strcmp(values[7],"middlenames")) field=personfieldtype_MIDDLENAMES;
				else if (!strcmp(values[7],"fullname")) field=personfieldtype_FULLNAME;
				else if (!strcmp(values[7],"title")) field=personfieldtype_TITLE;
				else if (!strcmp(values[7],"titledname")) field=personfieldtype_TITLEDNAME;
				else if (!strcmp(values[7],"titledname")) field=personfieldtype_TITLEDFULLNAME;
				else if (!strcmp(values[7],"sex")) field=personfieldtype_SEX;
				else if (!strcmp(values[7],"dob")) field=personfieldtype_DOB;
				else if (!strcmp(values[7],"dod")) field=personfieldtype_DOD;
				else if (!strcmp(values[7],"birthplace")) field=personfieldtype_BIRTHPLACE;
				else if (!strcmp(values[7],"user1")) field=personfieldtype_USER1;
				else if (!strcmp(values[7],"user2")) field=personfieldtype_USER2;
				else if (!strcmp(values[7],"user3")) field=personfieldtype_USER3;
				else {
					Desk_Msgs_Report(1,"Error.SynP:Syntax error %d",linenum);
					break;
				}
				graphicsdata.personfields[field].plot=Desk_TRUE;
				graphicsdata.personfields[field].type=graphictype;
				graphicsdata.personfields[field].textproperties.x=Graphics_ConvertToOS(values[1]);
				graphicsdata.personfields[field].textproperties.y=Graphics_ConvertToOS(values[2]);
				graphicsdata.personfields[field].textproperties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.personfields[field].textproperties.bgcolour=Graphics_RGBToPalette(values[5]);
				size=(int)strtol(values[3],NULL,10);
				strcpy(graphicsdata.personfields[field].textproperties.fontname,values[6]);
				graphicsdata.personfields[field].textproperties.size=size;
			}
			break;
	}
}

static void Graphics_StoreMarriageDetails(char *values[],int numvalues,int linenum)
{
	graphictype graphictype=graphictype_INVALID;
	if (!strcmp(values[0],"line")) graphictype=graphictype_LINE;
	else if (!strcmp(values[0],"childline")) graphictype=graphictype_CHILDLINE;
	else if (!strcmp(values[0],"siblingline")) graphictype=graphictype_SIBLINGLINE;
	else if (!strcmp(values[0],"box")) graphictype=graphictype_RECTANGLE;
	else if (!strcmp(values[0],"filledbox")) graphictype=graphictype_FILLEDRECTANGLE;
	else if (!strcmp(values[0],"text")) graphictype=graphictype_TEXTLABEL;
	else if (!strcmp(values[0],"field")) graphictype=graphictype_FIELD;
	else if (!strcmp(values[0],"centredfield")) graphictype=graphictype_CENTREDFIELD;
	else Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
	switch (graphictype) {
		case graphictype_SIBLINGLINE:
			if (numvalues!=3) {
				Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
			} else {
				graphicsdata.siblinglinethickness=Graphics_ConvertToOS(values[1]);
				graphicsdata.siblinglinecolour=Graphics_RGBToPalette(values[2]);
			}
			break;
		case graphictype_LINE:
		case graphictype_CHILDLINE:
			if (numvalues!=7) {
				Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriage),(++graphicsdata.nummarriageobjects)*sizeof(object));
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].type=graphictype;
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x0=Graphics_ConvertToOS(values[1]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y0=Graphics_ConvertToOS(values[2]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x1=Graphics_ConvertToOS(values[3]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y1=Graphics_ConvertToOS(values[4]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.thickness=Graphics_ConvertToOS(values[5]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
			}
			break;
		case graphictype_RECTANGLE:
			if (numvalues!=7) {
				Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriage),(++graphicsdata.nummarriageobjects)*sizeof(object));
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].type=graphictype_RECTANGLE;
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x0=Graphics_ConvertToOS(values[1]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y0=Graphics_ConvertToOS(values[2]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x1=Graphics_ConvertToOS(values[3]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y1=Graphics_ConvertToOS(values[4]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.thickness=Graphics_ConvertToOS(values[5]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
			}
			break;
		case graphictype_FILLEDRECTANGLE:
			if (numvalues!=6) {
				Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriage),(++graphicsdata.nummarriageobjects)*sizeof(object));
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].type=graphictype_FILLEDRECTANGLE;
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x0=Graphics_ConvertToOS(values[1]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y0=Graphics_ConvertToOS(values[2]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x1=Graphics_ConvertToOS(values[3]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y1=Graphics_ConvertToOS(values[4]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[5]);
			}
			break;
		case graphictype_CENTREDTEXTLABEL:
		case graphictype_TEXTLABEL:
			if (numvalues!=8) {
				Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
			} else {
				int size;
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriage),(++graphicsdata.nummarriageobjects)*sizeof(object));
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].type=graphictype;
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.x=Graphics_ConvertToOS(values[1]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.y=Graphics_ConvertToOS(values[2]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.bgcolour=Graphics_RGBToPalette(values[5]);
				strcpy(graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.text,values[7]);
				size=(int)strtol(values[3],NULL,10);
			}
			break;
		case graphictype_CENTREDFIELD:
		case graphictype_FIELD:
			if (numvalues!=8) {
				Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
			} else {
				marriagefieldtype field=marriagefieldtype_UNKNOWN;
				int size;
				AJWLib_Str_LowerCase(values[7]);
				if (!strcmp(values[7],"place")) field=marriagefieldtype_PLACE;
				else if (!strcmp(values[7],"date")) field=marriagefieldtype_DATE;
				else if (!strcmp(values[7],"divorce")) field=marriagefieldtype_DIVORCE;
				else {
					Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
					break;
				}
				graphicsdata.marriagefields[field].plot=Desk_TRUE;
				graphicsdata.marriagefields[field].type=graphictype;
				graphicsdata.marriagefields[field].textproperties.x=Graphics_ConvertToOS(values[1]);
				graphicsdata.marriagefields[field].textproperties.y=Graphics_ConvertToOS(values[2]);
				graphicsdata.marriagefields[field].textproperties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.marriagefields[field].textproperties.bgcolour=Graphics_RGBToPalette(values[5]);
				size=(int)strtol(values[3],NULL,10);
				strcpy(graphicsdata.marriagefields[field].textproperties.fontname,values[6]);
				graphicsdata.marriagefields[field].textproperties.size=size;
			}
			break;
	}
}

static void Graphics_ReadFile(char *style,char *filename,void (*decodefn)(char *values[],int numvalues,int linenum))
{
	FILE *file=NULL;
	char fullfilename[256];
	int ch=0,line=0;
	sprintf(fullfilename,"%s.%s",GRAPHICSDIR,style);
	if (!Desk_File_IsDirectory(fullfilename)) AJWLib_Error2_HandleMsgs2("Error.NoDir:Dir %s does not exist",style);
	sprintf(fullfilename,"%s.%s.%s",GRAPHICSDIR,style,filename);
	if (!Desk_File_Exists(fullfilename)) AJWLib_Error2_HandleMsgs3("Error.NoFile:File %s does not exist in dir %s",filename,style);
	file=AJWLib_File_fopen(fullfilename,"r"); /*Error will be caught by caller*/
	while (ch!=EOF) {
		char str[256];
		int i=-1;
		ch=fgetc(file);
		while (ch!=EOF && ch!='\n' && i<254) {
			str[++i]=ch;
			ch=fgetc(file);
		}
		str[++i]='\0';
		line++;
		if (str[0]!='#' && str[0]!='\0') {
			char *values[10];
			int j=0,k,len=strlen(str);
			values[j++]=str;
			for (k=0;k<len;k++) {
				if (str[k]==',') {
					str[k]='\0';
					values[j++]=str+k+1;
				}
			}
			AJWLib_Str_LowerCase(values[0]);
			(*decodefn)(values,j,line);
		}
	}
	AJWLib_File_fclose(file);
}

int Graphics_PersonHeight(void)
{
	return graphicsdata.personheight;
}

int Graphics_PersonWidth(void)
{
	return graphicsdata.personwidth;
}

int Graphics_UnlinkedGapHeight(void)
{
	return graphicsdata.gapheightunlinked;
}

int Graphics_GapHeightAbove(void)
{
	return graphicsdata.gapheightabove;
}

int Graphics_GapHeightBelow(void)
{
	return graphicsdata.gapheightbelow;
}

int Graphics_GapWidth(void)
{
	return graphicsdata.gapwidth;
}

int Graphics_MarriageWidth(void)
{
	return graphicsdata.marriagewidth;
}

int Graphics_SecondMarriageGap(void)
{
	return graphicsdata.secondmarriagegap;
}

int Graphics_WindowBorder(void)
{
	return graphicsdata.windowborder;
}

int Graphics_TitleHeight(void)
{
	return graphicsdata.titleheight;
	return 0;
}

int Graphics_GetSize(void)
{
	int size=0;
	if (graphicsdata.person!=NULL && graphicsdata.marriage!=NULL) {
		size+=sizeof(tag)+sizeof(int);
		size+=sizeof(graphicsdata);
		size+=sizeof(object)*graphicsdata.numpersonobjects;
		size+=sizeof(object)*graphicsdata.nummarriageobjects;
		size+=sizeof(int);
		size+=strlen(currentstyle);
		size=(size+4) & ~3; /*Add null and round to a word boundary*/
	}
	return size;
}

static void Graphics_ClaimFonts(void)
{
	int i;
	AJWLib_Assert(graphicsdata.person!=NULL);
	AJWLib_Assert(graphicsdata.marriage!=NULL);
	graphicsdata.title.font=NULL;
	Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.title.font),graphicsdata.title.fontname,16*graphicsdata.title.size,16*graphicsdata.title.size));
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		if (graphicsdata.person[i].type==graphictype_CENTREDTEXTLABEL || graphicsdata.person[i].type==graphictype_TEXTLABEL) {
			graphicsdata.person[i].details.textlabel.properties.font=NULL;
			Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.person[i].details.textlabel.properties.font),graphicsdata.person[i].details.textlabel.properties.fontname,16*graphicsdata.person[i].details.textlabel.properties.size,16*graphicsdata.person[i].details.textlabel.properties.size));
		}
	}
	for (i=personfieldtype_SURNAME;i<=personfieldtype_USER3;i++) {
		if (graphicsdata.personfields[i].plot) {
			graphicsdata.personfields[i].textproperties.font=NULL;
			Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.personfields[i].textproperties.font),graphicsdata.personfields[i].textproperties.fontname,16*graphicsdata.personfields[i].textproperties.size,16*graphicsdata.personfields[i].textproperties.size));
		}
	}
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		if (graphicsdata.marriage[i].type==graphictype_CENTREDTEXTLABEL || graphicsdata.marriage[i].type==graphictype_TEXTLABEL) {
			graphicsdata.marriage[i].details.textlabel.properties.font=NULL;
			Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.marriage[i].details.textlabel.properties.font),graphicsdata.marriage[i].details.textlabel.properties.fontname,16*graphicsdata.marriage[i].details.textlabel.properties.size,16*graphicsdata.marriage[i].details.textlabel.properties.size));
		}
	}
	for (i=marriagefieldtype_PLACE;i<=marriagefieldtype_DATE;i++) {
		if (graphicsdata.marriagefields[i].plot) {
			graphicsdata.marriagefields[i].textproperties.font=NULL;
			Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.marriagefields[i].textproperties.font),graphicsdata.marriagefields[i].textproperties.fontname,16*graphicsdata.marriagefields[i].textproperties.size,16*graphicsdata.marriagefields[i].textproperties.size));
		}
	}
}

static void Graphics_ReleaseFonts(void)
{
	int i;
	Desk_Font2_ReleaseFont(&(graphicsdata.title.font));
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		if (graphicsdata.person[i].type==graphictype_CENTREDTEXTLABEL || graphicsdata.person[i].type==graphictype_TEXTLABEL) {
			Desk_Font2_ReleaseFont(&(graphicsdata.person[i].details.textlabel.properties.font));
		}
	}
	for (i=personfieldtype_SURNAME;i<=personfieldtype_USER3;i++) {
		if (graphicsdata.personfields[i].plot) {
			Desk_Font2_ReleaseFont(&(graphicsdata.personfields[i].textproperties.font));
		}
	}
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		if (graphicsdata.marriage[i].type==graphictype_CENTREDTEXTLABEL || graphicsdata.marriage[i].type==graphictype_TEXTLABEL) {
			Desk_Font2_ReleaseFont(&(graphicsdata.marriage[i].details.textlabel.properties.font));
		}
	}
	for (i=marriagefieldtype_PLACE;i<=marriagefieldtype_DATE;i++) {
		if (graphicsdata.personfields[i].plot) {
			Desk_Font2_ReleaseFont(&(graphicsdata.marriagefields[i].textproperties.font));
		}
	}
}

void Graphics_Load(FILE *file)
{
	int size;
	char filename[256];
	AJWLib_Assert(file!=NULL);
	AJWLib_Assert(graphicsdata.person==NULL);
	AJWLib_Assert(graphicsdata.marriage==NULL);
	AJWLib_File_fread(&graphicsdata,sizeof(graphics),1,file);
	AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.person),sizeof(object)*graphicsdata.numpersonobjects+1); /*An extra byte incase there are no objects*/
	AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.marriage),sizeof(object)*graphicsdata.nummarriageobjects+1);
	AJWLib_File_fread(graphicsdata.person,sizeof(object),graphicsdata.numpersonobjects,file);
	AJWLib_File_fread(graphicsdata.marriage,sizeof(object),graphicsdata.nummarriageobjects,file);
	AJWLib_File_fread(&size,sizeof(int),1,file);
	AJWLib_File_fread(&currentstyle,sizeof(char),size,file);
	sprintf(filename,"%s.%s",GRAPHICSDIR,currentstyle);
	Graphics_ClaimFonts();
	/*Import style? ie save style into graphic dir if doesn't already exist*/
}

void Graphics_Save(FILE *file)
{
	tag tag=tag_GRAPHICS;
	int size;
	AJWLib_Assert(file!=NULL);
	AJWLib_Assert(graphicsdata.person!=NULL);
	AJWLib_Assert(graphicsdata.marriage!=NULL);
	size=Graphics_GetSize();
	AJWLib_File_fwrite(&tag,sizeof(tag),1,file);
	AJWLib_File_fwrite(&size,sizeof(int),1,file);
	AJWLib_File_fwrite(&graphicsdata,sizeof(graphicsdata),1,file);
	AJWLib_File_fwrite(graphicsdata.person,sizeof(object),graphicsdata.numpersonobjects,file);
	AJWLib_File_fwrite(graphicsdata.marriage,sizeof(object),graphicsdata.nummarriageobjects,file);
	size=strlen(currentstyle);
	size=(size+4) & ~3; /*Add null and round to a word boundary*/
	AJWLib_File_fwrite(&size,sizeof(int),1,file);
	AJWLib_File_fwrite(&currentstyle,sizeof(char),size,file);
}

void Graphics_RemoveStyle(void)
{
	AJWLib_AssertWarning(graphicsdata.person!=NULL);
	AJWLib_AssertWarning(graphicsdata.marriage!=NULL);
	Graphics_ReleaseFonts();
	graphicsdata.numpersonobjects=0;
	graphicsdata.nummarriageobjects=0;
	if (graphicsdata.person!=NULL) AJWLib_Flex_Free((flex_ptr)&(graphicsdata.person));
	if (graphicsdata.marriage!=NULL) AJWLib_Flex_Free((flex_ptr)&(graphicsdata.marriage));
}

static void Graphics_DefaultStyle(void)
{
	int i;
	graphicsdata.personwidth=200;
	graphicsdata.personheight=100;
	graphicsdata.gapheightabove=40;
	graphicsdata.gapheightbelow=40;
	graphicsdata.gapwidth=60;
	graphicsdata.marriagewidth=100;
	graphicsdata.windowborder=20;
	graphicsdata.gapheightunlinked=30;
	graphicsdata.siblinglinethickness=0;
	graphicsdata.siblinglinecolour=0;
	graphicsdata.titleheight=40;
	graphicsdata.title.size=24;
	graphicsdata.title.colour=0;
	graphicsdata.title.bgcolour=0xFFFFFF00;
	strcpy(graphicsdata.title.fontname,"Homerton.Bold");
	graphicsdata.numpersonobjects=0;
	graphicsdata.nummarriageobjects=0;
	for (i=0;i<NUMPERSONFIELDS;i++) graphicsdata.personfields[i].plot=Desk_FALSE;
	for (i=0;i<NUMMARRIAGEFIELDS;i++) graphicsdata.marriagefields[i].plot=Desk_FALSE;
}

void Graphics_LoadStyle(char *style)
{
	AJWLib_Assert(graphicsdata.person==NULL);
	AJWLib_Assert(graphicsdata.marriage==NULL);
	AJWLib_Assert(style!=NULL);
	Graphics_DefaultStyle();
	Desk_Error2_Try {
		AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.marriage),1);
		Graphics_ReadFile(style,"Person",Graphics_StorePersonDetails);
		Graphics_ReadFile(style,"Dimensions",Graphics_StoreDimensionDetails);
		Graphics_ReadFile(style,"Marriage",Graphics_StoreMarriageDetails);
		Graphics_ReadFile(style,"Title",Graphics_StoreTitleDetails);
		strcpy(currentstyle,style);
		Graphics_ClaimFonts();
	} Desk_Error2_Catch {
		Graphics_RemoveStyle();
		Graphics_DefaultStyle();
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
}

void Graphics_PlotPerson(int scale,int originx,int originy,elementptr person,int x,int y,Desk_bool child,Desk_bool selected)
{
	int i;
	AJWLib_Assert(graphicsdata.person!=NULL);
	AJWLib_Assert(graphicsdata.marriage!=NULL);
	AJWLib_Assert(person>0);
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		int xcoord=0;
		switch (graphicsdata.person[i].type) {
			case graphictype_RECTANGLE:
				Graphics_PlotRectangle(scale,originx,originy,x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
				break;
			case graphictype_FILLEDRECTANGLE:
				Graphics_PlotRectangleFilled(scale,originx,originy,x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
				break;
			case graphictype_CHILDLINE:
				if (!child) break;
				/*A line that is only plotted if there is a child and child==Desk_FALSE ?*/
			case graphictype_LINE:
				Graphics_PlotLine(scale,originx,originy,x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,x+graphicsdata.person[i].details.linebox.x1,y+graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
				break;
			case graphictype_CENTREDTEXTLABEL:
				xcoord=-AJWLib_Font_GetWidth(graphicsdata.person[i].details.textlabel.properties.font->handle,graphicsdata.person[i].details.textlabel.text)/2;
			case graphictype_TEXTLABEL:
				Graphics_PlotText(scale,originx,originy,x+xcoord+graphicsdata.person[i].details.textlabel.properties.x,y+graphicsdata.person[i].details.textlabel.properties.y,graphicsdata.person[i].details.textlabel.properties.font->handle,graphicsdata.person[i].details.textlabel.properties.fontname,graphicsdata.person[i].details.textlabel.properties.size,graphicsdata.person[i].details.textlabel.properties.bgcolour,graphicsdata.person[i].details.textlabel.properties.colour,graphicsdata.person[i].details.textlabel.text);
				break;
		}
	}
	for (i=0;i<NUMPERSONFIELDS;i++) {
		if (graphicsdata.personfields[i].plot) {
			char fieldtext[256]=""; /*what is max field length?*/
			int xcoord=0;
			switch (i) {
				case personfieldtype_SURNAME:
					strcpy(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_FORENAME:
					strcpy(fieldtext,Database_GetPersonData(person)->forename);
					break;
				case personfieldtype_MIDDLENAMES:
					strcpy(fieldtext,Database_GetPersonData(person)->middlenames);
					break;
				case personfieldtype_TITLEDNAME:
					strcpy(fieldtext,Database_GetTitledName(person));
					break;
				case personfieldtype_NAME:
					strcpy(fieldtext,Database_GetName(person));
					break;
				case personfieldtype_TITLEDFULLNAME:
					strcpy(fieldtext,Database_GetTitledFullName(person));
					break;
				case personfieldtype_FULLNAME:
					strcpy(fieldtext,Database_GetFullName(person));
					break;
				case personfieldtype_TITLE:
					strcpy(fieldtext,Database_GetPersonData(person)->title);
					break;
				case personfieldtype_SEX:
					sprintf(fieldtext,"%c",Database_GetPersonData(person)->sex);
					break;
				case personfieldtype_DOB:
					strcpy(fieldtext,Database_GetPersonData(person)->dob);
					break;
				case personfieldtype_DOD:
					strcpy(fieldtext,Database_GetPersonData(person)->dod);
					break;
				case personfieldtype_BIRTHPLACE:
					strcpy(fieldtext,Database_GetPersonData(person)->placeofbirth);
					break;
				case personfieldtype_USER1:
					strcpy(fieldtext,Database_GetPersonData(person)->userdata[0]);
					break;
				case personfieldtype_USER2:
					strcpy(fieldtext,Database_GetPersonData(person)->userdata[1]);
					break;
				case personfieldtype_USER3:
					strcpy(fieldtext,Database_GetPersonData(person)->userdata[2]);
					break;
				default:
					strcpy(fieldtext,"Unimplemented");
			}
			if (graphicsdata.personfields[i].type==graphictype_CENTREDFIELD) xcoord=-AJWLib_Font_GetWidth(graphicsdata.personfields[i].textproperties.font->handle,fieldtext)/2;
			Graphics_PlotText(scale,originx,originy,x+xcoord+graphicsdata.personfields[i].textproperties.x,y+graphicsdata.personfields[i].textproperties.y,graphicsdata.personfields[i].textproperties.font->handle,graphicsdata.personfields[i].textproperties.fontname,graphicsdata.personfields[i].textproperties.size,graphicsdata.personfields[i].textproperties.bgcolour,graphicsdata.personfields[i].textproperties.colour,fieldtext);
		}
	}
	if (selected) Draw_EORRectangleFilled(scale,originx,originy,x,y,Graphics_PersonWidth(),Graphics_PersonHeight(),EORCOLOUR);
}

static void Graphics_PlotMarriage(int scale,int originx,int originy,int x,int y,elementptr marriage,Desk_bool childline,Desk_bool selected)
{
	int i,xcoord=0;
	AJWLib_Assert(graphicsdata.person!=NULL);
	AJWLib_Assert(graphicsdata.marriage!=NULL);
	AJWLib_Assert(marriage>0);
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		switch (graphicsdata.marriage[i].type) {
			case graphictype_RECTANGLE:
				Graphics_PlotRectangle(scale,originx,originy,x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
				break;
			case graphictype_FILLEDRECTANGLE:
				Graphics_PlotRectangleFilled(scale,originx,originy,x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
				break;
			case graphictype_CHILDLINE:
				if (!childline) break;
			case graphictype_LINE:
				Graphics_PlotLine(scale,originx,originy,x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,x+graphicsdata.marriage[i].details.linebox.x1,y+graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
				break;
			case graphictype_CENTREDTEXTLABEL:
				xcoord=-AJWLib_Font_GetWidth(graphicsdata.marriage[i].details.textlabel.properties.font->handle,graphicsdata.marriage[i].details.textlabel.text)/2;
			case graphictype_TEXTLABEL:
				Graphics_PlotText(scale,originx,originy,x+xcoord+graphicsdata.marriage[i].details.textlabel.properties.x,y+graphicsdata.marriage[i].details.textlabel.properties.y,graphicsdata.marriage[i].details.textlabel.properties.font->handle,graphicsdata.marriage[i].details.textlabel.properties.fontname,graphicsdata.marriage[i].details.textlabel.properties.size,graphicsdata.marriage[i].details.textlabel.properties.bgcolour,graphicsdata.marriage[i].details.textlabel.properties.colour,graphicsdata.marriage[i].details.textlabel.text);
				break;
		}
	}
	for (i=0;i<NUMMARRIAGEFIELDS;i++) {
		if (graphicsdata.marriagefields[i].plot) {
			char fieldtext[256]=""; /*what is max field length?*/
			switch (i) {
				case marriagefieldtype_PLACE:
					strcpy(fieldtext,Database_GetMarriageData(marriage)->place);
					break;
				case marriagefieldtype_DATE:
					strcpy(fieldtext,Database_GetMarriageData(marriage)->date);
					break;
				case marriagefieldtype_DIVORCE:
					strcpy(fieldtext,Database_GetMarriageData(marriage)->divorce);
					break;
				default:
					strcpy(fieldtext,"Unimplemented");
			}
			if (graphicsdata.marriagefields[i].type==graphictype_CENTREDFIELD) xcoord=-AJWLib_Font_GetWidth(graphicsdata.marriagefields[i].textproperties.font->handle,fieldtext)/2;
			Graphics_PlotText(scale,originx,originy,x+xcoord+graphicsdata.marriagefields[i].textproperties.x,y+graphicsdata.marriagefields[i].textproperties.y,graphicsdata.marriagefields[i].textproperties.font->handle,graphicsdata.marriagefields[i].textproperties.fontname,graphicsdata.marriagefields[i].textproperties.size,graphicsdata.marriagefields[i].textproperties.bgcolour,graphicsdata.marriagefields[i].textproperties.colour,fieldtext);
		}
	}
	if (selected) Draw_EORRectangleFilled(scale,originx,originy,x,y,Graphics_MarriageWidth(),Graphics_PersonHeight(),EORCOLOUR);
}

static void Graphics_PlotChildren(int scale,int originx,int originy,int leftx,int rightx,int y)
{
	AJWLib_Assert(graphicsdata.person!=NULL);
	AJWLib_Assert(graphicsdata.marriage!=NULL);
	Graphics_PlotLine(scale,originx,originy,leftx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),rightx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),graphicsdata.siblinglinethickness,graphicsdata.siblinglinecolour);
}

void Graphics_Redraw(layout *layout,int scale,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection,plotfn plotline,plotfn plotrect,plotfn plotrectfilled,plottextfn plottext)
{
	int i;
	AJWLib_Assert(graphicsdata.person!=NULL);
	AJWLib_Assert(graphicsdata.marriage!=NULL);
	AJWLib_Assert(layout!=NULL);
	/*use the clip rect*/
	Graphics_PlotLine=plotline;
	Graphics_PlotRectangle=plotrect;
	Graphics_PlotRectangleFilled=plotrectfilled;
	Graphics_PlotText=plottext;
	if (layout->title.x!=INFINITY || layout->title.y!=INFINITY) {
		Desk_wimp_point *coords;
		coords=AJWLib_Font_GetWidthAndHeight(graphicsdata.title.font->handle,Database_GetTitle());
		Graphics_PlotText(scale,originx,originy,layout->title.x-coords->x/2,layout->title.y-coords->y/2,graphicsdata.title.font->handle,graphicsdata.title.fontname,graphicsdata.title.size,graphicsdata.title.bgcolour,graphicsdata.title.colour,Database_GetTitle());
	}
	for (i=0;i<layout->numchildren;i++) {
		Graphics_PlotChildren(scale,originx,originy,layout->children[i].leftx,layout->children[i].rightx,layout->children[i].y);
	}
	for (i=0;i<layout->nummarriages;i++) {
		Graphics_PlotMarriage(scale,originx,originy,layout->marriage[i].x,layout->marriage[i].y,layout->marriage[i].marriage,layout->marriage[i].childline,plotselection ? layout->marriage[i].selected : Desk_FALSE);
	}
	for (i=0;i<layout->numpeople;i++) {
		Graphics_PlotPerson(scale,originx,originy,layout->person[i].person,layout->person[i].x,layout->person[i].y,layout->person[i].child,plotselection ? layout->person[i].selected : Desk_FALSE);
	}
}

