/*
	FT - Layout
	© Alex Waugh 1999

	$Log: Layout.c,v $
	Revision 1.12  1999/10/12 16:29:15  AJW
	Added Layout_TraverseAncestorTree - but it doesn't quit work yet

	Revision 1.11  1999/10/11 22:27:16  AJW
	Changed to use Error2

	Revision 1.10  1999/10/10 20:54:40  AJW
	Modified to use Desk

	Revision 1.9  1999/10/02 17:48:24  AJW
	Added Layout_RemoveMarriage

	Revision 1.8  1999/09/29 17:51:38  AJW
	Added Layout_AlterMarriageChildLine

	Revision 1.7  1999/09/29 17:42:49  AJW
	Added Layout_AlterChildline

	Revision 1.6  1999/09/29 17:12:11  AJW
	Corrected childline status in Layout_AddMarriage

	Revision 1.5  1999/09/29 17:07:25  AJW
	Added Layout_FindYCoord

	Revision 1.4  1999/09/29 15:56:32  AJW
	Added Layout_AddMarriage

	Revision 1.3  1999/09/29 15:51:40  AJW
	Added Layout_AddPerson

	Revision 1.2  1999/09/27 16:57:51  AJW
	Added Layout_FindXCoord and Layout_FindMarriageXCoord

	Revision 1.1  1999/09/27 15:33:10  AJW
	Initial revision


*/

#include "Desk.Core.h"
#include "Desk.Error2.h"
#include "Desk.Window.h"
#include "Desk.Event.h"
#include "Desk.DeskMem.h"

#include "AJWLib.Flex.h"

#include <stdlib.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "GConfig.h"
#include "Layout.h"


#define LARGENUMBERINCREMENT 10000

typedef struct {
	int minx;
	int maxx;
} line;


static line *spaces;
static int mingeneration,maxgeneration;
#if DEBUG
layout lout;
layout *layouts=&lout;
Desk_bool halt;
#else
static layout *layouts;
#endif
static int numgenerations,addamount,firstplot,largenumber;
static Desk_bool selectmarriages;

void Layout_TraverseTree(elementptr person,int domarriage,int doallsiblings,Desk_bool dochild,Desk_bool doparents,Desk_bool lefttoright,int generation,void (*fn)(int,int,Desk_bool,Desk_bool));

elementptr Test(elementptr person)
{
/*	elementptr principal;
	principal=Database_GetPrincipalFromMarriage(Database_GetMarriage(person));
	if (principal!=none) person=principal;
*/	while (Database_GetMarriageLtoR(person)!=none) person=Database_GetMarriageLtoR(person);
	return person;
}

layout *Layout_LayoutUnlinked(void)
{
	elementptr person=none;
#if DEBUG
return layouts;
#endif
	layouts=Desk_DeskMem_Malloc(sizeof(layout));
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->person),sizeof(personlayout));
	/*Free layouts if there is a Flex error?*/
	/*layouts as local var?*/
	layouts->marriage=NULL;
	layouts->children=NULL;
	layouts->nummarriages=0;
	layouts->numchildren=0;
	layouts->numpeople=0;
	person=Database_GetUnlinked(person);
	while (person!=none) {
		AJWLib_Flex_Extend((flex_ptr)&(layouts->person),sizeof(personlayout)*(layouts->numpeople+1));
		layouts->person[layouts->numpeople].x=0;
		layouts->person[layouts->numpeople].y=-(Graphics_UnlinkedGapHeight()+Graphics_PersonHeight())*(layouts->numpeople-1);
		layouts->person[layouts->numpeople].person=person;
		layouts->person[layouts->numpeople].child=Desk_FALSE;
		layouts->person[layouts->numpeople].selected=Desk_FALSE;
		person=Database_GetUnlinked(person);
		layouts->numpeople++;
	}
	return layouts;
}

void Layout_AlterChildline(layout *layout,elementptr person,Desk_bool on)
{
	int i;
	for (i=0;i<layout->numpeople;i++) if (layout->person[i].person==person) layout->person[i].child=on;
}

void Layout_AlterMarriageChildline(layout *layout,elementptr marriage,Desk_bool on)
{
	int i;
	for (i=0;i<layout->nummarriages;i++) if (layout->marriage[i].marriage==marriage) layout->marriage[i].childline=on;
}

