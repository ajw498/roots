/*
	FT - Graphics Configuration
	© Alex Waugh 1999

	$Log: Graphics.c,v $
	Revision 1.1  1999/09/27 15:32:44  AJW
	Initial revision


*/

/*	Includes  */

#include "DeskLib:Window.h"
#include "DeskLib:Error.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:Handler.h"
#include "DeskLib:Hourglass.h"
#include "DeskLib:Icon.h"
#include "DeskLib:Menu.h"
#include "DeskLib:Msgs.h"
#include "DeskLib:Drag.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Template.h"
#include "DeskLib:File.h"
#include "DeskLib:Filing.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:GFX.h"
#include "DeskLib:Font2.h"
#include "DeskLib:ColourTran.h"

#include "AJWLib:Window.h"
#include "AJWLib:Menu.h"
#include "AJWLib:Msgs.h"
#include "AJWLib:Misc.h"
#include "AJWLib:Handler.h"
#include "AJWLib:Icon.h"
#include "AJWLib:Error.h"
#include "AJWLib:Flex.h"
#include "AJWLib:Str.h"
#include "AJWLib:Save.h"
#include "AJWLib:Draw.h"
#include "AJWLib:DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "GConfig.h"
#include "Layout.h"

graphics graphicsdata;

static unsigned int Graphics_RGBToPalette(char *str)
{
	unsigned int colour=(unsigned int)strtol(str,NULL,16);
	return (((colour & 0x00FF0000)>>8) | ((colour & 0x0000FF00)<<8) | ((colour & 0x000000FF)<<24));
}

