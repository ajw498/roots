/*
	Roots - Graphics Configuration
	© Alex Waugh 1999

	$Id: Graphics.c,v 1.59 2000/11/21 23:52:28 AJW Exp $

*/

#include "Desk.Window.h"
#include "Desk.Error.h"
#include "Desk.DeskMem.h"
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
#include "Desk.Str.h"

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
#include <ctype.h>

#define LUA_REENTRANT
#include "Lua.Lua.h"
#include "Lua.LuaLib.h"

#include "Database.h"
#include "Graphics.h"
#include "Windows.h"
#include "Config.h"
#include "Layout.h"
#include "Draw.h"
#include "File.h"


#define SWI_PDriver_DeclareFont 0x80155


#define PERSONWIDTH       "personwidth"
#define PERSONHEIGHT      "personheight"
#define GAPHEIGHTABOVE    "gapheightabove"
#define GAPHEIGHTBELOW    "gapheightbelow"
#define GAPWIDTH          "gapwidth"
#define MARRIAGEWIDTH     "marriagewidth"
#define SECONDMARRIAGEGAP "secondmarriagegap"
#define WINDOWBORDER      "windowborder"

#define FILETITLE "filetitle"

#define LINE         "line"
#define CHILDLINE    "childline"
#define SIBLINGLINE  "siblingline"
#define BOX          "box"
#define FILLEDBOX    "filledbox"
#define TEXT         "text"
#define FIELD        "field"
#define CENTREDTEXT  "centredtext"
#define CENTREDFIELD "centredfield"

#define SURNAME        "surname"
#define FORENAME       "forename"
#define NAME           "name"
#define MIDDLENAMES    "middlenames"
#define FULLNAME       "fullname"
#define INITIALEDMIDDLENAME "initialedmiddlename"
#define INITIALEDNAME "initialedname"
#define SEX            "sex"

#define MAXFONTS 255

#define Graphics_LuaGetGlobal(state,name,var) {\
	lua_getglobal(state,name);\
	if (lua_isnumber(state,-1)) var=(int)lua_tonumber(state,-1);\
	lua_remove(state,-1);\
}


typedef enum personfieldtype {
	personfieldtype_UNKNOWN=NUMBERPERSONUSERFIELDS,
	personfieldtype_SURNAME,
	personfieldtype_FORENAME,
	personfieldtype_MIDDLENAMES,
	personfieldtype_FULLNAME,
	personfieldtype_NAME,
	personfieldtype_INITIALEDMIDDLENAME,
	personfieldtype_INITIALEDNAME,
	personfieldtype_SEX
} personfieldtype;

typedef enum marriagefieldtype {
	marriagefieldtype_UNKNOWN=NUMBERMARRIAGEUSERFIELDS
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
	unsigned int colour;
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
	sextype sex;
	details details;
} object;

typedef struct fieldproperties {
	graphictype type;
    personfieldtype personfieldtype;
    marriagefieldtype marriagefieldtype;
	sextype sex;
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
	int siblinglinethickness;
	unsigned int siblinglinecolour;
	int titleheight;
	textproperties title;
	int numpersonobjects;
	object *person;
	int nummarriageobjects;
	object *marriage;
	fieldproperties *personfields;
	int numpersonfields;
	fieldproperties *marriagefields;
	int nummarriagefields;
} graphics;

static struct luadetails {
	int originx;
	int originy;
	int scale;
	lua_State *state;
	char *error;
	int numberoffonts;
	struct fonts {
		Desk_font2_block *handle;
		char *fontname;
		int size;
	} fonts[MAXFONTS];
} luadetails={0,0,100,NULL,NULL,0};

static graphics graphicsdata;
static char *personfile=NULL,*marriagefile=NULL,*dimensionsfile=NULL,*titlefile=NULL,*luafile=NULL;
static char currentstyle[256]="";
static plotfn Graphics_PlotLine=NULL,Graphics_PlotRectangle=NULL,Graphics_PlotRectangleFilled=NULL;
static plottextfn Graphics_PlotText=NULL;
static layout *redrawlayout=NULL;
static Desk_bool uselua=Desk_FALSE;

static void Graphics_LuaInit(void);

static unsigned int Graphics_RGBToPalette(char *str)
/* Convert RRGGBB hex colour to a palette format 0xBBGGRR00*/
{
	unsigned int colour=(unsigned int)strtol(str,NULL,16);
	return (((colour & 0x00FF0000)>>8) | ((colour & 0x0000FF00)<<8) | ((colour & 0x000000FF)<<24));
}

static void Graphics_StoreDimensionDetails(char *values[],int numvalues,int linenum)
/* Read a set of values into the dimensions block*/
{
	if (numvalues!=2) Desk_Msgs_Report(1,"Error.SynD:Syntax error %d",linenum);
	if (!strcmp(values[0],PERSONWIDTH)) graphicsdata.personwidth=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],PERSONHEIGHT)) graphicsdata.personheight=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],GAPHEIGHTABOVE)) graphicsdata.gapheightabove=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],GAPHEIGHTBELOW)) graphicsdata.gapheightbelow=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],GAPWIDTH)) graphicsdata.gapwidth=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],MARRIAGEWIDTH)) graphicsdata.marriagewidth=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],SECONDMARRIAGEGAP)) graphicsdata.secondmarriagegap=Graphics_ConvertToOS(values[1]);
	else if (!strcmp(values[0],WINDOWBORDER)) graphicsdata.windowborder=Graphics_ConvertToOS(values[1]);
	else Desk_Msgs_Report(1,"Error.SynD:Syntax error %d",linenum);
}

static void Graphics_StoreTitleDetails(char *values[],int numvalues,int linenum)
/* Read a set of values into the title block*/
{
	if (!strcmp(values[0],FILETITLE)) {
		if (numvalues!=6) {
			Desk_Msgs_Report(1,"Error.SynT:Syntax error %d",linenum);
		} else {
			graphicsdata.titleheight=Graphics_ConvertToOS(values[1]);
			graphicsdata.title.size=16*(int)strtol(values[2],NULL,10);
			graphicsdata.title.colour=Graphics_RGBToPalette(values[3]);
			graphicsdata.title.bgcolour=Graphics_RGBToPalette(values[4]);
			strcpy(graphicsdata.title.fontname,values[5]);
		}
	} else Desk_Msgs_Report(1,"Error.SynT:Syntax error %d",linenum);
}

