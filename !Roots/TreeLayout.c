/*
	FT - Layout routines
	© Alex Waugh 1999

	$Id: TreeLayout.c,v 1.27 2000/02/28 17:21:08 uid1 Exp $

*/

#include "Desk.Core.h"
#include "Desk.Error2.h"
#include "Desk.Window.h"
#include "Desk.Event.h"
#include "Desk.DeskMem.h"

#include "AJWLib.Flex.h"
#include "AJWLib.File.h"
#include "AJWLib.Assert.h"

#include <stdlib.h>
#include <stdio.h>

#include "Database.h"
#include "Graphics.h"
#include "Modules.h"
#include "Windows.h"
#include "Layout.h"
#include "File.h"
#include "Config.h"


#define LARGENUMBERINCREMENT 10000

typedef struct {
	int minx;
	int maxx;
} line;

typedef void (*callfn)(layout *layout,elementptr person,int,Desk_bool,Desk_bool);

static line *spaces;
static int mingeneration,maxgeneration;
#ifdef DEBUG
layout *debuglayout=NULL;
Desk_bool halt;
#endif
static int numgenerations,addamount,firstplot,largenumber;
static Desk_bool selectmarriages;

void Layout_TraverseTree(layout *layout,elementptr person,int domarriage,int doallsiblings,Desk_bool dochild,Desk_bool doparents,Desk_bool lefttoright,int generation,callfn fn);

layout *Layout_LayoutUnlinked(void)
{
	elementptr person=none;
	layout *layout=NULL;
	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
	layout->person=NULL;
	layout->marriage=NULL;
	layout->children=NULL;
	layout->nummarriages=0;
	layout->numchildren=0;
	layout->numpeople=0;
	layout->title.x=INFINITY;
	layout->title.y=INFINITY;
	Desk_Error2_Try {
		AJWLib_Flex_Alloc((flex_ptr)&(layout->person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->marriage),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->children),1);
		person=Database_GetUnlinked(person);
		while (person!=none) {
			AJWLib_Flex_Extend((flex_ptr)&(layout->person),sizeof(personlayout)*(layout->numpeople+1));
			layout->person[layout->numpeople].x=0;
			layout->person[layout->numpeople].y=-(Graphics_UnlinkedGapHeight()+Graphics_PersonHeight())*(layout->numpeople-1);
			layout->person[layout->numpeople].person=person;
			layout->person[layout->numpeople].child=Desk_FALSE;
			layout->person[layout->numpeople].selected=Desk_FALSE;
			person=Database_GetUnlinked(person);
			layout->numpeople++;
		}
	} Desk_Error2_Catch {
		if (layout->person) AJWLib_Flex_Free((flex_ptr)&(layout->person));
		if (layout->marriage) AJWLib_Flex_Free((flex_ptr)&(layout->marriage));
		if (layout->children) AJWLib_Flex_Free((flex_ptr)&(layout->children));
		free(layout);
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
	return layout;
}

void Layout_AlterChildline(layout *layout,elementptr person,Desk_bool on)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	for (i=0;i<layout->numpeople;i++) if (layout->person[i].person==person) layout->person[i].child=on;
}

void Layout_AlterMarriageChildline(layout *layout,elementptr marriage,Desk_bool on)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	for (i=0;i<layout->nummarriages;i++) if (layout->marriage[i].marriage==marriage) layout->marriage[i].childline=on;
}

void Layout_LayoutTitle(layout *layout)
{
	Desk_wimp_rect bbox;
	AJWLib_Assert(layout!=NULL);
	if (!Config_Title()) return;
	if (layout->title.x==INFINITY && layout->title.y==INFINITY) return;
	bbox=Layout_FindExtent(layout,Desk_FALSE);
	layout->title.x=(bbox.max.x+bbox.min.x)/2;
	layout->title.y=bbox.max.y+Graphics_TitleHeight()/2;
}

void Layout_AddPerson(layout *layout,elementptr person,int x,int y)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	AJWLib_Flex_Extend((flex_ptr)&(layout->person),sizeof(personlayout)*(layout->numpeople+1));
	layout->person[layout->numpeople].x=x;
	layout->person[layout->numpeople].y=y;
	layout->person[layout->numpeople].person=person;
	layout->person[layout->numpeople].child=(Database_GetMother(person)==none ? Desk_FALSE : Desk_TRUE);
	layout->person[layout->numpeople].selected=Desk_FALSE;
	layout->numpeople++;
	Modules_ChangedLayout();
}

