/*
	FT - Draw
	© Alex Waugh 1999

	$Log: Drawfile.c,v $
	Revision 1.2  2000/01/06 17:17:47  AJW
	Saves Child lines (but coords currently wrong)

	Revision 1.1  1999/10/24 13:36:22  AJW
	Initial revision


*/

/*	Includes  */

#include "Desk.Window.h"
#include "Desk.Error2.h"
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
#include "Desk.Screen.h"
#include "Desk.GFX.h"
#include "Desk.Font2.h"
#include "Desk.ColourTran.h"

#include "AJWLib.Window.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Assert.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Icon.h"
#include "AJWLib.Flex.h"
#include "AJWLib.Font.h"
#include "AJWLib.Str.h"
#include "AJWLib.Draw.h"
#include "AJWLib.DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "Modules.h"
#include "GConfig.h"
#include "Config.h"
#include "Layout.h"

extern graphics graphicsdata;

/*void Draw_PlotPerson(elementptr person,int x,int y,Desk_bool child,Desk_bool selected)
{
	int i;
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		switch (graphicsdata.person[i].type) {
			case graphictype_RECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0or 1<<8 to use ECFs ?,0);
				AJWLib_Draw_PlotRectangle(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_FILLEDRECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0or 1<<8 to use ECFs ?,0);
				AJWLib_Draw_PlotRectangleFilled(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,&matrix);
				break;
			case graphictype_CHILDLINE:
				if (!child) break;
				A line that is only plotted if there is a child and child==Desk_FALSE ?
			case graphictype_LINE:
				Desk_ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0or 1<<8 to use ECFs ?,0);
				AJWLib_Draw_PlotLine(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,x+graphicsdata.person[i].details.linebox.x1,y+graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_TEXTLABEL:
				Desk_Font_SetFont(graphicsdata.person[i].details.textlabel.properties.font->handle);
				Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.person[i].details.textlabel.properties.bgcolour,graphicsdata.person[i].details.textlabel.properties.colour,14);
				Desk_Font_Paint(graphicsdata.person[i].details.textlabel.text,Desk_font_plot_OSCOORS,x+graphicsdata.person[i].details.textlabel.properties.x,y+graphicsdata.person[i].details.textlabel.properties.y);
				break;
		}
	}
	for (i=0;i<NUMPERSONFIELDS;i++) {
		if (graphicsdata.personfields[i].plot) {
			char fieldtext[256]=""; what is max field length?
			switch (i) {
			A centered field?
				case personfieldtype_SURNAME:
					strcpy(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_FORENAME:
					strcpy(fieldtext,Database_GetPersonData(person)->forename);
					break;
				case personfieldtype_MIDDLENAMES:
					strcpy(fieldtext,Database_GetPersonData(person)->middlenames);
					break;
				case personfieldtype_NAME:
					strcpy(fieldtext,Database_GetPersonData(person)->forename);
					strcat(fieldtext," ");
					strcat(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_FULLNAME:
					strcpy(fieldtext,Database_GetPersonData(person)->forename);
					strcat(fieldtext," ");
					strcat(fieldtext,Database_GetPersonData(person)->middlenames);
					strcat(fieldtext," ");
					strcat(fieldtext,Database_GetPersonData(person)->surname);
					break;
				case personfieldtype_TITLE:
					strcpy(fieldtext,Database_GetPersonData(person)->title);
					break;
			}
			Desk_Font_SetFont(graphicsdata.personfields[i].textproperties.font->handle);
			Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.personfields[i].textproperties.bgcolour,graphicsdata.personfields[i].textproperties.colour,14));
			Desk_Font_Paint(fieldtext,Desk_font_plot_OSCOORS,x+graphicsdata.personfields[i].textproperties.x,y+graphicsdata.personfields[i].textproperties.y);
		}
	}
	if (selected) {
		Desk_ColourTrans_SetGCOL(EORCOLOUR,0,3);
		AJWLib_Draw_PlotRectangleFilled(x,y,Graphics_PersonWidth(),Graphics_PersonHeight(),&matrix);

	}
}

void Draw_PlotMarriage(int x,int y,elementptr marriage,Desk_bool childline,Desk_bool selected)
{
	int i;
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		switch (graphicsdata.marriage[i].type) {
			case graphictype_RECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0or 1<<8 to use ECFs ?,0);
				AJWLib_Draw_PlotRectangle(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_FILLEDRECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0or 1<<8 to use ECFs ?,0);
				AJWLib_Draw_PlotRectangleFilled(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,&matrix);
				break;
			case graphictype_CHILDLINE:
				if (!childline) break;
			case graphictype_LINE:
				Desk_ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0or 1<<8 to use ECFs ?,0);
				AJWLib_Draw_PlotLine(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,x+graphicsdata.marriage[i].details.linebox.x1,y+graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_TEXTLABEL:
				Desk_Font_SetFont(graphicsdata.marriage[i].details.textlabel.properties.font->handle);
				Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.marriage[i].details.textlabel.properties.bgcolour,graphicsdata.marriage[i].details.textlabel.properties.colour,14);
				Desk_Font_Paint(graphicsdata.marriage[i].details.textlabel.text,Desk_font_plot_OSCOORS,x+graphicsdata.marriage[i].details.textlabel.properties.x,y+graphicsdata.marriage[i].details.textlabel.properties.y);
				break;
		}
	}
	for (i=0;i<NUMMARRIAGEFIELDS;i++) {
		if (graphicsdata.marriagefields[i].plot) {
			char fieldtext[256]=""; what is max field length?
			switch (i) {
				case marriagefieldtype_PLACE:
					strcpy(fieldtext,Database_GetMarriageData(marriage)->place);
					break;
			}
			Desk_Font_SetFont(graphicsdata.marriagefields[i].textproperties.font->handle);
			Desk_Error2_CheckOS(Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.marriagefields[i].textproperties.bgcolour,graphicsdata.marriagefields[i].textproperties.colour,14));
			Desk_Font_Paint(fieldtext,Desk_font_plot_OSCOORS,x+graphicsdata.marriagefields[i].textproperties.x,y+graphicsdata.marriagefields[i].textproperties.y);
		}
	}
	if (selected) {
		Desk_ColourTrans_SetGCOL(EORCOLOUR,0,3);
		AJWLib_Draw_PlotRectangleFilled(x,y,Graphics_MarriageWidth(),Graphics_PersonHeight(),&matrix);

	}
}
void Draw_CreateRectanglePath(int x,int y,int width,int height,int *block)
{
	block[0]=2;
	block[1]=x;
	block[2]=y;
	block[3]=8;
	block[4]=x+width;
	block[5]=y;
	block[6]=8;
	block[7]=x+width;
	block[8]=y+height;
	block[9]=8;
	block[10]=x;
	block[11]=y+height;
	block[12]=5;
	block[13]=0;
	block[14]=1;
}

*/
void Draw_PlotLine(drawfile_diagram **drawfile,int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour)
{
	int *object;
	const int sizeofpath=68;
	int currentsize=AJWLib_Flex_Size((flex_ptr)drawfile);
	AJWLib_Flex_Extend((flex_ptr)drawfile,currentsize+sizeofpath);
	object=(int *)((char*)(*drawfile)+currentsize);
	object[0]=2; /*Path object*/
	object[1]=sizeofpath;
	object[2]=minx; /*Boundingbox*/
	object[2]=miny;
	object[2]=maxx;
	object[2]=maxy;
	object[6]=-1; /*Fill colour*/
	object[7]=colour;
	object[8]=linethickness;
	object[9]=0; /*? Path style*/
	object[10]=2; /*Move*/
	object[11]=minx;
	object[12]=miny;
	object[13]=8; /*Line*/
	object[14]=maxx;
	object[15]=maxy;
	object[16]=0; /*End path*/
}

