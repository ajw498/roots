/*
	FT - Graphics Configuration
	© Alex Waugh 1999

	$Log: Graphics.c,v $
	Revision 1.11  2000/01/14 13:50:10  AJW
	Renamed Graphics.h to Windos.h etc

	Revision 1.10  2000/01/14 13:14:55  AJW
	Changed to Graphics_
	Added Graphics_Redraw etc.

	Revision 1.9  2000/01/11 17:12:15  AJW
	Uses newer version of AJWLib_File_*

	Revision 1.8  2000/01/09 12:19:10  AJW
	Added reading and stroing of font name and size

	Revision 1.7  1999/10/25 16:09:28  AJW
	Added centering of fields

	Revision 1.6  1999/10/24 23:31:30  AJW
	Added centred text fields

	Revision 1.5  1999/10/24 18:01:43  AJW
	Added extra field types

	Revision 1.4  1999/10/12 14:32:05  AJW
	Modified to use Config

	Revision 1.3  1999/10/11 20:55:56  AJW
	Modified to use Error2

	Revision 1.2  1999/10/10 20:54:06  AJW
	Modified to use Desk

	Revision 1.1  1999/09/27 15:32:44  AJW
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
#include "AJWLib.File.h"
#include "AJWLib.Font.h"
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
#include "Draw.h"


#define GRAPHICSDIR "<Roots$Dir>.Graphics"

static graphics graphicsdata;
static plotfn Graphics_PlotLine=NULL,Graphics_PlotRectangle=NULL,Graphics_PlotRectangleFilled=NULL;
static plottextfn Graphics_PlotText=NULL;

static unsigned int Graphics_RGBToPalette(char *str)
{
	unsigned int colour=(unsigned int)strtol(str,NULL,16);
	return (((colour & 0x00FF0000)>>8) | ((colour & 0x0000FF00)<<8) | ((colour & 0x000000FF)<<24));
}

void Graphics_StoreDimensionDetails(char *values[],int numvalues,int linenum)
{
	if (numvalues!=2) Desk_Error_Report(1,"Syntax error in dimensions file, line %d",linenum);
	if (!strcmp(values[0],"personwidth")) graphicsdata.personwidth=(int)strtol(values[1],NULL,10); /*Use a messages file?*/
	else if (!strcmp(values[0],"personheight")) graphicsdata.personheight=(int)strtol(values[1],NULL,10);
	else if (!strcmp(values[0],"gapheightabove")) graphicsdata.gapheightabove=(int)strtol(values[1],NULL,10);
	else if (!strcmp(values[0],"gapheightbelow")) graphicsdata.gapheightbelow=(int)strtol(values[1],NULL,10);
	else if (!strcmp(values[0],"gapwidth")) graphicsdata.gapwidth=(int)strtol(values[1],NULL,10);
	else if (!strcmp(values[0],"marriagewidth")) graphicsdata.marriagewidth=(int)strtol(values[1],NULL,10);
	else if (!strcmp(values[0],"secondmarriagegap")) graphicsdata.secondmarriagegap=(int)strtol(values[1],NULL,10);
	else if (!strcmp(values[0],"windowborder")) graphicsdata.windowborder=(int)strtol(values[1],NULL,10);
	else if (!strcmp(values[0],"gapheightunlinked")) graphicsdata.gapheightunlinked=(int)strtol(values[1],NULL,10);
}

