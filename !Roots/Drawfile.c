/*
	FT - Draw
	© Alex Waugh 1999

	$Log: Drawfile.c,v $
	Revision 1.3  2000/01/07 17:52:27  AJW
	Got Lines and boxes outputting to a drawfile correctly

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
int xoffset=0,yoffset=0; /*Euch!*/

void Draw_PlotRectangle(drawfile_diagram **drawfile,int x,int y,int width,int height,int linethickness,unsigned int colour,Desk_bool filled)
{
	int *object;
	const int sizeofpath=96;
	int currentsize=AJWLib_Flex_Size((flex_ptr)drawfile);
	AJWLib_Flex_Extend((flex_ptr)drawfile,currentsize+sizeofpath);
	object=(int *)((char*)(*drawfile)+currentsize);
	object[0]=2; /*Path object*/
	object[1]=sizeofpath;
	object[2]=(x+xoffset)<<8; /*Boundingbox*/
	object[3]=(y+yoffset)<<8;
	object[4]=(x+xoffset+width)<<8;
	object[5]=(y+yoffset+height)<<8;
	object[6]=filled ? colour : -1; /*Fill colour*/
	object[7]=filled ? -1 : colour; /*Line colour*/
	object[8]=linethickness;
	object[9]=0; /*? Path style*/
	object[10]=2; /*Move*/
	object[11]=(x+xoffset)<<8;
	object[12]=(y+yoffset)<<8;
	object[13]=8; /*Line*/
	object[14]=(x+xoffset+width)<<8;
	object[15]=(y+yoffset)<<8;
	object[16]=8; /*Line*/
	object[17]=(x+xoffset+width)<<8;
	object[18]=(y+yoffset+height)<<8;
	object[19]=8; /*Line*/
	object[20]=(x+xoffset)<<8;
	object[21]=(y+yoffset+height)<<8;
	object[22]=5; /*Close subpath*/
	object[23]=0; /*End path*/
}


void Draw_PlotLine(drawfile_diagram **drawfile,int minx,int miny,int maxx,int maxy,int linethickness,unsigned int colour)
{
	int *object;
	const int sizeofpath=68;
	int currentsize=AJWLib_Flex_Size((flex_ptr)drawfile);
	AJWLib_Flex_Extend((flex_ptr)drawfile,currentsize+sizeofpath);
	object=(int *)((char*)(*drawfile)+currentsize);
	object[0]=2; /*Path object*/
	object[1]=sizeofpath;
	object[2]=(minx+xoffset)<<8; /*Boundingbox*/
	object[3]=(miny+yoffset)<<8;
	object[4]=(maxx+xoffset)<<8;
	object[5]=(maxy+yoffset)<<8;
	object[6]=-1; /*Fill colour*/
	object[7]=colour;
	object[8]=linethickness;
	object[9]=0; /*? Path style*/
	object[10]=2; /*Move*/
	object[11]=(minx+xoffset)<<8;
	object[12]=(miny+yoffset)<<8;
	object[13]=8; /*Line*/
	object[14]=(maxx+xoffset)<<8;
	object[15]=(maxy+yoffset)<<8;
	object[16]=0; /*End path*/
}

void Draw_PlotPerson(drawfile_diagram **drawfile,int x,int y,Desk_bool child)
{
	int i;
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		switch (graphicsdata.person[i].type) {
			case graphictype_RECTANGLE:
				Draw_PlotRectangle(drawfile,x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour,Desk_FALSE);
				break;
			case graphictype_FILLEDRECTANGLE:
				Draw_PlotRectangle(drawfile,x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour,Desk_TRUE);
				break;
			case graphictype_CHILDLINE:
				if (!child) break;
				/*A line that is only plotted if there is a child and child==Desk_FALSE ?*/
			case graphictype_LINE:
				Draw_PlotLine(drawfile,x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,x+graphicsdata.person[i].details.linebox.x1,y+graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,graphicsdata.person[i].details.linebox.colour);
				break;
/*			case graphictype_TEXTLABEL:
				Desk_Font_SetFont(graphicsdata.person[i].details.textlabel.properties.font->handle);
				Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.person[i].details.textlabel.properties.bgcolour,graphicsdata.person[i].details.textlabel.properties.colour,14);
				Desk_Font_Paint(graphicsdata.person[i].details.textlabel.text,Desk_font_plot_OSCOORS,x+graphicsdata.person[i].details.textlabel.properties.x,y+graphicsdata.person[i].details.textlabel.properties.y);
				break;
*/		}
	}