void Layout_AddPerson(layout *layout,elementptr person,int x,int y)
{
		AJWLib_Flex_Extend((flex_ptr)&(layout->person),sizeof(personlayout)*(layout->numpeople+1));
		layout->person[layout->numpeople].x=x;
		layout->person[layout->numpeople].y=y;
		layout->person[layout->numpeople].person=person;
		layout->person[layout->numpeople].child=(Database_GetMother(person)==none ? Desk_FALSE : Desk_TRUE);
		layout->person[layout->numpeople].selected=Desk_FALSE;
		layout->numpeople++;
}

void Layout_AddMarriage(layout *layout,elementptr marriage,int x,int y)
{
		AJWLib_Flex_Extend((flex_ptr)&(layout->marriage),sizeof(marriagelayout)*(layout->nummarriages+1));
		layout->marriage[layout->nummarriages].x=x;
		layout->marriage[layout->nummarriages].y=y;
		layout->marriage[layout->nummarriages].marriage=marriage;
		layout->marriage[layout->nummarriages].childline=(Database_GetLeftChild(marriage)==none ? Desk_FALSE : Desk_TRUE);
		layout->marriage[layout->nummarriages].selected=Desk_FALSE;
		layout->nummarriages++;
}

Desk_bool Layout_Selected(layout *layout,elementptr person)
{
	int i;
	for (i=0;i<layout->numpeople;i++) if (layout->person[i].person==person) return layout->person[i].selected;
	return Desk_FALSE;
}

int Layout_FindXCoord(layout *layout,elementptr person)
{
	int i;
	for (i=0;i<layout->numpeople;i++) if (layout->person[i].person==person) return layout->person[i].x;
	return 0;
}

int Layout_FindYCoord(layout *layout,elementptr person)
{
	int i;
	for (i=0;i<layout->numpeople;i++) if (layout->person[i].person==person) return layout->person[i].y;
	return 0;
}

int Layout_FindMarriageXCoord(layout *layout,elementptr marriage)
{
	int i;
	for (i=0;i<layout->nummarriages;i++) if (layout->marriage[i].marriage==marriage) return layout->marriage[i].x;
	return 0;
}

void Layout_ExtendGeneration(int generation)
{
	int i;
	if (generation<mingeneration) {
		AJWLib_Flex_MidExtend((flex_ptr)&spaces,0,sizeof(line)*(mingeneration-generation));
		for (i=0;i<mingeneration-generation;i++) {
			spaces[i].minx=0;
			spaces[i].maxx=0;
		}
		mingeneration=generation;
	}
	if (generation>maxgeneration) {
		AJWLib_Flex_Extend((flex_ptr)&spaces,sizeof(line)*(generation-mingeneration+1));
		for (i=maxgeneration-mingeneration+1;i<generation-mingeneration+1;i++) {
			spaces[i].minx=0;
			spaces[i].maxx=0;
		}
		maxgeneration=generation;
	}
}

void Layout_PlotChildLine(layout *layout,elementptr person,int y)
{
	int i;
	if (Database_GetSiblingRtoL(person)==none) {
		if (Database_GetMother(person)!=none) {
			elementptr marriage;
			int leftpos=-99,rightpos=-99,marriagepos=-99;
			marriage=Database_GetMarriage(Database_GetMother(person));
			do {
				for (i=0;i<layout->numpeople;i++) {
					if (layout->person[i].person==person) {
						if (layout->person[i].x<leftpos || leftpos==-99) leftpos=layout->person[i].x;
						if (layout->person[i].x>rightpos || rightpos==-99) rightpos=layout->person[i].x;
						i=layout->numpeople;
					}
					/*Optimise loop?*/
				}
			} while ((person=Database_GetSiblingLtoR(person))!=none);
			leftpos+=Graphics_PersonWidth()/2;
			rightpos+=Graphics_PersonWidth()/2;
			for (i=0;i<layout->nummarriages;i++) {
				if (layout->marriage[i].marriage==marriage) marriagepos=layout->marriage[i].x+Graphics_MarriageWidth()/2;
				/*Optimise loop?*/
			}
			if (marriagepos==-99) return;
			if (marriagepos<leftpos) leftpos=marriagepos;
			if (marriagepos>rightpos) rightpos=marriagepos;
			AJWLib_Flex_Extend((flex_ptr)&(layout->children),sizeof(childlinelayout)*(layout->numchildren+1));
			layout->children[layout->numchildren].rightx=rightpos;
			layout->children[layout->numchildren].leftx=leftpos;
			layout->children[layout->numchildren].y=y;
			layout->numchildren++;
		}
	}
}