void Draw_PlotChildren(drawfile_diagram **drawfile,int leftx,int rightx,int y)
{
	Draw_PlotLine(drawfile,leftx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),rightx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),graphicsdata.siblinglinethickness,graphicsdata.siblinglinecolour);
}

void Draw_Save(char *filename,layout *layout)
{
	int i=0;
	drawfile_diagram *drawfile;
	Desk_SWI(0,0,0x107);
	AJWLib_Flex_Alloc((flex_ptr)&drawfile,40);
	strcpy(drawfile->tag,"Draw");
	drawfile->major_version=201;
	drawfile->minor_version=0;
	strcpy(drawfile->source,"FT          "); /*Padded with spaces*/
	drawfile->bbox.min.x=-1000;
	drawfile->bbox.min.y=-1000;
	drawfile->bbox.max.x=1000;
	drawfile->bbox.max.y=1000;
	for (i=0;i<layout->numchildren;i++) {
		Draw_PlotChildren(&drawfile,layout->children[i].leftx,layout->children[i].rightx,layout->children[i].y);
	}
/*		for (i=layout->nummarriages-1;i>=0;i--) {
			Draw_PlotMarriage(drawfile,Layout->marriage[i].x,layout->marriage[i].y,layout->marriage[i].marriage,layout->marriage[i].childline);
		}
		for (i=windowdata->layout->numpeople-1;i>=0;i--) {
			Graphics_PlotPerson(drawfile,layout->person[i].person,layout->person[i].x,layout->person[i].y,layout->person[i].child);
		}
*/	Desk_File_SaveMemory2(filename,drawfile,AJWLib_Flex_Size((flex_ptr)&drawfile),Desk_filetype_DRAWFILE);
	AJWLib_Flex_Free((flex_ptr)&drawfile);
}