void Graphics_StoreDimensionDetails(char *values[],int numvalues,int linenum)
{
	if (numvalues!=2) Error_Report(1,"Syntax error in dimensions file, line %d",linenum);
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
	graphictype graphictype;
	if (!strcmp(values[0],"line")) graphictype=graphictype_LINE; /*Use a messages file?*/
	else if (!strcmp(values[0],"childline")) graphictype=graphictype_CHILDLINE;
	else if (!strcmp(values[0],"box")) graphictype=graphictype_RECTANGLE;
	else if (!strcmp(values[0],"filledbox")) graphictype=graphictype_FILLEDRECTANGLE;
	else if (!strcmp(values[0],"text")) graphictype=graphictype_TEXTLABEL;
	else if (!strcmp(values[0],"field")) graphictype=graphictype_FIELD;
	else graphictype=graphictype_INVALID;
	switch (graphictype) {
		case graphictype_LINE:
		case graphictype_CHILDLINE:
			if (numvalues!=7) {
				Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
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
				Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
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
				Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype_FILLEDRECTANGLE;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x0=(int)strtol(values[1],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y0=(int)strtol(values[2],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.x1=(int)strtol(values[3],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.y1=(int)strtol(values[4],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.linebox.colour=Graphics_RGBToPalette(values[5]);
			}
			break;
		case graphictype_TEXTLABEL:
			if (numvalues!=8) {
				Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				int size;
				Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.person[graphicsdata.numpersonobjects-1].type=graphictype_TEXTLABEL;
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.x=(int)strtol(values[1],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.y=(int)strtol(values[2],NULL,10);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.bgcolour=Graphics_RGBToPalette(values[5]);
				strcpy(graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.text,values[7]);
				size=(int)strtol(values[3],NULL,10);
				Font2_ClaimFont(&(graphicsdata.person[graphicsdata.numpersonobjects-1].details.textlabel.properties.font),values[6],16*size,16*size);
			}
			break;
		case graphictype_FIELD:
			if (numvalues!=8) {
				Error_Report(1,"Syntax error in person file, line %d",linenum);
			} else {
				personfieldtype field;
				int size;
				Flex_Extend((flex_ptr)&(graphicsdata.person),((graphicsdata.numpersonobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				Str_LowerCase(values[7]);
				if (!strcmp(values[7],"surname")) field=personfieldtype_SURNAME;
				else if (!strcmp(values[7],"forename")) field=personfieldtype_FORENAME;
				else if (!strcmp(values[7],"name")) field=personfieldtype_NAME;
				else if (!strcmp(values[7],"middlenames")) field=personfieldtype_MIDDLENAMES;
				else if (!strcmp(values[7],"fullname")) field=personfieldtype_FULLNAME;
				else if (!strcmp(values[7],"title")) field=personfieldtype_TITLE;
				else Error_Report(1,"Syntax error in person file, line %d",linenum); /*what is field?*/
				graphicsdata.personfields[field].plot=TRUE;
				graphicsdata.personfields[field].textproperties.x=(int)strtol(values[1],NULL,10);
				graphicsdata.personfields[field].textproperties.y=(int)strtol(values[2],NULL,10);
				graphicsdata.personfields[field].textproperties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.personfields[field].textproperties.bgcolour=Graphics_RGBToPalette(values[5]);
				size=(int)strtol(values[3],NULL,10);
				Font2_ClaimFont(&(graphicsdata.personfields[field].textproperties.font),values[6],16*size,16*size);
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
				Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				graphicsdata.siblinglinethickness=(int)strtol(values[1],NULL,10);
				graphicsdata.siblinglinecolour=Graphics_RGBToPalette(values[2]);
			}
			break;
		case graphictype_LINE:
		case graphictype_CHILDLINE:
			if (numvalues!=7) {
				Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
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
				Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
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
				Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
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
				Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				int size;
				Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].type=graphictype_TEXTLABEL;
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.x=(int)strtol(values[1],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.y=(int)strtol(values[2],NULL,10);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.bgcolour=Graphics_RGBToPalette(values[5]);
				strcpy(graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.text,values[7]);
				size=(int)strtol(values[3],NULL,10);
				Font2_ClaimFont(&(graphicsdata.marriage[graphicsdata.nummarriageobjects-1].details.textlabel.properties.font),values[6],16*size,16*size);
			}
			break;
		case graphictype_FIELD:
			if (numvalues!=8) {
				Error_Report(1,"Syntax error in marriage file, line %d",linenum);
			} else {
				marriagefieldtype field;
				int size;
				Flex_Extend((flex_ptr)&(graphicsdata.marriage),((graphicsdata.nummarriageobjects++)+1)*sizeof(object));
				/*Check for flex error*/
				Str_LowerCase(values[7]);
				if (!strcmp(values[7],"place")) field=marriagefieldtype_PLACE;
				else if (!strcmp(values[7],"date")) field=marriagefieldtype_DATE;
				else Error_Report(1,"Syntax error in marriage file, line %d",linenum); /*what is field?*/
				graphicsdata.personfields[field].plot=TRUE;
				graphicsdata.personfields[field].textproperties.x=(int)strtol(values[1],NULL,10);
				graphicsdata.personfields[field].textproperties.y=(int)strtol(values[2],NULL,10);
				graphicsdata.personfields[field].textproperties.colour=Graphics_RGBToPalette(values[4]);
				graphicsdata.personfields[field].textproperties.bgcolour=Graphics_RGBToPalette(values[5]);
				size=(int)strtol(values[3],NULL,10);
				Font2_ClaimFont(&(graphicsdata.personfields[field].textproperties.font),values[6],16*size,16*size);
				/*errors - font not found*/
			}
			break;
	}
}

void Graphics_ReadFile(char *filename,void (*decodefn)(char *values[],int numvalues,int linenum))
{
	FILE *file;
	int ch=0,line=0;
	file=fopen(filename,"r");
	if (file==NULL) { Error_Report(1,"File error"); /*Proper error*/ return; }
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
			Str_LowerCase(values[0]);
			(*decodefn)(values,j,line);
		}
	}
	fclose(file);
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

void Graphics_Init2(void)
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
	Flex_Alloc((flex_ptr)&(graphicsdata.person),1); /*Errors*/
	graphicsdata.nummarriageobjects=0;
	Flex_Alloc((flex_ptr)&(graphicsdata.marriage),1);
	for (i=0;i<NUMPERSONFIELDS;i++) graphicsdata.personfields[i].plot=FALSE;
	for (i=0;i<NUMMARRIAGEFIELDS;i++) graphicsdata.marriagefields[i].plot=FALSE;
	Graphics_ReadFile("Graphics.Person",Graphics_StorePersonDetails);
	Graphics_ReadFile("Graphics.Dimensions",Graphics_StoreDimensionDetails);
	Graphics_ReadFile("Graphics.Marriage",Graphics_StoreMarriageDetails);
}