void Layout_PlotPerson(elementptr person,int generation,Desk_bool lefttoright,Desk_bool child)
{
	elementptr marriage=Database_GetMarriage(person);
	AJWLib_Flex_Extend((flex_ptr)&(layouts->person),sizeof(personlayout)*(layouts->numpeople+1));
	layouts->numpeople++; /*Only incremented if there was no error*/
	if (lefttoright) {
		if (marriage && Database_GetPrincipalFromMarriage(marriage)!=person) {
			spaces[generation-mingeneration].maxx+=Graphics_MarriageWidth();
			if (!Database_IsFirstMarriage(marriage)) spaces[generation-mingeneration].maxx+=Graphics_SecondMarriageGap();
		} else {
			spaces[generation-mingeneration].maxx+=Graphics_GapWidth();
		}
		layouts->person[layouts->numpeople-1].x=spaces[generation-mingeneration].maxx;
		spaces[generation-mingeneration].maxx+=Graphics_PersonWidth();
	} else {
		spaces[generation-mingeneration].minx-=Graphics_PersonWidth();
		layouts->person[layouts->numpeople-1].x=spaces[generation-mingeneration].minx;
		if (marriage && Database_GetPrincipalFromMarriage(marriage)!=person) {
			spaces[generation-mingeneration].minx-=Graphics_MarriageWidth();
			if (!Database_IsFirstMarriage(marriage)) spaces[generation-mingeneration].minx-=Graphics_SecondMarriageGap();
		} else {
			spaces[generation-mingeneration].minx-=Graphics_GapWidth();
		}
	}
	layouts->person[layouts->numpeople-1].y=(generation)*-(Graphics_GapHeightAbove()+Graphics_GapHeightBelow()+Graphics_PersonHeight());
	layouts->person[layouts->numpeople-1].person=person;
	if (child && Database_GetFather(person)) layouts->person[layouts->numpeople-1].child=Desk_TRUE; else layouts->person[layouts->numpeople-1].child=Desk_FALSE;
	layouts->person[layouts->numpeople-1].selected=Desk_FALSE;
#if DEBUG
halt=Desk_TRUE;
while (halt) Desk_Event_Poll();
Desk_Window_ForceRedraw(-1,0,0,100000,100000);
#endif
}

void Layout_PlotMarriage(layout *layout,elementptr person,int x,int y,Desk_bool children)
{
	elementptr marriage;
	marriage=Database_GetMarriage(person);
	if (marriage==none || Database_GetPrincipalFromMarriage(marriage)==person) return;
	x-=Graphics_MarriageWidth();
	AJWLib_Flex_Extend((flex_ptr)&(layout->marriage),sizeof(marriagelayout)*(layout->nummarriages+1));
	layout->nummarriages++;
	layout->marriage[layout->nummarriages-1].x=x;
	layout->marriage[layout->nummarriages-1].y=y;
	layout->marriage[layout->nummarriages-1].marriage=marriage;
	layout->marriage[layout->nummarriages-1].childline=(Database_GetLeftChild(marriage) ? children : Desk_FALSE);
	layout->marriage[layout->nummarriages-1].selected=Desk_FALSE;
}

int Layout_FindChildCoords(elementptr marriage)
{
	elementptr leftchild,rightchild;
	int i,leftx=0,rightx=0; /*a better method of error checking?*/
	leftchild=Database_GetLeftChild(marriage);
	rightchild=leftchild;
	while (Database_GetSiblingLtoR(rightchild)!=none) rightchild=Database_GetSiblingLtoR(rightchild);
	for (i=0;i<layouts->numpeople;i++) {
		if (layouts->person[i].person==leftchild) leftx=layouts->person[i].x;
		if (layouts->person[i].person==rightchild) rightx=layouts->person[i].x;
		/*optimise this loop*/
	}
	return (leftx+rightx+Graphics_PersonWidth())/2;
}

