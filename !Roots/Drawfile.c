/*
	FT - Draw
	© Alex Waugh 1999

	$Log: Drawfile.c,v $
	Revision 1.1  1999/10/24 13:36:22  AJW
	Initial revision


*/

void Graphics_PlotPerson(elementptr person,int x,int y,Desk_bool child,Desk_bool selected)
{
	int i;
	for (i=0;i<graphicsdata.numpersonobjects;i++) {
		switch (graphicsdata.person[i].type) {
			case graphictype_RECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotRectangle(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,graphicsdata.person[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_FILLEDRECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotRectangleFilled(x+graphicsdata.person[i].details.linebox.x0,y+graphicsdata.person[i].details.linebox.y0,graphicsdata.person[i].details.linebox.x1,graphicsdata.person[i].details.linebox.y1,&matrix);
				break;
			case graphictype_CHILDLINE:
				if (!child) break;
				/*A line that is only plotted if there is a child and child==Desk_FALSE ?*/
			case graphictype_LINE:
				Desk_ColourTrans_SetGCOL(graphicsdata.person[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
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
			char fieldtext[256]=""; /*what is max field length?*/
			switch (i) {
			/*A centered field?*/
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

void Graphics_PlotMarriage(int x,int y,elementptr marriage,Desk_bool childline,Desk_bool selected)
{
	int i;
	for (i=0;i<graphicsdata.nummarriageobjects;i++) {
		switch (graphicsdata.marriage[i].type) {
			case graphictype_RECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotRectangle(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,graphicsdata.marriage[i].details.linebox.thickness,&matrix);
				break;
			case graphictype_FILLEDRECTANGLE:
				Desk_ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
				AJWLib_Draw_PlotRectangleFilled(x+graphicsdata.marriage[i].details.linebox.x0,y+graphicsdata.marriage[i].details.linebox.y0,graphicsdata.marriage[i].details.linebox.x1,graphicsdata.marriage[i].details.linebox.y1,&matrix);
				break;
			case graphictype_CHILDLINE:
				if (!childline) break;
			case graphictype_LINE:
				Desk_ColourTrans_SetGCOL(graphicsdata.marriage[i].details.linebox.colour,0/*or 1<<8 to use ECFs ?*/,0);
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
			char fieldtext[256]=""; /*what is max field length?*/
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

void Graphics_PlotChildren(int leftx,int rightx,int y)
{
	Desk_ColourTrans_SetGCOL(graphicsdata.siblinglinecolour,0/*or 1<<8 to use ECFs ?*/,0);
	AJWLib_Draw_PlotLine(leftx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),rightx,y+Graphics_PersonHeight()+Graphics_GapHeightAbove(),graphicsdata.siblinglinethickness,&matrix);
}

static Desk_bool Graphics_Redraw(Desk_event_pollblock *block,windowdata *windowdata)
{
	Desk_window_redrawblock blk;
	Desk_bool more=Desk_FALSE;
	blk.window=block->data.openblock.window;
	Desk_Wimp_RedrawWindow(&blk,&more);
	while (more) {
		int i=0;
#if DEBUG
Desk_ColourTrans_SetGCOL(0x00000000,0,0);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
Desk_ColourTrans_SetGCOL(0xFF000000,0,0);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+1000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-1000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
Desk_ColourTrans_SetGCOL(0x0000FF00,0,0);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+2000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-2000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
Desk_ColourTrans_SetGCOL(0x00FF0000,0,0);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x+3000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
AJWLib_Draw_PlotRectangleFilled(blk.rect.min.x-blk.scroll.x-3000,blk.rect.max.y-blk.scroll.y-10000,10,20000,&matrix);
#endif
		for (i=0;i<windowdata->layout->numchildren;i++) {
			Graphics_PlotChildren(blk.rect.min.x-blk.scroll.x+windowdata->layout->children[i].leftx,blk.rect.min.x-blk.scroll.x+windowdata->layout->children[i].rightx,blk.rect.max.y-blk.scroll.y+windowdata->layout->children[i].y);
		}
		for (i=windowdata->layout->nummarriages-1;i>=0;i--) {
			Graphics_PlotMarriage(blk.rect.min.x-blk.scroll.x+windowdata->layout->marriage[i].x,blk.rect.max.y-blk.scroll.y+windowdata->layout->marriage[i].y,windowdata->layout->marriage[i].marriage,windowdata->layout->marriage[i].childline,windowdata->layout->marriage[i].selected);
		}
		for (i=windowdata->layout->numpeople-1;i>=0;i--) {
			Graphics_PlotPerson(windowdata->layout->person[i].person,blk.rect.min.x-blk.scroll.x+windowdata->layout->person[i].x,blk.rect.max.y-blk.scroll.y+windowdata->layout->person[i].y,windowdata->layout->person[i].child,windowdata->layout->person[i].selected);
		}
		Desk_Wimp_GetRectangle(&blk,&more);
		/*Use clip rectangle??*/
	}
	return Desk_TRUE;
}