static void Graphics_StorePersonDetails(char *values[],int numvalues,int linenum)
/* Read a set of values into the person block*/
{
	graphictype graphictype=graphictype_INVALID;
	sextype sex=sex_ANY;
	AJWLib_Assert(graphicsdata.person!=NULL);
	AJWLib_Assert(graphicsdata.marriage!=NULL);
	if (values[0][1]=='_') {
		sex=(sextype)toupper(values[0][0]);
		values[0]+=2;
	}
	if (!strcmp(values[0],LINE)) graphictype=graphictype_LINE;
	else if (!strcmp(values[0],CHILDLINE)) graphictype=graphictype_CHILDLINE;
	else if (!strcmp(values[0],BOX)) graphictype=graphictype_RECTANGLE;
	else if (!strcmp(values[0],FILLEDBOX)) graphictype=graphictype_FILLEDRECTANGLE;
	else if (!strcmp(values[0],TEXT)) graphictype=graphictype_TEXTLABEL;
	else if (!strcmp(values[0],FIELD)) graphictype=graphictype_FIELD;
	else if (!strcmp(values[0],CENTREDTEXT)) graphictype=graphictype_CENTREDTEXTLABEL;
	else if (!strcmp(values[0],CENTREDFIELD)) graphictype=graphictype_CENTREDFIELD;
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
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.thickness=(int)strtol(values[5],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].sex=sex;
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
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.thickness=(int)strtol(values[5],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].sex=sex;
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
				graphicsdata.person[graphicsdata.numpersonobjects-1].sex=sex;
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
				size=16*(int)strtol(values[3],NULL,10);
				strcpy(graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.fontname,values[6]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.size=size;
				graphicsdata.person[graphicsdata.numpersonobjects-1].sex=sex;
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
				if (!strcmp(values[7],SURNAME)) field=personfieldtype_SURNAME;
				else if (!strcmp(values[7],FORENAME)) field=personfieldtype_FORENAME;
				else if (!strcmp(values[7],NAME)) field=personfieldtype_NAME;
				else if (!strcmp(values[7],MIDDLENAMES)) field=personfieldtype_MIDDLENAMES;
				else if (!strcmp(values[7],FULLNAME)) field=personfieldtype_FULLNAME;
				else if (!strcmp(values[7],INITIALEDMIDDLENAME)) field=personfieldtype_INITIALEDMIDDLENAME;
				else if (!strcmp(values[7],INITIALEDNAME)) field=personfieldtype_INITIALEDNAME;
				else if (!strcmp(values[7],SEX)) field=personfieldtype_SEX;
				else {
					int i;

					for (i=0;i<NUMBERPERSONUSERFIELDS;i++) {
						char buffer[FIELDSIZE], *ptr, *desc;

						ptr=buffer;
						desc=Database_GetPersonUserDesc(i);
						do {
							if (!isspace(*desc)) *ptr++=tolower(*desc);
						} while (*desc++!='\0');
						if (!strcmp(values[7],buffer)) field=(personfieldtype)i;
					}
				}
				if (field!=personfieldtype_UNKNOWN) {
					AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.personfields),(++graphicsdata.numpersonfields)*sizeof(fieldproperties));
					graphicsdata.personfields[graphicsdata.numpersonfields-1].type=graphictype;
					graphicsdata.personfields[graphicsdata.numpersonfields-1].personfieldtype=field;
					graphicsdata.personfields[graphicsdata.numpersonfields-1].textproperties.x=Graphics_ConvertToOS(values[1]);
					graphicsdata.personfields[graphicsdata.numpersonfields-1].textproperties.y=Graphics_ConvertToOS(values[2]);
					graphicsdata.personfields[graphicsdata.numpersonfields-1].textproperties.colour=Graphics_RGBToPalette(values[4]);
					graphicsdata.personfields[graphicsdata.numpersonfields-1].textproperties.bgcolour=Graphics_RGBToPalette(values[5]);
					size=16*(int)strtol(values[3],NULL,10);
					strcpy(graphicsdata.personfields[graphicsdata.numpersonfields-1].textproperties.fontname,values[6]);
					graphicsdata.personfields[graphicsdata.numpersonfields-1].textproperties.size=size;
					graphicsdata.personfields[graphicsdata.numpersonfields-1].sex=sex;
				}
			}
			break;
		default:
			break;
	}
}

static void Graphics_StoreMarriageDetails(char *values[],int numvalues,int linenum)
/* Read a set of values into the marriage block*/
{
	graphictype graphictype=graphictype_INVALID;
	if (!strcmp(values[0],LINE)) graphictype=graphictype_LINE;
	else if (!strcmp(values[0],CHILDLINE)) graphictype=graphictype_CHILDLINE;
	else if (!strcmp(values[0],SIBLINGLINE)) graphictype=graphictype_SIBLINGLINE;
	else if (!strcmp(values[0],BOX)) graphictype=graphictype_RECTANGLE;
	else if (!strcmp(values[0],FILLEDBOX)) graphictype=graphictype_FILLEDRECTANGLE;
	else if (!strcmp(values[0],TEXT)) graphictype=graphictype_TEXTLABEL;
	else if (!strcmp(values[0],FIELD)) graphictype=graphictype_FIELD;
	else if (!strcmp(values[0],CENTREDFIELD)) graphictype=graphictype_CENTREDFIELD;
	else Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
	switch (graphictype) {
		case graphictype_SIBLINGLINE:
			if (numvalues!=3) {
				Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
			} else {
				graphicsdata.siblinglinethickness=(int)strtol(values[1],NULL,10);
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
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.thickness=(int)strtol(values[5],NULL,10);
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
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.thickness=(int)strtol(values[5],NULL,10);
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
				size=16*(int)strtol(values[3],NULL,10);
			}
			break;
		case graphictype_CENTREDFIELD:
		case graphictype_FIELD:
			if (numvalues!=8) {
				Desk_Msgs_Report(1,"Error.SynM:Syntax error %d",linenum);
			} else {
				marriagefieldtype field=marriagefieldtype_UNKNOWN;
				int size,i;
				AJWLib_Str_LowerCase(values[7]);
				for (i=0;i<NUMBERMARRIAGEUSERFIELDS;i++) {
					char buffer[FIELDSIZE], *ptr, *desc;

					ptr=buffer;
					desc=Database_GetMarriageUserDesc(i);
					do {
						if (!isspace(*desc)) *ptr++=tolower(*desc);
					} while (*desc++!='\0');
					if (!strcmp(values[7],buffer)) field=(marriagefieldtype)i;
				}
				if (field!=marriagefieldtype_UNKNOWN) {
					AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriagefields),(++graphicsdata.nummarriagefields)*sizeof(fieldproperties));
					graphicsdata.marriagefields[graphicsdata.nummarriagefields-1].type=graphictype;
					graphicsdata.marriagefields[graphicsdata.nummarriagefields-1].marriagefieldtype=field;
					graphicsdata.marriagefields[graphicsdata.nummarriagefields-1].textproperties.x=Graphics_ConvertToOS(values[1]);
					graphicsdata.marriagefields[graphicsdata.nummarriagefields-1].textproperties.y=Graphics_ConvertToOS(values[2]);
					graphicsdata.marriagefields[graphicsdata.nummarriagefields-1].textproperties.colour=Graphics_RGBToPalette(values[4]);
					graphicsdata.marriagefields[graphicsdata.nummarriagefields-1].textproperties.bgcolour=Graphics_RGBToPalette(values[5]);
					size=16*(int)strtol(values[3],NULL,10);
					strcpy(graphicsdata.marriagefields[graphicsdata.nummarriagefields-1].textproperties.fontname,values[6]);
					graphicsdata.marriagefields[graphicsdata.nummarriagefields-1].textproperties.size=size;
				}
			}
			break;
		default:
			break;
	}
}