void Layout_Add(elementptr person,int generation,Desk_bool dummy1,Desk_bool dummy2)
{
	int i,amount;
	elementptr marriage=Database_GetMarriage(person);
	for (i=0;i<layouts->numpeople;i++) {
		if (layouts->person[i].person==person) {
			if (addamount<0 && spaces[generation-mingeneration].maxx==layouts->person[i].x+Graphics_PersonWidth()) spaces[generation-mingeneration].maxx+=addamount;
			if (addamount>0 && spaces[generation-mingeneration].maxx==layouts->person[i].x+Graphics_PersonWidth()) spaces[generation-mingeneration].maxx+=addamount;
			if (marriage && Database_GetPrincipalFromMarriage(marriage)!=person) {
				amount=Graphics_MarriageWidth();
				if (!Database_IsFirstMarriage(marriage)) amount+=Graphics_SecondMarriageGap();
			} else {
				amount=Graphics_GapWidth();
			}
			if (addamount>0 && spaces[generation-mingeneration].minx==layouts->person[i].x-amount) spaces[generation-mingeneration].minx+=addamount;
			if (addamount<0 && spaces[generation-mingeneration].minx==layouts->person[i].x-amount) spaces[generation-mingeneration].minx+=addamount;
			layouts->person[i].x+=addamount;
		}
		/*optimise this loop*/
	}
}

void Layout_Select(elementptr person,int dummy0,Desk_bool dummy1,Desk_bool dummy2)
{
	int i;
	elementptr marriage;
	for (i=0;i<layouts->numpeople;i++) {
		if (layouts->person[i].person==person) {
			layouts->person[i].selected=Desk_TRUE;
			i=layouts->numpeople;
		}
	}
	marriage=Database_GetMarriage(person);
	if (Database_GetPrincipalFromMarriage(marriage)!=person) {
		for (i=0;i<layouts->nummarriages;i++) {
			if (layouts->marriage[i].marriage==marriage) {
				layouts->marriage[i].selected=Desk_TRUE;
				i=layouts->nummarriages;
			}
		}
	}
}

void Layout_Plot(elementptr person,int generation,Desk_bool lefttoright,Desk_bool child)
{
	elementptr marriage;
	Layout_ExtendGeneration(generation);
	marriage=Database_GetMarriage(person);
	if (firstplot==Desk_FALSE && spaces[generation-mingeneration].minx==spaces[generation-mingeneration].maxx) {
		if (lefttoright) {
			spaces[generation-mingeneration].minx=-(largenumber+=LARGENUMBERINCREMENT);
			spaces[generation-mingeneration].maxx=-largenumber;
		} else {
			spaces[generation-mingeneration].minx=(largenumber+=LARGENUMBERINCREMENT);
			spaces[generation-mingeneration].maxx=largenumber;
		}
	}
	if (firstplot==Desk_TRUE) firstplot=Desk_FALSE;
	if (marriage) {
		if (Database_GetLeftChild(marriage)!=none) {
			int childcoords;
			childcoords=Layout_FindChildCoords(marriage);
			if (lefttoright) {
				if (Database_GetMarriageRtoL(person)!=Database_GetPrincipalFromMarriage(marriage)) {
					int maxx=spaces[generation-mingeneration].maxx+Graphics_MarriageWidth()/2;
					if (Database_GetPrincipalFromMarriage(marriage)==person) maxx+=Graphics_PersonWidth()+Graphics_GapWidth();
	                if (!Database_IsFirstMarriage(marriage)) maxx+=Graphics_SecondMarriageGap();
					if (childcoords>=maxx) {
						spaces[generation-mingeneration].maxx+=childcoords-maxx;
					} else {
						int newmaxx;
						addamount=maxx-childcoords;
						Layout_TraverseTree(Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);
						newmaxx=spaces[generation-mingeneration].maxx+Graphics_MarriageWidth()/2;
						if (Database_GetPrincipalFromMarriage(marriage)==person) newmaxx+=Graphics_PersonWidth()+Graphics_GapWidth();
						if (!Database_IsFirstMarriage(marriage)) newmaxx+=Graphics_SecondMarriageGap();
						if (newmaxx!=maxx) {
							addamount=maxx-newmaxx;
							Layout_TraverseTree(Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);
						}
					}
				}
			} else {
				if (Database_GetPrincipalFromMarriage(marriage)!=person) {
					int minx=spaces[generation-mingeneration].minx-Graphics_PersonWidth()-Graphics_MarriageWidth()/2;
					if (childcoords<minx) {
						spaces[generation-mingeneration].minx-=minx-childcoords;
					} else {
						int newminx;
						addamount=minx-childcoords;
						Layout_TraverseTree(Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);
						newminx=spaces[generation-mingeneration].minx-Graphics_PersonWidth()-Graphics_MarriageWidth()/2;
						if (newminx!=minx) {
							addamount=minx-newminx;
							Layout_TraverseTree(Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);
						}
					}
				}
			}
		}
	}
	Layout_PlotPerson(person,generation,lefttoright,child);
}