void Layout_AddMarriage(layout *layout,elementptr marriage,int x,int y)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(marriage!=none);
	AJWLib_Flex_Extend((flex_ptr)&(layout->marriage),sizeof(marriagelayout)*(layout->nummarriages+1));
	layout->marriage[layout->nummarriages].x=x;
	layout->marriage[layout->nummarriages].y=y;
	layout->marriage[layout->nummarriages].marriage=marriage;
	layout->marriage[layout->nummarriages].childline=(Database_GetLeftChild(marriage)==none ? Desk_FALSE : Desk_TRUE);
	layout->marriage[layout->nummarriages].selected=Desk_FALSE;
	layout->nummarriages++;
	Modules_ChangedLayout();
}

Desk_bool Layout_Selected(layout *layout,elementptr person)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	for (i=0;i<layout->numpeople;i++) if (layout->person[i].person==person) return layout->person[i].selected;
	return Desk_FALSE;
}

int Layout_FindXCoord(layout *layout,elementptr person)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	for (i=0;i<layout->numpeople;i++) if (layout->person[i].person==person) return layout->person[i].x;
	return 0;
}

int Layout_FindYCoord(layout *layout,elementptr person)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	for (i=0;i<layout->numpeople;i++) if (layout->person[i].person==person) return layout->person[i].y;
	return 0;
}

int Layout_FindMarriageXCoord(layout *layout,elementptr marriage)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	for (i=0;i<layout->nummarriages;i++) if (layout->marriage[i].marriage==marriage) return layout->marriage[i].x;
	return 0;
}

static void Layout_ExtendGeneration(int generation)
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

static void Layout_PlotChildLine(layout *layout,elementptr person,int y)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
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

static void Layout_PlotPerson(layout *layout,elementptr person,int generation,Desk_bool lefttoright,Desk_bool child)
{
	elementptr marriage;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	marriage=Database_GetMarriage(person);
	AJWLib_Flex_Extend((flex_ptr)&(layout->person),sizeof(personlayout)*(layout->numpeople+1));
	layout->numpeople++; /*Only incremented if there was no error*/
	if (lefttoright) {
		if (marriage && Database_GetPrincipalFromMarriage(marriage)!=person) {
			spaces[generation-mingeneration].maxx+=Graphics_MarriageWidth();
			if (!Database_IsFirstMarriage(marriage)) spaces[generation-mingeneration].maxx+=Graphics_SecondMarriageGap();
		} else {
			spaces[generation-mingeneration].maxx+=Graphics_GapWidth();
		}
		layout->person[layout->numpeople-1].x=spaces[generation-mingeneration].maxx;
		spaces[generation-mingeneration].maxx+=Graphics_PersonWidth();
	} else {
		spaces[generation-mingeneration].minx-=Graphics_PersonWidth();
		layout->person[layout->numpeople-1].x=spaces[generation-mingeneration].minx;
		if (marriage && Database_GetPrincipalFromMarriage(marriage)!=person) {
			spaces[generation-mingeneration].minx-=Graphics_MarriageWidth();
			if (!Database_IsFirstMarriage(marriage)) spaces[generation-mingeneration].minx-=Graphics_SecondMarriageGap();
		} else {
			spaces[generation-mingeneration].minx-=Graphics_GapWidth();
		}
	}
	layout->person[layout->numpeople-1].y=(generation)*-(Graphics_GapHeightAbove()+Graphics_GapHeightBelow()+Graphics_PersonHeight());
	layout->person[layout->numpeople-1].person=person;
	if (child && Database_GetFather(person)) layout->person[layout->numpeople-1].child=Desk_TRUE; else layout->person[layout->numpeople-1].child=Desk_FALSE;
	layout->person[layout->numpeople-1].selected=Desk_FALSE;
#ifdef DEBUG
halt=Desk_TRUE;
debuglayout=layout;
while (halt) Desk_Event_Poll();
Desk_Window_ForceRedraw(-1,0,0,100000,100000);
#endif
}