static void Graphics_ParseFile(char **file,void (*decodefn)(char *values[],int numvalues,int linenum))
/* Split a line from the file into an array of values, then pass to the specified function*/
{
	int ch=0,m=0,line=0;
	AJWLib_Assert(*file!=NULL);
	while ((*file)[m]!='\0') {
		char str[256];
		int i=-1;
		ch=(*file)[m++];
		while (ch!='\0' && ch!='\n' && i<254) {
			str[++i]=ch;
			ch=(*file)[m++];
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
			decodefn(values,j,line);
		}
	}
}

int Graphics_PersonHeight(void)
{
	return graphicsdata.personheight;
}

int Graphics_PersonWidth(void)
{
	return graphicsdata.personwidth;
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
}

static void Graphics_ClaimFonts(void)
/* Claim a font handle for each font used*/
{
	int i;
	AJWLib_Assert(graphicsdata.person!=NULL);
	AJWLib_Assert(graphicsdata.marriage!=NULL);
	graphicsdata.title.font=NULL;
	Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.title.font),graphicsdata.title.fontname,graphicsdata.title.size,graphicsdata.title.size));
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		if (graphicsdata.person[i].type==graphictype_CENTREDTEXTLABEL || graphicsdata.person[i].type==graphictype_TEXTLABEL) {
			graphicsdata.person[i].details.textlabel.properties.font=NULL;
			Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.person[i].details.textlabel.properties.font),graphicsdata.person[i].details.textlabel.properties.fontname,graphicsdata.person[i].details.textlabel.properties.size,graphicsdata.person[i].details.textlabel.properties.size));
		}
	}
	for (i=0;i<graphicsdata.numpersonfields;i++) {
		graphicsdata.personfields[i].textproperties.font=NULL;
		Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.personfields[i].textproperties.font),graphicsdata.personfields[i].textproperties.fontname,graphicsdata.personfields[i].textproperties.size,graphicsdata.personfields[i].textproperties.size));
	}
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		if (graphicsdata.marriage[i].type==graphictype_CENTREDTEXTLABEL || graphicsdata.marriage[i].type==graphictype_TEXTLABEL) {
			graphicsdata.marriage[i].details.textlabel.properties.font=NULL;
			Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.marriage[i].details.textlabel.properties.font),graphicsdata.marriage[i].details.textlabel.properties.fontname,graphicsdata.marriage[i].details.textlabel.properties.size,graphicsdata.marriage[i].details.textlabel.properties.size));
		}
	}
	for (i=0;i<graphicsdata.nummarriagefields;i++) {
		graphicsdata.marriagefields[i].textproperties.font=NULL;
		Desk_Error2_CheckOS(Desk_Font2_ClaimFont(&(graphicsdata.marriagefields[i].textproperties.font),graphicsdata.marriagefields[i].textproperties.fontname,graphicsdata.marriagefields[i].textproperties.size,graphicsdata.marriagefields[i].textproperties.size));
	}
}

static void Graphics_ReleaseFonts(void)
/* Release all font handles*/
{
	int i;
	Desk_Font2_ReleaseFont(&(graphicsdata.title.font));
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		if (graphicsdata.person[i].type==graphictype_CENTREDTEXTLABEL || graphicsdata.person[i].type==graphictype_TEXTLABEL) {
			Desk_Font2_ReleaseFont(&(graphicsdata.person[i].details.textlabel.properties.font));
		}
	}
	for (i=0;i<graphicsdata.numpersonfields;i++) {
		Desk_Font2_ReleaseFont(&(graphicsdata.personfields[i].textproperties.font));
	}
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		if (graphicsdata.marriage[i].type==graphictype_CENTREDTEXTLABEL || graphicsdata.marriage[i].type==graphictype_TEXTLABEL) {
			Desk_Font2_ReleaseFont(&(graphicsdata.marriage[i].details.textlabel.properties.font));
		}
	}
	for (i=0;i<graphicsdata.nummarriagefields;i++) {
		Desk_Font2_ReleaseFont(&(graphicsdata.marriagefields[i].textproperties.font));
	}
}

static void Graphics_DefaultStyle(void)
{
	graphicsdata.personwidth=200;
	graphicsdata.personheight=100;
	graphicsdata.gapheightabove=40;
	graphicsdata.gapheightbelow=40;
	graphicsdata.gapwidth=60;
	graphicsdata.marriagewidth=100;
	graphicsdata.windowborder=20;
	graphicsdata.siblinglinethickness=0;
	graphicsdata.siblinglinecolour=0;
	graphicsdata.titleheight=40;
	graphicsdata.title.size=24*16;
	graphicsdata.title.colour=0;
	graphicsdata.title.bgcolour=0xFFFFFF00;
	strcpy(graphicsdata.title.fontname,"Homerton.Bold");
	graphicsdata.numpersonobjects=0;
	graphicsdata.nummarriageobjects=0;
	graphicsdata.numpersonfields=0;
	graphicsdata.nummarriagefields=0;
}

static void Graphics_ParseStyle(void)
{
    AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.person),1);
	AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.marriage),1);
    AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.personfields),1);
	AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.marriagefields),1);
	Graphics_ParseFile(&personfile,Graphics_StorePersonDetails);
	Graphics_ParseFile(&dimensionsfile,Graphics_StoreDimensionDetails);
	Graphics_ParseFile(&marriagefile,Graphics_StoreMarriageDetails);
	Graphics_ParseFile(&titlefile,Graphics_StoreTitleDetails);
	Graphics_ClaimFonts();
}

void Graphics_LoadPersonFileLine(char *line)
/* Add the line to the personfile block, allocating as nessacery*/
{
	int offset;

	AJWLib_Assert(line!=NULL);
	if (personfile==NULL) {
		AJWLib_Flex_Alloc((flex_ptr)&personfile,strlen(line)+2);
		offset=0;
	} else {
		offset=AJWLib_Flex_Size((flex_ptr)&personfile);
		AJWLib_Flex_Extend((flex_ptr)&personfile,offset+strlen(line)+1);
		/* Overwrite existing '\0'*/
		offset--;
	}
	strcpy(personfile+offset,line);
	personfile[offset+strlen(line)]='\n';
	personfile[offset+strlen(line)+1]='\0';
}

void Graphics_LoadLuaFileLine(char *line)
/* Add the line to the personfile block, allocating as nessacery*/
{
	int offset;

	AJWLib_Assert(line!=NULL);
	if (luafile==NULL) {
		AJWLib_Flex_Alloc((flex_ptr)&luafile,strlen(line)+2);
		offset=0;
	} else {
		offset=AJWLib_Flex_Size((flex_ptr)&luafile);
		AJWLib_Flex_Extend((flex_ptr)&luafile,offset+strlen(line)+1);
		/* Overwrite existing '\0'*/
		offset--;
	}
	strcpy(luafile+offset,line);
	luafile[offset+strlen(line)]='\n';
	luafile[offset+strlen(line)+1]='\0';
}