void Layout_RemovePerson(layout *layout,elementptr person)
{
	int i;
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].person==person) {
			AJWLib_Flex_MidExtend((flex_ptr)&(layout->person),sizeof(personlayout)*(i+1),-sizeof(personlayout));
			layout->numpeople--;
			return;
		}
	}
}

void Layout_RemoveMarriage(layout *layout,elementptr marriage)
{
	int i;
	for (i=0;i<layout->nummarriages;i++) {
		if (layout->marriage[i].marriage==marriage) {
			AJWLib_Flex_MidExtend((flex_ptr)&(layout->marriage),sizeof(marriagelayout)*(i+1),-sizeof(marriagelayout));
			layout->nummarriages--;
			return;
		}
	}
}

Desk_wimp_rect Layout_FindExtent(layout *layout,Desk_bool selection)
{
	Desk_wimp_rect box;
	int i;
	box.min.x=INFINITY;
	box.min.y=INFINITY;
	box.max.x=-INFINITY;
	box.max.y=-INFINITY;
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].selected || !selection) {
			if (layout->person[i].x<box.min.x) box.min.x=layout->person[i].x;
			if (layout->person[i].x+Graphics_PersonWidth()>box.max.x) box.max.x=layout->person[i].x+Graphics_PersonWidth();
			if (layout->person[i].y<box.min.y) box.min.y=layout->person[i].y;
			if (layout->person[i].y>box.max.y) box.max.y=layout->person[i].y;
		}
	}
	for (i=0;i<layout->nummarriages;i++) {
		if (layout->marriage[i].selected || !selection) {
			if (layout->marriage[i].x<box.min.x) box.min.x=layout->marriage[i].x;
			if (layout->marriage[i].x+Graphics_MarriageWidth()>box.max.x) box.max.x=layout->marriage[i].x+Graphics_MarriageWidth();
			if (layout->marriage[i].y<box.min.y) box.min.y=layout->marriage[i].y;
			if (layout->marriage[i].y>box.max.y) box.max.y=layout->marriage[i].y;
		}
	}
	box.max.y+=Graphics_PersonHeight();
	return box;
}