static void Layout_PlotMarriage(layout *layout,elementptr person,int x,int y,Desk_bool children)
{
	elementptr marriage;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
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

static int Layout_FindChildCoords(layout *layout,elementptr marriage)
{
	elementptr leftchild,rightchild;
	int i,leftx=0,rightx=0; /*a better method of error checking?*/
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(marriage!=none);
	leftchild=Database_GetLeftChild(marriage);
	rightchild=leftchild;
	while (Database_GetSiblingLtoR(rightchild)!=none) rightchild=Database_GetSiblingLtoR(rightchild);
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].person==leftchild) leftx=layout->person[i].x;
		if (layout->person[i].person==rightchild) rightx=layout->person[i].x;
		/*optimise this loop*/
	}
	return (leftx+rightx+Graphics_PersonWidth())/2;
}

static void Layout_Add(layout *layout,elementptr person,int generation,Desk_bool dummy1,Desk_bool dummy2)
{
	int i,amount;
	elementptr marriage;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	marriage=Database_GetMarriage(person);
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].person==person) {
			if (addamount<0 && spaces[generation-mingeneration].maxx==layout->person[i].x+Graphics_PersonWidth()) spaces[generation-mingeneration].maxx+=addamount;
			if (addamount>0 && spaces[generation-mingeneration].maxx==layout->person[i].x+Graphics_PersonWidth()) spaces[generation-mingeneration].maxx+=addamount;
			if (marriage && Database_GetPrincipalFromMarriage(marriage)!=person) {
				amount=Graphics_MarriageWidth();
				if (!Database_IsFirstMarriage(marriage)) amount+=Graphics_SecondMarriageGap();
			} else {
				amount=Graphics_GapWidth();
			}
			if (addamount>0 && spaces[generation-mingeneration].minx==layout->person[i].x-amount) spaces[generation-mingeneration].minx+=addamount;
			if (addamount<0 && spaces[generation-mingeneration].minx==layout->person[i].x-amount) spaces[generation-mingeneration].minx+=addamount;
			layout->person[i].x+=addamount;
		}
		/*optimise this loop*/
	}
}

static void Layout_Select(layout *layout,elementptr person,int dummy0,Desk_bool dummy1,Desk_bool dummy2)
{
	int i;
	elementptr marriage;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].person==person) {
			layout->person[i].selected=Desk_TRUE;
			i=layout->numpeople;
		}
	}
	marriage=Database_GetMarriage(person);
	if (Database_GetPrincipalFromMarriage(marriage)!=person) {
		for (i=0;i<layout->nummarriages;i++) {
			if (layout->marriage[i].marriage==marriage) {
				layout->marriage[i].selected=Desk_TRUE;
				i=layout->nummarriages;
			}
		}
	}
}

static void Layout_Plot(layout *layout,elementptr person,int generation,Desk_bool lefttoright,Desk_bool child)
{
	elementptr marriage;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
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
			childcoords=Layout_FindChildCoords(layout,marriage);
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
						Layout_TraverseTree(layout,Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);
						newmaxx=spaces[generation-mingeneration].maxx+Graphics_MarriageWidth()/2;
						if (Database_GetPrincipalFromMarriage(marriage)==person) newmaxx+=Graphics_PersonWidth()+Graphics_GapWidth();
						if (!Database_IsFirstMarriage(marriage)) newmaxx+=Graphics_SecondMarriageGap();
						if (newmaxx!=maxx) {
							addamount=maxx-newmaxx;
							Layout_TraverseTree(layout,Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);
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
						Layout_TraverseTree(layout,Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);
						newminx=spaces[generation-mingeneration].minx-Graphics_PersonWidth()-Graphics_MarriageWidth()/2;
						if (newminx!=minx) {
							addamount=minx-newminx;
							Layout_TraverseTree(layout,Database_GetRightChild(marriage),2,2,Desk_TRUE,Desk_FALSE,Desk_FALSE,generation+1,Layout_Add);
						}
					}
				}
			}
		}
	}
	Layout_PlotPerson(layout,person,generation,lefttoright,child);
}