void Graphics_LoadMarriageFileLine(char *line)
/* Add the line to the marriagefile block, allocating as nessacery*/
{
	int offset;

	AJWLib_Assert(line!=NULL);
	if (marriagefile==NULL) {
		AJWLib_Flex_Alloc((flex_ptr)&marriagefile,strlen(line)+2);
		offset=0;
	} else {
		offset=AJWLib_Flex_Size((flex_ptr)&marriagefile);
		AJWLib_Flex_Extend((flex_ptr)&marriagefile,offset+strlen(line)+1);
		/* Overwrite existing '\0'*/
		offset--;
	}
	strcpy(marriagefile+offset,line);
	marriagefile[offset+strlen(line)]='\n';
	marriagefile[offset+strlen(line)+1]='\0';
}

void Graphics_LoadDimensionsFileLine(char *line)
/* Add the line to the dimensionsfile block, allocating as nessacery*/
{
	int offset;

	AJWLib_Assert(line!=NULL);
	if (dimensionsfile==NULL) {
		AJWLib_Flex_Alloc((flex_ptr)&dimensionsfile,strlen(line)+2);
		offset=0;
	} else {
		offset=AJWLib_Flex_Size((flex_ptr)&dimensionsfile);
		AJWLib_Flex_Extend((flex_ptr)&dimensionsfile,offset+strlen(line)+1);
		/* Overwrite existing '\0'*/
		offset--;
	}
	strcpy(dimensionsfile+offset,line);
	dimensionsfile[offset+strlen(line)]='\n';
	dimensionsfile[offset+strlen(line)+1]='\0';
}

void Graphics_LoadTitleFileLine(char *line)
/* Add the line to the titlefile block, allocating as nessacery*/
{
	int offset;

	AJWLib_Assert(line!=NULL);
	if (titlefile==NULL) {
		AJWLib_Flex_Alloc((flex_ptr)&titlefile,strlen(line)+2);
		offset=0;
	} else {
		offset=AJWLib_Flex_Size((flex_ptr)&titlefile);
		AJWLib_Flex_Extend((flex_ptr)&titlefile,offset+strlen(line)+1);
		/* Overwrite existing '\0'*/
		offset--;
	}
	strcpy(titlefile+offset,line);
	titlefile[offset+strlen(line)]='\n';
	titlefile[offset+strlen(line)+1]='\0';
}

void Graphics_SetGraphicsStyle(char *style)
/* Set the name of the current graphics style, then import it if needed and parse it*/
{
	char filename[256];

	AJWLib_Assert(style!=NULL);
	strcpy(currentstyle,style);

	if (luafile!=NULL) {
		uselua=Desk_TRUE;
		Graphics_LuaInit();
	} else {
		uselua=Desk_FALSE;
		Graphics_ParseStyle();
	}

	Desk_Error2_Try {
		if (Config_ImportGraphicsStyle()) {
			int type;
			sprintf(filename,"%s.%s.%s",choicesread,GRAPHICSDIR,currentstyle);
			Desk_Error2_CheckOS(Desk_SWI(2,1,Desk_SWI_OS_File,17,filename,&type));
			if (type==0) {
				/*Not found, so import*/
				if (uselua) {
					sprintf(filename,"%s.%s",choiceswrite,GRAPHICSDIR);
					if (!Desk_File_IsDirectory(filename)) Desk_File_EnsureDirectory(filename);
					sprintf(filename,"%s.%s.%s",choiceswrite,GRAPHICSDIR,currentstyle);
					Desk_File_SaveMemory(filename,luafile,strlen(luafile));
				} else {
					sprintf(filename,"%s.%s.%s",choiceswrite,GRAPHICSDIR,currentstyle);
					if (!Desk_File_IsDirectory(filename)) Desk_File_EnsureDirectory(filename);
					sprintf(filename,"%s.%s.%s.%s",choiceswrite,GRAPHICSDIR,currentstyle,"Person");
					Desk_File_SaveMemory(filename,personfile,strlen(personfile));
					sprintf(filename,"%s.%s.%s.%s",choiceswrite,GRAPHICSDIR,currentstyle,"Marriage");
					Desk_File_SaveMemory(filename,marriagefile,strlen(marriagefile));
					sprintf(filename,"%s.%s.%s.%s",choiceswrite,GRAPHICSDIR,currentstyle,"Dimensions");
					Desk_File_SaveMemory(filename,dimensionsfile,strlen(dimensionsfile));
					sprintf(filename,"%s.%s.%s.%s",choiceswrite,GRAPHICSDIR,currentstyle,"Title");
					Desk_File_SaveMemory(filename,titlefile,strlen(titlefile));
				}
			}
		}
	} Desk_Error2_Catch {
		AJWLib_Error2_ReportMsgs("Error.GraphS:%s");
	} Desk_Error2_EndCatch
}

void Graphics_SaveGEDCOM(FILE *file)
{
	int i;
	
	AJWLib_Assert(file!=NULL);

	fprintf(file,"0 @G1@ _GRAPHICS\n");
	if (uselua) {
		fprintf(file,"1 _LUAFILE\n2 _LINE ");
		for (i=0;luafile[i]!='\0';i++) {
			if (luafile[i]=='\n') {
				fprintf(file,"\n2 _LINE ");
			} else {
				fputc(luafile[i],file);
			}
		}
		fputc('\n',file);
	} else {
		fprintf(file,"1 _PERSONFILE\n2 _LINE ");
		for (i=0;personfile[i]!='\0';i++) {
			if (personfile[i]=='\n') {
				fprintf(file,"\n2 _LINE ");
			} else {
				fputc(personfile[i],file);
			}
		}
		fputc('\n',file);
	
		fprintf(file,"1 _MARRIAGEFILE\n2 _LINE ");
		for (i=0;marriagefile[i]!='\0';i++) {
			if (marriagefile[i]=='\n') {
				fprintf(file,"\n2 _LINE ");
			} else {
				fputc(marriagefile[i],file);
			}
		}
		fputc('\n',file);
	
		fprintf(file,"1 _TITLEFILE\n2 _LINE ");
		for (i=0;titlefile[i]!='\0';i++) {
			if (titlefile[i]=='\n') {
				fprintf(file,"\n2 _LINE ");
			} else {
				fputc(titlefile[i],file);
			}
		}
		fputc('\n',file);
	
		fprintf(file,"1 _DIMENSIONSFILE\n2 _LINE ");
		for (i=0;dimensionsfile[i]!='\0';i++) {
			if (dimensionsfile[i]=='\n') {
				fprintf(file,"\n2 _LINE ");
			} else {
				fputc(dimensionsfile[i],file);
			}
		}
		fputc('\n',file);
	}

	fprintf(file,"1 _CURRENTSTYLE %s\n",currentstyle);
}