void Layout_TraverseTree(elementptr person,int domarriage,int doallsiblings,Desk_bool dochild,Desk_bool doparents,Desk_bool lefttoright,int generation,void (*fn)(int,int,Desk_bool,Desk_bool))
{
	if (person==none) return;
	if (domarriage>=2 && lefttoright)  Layout_TraverseTree(Database_GetMarriageRtoL(person),3,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if (domarriage>=2 && !lefttoright) Layout_TraverseTree(Database_GetMarriageLtoR(person),3,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if (doallsiblings>=2 && lefttoright)  Layout_TraverseTree(Database_GetSiblingRtoL(person),2,3,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if (doallsiblings>=2 && !lefttoright) Layout_TraverseTree(Database_GetSiblingLtoR(person),2,3,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if (dochild && Database_GetMarriage(person)) {
		if (lefttoright) {
			if (Database_GetMarriageRtoL(person)!=Database_GetPrincipalFromMarriage(Database_GetMarriage(person))) {
				Layout_TraverseTree(Database_GetLeftChild(Database_GetMarriage(person)),2,2,Desk_TRUE,Desk_FALSE,lefttoright,generation+1,fn);
			}
		} else {
			if (Database_GetPrincipalFromMarriage(Database_GetMarriage(person))!=person) {
				Layout_TraverseTree(Database_GetRightChild(Database_GetMarriage(person)),2,2,Desk_TRUE,Desk_FALSE,lefttoright,generation+1,fn);
			}
		}
	}
	(*fn)(person,generation,lefttoright,Desk_TRUE);
	if ((domarriage==Desk_TRUE || domarriage==2) && lefttoright)  Layout_TraverseTree(Database_GetMarriageLtoR(person),Desk_TRUE,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if ((domarriage==Desk_TRUE || domarriage==2) && !lefttoright) Layout_TraverseTree(Database_GetMarriageRtoL(person),Desk_TRUE,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if ((doallsiblings==1 || doallsiblings==2) &&lefttoright)  Layout_TraverseTree(Database_GetSiblingLtoR(person),2,1,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if ((doallsiblings==1 || doallsiblings==2) &&!lefttoright) Layout_TraverseTree(Database_GetSiblingRtoL(person),2,1,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if (doparents) {
		if (lefttoright && Database_GetMarriageRtoL(Database_GetMother(person))==Database_GetFather(person)) {
			Layout_TraverseTree(Database_GetFather(person),2,2,Desk_FALSE,Desk_TRUE,lefttoright,generation-1,fn);
		} else {
			Layout_TraverseTree(Database_GetMother(person),2,2,Desk_FALSE,Desk_TRUE,lefttoright,generation-1,fn);
		}
	}
}

void Layout_TraverseAncestorTree(elementptr person,int domarriage,int doallsiblings,int generation,void (*fn)(int,int,Desk_bool,Desk_bool))
{
	if (person==none) return;
	if (domarriage>=2) Layout_TraverseAncestorTree(Database_GetMarriageLtoR(person),3,2,generation,fn);
	if (doallsiblings>=2) Layout_TraverseAncestorTree(Database_GetSiblingLtoR(person),2,3,generation,fn);
	(*fn)(person,generation,Desk_FALSE,Desk_TRUE);
	if (domarriage==Desk_TRUE || domarriage==2) Layout_TraverseAncestorTree(Database_GetMarriageRtoL(person),Desk_TRUE,2,generation,fn);
	if (doallsiblings==1 || doallsiblings==2) Layout_TraverseAncestorTree(Database_GetSiblingRtoL(person),2,1,generation,fn);
	if (generation>1-numgenerations && Database_GetSiblingRtoL(person)==none) Layout_TraverseAncestorTree(Database_GetMother(person),2,2,generation-1,fn);
}

void Layout_TraverseDescendentTree(elementptr person,int domarriage,int generation,void (*fn)(int,int,Desk_bool,Desk_bool))
{
	if (person==none) return;
	if (domarriage>=2) Layout_TraverseDescendentTree(Database_GetMarriageLtoR(person),3,generation,fn);
	if (Database_GetPrincipalFromMarriage(Database_GetMarriage(person))!=person && generation<numgenerations-1) Layout_TraverseDescendentTree(Database_GetRightChild(Database_GetMarriage(person)),2,generation+1,fn);
	(*fn)(person,generation,Desk_FALSE,Desk_TRUE);
	if (domarriage==Desk_TRUE || domarriage==2) Layout_TraverseDescendentTree(Database_GetMarriageRtoL(person),Desk_TRUE,generation,fn);
	if (generation>0) Layout_TraverseDescendentTree(Database_GetSiblingRtoL(person),2,generation,fn);
}

void Layout_LayoutMarriages(layout *layout)
{
	int i;
	layout->nummarriages=0;
	for (i=0;i<layout->numpeople;i++) Layout_PlotMarriage(layout,layout->person[i].person,layout->person[i].x,layout->person[i].y,Desk_TRUE);
}

void Layout_LayoutLines(layout *layout)
{
	int i;
	layout->numchildren=0;
	for (i=0;i<layout->numpeople;i++) Layout_PlotChildLine(layout,layout->person[i].person,layout->person[i].y);
}

void Layout_SelectDescendents(layout *layout,elementptr person)
{
	layouts=layout;
	numgenerations=INFINITY;
	selectmarriages=Desk_TRUE;
	Layout_TraverseDescendentTree(person,2,0,Layout_Select);
}

void Layout_SelectAncestors(layout *layout,elementptr person)
{
	layouts=layout;
	numgenerations=INFINITY;
	selectmarriages=Desk_TRUE;
	Layout_TraverseAncestorTree(person,2,2,0,Layout_Select);
}

void Layout_SelectSiblings(layout *layout,elementptr person)
{
	elementptr person2=person;
	layouts=layout;
	selectmarriages=Desk_FALSE;
	while ((person=Database_GetSiblingLtoR(person))!=none) Layout_Select(person,0,Desk_FALSE,Desk_FALSE);
	do Layout_Select(person2,0,Desk_FALSE,Desk_FALSE); while ((person2=Database_GetSiblingRtoL(person2))!=none);
}

void Layout_SelectSpouses(layout *layout,elementptr person)
{
	elementptr person2=person;
	layouts=layout;
	selectmarriages=Desk_TRUE;
	while ((person=Database_GetMarriageLtoR(person))!=none) Layout_Select(person,0,Desk_FALSE,Desk_FALSE);
	do Layout_Select(person2,0,Desk_FALSE,Desk_FALSE); while ((person2=Database_GetMarriageRtoL(person2))!=none);
}

layout *Layout_LayoutNormal(void)
{
	elementptr person=none;
#if DEBUG
static Desk_bool flag=Desk_TRUE;
if (flag) {
#else
	layouts=Desk_DeskMem_Malloc(sizeof(layout));
#endif
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->person),1);
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->marriage),1);
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->children),1);
	/*Free others if errors?*/
#if DEBUG
	flag=Desk_FALSE;
} else {
	AJWLib_Flex_Extend((flex_ptr)&(layouts->person),1);
	AJWLib_Flex_Extend((flex_ptr)&(layouts->marriage),1);
	AJWLib_Flex_Extend((flex_ptr)&(layouts->children),1);
}
#endif
	AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(line));
	spaces[0].minx=0;
	spaces[0].maxx=0;
	mingeneration=0;
	maxgeneration=0;
	layouts->numpeople=0;
	layouts->nummarriages=0;
	layouts->numchildren=0;
	person=Database_GetLinked();
	while (Database_GetFather(person)!=none) person=Database_GetFather(person); /*This should not be nessercery?*/
	firstplot=2;
	Layout_TraverseTree(Test(person),2,2,Desk_TRUE,Desk_FALSE,Desk_TRUE,0,Layout_Plot);
	Layout_LayoutMarriages(layouts);
	Layout_LayoutLines(layouts);
	AJWLib_Flex_Free((flex_ptr)&spaces);
	return layouts;
}

layout *Layout_LayoutDescendents(elementptr person,int generations)
{
	layouts=Desk_DeskMem_Malloc(sizeof(layout));
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->person),1);
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->marriage),1);
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->children),1);
	AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(line));
	/*free others if an error occours?*/
	spaces[0].minx=0;
	spaces[0].maxx=0;
	mingeneration=0;
	maxgeneration=0;
	numgenerations=generations;
	layouts->numpeople=0;
	layouts->nummarriages=0;
	layouts->numchildren=0;
	largenumber=0;
	firstplot=Desk_TRUE;
	Layout_TraverseDescendentTree(person,2,0,Layout_Plot);
	Layout_LayoutMarriages(layouts);
	Layout_LayoutLines(layouts);
	AJWLib_Flex_Free((flex_ptr)&spaces);
	return layouts;
}