void Layout_RemovePerson(layout *layout,elementptr person)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	for (i=0;i<layout->numpeople;i++) {
		if (layout->person[i].person==person) {
			AJWLib_Flex_MidExtend((flex_ptr)&(layout->person),sizeof(personlayout)*(i+1),-sizeof(personlayout));
			layout->numpeople--;
			Modules_ChangedLayout();
			return;
		}
	}
}

void Layout_RemoveMarriage(layout *layout,elementptr marriage)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(marriage!=none);
	for (i=0;i<layout->nummarriages;i++) {
		if (layout->marriage[i].marriage==marriage) {
			AJWLib_Flex_MidExtend((flex_ptr)&(layout->marriage),sizeof(marriagelayout)*(i+1),-sizeof(marriagelayout));
			layout->nummarriages--;
			Modules_ChangedLayout();
			return;
		}
	}
}

Desk_wimp_rect Layout_FindExtent(layout *layout,Desk_bool selection)
{
	Desk_wimp_rect box;
	int i;
	AJWLib_Assert(layout!=NULL);
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
	if (box.min.x==INFINITY) {
		box.min.x=0;
		box.min.y=0;
		box.max.x=Graphics_PersonWidth();
		box.max.y=Graphics_PersonHeight();
	} else {
		box.max.y+=Graphics_PersonHeight();
	}
	return box;
}