void Graphics_StorePersonDetails(char *values[],int numvalues,int linenum)
{
	graphictype graphictype=graphictype_INVALID;
	if (!strcmp(values[0],"line")) graphictype=graphictype_LINE; /*Use a messages file?*/
	else if (!strcmp(values[0],"childline")) graphictype=graphictype_CHILDLINE;
	else if (!strcmp(values[0],"box")) graphictype=graphictype_RECTANGLE;
	else if (!strcmp(values[0],"filledbox")) graphictype=graphictype_FILLEDRECTANGLE;
	else if (!strcmp(values[0],"text")) graphictype=graphictype_TEXTLABEL;
	else if (!strcmp(values[0],"field")) graphictype=graphictype_FIELD;
	else if (!strcmp(values[0],"centredtext")) graphictype=graphictype_CENTREDTEXTLABEL;
	else if (!strcmp(values[0],"centredfield")) graphictype=graphictype_CENTREDFIELD;
	else Desk_Error_Report(1,"Syntax error in person file, line %d",linenum);
	switch (graphictype) {
		case graphictype_LINE:
		case graphictype_CHILDLINE:
			if (numvalues!=7) {
				Desk_Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x0=(int)strtol(values[1],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y0=(int)strtol(values[2],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x1=(int)strtol(values[3],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y1=(int)strtol(values[4],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.thickness=(int)strtol(values[5],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
			}
			break;
		case graphictype_RECTANGLE:
			if (numvalues!=7) {
				Desk_Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype_RECTANGLE;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x0=(int)strtol(values[1],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y0=(int)strtol(values[2],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x1=(int)strtol(values[3],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y1=(int)strtol(values[4],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.thickness=(int)strtol(values[5],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
			}
			break;
		case graphictype_FILLEDRECTANGLE:
			if (numvalues!=6) {
				Desk_Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype_FILLEDRECTANGLE;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x0=(int)strtol(values[1],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y0=(int)strtol(values[2],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x1=(int)strtol(values[3],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y1=(int)strtol(values[4],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[5]);
			}
			break;
		case graphictype_CENTREDTEXTLABEL:
		case graphictype_TEXTLABEL:
			if (numvalues!=8) {
				Desk_Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				int size;
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.x=(int)strtol(values[1],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.y=(int)strtol(values[2],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.bgcolour=Graphics_RGBToPalette(values[5]);
				strcpy(graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.text,values[7]);
				size=(int)strtol(values[3],NULL,10);
				strcpy(graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.fontname,values[6]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.size=size;
				Desk_Font2_ClaimFont(&(graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.font),values[6],16*size,16*size);
			}
			break;
		case graphictype_CENTREDFIELD:
		case graphictype_FIELD:
			if (numvalues!=8) {
				Desk_Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				personfieldtype field;
				int size;
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
				/*Check for flex error*/
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
				else Desk_Error_Report(1,"Syntax error in person file, line %d (unknown field type)",linenum); /*what is field?*/
				/*Error2 error?*/
				graphicsdata.personfields[field].plot=Desk_TRUE;
				graphicsdata.personfields[field].type=graphictype;
				graphicsdata.personfields[field].textproperties.x=(int)strtol(values[1],NULL,10);
				graphicsdata.personfields[field].textproperties.y=(int)strtol(values[2],NULL,10);
				graphicsdata.personfields[field].textproperties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.personfields[field].textproperties.bgcolour=Graphics_RGBToPalette(values[5]);
				size=(int)strtol(values[3],NULL,10);
				strcpy(graphicsdata.personfields[field].textproperties.fontname,values[6]);
				graphicsdata.personfields[field].textproperties.size=size;
				Desk_Font2_ClaimFont(&(graphicsdata.personfields[field].textproperties.font),values[6],16*size,16*size);
				/*errors - font not found*/
			}
			break;
	}
}

void Graphics_StoreMarriageDetails(char *values[],int numvalues,int linenum)
{
	graphictype graphictype;
	if (!strcmp(values[0],"line")) graphictype=graphictype_LINE; /*Use a messages file?*/
	else if (!strcmp(values[0],"childline")) graphictype=graphictype_CHILDLINE;
	else if (!strcmp(values[0],"siblingline")) graphictype=graphictype_SIBLINGLINE;
	else if (!strcmp(values[0],"box")) graphictype=graphictype_RECTANGLE;
	else if (!strcmp(values[0],"filledbox")) graphictype=graphictype_FILLEDRECTANGLE;
	else if (!strcmp(values[0],"text")) graphictype=graphictype_TEXTLABEL;
	else if (!strcmp(values[0],"field")) graphictype=graphictype_FIELD;
	else graphictype=graphictype_INVALID;
	switch (graphictype) {
		case graphictype_SIBLINGLINE:
			if (numvalues!=3) {
				Desk_Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				graphicsdata.siblinglinethickness=(int)strtol(values[1],NULL,10);
				graphicsdata.siblinglinecolour=Graphics_RGBToPalette(values[2]);
			}
			break;
		case graphictype_LINE:
		case graphictype_CHILDLINE:
			if (numvalues!=7) {
				Desk_Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].type=graphictype;
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x0=(int)strtol(values[1],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y0=(int)strtol(values[2],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x1=(int)strtol(values[3],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y1=(int)strtol(values[4],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.thickness=(int)strtol(values[5],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
			}
			break;
		case graphictype_RECTANGLE:
			if (numvalues!=7) {
				Desk_Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].type=graphictype_RECTANGLE;
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x0=(int)strtol(values[1],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y0=(int)strtol(values[2],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x1=(int)strtol(values[3],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y1=(int)strtol(values[4],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.thickness=(int)strtol(values[5],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[6]);
			}
			break;
		case graphictype_FILLEDRECTANGLE:
			if (numvalues!=6) {
				Desk_Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].type=graphictype_FILLEDRECTANGLE;
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x0=(int)strtol(values[1],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y0=(int)strtol(values[2],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.x1=(int)strtol(values[3],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.y1=(int)strtol(values[4],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[5]);
			}
			break;
		case graphictype_TEXTLABEL:
			if (numvalues!=8) {
				Desk_Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				int size;
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].type=graphictype_TEXTLABEL;
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.x=(int)strtol(values[1],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.y=(int)strtol(values[2],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.bgcolour=Graphics_RGBToPalette(values[5]);
				strcpy(graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.text,values[7]);
				size=(int)strtol(values[3],NULL,10);
				Desk_Font2_ClaimFont(&(graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.font),values[6],16*size,16*size);
			}
			break;
		case graphictype_FIELD:
			if (numvalues!=8) {
				Desk_Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				marriagefieldtype field;
				int size;
				AJWLib_Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				AJWLib_Str_LowerCase(values[7]);
				if (!strcmp(values[7],"place")) field=marriagefieldtype_PLACE;
				else if (!strcmp(values[7],"date")) field=marriagefieldtype_DATE;
				else Desk_Error_Report(1,"Syntax error in marriage file, line %d",linenum); /*what is field?*/
				graphicsdata.personfields[field].plot=Desk_TRUE;
				graphicsdata.personfields[field].textproperties.x=(int)strtol(values[1],NULL,10);
				graphicsdata.personfields[field].textproperties.y=(int)strtol(values[2],NULL,10);
				graphicsdata.personfields[field].textproperties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.personfields[field].textproperties.bgcolour=Graphics_RGBToPalette(values[5]);
				size=(int)strtol(values[3],NULL,10);
				strcpy(graphicsdata.personfields[field].textproperties.fontname,values[6]);
				graphicsdata.personfields[field].textproperties.size=size;
				Desk_Font2_ClaimFont(&(graphicsdata.personfields[field].textproperties.font),values[6],16*size,16*size);
				/*errors - font not found*/
			}
			break;
	}
}

void Graphics_ReadFile(char *filename,void (*decodefn)(char *values[],int numvalues,int linenum))
{
	FILE *file;
	char fullfilename[256];
	int ch=0,line=0;
	sprintf(fullfilename,"%s.%s.%s",GRAPHICSDIR,Config_GraphicsStyle(),filename);
	file=AJWLib_File_fopen(fullfilename,"r");
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

void Graphics_Init(void)
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
	graphicsdata.numpersonobjects=0;
	AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.person),1);
	graphicsdata.nummarriageobjects=0;
	AJWLib_Flex_Alloc((flex_ptr)&(graphicsdata.marriage),1);
	for (i=0;i<NUMPERSONFIELDS;i++) graphicsdata.personfields[i].plot=Desk_FALSE;
	for (i=0;i<NUMMARRIAGEFIELDS;i++) graphicsdata.marriagefields[i].plot=Desk_FALSE;
	Graphics_ReadFile("Person",Graphics_StorePersonDetails);
	Graphics_ReadFile("Dimensions",Graphics_StoreDimensionDetails);
	Graphics_ReadFile("Marriage",Graphics_StoreMarriageDetails);
}


void Graphics_PlotPerson(elementptr person,int x,int y,Desk_bool child,Desk_bool selected)
{
	int i;
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		int xcoord=0;
		switch (graphicsdata.person[i].type) {
			case graphictype_RECTANGLE:
				Graphics_PlotRectangle(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
				break;
			case graphictype_FILLEDRECTANGLE:
				Graphics_PlotRectangleFilled(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
				break;
			case graphictype_CHILDLINE:
				if (!child) break;
				/*A line that is only plotted if there is a child and child==Desk_FALSE ?*/
			case graphictype_LINE:
				Graphics_PlotLine(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,x+graphicsdata.person[i].details.linebox.x1,y+graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
				break;
			case graphictype_CENTREDTEXTLABEL:
/*Won't work for drawfile*/				xcoord=-AJWLib_Font_GetWidth(graphicsdata.person[i].details.textlabel.properties.font->handle,graphicsdata.person[i].details.textlabel.text)/2;
			case graphictype_TEXTLABEL:
				Graphics_PlotText(x+xcoord+graphicsdata.person[i].details.textlabel.properties.x,y+graphicsdata.person[i].details.textlabel.properties.y,graphicsdata.person[i].details.textlabel.properties.font->handle,graphicsdata.person[i].details.textlabel.properties.fontname,graphicsdata.person[i].details.textlabel.properties.size,graphicsdata.person[i].details.textlabel.properties.bgcolour,graphicsdata.person[i].details.textlabel.properties.colour,graphicsdata.person[i].details.textlabel.text);
				break;
		}
	}
	for (i=0;i<NUMPERSONFIELDS;i++) {
		if (graphicsdata.personfields[i].plot) {
			char fieldtext[256]=""; /*what is max field length?*/
			int xcoord=0;
			switch (i) {
				case personfieldtype_SURNAME:
					strcat(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_FORENAME:
					strcat(fieldtext,Database_GetPersonData(person)->forename);
					break;
				case personfieldtype_MIDDLENAMES:
					strcat(fieldtext,Database_GetPersonData(person)->middlenames);
					break;
				case personfieldtype_TITLEDNAME:
					strcat(fieldtext,Database_GetTitledName(person));
					break;
				case personfieldtype_NAME:
					strcat(fieldtext,Database_GetName(person));
					break;
				case personfieldtype_TITLEDFULLNAME:
					strcat(fieldtext,Database_GetTitledFullName(person));
					break;
				case personfieldtype_FULLNAME:
					strcat(fieldtext,Database_GetFullName(person));
					break;
				case personfieldtype_TITLE:
					strcat(fieldtext,Database_GetPersonData(person)->title);
					break;
/*				case personfieldtype_SEX:
					strcat(fieldtext,Database_GetPersonData(person)->sex);
					break;
				case personfieldtype_DOB:
					strcat(fieldtext,Database_GetPersonData(person)->dob);
					break;
				case personfieldtype_DOD:
					strcat(fieldtext,Database_GetPersonData(person)->dod);
					break;
*/				case personfieldtype_BIRTHPLACE:
					strcat(fieldtext,Database_GetPersonData(person)->placeofbirth);
					break;
				case personfieldtype_USER1:
					strcat(fieldtext,Database_GetPersonData(person)->userdata[0]);
					break;
				case personfieldtype_USER2:
					strcat(fieldtext,Database_GetPersonData(person)->userdata[1]);
					break;
				case personfieldtype_USER3:
					strcat(fieldtext,Database_GetPersonData(person)->userdata[2]);
					break;
				default:
					strcat(fieldtext,"Unimplemented");
			}
/*Doesn't work for drawfile*/			if (graphicsdata.personfields[i].type==graphictype_CENTREDFIELD) xcoord=-AJWLib_Font_GetWidth(graphicsdata.personfields[i].textproperties.font->handle,fieldtext)/2;
			Graphics_PlotText(x+xcoord+graphicsdata.personfields[i].textproperties.x,y+graphicsdata.personfields[i].textproperties.y,graphicsdata.personfields[i].textproperties.font->handle,graphicsdata.personfields[i].textproperties.fontname,graphicsdata.personfields[i].textproperties.size,graphicsdata.personfields[i].textproperties.bgcolour,graphicsdata.personfields[i].textproperties.colour,fieldtext);
		}
	}
	if (selected) Draw_EORRectangleFilled(x,y,Graphics_PersonWidth(),Graphics_PersonHeight(),EORCOLOUR);
}

void Graphics_PlotMarriage(int x,int y,elementptr marriage,Desk_bool childline,Desk_bool selected)
{
	int i;
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		switch (graphicsdata.marriage[i].type) {
			case graphictype_RECTANGLE:
				Graphics_PlotRectangle(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
				break;
			case graphictype_FILLEDRECTANGLE:
				Graphics_PlotRectangleFilled(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
				break;
			case graphictype_CHILDLINE:
				if (!childline) break;
			case graphictype_LINE:
				Graphics_PlotLine(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,x+graphicsdata.marriage[i].details.linebox.x1,y+graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
				break;
			case graphictype_TEXTLABEL:
				Graphics_PlotText(x+graphicsdata.marriage[i].details.textlabel.properties.x,y+graphicsdata.marriage[i].details.textlabel.properties.y,graphicsdata.marriage[i].details.textlabel.properties.font->handle,graphicsdata.marriage[i].details.textlabel.properties.fontname,graphicsdata.marriage[i].details.textlabel.properties.size,graphicsdata.marriage[i].details.textlabel.properties.bgcolour,graphicsdata.marriage[i].details.textlabel.properties.colour,graphicsdata.marriage[i].details.textlabel.text);
				break;
		}
	}
	for (i=0;i<NUMMARRIAGEFIELDS;i++) {
		if (graphicsdata.marriagefields[i].plot) {
			char fieldtext[256]=""; /*what is max field length?*/
			switch (i) {
				case marriagefieldtype_PLACE:
					strcat(fieldtext,Database_GetMarriageData(marriage)->place);
					break;
/*				case marriagefieldtype_DATE:
					strcat(fieldtext,Database_GetMarriageData(marriage)->place);
					break;
*/				default:
					strcat(fieldtext,"Unimplemented");
			}
			Graphics_PlotText(x+graphicsdata.marriagefields[i].textproperties.x,y+graphicsdata.marriagefields[i].textproperties.y,graphicsdata.marriagefields[i].textproperties.font->handle,graphicsdata.marriagefields[i].textproperties.fontname,graphicsdata.marriagefields[i].textproperties.size,graphicsdata.marriagefields[i].textproperties.bgcolour,graphicsdata.marriagefields[i].textproperties.colour,fieldtext);
		}
	}
	if (selected) Draw_EORRectangleFilled(x,y,Graphics_MarriageWidth(),Graphics_PersonHeight(),EORCOLOUR);
}

void Graphics_PlotChildren(int leftx,int rightx,int y)
{
	Graphics_PlotLine(leftx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),rightx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),graphicsdata.siblinglinethickness,graphicsdata.siblinglinecolour);
}

void Graphics_Redraw(layout *layout,int originx,int originy,Desk_wimp_box *cliprect,Desk_bool plotselection,plotfn plotline,plotfn plotrect,plotfn plotrectfilled,plottextfn plottext)
{
	int i;
	/*use the clip rect*/
	Graphics_PlotLine=plotline;
	Graphics_PlotRectangle=plotrect;
	Graphics_PlotRectangleFilled=plotrectfilled;
	Graphics_PlotText=plottext;
	for (i=0;i<layout->numchildren;i++) {
		Graphics_PlotChildren(originx+layout->children[i].leftx,originx+layout->children[i].rightx,originy+layout->children[i].y);
	}
	for (i=layout->nummarriages-1;i>=0;i--) {
		Graphics_PlotMarriage(originx+layout->marriage[i].x,originy+layout->marriage[i].y,layout->marriage[i].marriage,layout->marriage[i].childline,plotselection ? layout->marriage[i].selected : Desk_FALSE);
	}
	for (i=layout->numpeople-1;i>=0;i--) {
		Graphics_PlotPerson(layout->person[i].person,originx+layout->person[i].x,originy+layout->person[i].y,layout->person[i].child,plotselection ? layout->person[i].selected : Desk_FALSE);
	}
}