/*	for (i=0;i<NUMPERSONFIELDS;i++) {
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
	}*/
}

/*char fontarraythingy
void Draw_PlotText(drawfile_diagram **drawfile,int x,int y,char *font,char *text,unsigned int bgcolour, unsigned int fgcolour)
{
	d
}
*/
void Draw_PlotMarriage(drawfile_diagram **drawfile,int x,int y,elementptr marriage,Desk_bool childline)
{
	int i;
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		switch (graphicsdata.marriage[i].type) {
			case graphictype_RECTANGLE:
				Draw_PlotRectangle(drawfile,x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour,Desk_FALSE);
				break;
			case graphictype_FILLEDRECTANGLE:
				Draw_PlotRectangle(drawfile,x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour,Desk_TRUE);
				break;
			case graphictype_CHILDLINE:
				if (!childline) break;
			case graphictype_LINE:
				Draw_PlotLine(drawfile,x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,x+graphicsdata.marriage[i].details.linebox.x1,y+graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,graphicsdata.marriage[i].details.linebox.colour);
				break;
/*			case graphictype_TEXTLABEL:
				Desk_Font_SetFont(graphicsdata.marriage[i].details.textlabel.properties.font->handle);
				Desk_SWI(4,0,SWI_ColourTrans_SetFontColours,0,graphicsdata.marriage[i].details.textlabel.properties.bgcolour,graphicsdata.marriage[i].details.textlabel.properties.colour,14);
				Desk_Font_Paint(graphicsdata.marriage[i].details.textlabel.text,Desk_font_plot_OSCOORS,x+graphicsdata.marriage[i].details.textlabel.properties.x,y+graphicsdata.marriage[i].details.textlabel.properties.y);
				break;
*/		}
	}
/*	for (i=0;i<NUMMARRIAGEFIELDS;i++) {
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
	}*/
}

void Draw_PlotChildren(drawfile_diagram **drawfile,int leftx,int rightx,int y)
{
	Draw_PlotLine(drawfile,leftx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),rightx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),graphicsdata.siblinglinethickness,graphicsdata.siblinglinecolour);
}

void Draw_Save(char *filename,layout *layout)
{
	int i=0;
	drawfile_diagram *drawfile;
	Desk_wimp_rect box;
	AJWLib_Flex_Alloc((flex_ptr)&drawfile,40);
	strcpy(drawfile->tag,"Draw");
	drawfile->major_version=201;
	drawfile->minor_version=0;
	strcpy(drawfile->source,"FT          "); /*Padded with spaces*/
	box=Layout_FindExtent(layout,Desk_FALSE);
	xoffset=-box.min.x; /*Not global? add a bit more to give a gap?*/
	yoffset=-box.min.y;
	drawfile->bbox.min.x=0;
	drawfile->bbox.min.y=0;
	drawfile->bbox.max.x=(box.max.x+xoffset)<<8;
	drawfile->bbox.max.y=(box.max.y+yoffset)<<8;
	for (i=0;i<layout->numchildren;i++) {
		Draw_PlotChildren(&drawfile,layout->children[i].leftx,layout->children[i].rightx,layout->children[i].y);
	}
		for (i=layout->nummarriages-1;i>=0;i--) {
			Draw_PlotMarriage(&drawfile,layout->marriage[i].x,layout->marriage[i].y,layout->marriage[i].marriage,layout->marriage[i].childline);
		}
		for (i=layout->numpeople-1;i>=0;i--) {
			Draw_PlotPerson(&drawfile,layout->person[i].x,layout->person[i].y,layout->person[i].child);
		}
	Desk_File_SaveMemory2(filename,drawfile,AJWLib_Flex_Size((flex_ptr)&drawfile),Desk_filetype_DRAWFILE);
	AJWLib_Flex_Free((flex_ptr)&drawfile);
}