void Layout_TraverseTree(layout *layout,elementptr person,int domarriage,int doallsiblings,Desk_bool dochild,Desk_bool doparents,Desk_bool lefttoright,int generation,callfn fn)
{
	AJWLib_Assert(layout!=NULL);
	if (person==none) return;
	if (domarriage>=2 && lefttoright)  Layout_TraverseTree(layout,Database_GetMarriageRtoL(person),3,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if (domarriage>=2 && !lefttoright) Layout_TraverseTree(layout,Database_GetMarriageLtoR(person),3,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if (doallsiblings>=2 && lefttoright)  Layout_TraverseTree(layout,Database_GetSiblingRtoL(person),2,3,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if (doallsiblings>=2 && !lefttoright) Layout_TraverseTree(layout,Database_GetSiblingLtoR(person),2,3,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if (dochild && Database_GetMarriage(person)) {
		if (lefttoright) {
			if (Database_GetMarriageRtoL(person)!=Database_GetPrincipalFromMarriage(Database_GetMarriage(person))) {
				Layout_TraverseTree(layout,Database_GetLeftChild(Database_GetMarriage(person)),2,2,Desk_TRUE,Desk_FALSE,lefttoright,generation+1,fn);
			}
		} else {
			if (Database_GetPrincipalFromMarriage(Database_GetMarriage(person))!=person) {
				Layout_TraverseTree(layout,Database_GetRightChild(Database_GetMarriage(person)),2,2,Desk_TRUE,Desk_FALSE,lefttoright,generation+1,fn);
			}
		}
	}
	fn(layout,person,generation,lefttoright,Desk_TRUE);
	if ((domarriage==Desk_TRUE || domarriage==2) && lefttoright)  Layout_TraverseTree(layout,Database_GetMarriageLtoR(person),Desk_TRUE,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if ((domarriage==Desk_TRUE || domarriage==2) && !lefttoright) Layout_TraverseTree(layout,Database_GetMarriageRtoL(person),Desk_TRUE,2,Desk_TRUE,Desk_TRUE,lefttoright,generation,fn);
	if ((doallsiblings==1 || doallsiblings==2) &&lefttoright)  Layout_TraverseTree(layout,Database_GetSiblingLtoR(person),2,1,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if ((doallsiblings==1 || doallsiblings==2) &&!lefttoright) Layout_TraverseTree(layout,Database_GetSiblingRtoL(person),2,1,Desk_TRUE,Desk_FALSE,lefttoright,generation,fn);
	if (doparents) {
		if (lefttoright && Database_GetMarriageRtoL(Database_GetMother(person))==Database_GetFather(person)) {
			Layout_TraverseTree(layout,Database_GetFather(person),2,2,Desk_FALSE,Desk_TRUE,lefttoright,generation-1,fn);
		} else {
			Layout_TraverseTree(layout,Database_GetMother(person),2,2,Desk_FALSE,Desk_TRUE,lefttoright,generation-1,fn);
		}
	}
/*Traversing righttoleft is broken*/
}

static void Layout_TraverseAncestorTree(layout *layout,elementptr person,int domarriage,int doallsiblings,Desk_bool doparents,int generation,callfn fn)
{
	AJWLib_Assert(layout!=NULL);
	Desk_UNUSED(doallsiblings);
	if (person==none) return;
	if (domarriage>=2)  Layout_TraverseAncestorTree(layout,Database_GetMarriageRtoL(person),3,0,Desk_TRUE,generation,fn);
	if (domarriage==3 && Database_GetMarriageRtoL(person)!=none) return;
/*	if (doallsiblings>=2)  Layout_TraverseAncestorTree(layout,Database_GetSiblingRtoL(person),2,3,Desk_FALSE,generation,fn);
*/	if (domarriage==2 || Database_GetMarriageRtoL(person)==none) fn(layout,person,generation,Desk_TRUE,Desk_TRUE);
	if ((domarriage==Desk_TRUE || domarriage==2))  Layout_TraverseAncestorTree(layout,Database_GetMarriageLtoR(person),Desk_TRUE,0,Desk_TRUE,generation,fn);
/*	if ((doallsiblings==1 || doallsiblings==2))  Layout_TraverseAncestorTree(layout,Database_GetSiblingLtoR(person),2,1,Desk_FALSE,generation,fn);
*/	if (doparents && (domarriage==2 || Database_GetMarriageRtoL(person)==none)) Layout_TraverseAncestorTree(layout,Database_GetMother(person),2,2,Desk_TRUE,generation-1,fn);
}

static void Layout_TraverseDescendentTree(layout *layout,elementptr person,int domarriage,int generation,callfn fn)
{
	AJWLib_Assert(layout!=NULL);
	if (person==none) return;
	if (domarriage>=2) Layout_TraverseDescendentTree(layout,Database_GetMarriageLtoR(person),3,generation,fn);
	if (Database_GetPrincipalFromMarriage(Database_GetMarriage(person))!=person && generation<numgenerations-1) Layout_TraverseDescendentTree(layout,Database_GetRightChild(Database_GetMarriage(person)),2,generation+1,fn);
	fn(layout,person,generation,Desk_FALSE,Desk_TRUE);
	if (domarriage==Desk_TRUE || domarriage==2) Layout_TraverseDescendentTree(layout,Database_GetMarriageRtoL(person),Desk_TRUE,generation,fn);
	if (generation>0) Layout_TraverseDescendentTree(layout,Database_GetSiblingRtoL(person),2,generation,fn);
}

void Layout_LayoutMarriages(layout *layout)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	layout->nummarriages=0;
	for (i=0;i<layout->numpeople;i++) Layout_PlotMarriage(layout,layout->person[i].person,layout->person[i].x,layout->person[i].y,Desk_TRUE);
}

void Layout_LayoutLines(layout *layout)
{
	int i;
	AJWLib_Assert(layout!=NULL);
	layout->numchildren=0;
	for (i=0;i<layout->numpeople;i++) Layout_PlotChildLine(layout,layout->person[i].person,layout->person[i].y);
}

void Layout_SelectDescendents(layout *layout,elementptr person)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	numgenerations=INFINITY;
	selectmarriages=Desk_TRUE;
	Layout_TraverseDescendentTree(layout,person,2,0,Layout_Select);
}

void Layout_SelectAncestors(layout *layout,elementptr person)
{
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	numgenerations=INFINITY;
	selectmarriages=Desk_TRUE;
/*	Layout_TraverseAncestorTree(layout,person,2,2,0,Layout_Select);*/
}

void Layout_SelectSiblings(layout *layout,elementptr person)
{
	elementptr person2=person;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	selectmarriages=Desk_FALSE;
	while ((person=Database_GetSiblingLtoR(person))!=none) Layout_Select(layout,person,0,Desk_FALSE,Desk_FALSE);
	do Layout_Select(layout,person2,0,Desk_FALSE,Desk_FALSE); while ((person2=Database_GetSiblingRtoL(person2))!=none);
}

void Layout_SelectSpouses(layout *layout,elementptr person)
{
	elementptr person2=person;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(person!=none);
	selectmarriages=Desk_TRUE;
	while ((person=Database_GetMarriageLtoR(person))!=none) Layout_Select(layout,person,0,Desk_FALSE,Desk_FALSE);
	do Layout_Select(layout,person2,0,Desk_FALSE,Desk_FALSE); while ((person2=Database_GetMarriageRtoL(person2))!=none);
}

layout *Layout_LayoutNormal(void)
{
	elementptr person=none;
	layout *layout;
	AJWLib_Assert(spaces==NULL);
	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
    layout->person=NULL;
    layout->marriage=NULL;
    layout->children=NULL;
	layout->numpeople=0;
	layout->nummarriages=0;
	layout->numchildren=0;
    Desk_Error2_Try {
		AJWLib_Flex_Alloc((flex_ptr)&(layout->person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->marriage),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->children),1);
		AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(line));
		spaces[0].minx=0;
		spaces[0].maxx=0;
		mingeneration=0;
		maxgeneration=0;
		if (Config_Title()) {
			layout->title.x=0;
			layout->title.y=0;
		} else {
			layout->title.x=INFINITY;
			layout->title.y=INFINITY;
		}
		person=Database_GetLinked();
		while (Database_GetFather(person)!=none) person=Database_GetFather(person); /*This should not be nessercery?*/
		firstplot=2;
		Layout_TraverseTree(layout,person,2,2,Desk_TRUE,Desk_TRUE,Desk_TRUE,0,Layout_Plot);
		AJWLib_Flex_Free((flex_ptr)&spaces);
		Layout_LayoutMarriages(layout);
		Layout_LayoutLines(layout);
		Layout_LayoutTitle(layout);
	} Desk_Error2_Catch {
		if (spaces) AJWLib_Flex_Free((flex_ptr)&spaces);
		if (layout->person) AJWLib_Flex_Free((flex_ptr)&(layout->person));
		if (layout->marriage) AJWLib_Flex_Free((flex_ptr)&(layout->marriage));
		if (layout->children) AJWLib_Flex_Free((flex_ptr)&(layout->children));
		free(layout);
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
	return layout;
}

layout *Layout_LayoutDescendents(elementptr person,int generations)
{
	layout *layout=NULL;
	AJWLib_Assert(person!=none);
	AJWLib_Assert(spaces==NULL);
	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
	layout->person=NULL;
	layout->marriage=NULL;
	layout->children=NULL;
	layout->numpeople=0;
	layout->nummarriages=0;
	layout->numchildren=0;
	Desk_Error2_Try {
		AJWLib_Flex_Alloc((flex_ptr)&(layout->person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->marriage),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->children),1);
		AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(line));
		spaces[0].minx=0;
		spaces[0].maxx=0;
		mingeneration=0;
		maxgeneration=0;
		numgenerations=generations;
		layout->title.x=INFINITY;
		layout->title.y=INFINITY;
		largenumber=0;
		firstplot=Desk_TRUE;
		Layout_TraverseDescendentTree(layout,person,2,0,Layout_Plot);
		Layout_LayoutMarriages(layout);
		Layout_LayoutLines(layout);
		AJWLib_Flex_Free((flex_ptr)&spaces);
	} Desk_Error2_Catch {
		if (spaces) AJWLib_Flex_Free((flex_ptr)&spaces);
		if (layout->person) AJWLib_Flex_Free((flex_ptr)&(layout->person));
		if (layout->marriage) AJWLib_Flex_Free((flex_ptr)&(layout->marriage));
		if (layout->children) AJWLib_Flex_Free((flex_ptr)&(layout->children));
		free(layout);
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
	return layout;
}

layout *Layout_LayoutAncestors(elementptr person,int generations)
{
	layout *layout=NULL;
	AJWLib_Assert(person!=none);
	AJWLib_Assert(spaces==NULL);
	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
	layout->person=NULL;
	layout->marriage=NULL;
	layout->children=NULL;
	layout->numpeople=0;
	layout->nummarriages=0;
	layout->numchildren=0;
	Desk_Error2_Try {
		AJWLib_Flex_Alloc((flex_ptr)&(layout->person),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->marriage),1);
		AJWLib_Flex_Alloc((flex_ptr)&(layout->children),1);
		AJWLib_Flex_Alloc((flex_ptr)&(spaces),sizeof(line));
		spaces[0].minx=0;
		spaces[0].maxx=0;
		mingeneration=0;
		maxgeneration=0;
		numgenerations=generations;
		layout->title.x=INFINITY;
		layout->title.y=INFINITY;
		largenumber=0;
		firstplot=Desk_TRUE;
		Layout_TraverseAncestorTree(layout,person,2,2,Desk_TRUE,0,Layout_Plot);
		Layout_LayoutMarriages(layout);
		Layout_LayoutLines(layout);
		AJWLib_Flex_Free((flex_ptr)&spaces);
	} Desk_Error2_Catch {
		if (spaces) AJWLib_Flex_Free((flex_ptr)&spaces);
		if (layout->person) AJWLib_Flex_Free((flex_ptr)&(layout->person));
		if (layout->marriage) AJWLib_Flex_Free((flex_ptr)&(layout->marriage));
		if (layout->children) AJWLib_Flex_Free((flex_ptr)&(layout->children));
		free(layout);
		Desk_Error2_ReThrow();
	} Desk_Error2_EndCatch
	return layout;
}

void Layout_Free(layout *layout)
{
	AJWLib_AssertWarning(layout!=NULL);
	if (layout==NULL) return;
	AJWLib_Flex_Free((flex_ptr)&(layout->person));
	AJWLib_Flex_Free((flex_ptr)&(layout->marriage));
	AJWLib_Flex_Free((flex_ptr)&(layout->children));
	Desk_DeskMem_Free(layout);
}

int Layout_GetSize(layout *layout)
{
	int size=0;
	if (layout!=NULL) {
		size+=sizeof(tag)+4*sizeof(int);
		size+=layout->numpeople*sizeof(personlayout);
		size+=layout->nummarriages*sizeof(marriagelayout);
		size+=layout->numchildren*sizeof(childlinelayout);
	}
	return size;
}

void Layout_Save(layout *layout,FILE *file)
{
	tag tag=tag_LAYOUT;
	int size;
	AJWLib_Assert(layout!=NULL);
	AJWLib_Assert(file!=NULL);
	size=Layout_GetSize(layout);
	AJWLib_File_fwrite(&tag,sizeof(tag),1,file);
	AJWLib_File_fwrite(&size,sizeof(int),1,file);
	AJWLib_File_fwrite(&(layout->numpeople),sizeof(layout->numpeople),1,file);
	AJWLib_File_fwrite(layout->person,sizeof(personlayout),layout->numpeople,file);
	AJWLib_File_fwrite(&(layout->nummarriages),sizeof(layout->nummarriages),1,file);
	AJWLib_File_fwrite(layout->marriage,sizeof(marriagelayout),layout->nummarriages,file);
	AJWLib_File_fwrite(&(layout->numchildren),sizeof(layout->numchildren),1,file);
	AJWLib_File_fwrite(layout->children,sizeof(childlinelayout),layout->numchildren,file);
}

layout *Layout_Load(FILE *file)
{
	layout *layout;
	AJWLib_Assert(file!=NULL);
	layout=Desk_DeskMem_Malloc(sizeof(struct layout));
	AJWLib_File_fread(&(layout->numpeople),sizeof(layout->numpeople),1,file);
	AJWLib_Flex_Alloc((flex_ptr)&(layout->person),layout->numpeople*sizeof(personlayout));
	AJWLib_File_fread(layout->person,sizeof(personlayout),layout->numpeople,file);
	AJWLib_File_fread(&(layout->nummarriages),sizeof(layout->nummarriages),1,file);
	AJWLib_Flex_Alloc((flex_ptr)&(layout->marriage),layout->nummarriages*sizeof(marriagelayout));
	AJWLib_File_fread(layout->marriage,sizeof(marriagelayout),layout->nummarriages,file);
	AJWLib_File_fread(&(layout->numchildren),sizeof(layout->numchildren),1,file);
	AJWLib_Flex_Alloc((flex_ptr)&(layout->children),layout->numchildren*sizeof(childlinelayout));
	AJWLib_File_fread(layout->children,sizeof(childlinelayout),layout->numchildren,file);
	return layout;
}