layout *Layout_LayoutAncestors(elementptr person,int generations)
{
#if DEBUG
static Desk_bool flag=Desk_TRUE;
if (flag) {
#else
	layouts=Desk_DeskMem_Malloc(sizeof(layout));
#endif
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->person),1);
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->marriage),1);
	AJWLib_Flex_Alloc((flex_ptr)&(layouts->children),1);
	/*Free others if errors?*/
#if DEBUG
	flag=Desk_FALSE;
} else {
	AJWLib_Flex_Extend((flex_ptr)&(layouts->person),1);
	AJWLib_Flex_Extend((flex_ptr)&(layouts->marriage),1);
	AJWLib_Flex_Extend((flex_ptr)&(layouts->children),1);
}
#endif
	AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(line));
	/*free others if an error occours?*/
	spaces[0].minx=0;
	spaces[0].maxx=0;
	mingeneration=0;
	maxgeneration=0;
	numgenerations=generations;
	layouts->numpeople=0;
	layouts->nummarriages=0;
	layouts->numchildren=0;
	largenumber=0;
	firstplot=Desk_TRUE;
	Layout_TraverseAncestorTree(person,2,2,0,Layout_Plot);
	Layout_LayoutMarriages(layouts);
	Layout_LayoutLines(layouts);
	AJWLib_Flex_Free((flex_ptr)&spaces);
	return layouts;
}