void Graphics_RemoveStyle(void)
{
	int i;
	
	if (uselua) {
		lua_close(luadetails.state);
		for (i=0;i<luadetails.numberoffonts;i++) Desk_Font2_ReleaseFont(&(luadetails.fonts[i].handle));
		luadetails.numberoffonts=0;
		if (luafile!=NULL) AJWLib_Flex_Free((flex_ptr)&luafile);
	} else {
		Graphics_ReleaseFonts();
		graphicsdata.numpersonobjects=0;
		graphicsdata.nummarriageobjects=0;
		graphicsdata.numpersonfields=0;
		graphicsdata.nummarriagefields=0;
		if (graphicsdata.person!=NULL) AJWLib_Flex_Free((flex_ptr)&(graphicsdata.person));
		if (graphicsdata.marriage!=NULL) AJWLib_Flex_Free((flex_ptr)&(graphicsdata.marriage));
		if (graphicsdata.personfields!=NULL) AJWLib_Flex_Free((flex_ptr)&(graphicsdata.personfields));
		if (graphicsdata.marriagefields!=NULL) AJWLib_Flex_Free((flex_ptr)&(graphicsdata.marriagefields));
		if (personfile!=NULL) AJWLib_Flex_Free((flex_ptr)&personfile);
		if (marriagefile!=NULL) AJWLib_Flex_Free((flex_ptr)&marriagefile);
		if (dimensionsfile!=NULL) AJWLib_Flex_Free((flex_ptr)&dimensionsfile);
		if (titlefile!=NULL) AJWLib_Flex_Free((flex_ptr)&titlefile);
	}
}

static void Graphics_LoadStyleFile(char *style,char *filename,char **anchor)
{
	int size;
	char fullfilename[256];
	AJWLib_Assert(style!=NULL);
	AJWLib_Assert(filename!=NULL);
	AJWLib_Assert(*anchor==NULL);
	sprintf(fullfilename,"%s.%s.%s.%s",choicesread,GRAPHICSDIR,style,filename);
	if (!Desk_File_Exists(fullfilename)) AJWLib_Error2_HandleMsgs("Error.NoFile:File %s does not exist in dir %s",filename,style);
	size=Desk_File_Size(fullfilename);
	AJWLib_Flex_Alloc((flex_ptr)anchor,size+1);
	Desk_File_LoadTo(fullfilename,*anchor,NULL);
	(*anchor)[size]='\0';
}

static void Graphics_LuaThrowError(lua_State *state,char *tag,...)
{
	va_list ap;
	char buffer[256];

	va_start(ap,tag);
    vsprintf(buffer,AJWLib_Msgs_TempLookup(tag),ap);
    va_end(ap);
    lua_error(state,buffer);
}

static int Graphics_LuaPlotLine(lua_State *state)
{
	int x0,y0,x1,y1,thickness;
	unsigned int colour;
	int i;

	if (lua_gettop(state)!=6) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","PlotLine");
	for (i=1;i<=6;i++) if (!lua_isnumber(state,i)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",i,"PlotLine");
	x0=(int)lua_tonumber(state,1);
	y0=(int)lua_tonumber(state,2);
	x1=(int)lua_tonumber(state,3);
	y1=(int)lua_tonumber(state,4);
	thickness=(int)lua_tonumber(state,5);
	colour=(unsigned int)lua_tonumber(state,6);
	Graphics_PlotLine(luadetails.scale,luadetails.originx,luadetails.originy,x0,y0,x1,y1,thickness,colour);
	return 0;
}

static int Graphics_LuaPlotRectangle(lua_State *state)
{
	int x,y,width,height,thickness;
	unsigned int colour;
	int i;

	if (lua_gettop(state)!=6) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","PlotRectangle");
	for (i=1;i<=6;i++) if (!lua_isnumber(state,i)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",i,"PlotRectangle");
	x=(int)lua_tonumber(state,1);
	y=(int)lua_tonumber(state,2);
	width=(int)lua_tonumber(state,3);
	height=(int)lua_tonumber(state,4);
	thickness=(int)lua_tonumber(state,5);
	colour=(unsigned int)lua_tonumber(state,6);
	Graphics_PlotRectangle(luadetails.scale,luadetails.originx,luadetails.originy,x,y,width,height,thickness,colour);
	return 0;
}

static int Graphics_LuaPlotRectangleFilled(lua_State *state)
{
	int x,y,width,height;
	unsigned int colour;
	int i;

	if (lua_gettop(state)!=5) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","PlotRectangleFilled");
	for (i=1;i<=5;i++) if (!lua_isnumber(state,i)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",i,"PlotRectangleFilled");
	x=(int)lua_tonumber(state,1);
	y=(int)lua_tonumber(state,2);
	width=(int)lua_tonumber(state,3);
	height=(int)lua_tonumber(state,4);
	colour=(unsigned int)lua_tonumber(state,5);
	Graphics_PlotRectangleFilled(luadetails.scale,luadetails.originx,luadetails.originy,x,y,width,height,0,colour);
	return 0;
}

static int Graphics_LuaPlotText(lua_State *state)
{
	int x,y;
	int handle;
	const char *text;
	unsigned int bgcolour,fgcolour;
	int i;

	if (lua_gettop(state)!=6) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","PlotText");
	for (i=1;i<=5;i++) if (!lua_isnumber(state,i)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",i,"PlotText");
	if (!lua_isstring(state,6)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",6,"PlotText");
	x=(int)lua_tonumber(state,1);
	y=(int)lua_tonumber(state,2);
	handle=(int)lua_tonumber(state,3);
	bgcolour=(unsigned int)lua_tonumber(state,4);
	fgcolour=(unsigned int)lua_tonumber(state,5);
	text=lua_tostring(state,6);
	Graphics_PlotText(luadetails.scale,luadetails.originx,luadetails.originy,x,y,luadetails.fonts[handle].handle->handle,luadetails.fonts[handle].fontname,luadetails.fonts[handle].size,bgcolour,fgcolour,(char *)text);
	return 0;
}

static int Graphics_LuaClaimFont(lua_State *state)
{
	int handle;
	const char *fontname;

	if (lua_gettop(state)!=2) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","ClaimFont");
	if (!lua_isstring(state,1)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",1,"ClaimFont");
	if (!lua_isnumber(state,2)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",2,"ClaimFont");
	handle=luadetails.numberoffonts;
	if (handle>=MAXFONTS) Graphics_LuaThrowError(state,"Error.Lua7:Too many fonts");
	fontname=lua_tostring(state,1);
	luadetails.fonts[handle].fontname=Desk_DeskMem_Malloc(strlen(fontname)+1);
	strcpy(luadetails.fonts[handle].fontname,fontname);
	luadetails.fonts[handle].size=(int)(16*lua_tonumber(state,2));
	Desk_Error2_Try {
		Desk_Font2_ClaimFont(&(luadetails.fonts[handle].handle),luadetails.fonts[handle].fontname,luadetails.fonts[handle].size,luadetails.fonts[handle].size);
	} Desk_Error2_Catch {
		if (Desk_Error2_globalblock.type==Desk_error2_type_OSERROR) {
			Graphics_LuaThrowError(state,"Error.NoFont:%s",Desk_Error2_globalblock.data.oserror->errmess);
		} else {
			Desk_Error2_ReThrow();
		}
	} Desk_Error2_EndCatch
	luadetails.numberoffonts++;
	lua_pushnumber(state,handle);
	return 1;
}

static int Graphics_LuaGetField(lua_State *state)
{
	char *fieldname;
	elementptr element;

	if (lua_gettop(state)!=2) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","GetField");
	if (!lua_isnumber(state,1)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",1,"GetField");
	if (!lua_isstring(state,2)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",2,"GetField");
	element=(elementptr)lua_tonumber(state,1);
	fieldname=(char *)lua_tostring(state,2);
	lua_pushstring(state,Database_GetField(element,fieldname));
	return 1;
}

static int Graphics_LuaGetCoords(lua_State *state)
{
	elementptr element;

	if (lua_gettop(state)!=1) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","GetCoords");
	if (!lua_isnumber(state,1)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",1,"GetCoords");
	element=(elementptr)lua_tonumber(state,1);
	lua_pushnumber(state,Layout_FindXCoord(redrawlayout,element));
	lua_pushnumber(state,Layout_FindYCoord(redrawlayout,element));
	return 2;
}

static int Graphics_LuaGetChild(lua_State *state)
{
	elementptr element;

	if (lua_gettop(state)!=1) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","GetChild");
	if (!lua_isnumber(state,1)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",1,"GetChild");
	element=(elementptr)lua_tonumber(state,1);
	element=Database_GetLeftChild(element);
	if (element!=none) {
		lua_pushnumber(state,element);
		return 1;
	}
	return 0;
}

static int Graphics_LuaGetParentsMarriage(lua_State *state)
{
	elementptr element;

	if (lua_gettop(state)!=1) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","GetParentsMarriage");
	if (!lua_isnumber(state,1)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",1,"GetParentsMarriage");
	element=(elementptr)lua_tonumber(state,1);
	element=Database_GetParentsMarriage(element);
	if (element!=none) {
		lua_pushnumber(state,element);
		return 1;
	}
	return 0;
}

static int Graphics_LuaGetTextDimensions(lua_State *state)
{
	int handle;
	char *text;
	Desk_wimp_point *bbox;

	if (lua_gettop(state)!=2) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","GetTextDimensions");
	if (!lua_isnumber(state,1)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",1,"GetTextDimensions");
	if (!lua_isstring(state,2)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",2,"GetTextDimensions");
	handle=(int)lua_tonumber(state,1);
	text=(char *)lua_tostring(state,2);
	bbox=AJWLib_Font_GetWidthAndHeight(luadetails.fonts[handle].handle->handle,text);
	lua_pushnumber(state,bbox->x);
	lua_pushnumber(state,bbox->y);
	return 2;
}

static int Graphics_LuaColour(lua_State *state)
{
	unsigned int colour,red,green,blue;
	int i;

	if (lua_gettop(state)!=3) Graphics_LuaThrowError(state,"Error.Lua5:Wrong args to %s","Colour");
	for (i=1;i<=3;i++) if (!lua_isnumber(state,i)) Graphics_LuaThrowError(state,"Error.Lua6:Bad arg %d to %s",i,"Colour");
	red=(unsigned int)lua_tonumber(state,1);
	green=(unsigned int)lua_tonumber(state,2);
	blue=(unsigned int)lua_tonumber(state,3);
	if (red>255) red=255;
	if (green>255) green=255;
	if (blue>255) blue=255;
	colour=((red<<8) | (green<<16) | (blue<<24));
	lua_pushnumber(state,colour);
	return 1;
}

static int Graphics_LuaError(lua_State *state)
{
	const char *err;
	int len;
	int i,j;

	err=lua_tostring(state,1);
	if (err==NULL) err="Error in LuaError";
	len=strlen(err);
	luadetails.error=Desk_DeskMem_Malloc(len+1);
	j=0;
	/*Strip control chars*/
	for (i=0;i<len;i++) if (err[i]>=' ') luadetails.error[j++]=err[i];
	luadetails.error[j]='\0';
	return 0;
}

static void Graphics_LuaCheckError(int returncode)
{
	char *errormsg="Unknown error";

	if (luadetails.error) errormsg=luadetails.error;
	switch (returncode) {
		case 0:
			if (luadetails.error==NULL) break;
		case LUA_ERRRUN:
			AJWLib_Error2_HandleMsgs("Error.Lua1:%s",errormsg);
			break;
		case LUA_ERRFILE:
			AJWLib_Error2_HandleMsgs("Error.Lua2:Unable to open file");
			break;
		case LUA_ERRSYNTAX:
			AJWLib_Error2_HandleMsgs("Error.Lua3:%s",errormsg);
			break;
		case LUA_ERRMEM:
			luadetails.error="Out of memory"; /*FIXME?*/
			AJWLib_Error2_HandleMsgs("Error.NoMem:Out of memory");
			break;
		default:
			AJWLib_Error2_HandleMsgs("Error.Lua4:%s",errormsg);
			break;
	}
}

static void Graphics_LuaInit(void)
{
	luadetails.error="Init Error"; /*Don't use lua if an error occours before we are initialised*/
	luadetails.numberoffonts=0;

	/*Initialise Lua*/
	luadetails.state=lua_open(0);
	lua_strlibopen(luadetails.state);
	/*Replace default error handler*/
	lua_register(luadetails.state,"_ERRORMESSAGE",Graphics_LuaError);
	/*Register functions that can be called from Lua*/
	lua_register(luadetails.state,"PlotLine",Graphics_LuaPlotLine);
	lua_register(luadetails.state,"PlotRectangle",Graphics_LuaPlotRectangle);
	lua_register(luadetails.state,"PlotRectangleFilled",Graphics_LuaPlotRectangleFilled);
	lua_register(luadetails.state,"PlotText",Graphics_LuaPlotText);
	lua_register(luadetails.state,"ClaimFont",Graphics_LuaClaimFont);
	lua_register(luadetails.state,"GetField",Graphics_LuaGetField);
	lua_register(luadetails.state,"GetCoords",Graphics_LuaGetCoords);
	lua_register(luadetails.state,"GetChild",Graphics_LuaGetChild);
	lua_register(luadetails.state,"GetParentsMarriage",Graphics_LuaGetParentsMarriage);
	lua_register(luadetails.state,"GetTextDimensions",Graphics_LuaGetTextDimensions);
	lua_register(luadetails.state,"Colour",Graphics_LuaColour);
	/*Load the lua file, and run any bits of it that are not functions*/
	luadetails.error=NULL;
	Desk_Error2_Try {
		Graphics_LuaCheckError(lua_dobuffer(luadetails.state,luafile,AJWLib_Flex_Size((flex_ptr)&luafile)-1,currentstyle));
		/*Load dimesions from global vars*/
		Graphics_LuaGetGlobal(luadetails.state,"gapheightabove",graphicsdata.gapheightabove);
		Graphics_LuaGetGlobal(luadetails.state,"gapheightbelow",graphicsdata.gapheightbelow);
		Graphics_LuaGetGlobal(luadetails.state,"personwidth",graphicsdata.personwidth);
		Graphics_LuaGetGlobal(luadetails.state,"personheight",graphicsdata.personheight);
		Graphics_LuaGetGlobal(luadetails.state,"gapwidth",graphicsdata.gapwidth);
		Graphics_LuaGetGlobal(luadetails.state,"marriagewidth",graphicsdata.marriagewidth);
		Graphics_LuaGetGlobal(luadetails.state,"windowborder",graphicsdata.windowborder);
		Graphics_LuaGetGlobal(luadetails.state,"titleheight",graphicsdata.titleheight);
	} Desk_Error2_Catch {
		if (luadetails.error==NULL) luadetails.error="Unknown error";
		AJWLib_Error2_Report("%s");
	} Desk_Error2_EndCatch
}

void Graphics_LoadStyle(char *style)
{
	char filename[256];
	AJWLib_Assert(graphicsdata.person==NULL);
	AJWLib_Assert(graphicsdata.marriage==NULL);
	AJWLib_Assert(personfile==NULL);
	AJWLib_Assert(marriagefile==NULL);
	AJWLib_Assert(dimensionsfile==NULL);
	AJWLib_Assert(titlefile==NULL);
	AJWLib_Assert(luafile==NULL);
	AJWLib_Assert(style!=NULL);

	Graphics_DefaultStyle();

	sprintf(filename,"%s.%s.%s",choicesread,GRAPHICSDIR,style);
	if (Desk_File_IsDirectory(filename)) uselua=Desk_FALSE; else uselua=Desk_TRUE;

	Desk_Error2_Try {
		strcpy(currentstyle,style);
		if (uselua) {
			int size;

			luadetails.error="Init Error"; /*Don't use lua if an error occours before we are initialised*/
			luadetails.numberoffonts=0;
			size=Desk_File_Size(filename);
			AJWLib_Flex_Alloc((flex_ptr)&luafile,size+1);
			if (size) Desk_File_LoadTo(filename,luafile,NULL);
			luafile[size]='\0';
			Graphics_LuaInit();
		} else {
			Graphics_LoadStyleFile(style,"Person",&personfile);
			Graphics_LoadStyleFile(style,"Marriage",&marriagefile);
			Graphics_LoadStyleFile(style,"Dimensions",&dimensionsfile);
			Graphics_LoadStyleFile(style,"Title",&titlefile);
			Graphics_ParseStyle();
		}
	} Desk_Error2_Catch {
		Graphics_RemoveStyle();
		Graphics_DefaultStyle();
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
}

char *Graphics_GetCurrentStyle(void)
{
	return currentstyle;
}

static void Graphics_PlotPerson(int scale,int originx,int originy,elementptr person,int x,int y,int width,int height,Desk_bool selected)
{
	int i;
	AJWLib_Assert(person>0);
	if (uselua) {
		luadetails.originx=originx;
		luadetails.originy=originy;
		luadetails.scale=scale;
		if (luadetails.error) return; /*Avoid infinite loop of error message causing redraw causing error...*/
		lua_getglobal(luadetails.state,"RedrawPerson");
		if (lua_isfunction(luadetails.state,-1)) {
			lua_pushnumber(luadetails.state,person);
			lua_pushnumber(luadetails.state,x);
			lua_pushnumber(luadetails.state,y);
			lua_pushnumber(luadetails.state,width);
			lua_pushnumber(luadetails.state,height);
			Graphics_LuaCheckError(lua_call(luadetails.state,5,0));
		}
	} else {
		for (i=0;i<graphicsdata.numpersonobjects;i++) {
			int xcoord=0;
			if (graphicsdata.person[i].sex==sex_ANY || graphicsdata.person[i].sex==Database_GetSex(person)) {
				switch (graphicsdata.person[i].type) {
					case graphictype_RECTANGLE:
						Graphics_PlotRectangle(scale,originx,originy,x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
						break;
					case graphictype_FILLEDRECTANGLE:
						Graphics_PlotRectangleFilled(scale,originx,originy,x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
						break;
					case graphictype_CHILDLINE:
						if (!Database_GetMother(person)) break;
					case graphictype_LINE:
						Graphics_PlotLine(scale,originx,originy,x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,x+graphicsdata.person[i].details.linebox.x1,y+graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
						break;
					case graphictype_CENTREDTEXTLABEL:
						xcoord=-AJWLib_Font_GetWidth(graphicsdata.person[i].details.textlabel.properties.font->handle,graphicsdata.person[i].details.textlabel.text)/2;
					case graphictype_TEXTLABEL:
						Graphics_PlotText(scale,originx,originy,x+xcoord+graphicsdata.person[i].details.textlabel.properties.x,y+graphicsdata.person[i].details.textlabel.properties.y,graphicsdata.person[i].details.textlabel.properties.font->handle,graphicsdata.person[i].details.textlabel.properties.fontname,graphicsdata.person[i].details.textlabel.properties.size,graphicsdata.person[i].details.textlabel.properties.bgcolour,graphicsdata.person[i].details.textlabel.properties.colour,graphicsdata.person[i].details.textlabel.text);
						break;
					default:
						break;
				}
			}
		}
		for (i=0;i<graphicsdata.numpersonfields;i++) {
			char fieldtext[3*FIELDSIZE+1]="";
			int xcoord=0;
			if (graphicsdata.personfields[i].sex==sex_ANY || graphicsdata.personfields[i].sex==Database_GetSex(person)) {
				if (graphicsdata.personfields[i].personfieldtype<personfieldtype_UNKNOWN) {
					strcpy(fieldtext,Database_GetPersonUserField(person,graphicsdata.personfields[i].personfieldtype));
				} else {
					switch (graphicsdata.personfields[i].personfieldtype) {
						case personfieldtype_SURNAME:
							strcpy(fieldtext,Database_GetSurname(person));
							break;
						case personfieldtype_FORENAME:
							strcpy(fieldtext,Database_GetForename(person));
							break;
						case personfieldtype_MIDDLENAMES:
							strcpy(fieldtext,Database_GetMiddleNames(person));
							break;
						case personfieldtype_NAME:
							strcpy(fieldtext,Database_GetName(person));
							break;
						case personfieldtype_FULLNAME:
							strcpy(fieldtext,Database_GetFullName(person));
							break;
						case personfieldtype_INITIALEDMIDDLENAME:
							strcpy(fieldtext,Database_GetInitialedMiddleName(person));
							break;
						case personfieldtype_INITIALEDNAME:
							strcpy(fieldtext,Database_GetInitialedName(person));
							break;
						case personfieldtype_SEX:
							sprintf(fieldtext,"%c",Database_GetSex(person));
							break;
						default:
							strcpy(fieldtext,"Unimplemented");
					}
				}
				if (graphicsdata.personfields[i].type==graphictype_CENTREDFIELD) xcoord=-AJWLib_Font_GetWidth(graphicsdata.personfields[i].textproperties.font->handle,fieldtext)/2;
				Graphics_PlotText(scale,originx,originy,x+xcoord+graphicsdata.personfields[i].textproperties.x,y+graphicsdata.personfields[i].textproperties.y,graphicsdata.personfields[i].textproperties.font->handle,graphicsdata.personfields[i].textproperties.fontname,graphicsdata.personfields[i].textproperties.size,graphicsdata.personfields[i].textproperties.bgcolour,graphicsdata.personfields[i].textproperties.colour,fieldtext);
			}
		}
	}
	if (selected) Draw_EORRectangleFilled(scale,originx,originy,x,y,width,height,EORCOLOUR);
}

static void Graphics_PlotMarriage(int scale,int originx,int originy,elementptr marriage,int x,int y,int width,int height,Desk_bool selected)
{
	int i,xcoord=0;
	AJWLib_Assert(marriage>0);
	if (uselua) {
		luadetails.originx=originx;
		luadetails.originy=originy;
		luadetails.scale=scale;
		if (luadetails.error) return; /*Avoid infinite loop of error message causeing redraw causeing error...*/
		lua_getglobal(luadetails.state,"RedrawMarriage");
		if (lua_isfunction(luadetails.state,-1)) {
			lua_pushnumber(luadetails.state,marriage);
			lua_pushnumber(luadetails.state,x);
			lua_pushnumber(luadetails.state,y);
			lua_pushnumber(luadetails.state,width);
			lua_pushnumber(luadetails.state,height);
			Graphics_LuaCheckError(lua_call(luadetails.state,5,0));
		}
	} else {
		for (i=0;i<graphicsdata.nummarriageobjects;i++) {
			switch (graphicsdata.marriage[i].type) {
				case graphictype_RECTANGLE:
					Graphics_PlotRectangle(scale,originx,originy,x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
					break;
				case graphictype_FILLEDRECTANGLE:
					Graphics_PlotRectangleFilled(scale,originx,originy,x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
					break;
				case graphictype_CHILDLINE:
					if (!Database_GetLeftChild(marriage)) break;
				case graphictype_LINE:
					Graphics_PlotLine(scale,originx,originy,x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,x+graphicsdata.marriage[i].details.linebox.x1,y+graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
					break;
				case graphictype_CENTREDTEXTLABEL:
					xcoord=-AJWLib_Font_GetWidth(graphicsdata.marriage[i].details.textlabel.properties.font->handle,graphicsdata.marriage[i].details.textlabel.text)/2;
				case graphictype_TEXTLABEL:
					Graphics_PlotText(scale,originx,originy,x+xcoord+graphicsdata.marriage[i].details.textlabel.properties.x,y+graphicsdata.marriage[i].details.textlabel.properties.y,graphicsdata.marriage[i].details.textlabel.properties.font->handle,graphicsdata.marriage[i].details.textlabel.properties.fontname,graphicsdata.marriage[i].details.textlabel.properties.size,graphicsdata.marriage[i].details.textlabel.properties.bgcolour,graphicsdata.marriage[i].details.textlabel.properties.colour,graphicsdata.marriage[i].details.textlabel.text);
					break;
				default:
					break;
			}
		}
		for (i=0;i<graphicsdata.nummarriagefields;i++) {
			char fieldtext[FIELDSIZE]="Unimplemented";
			if (graphicsdata.marriagefields[i].marriagefieldtype<marriagefieldtype_UNKNOWN) {
				strcpy(fieldtext,Database_GetMarriageUserField(marriage,graphicsdata.marriagefields[i].marriagefieldtype));
			}
			if (graphicsdata.marriagefields[i].type==graphictype_CENTREDFIELD) xcoord=-AJWLib_Font_GetWidth(graphicsdata.marriagefields[i].textproperties.font->handle,fieldtext)/2;
			Graphics_PlotText(scale,originx,originy,x+xcoord+graphicsdata.marriagefields[i].textproperties.x,y+graphicsdata.marriagefields[i].textproperties.y,graphicsdata.marriagefields[i].textproperties.font->handle,graphicsdata.marriagefields[i].textproperties.fontname,graphicsdata.marriagefields[i].textproperties.size,graphicsdata.marriagefields[i].textproperties.bgcolour,graphicsdata.marriagefields[i].textproperties.colour,fieldtext);
		}
	}
	if (selected) Draw_EORRectangleFilled(scale,originx,originy,x,y,width,height,EORCOLOUR);
}

static void Graphics_PlotChildLine(int scale,int originx,int originy,int x,int y,int width,int height)
{
	if (uselua) {
		luadetails.originx=originx;
		luadetails.originy=originy;
		luadetails.scale=scale;
		if (luadetails.error) return; /*Avoid infinite loop of error message causing redraw causing error...*/
		lua_getglobal(luadetails.state,"RedrawLine");
		if (lua_isfunction(luadetails.state,-1)) {
			lua_pushnumber(luadetails.state,x);
			lua_pushnumber(luadetails.state,y);
			lua_pushnumber(luadetails.state,width);
			lua_pushnumber(luadetails.state,height);
			Graphics_LuaCheckError(lua_call(luadetails.state,4,0));
		}
	} else {
		Graphics_PlotLine(scale,originx,originy,x,y,x+width,y+height,graphicsdata.siblinglinethickness,graphicsdata.siblinglinecolour);
	}
}

static void Graphics_PlotTitle(int scale,int originx,int originy,int x,int y,int width,int height)
{
	if (uselua) {
		luadetails.originx=originx;
		luadetails.originy=originy;
		luadetails.scale=scale;
		if (luadetails.error) return; /*Avoid infinite loop of error message causing redraw causing error...*/
		lua_getglobal(luadetails.state,"RedrawTitle");
		if (lua_isfunction(luadetails.state,-1)) {
			lua_pushstring(luadetails.state,Database_GetTitle());
			lua_pushnumber(luadetails.state,x);
			lua_pushnumber(luadetails.state,y);
			lua_pushnumber(luadetails.state,width);
			lua_pushnumber(luadetails.state,height);
			Graphics_LuaCheckError(lua_call(luadetails.state,5,0));
		}
	} else {
		Desk_wimp_point *fontbbox;

		fontbbox=AJWLib_Font_GetWidthAndHeightGiven(graphicsdata.title.fontname,graphicsdata.title.size,Database_GetTitle());
		Graphics_PlotText(scale,originx,originy,x+(width-fontbbox->x)/2,y+(height-fontbbox->y)/2,graphicsdata.title.font->handle,graphicsdata.title.fontname,graphicsdata.title.size,graphicsdata.title.bgcolour,graphicsdata.title.colour,Database_GetTitle());
	}
}

void Graphics_SetFunctions(plotfn plotline,plotfn plotrect,plotfn plotrectfilled,plottextfn plottext)
{
	Graphics_PlotLine=plotline;
	Graphics_PlotRectangle=plotrect;
	Graphics_PlotRectangleFilled=plotrectfilled;
	Graphics_PlotText=plottext;
}

void Graphics_PlotElement(layout *layout,elementptr element,int scale,int originx,int originy,int x,int y,int width,int height,Desk_bool plotselection)
{
	redrawlayout=layout;
	switch (Database_GetElementType(element)) {
		case element_LINE:
			Graphics_PlotChildLine(scale,originx,originy,x,y,width,height);
			break;
		case element_TITLE:
			Graphics_PlotTitle(scale,originx,originy,x,y,width,height);
			break;
		case element_PERSON:
			Graphics_PlotPerson(scale,originx,originy,element,x,y,width,height,plotselection ? Layout_GetSelect(element) : Desk_FALSE);
			break;
		case element_MARRIAGE:
			Graphics_PlotMarriage(scale,originx,originy,element,x,y,width,height,plotselection ? Layout_GetSelect(element) : Desk_FALSE);
			break;
	}
}


