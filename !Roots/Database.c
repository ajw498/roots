/*
	FT - Database
	© Alex Waugh 1999

	$Id: Database.c,v 1.31 2000/06/26 21:45:07 AJW Exp $

*/


#include "Desk.Window.h"
#include "Desk.Error2.h"
#include "Desk.Event.h"
#include "Desk.Template.h"
#include "Desk.File.h"
#include "Desk.Menu.h"
#include "Desk.DeskMem.h"
#include "Desk.Filing.h"

#include "AJWLib.Assert.h"
#include "AJWLib.Window.h"
#include "AJWLib.Error2.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Flex.h"
#include "AJWLib.File.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Modules.h"
#include "File.h"
#include "Layout.h"
#include "Config.h"


#define editpersonicon_SURNAME 1
#define editpersonicon_FORENAME 7
#define editpersonicon_MIDDLENAMES 8
#define editpersonicon_SEX 9
#define editpersonicon_DOB 10
#define editpersonicon_DOD 11
#define editpersonicon_BIRTHPLACE 12
#define editpersonicon_USER1 13
#define editpersonicon_USER2 14
#define editpersonicon_USER3 15
#define editpersonicon_USERDESC1 21
#define editpersonicon_USERDESC2 16
#define editpersonicon_USERDESC3 17
#define editpersonicon_OK 3
#define editpersonicon_CANCEL 2
#define editpersonicon_SEXMENU 22

#define edittitleicon_TEXT 0
#define edittitleicon_CANCEL 1
#define edittitleicon_OK 2

#define editmarriageicon_PRINCIPAL 2
#define editmarriageicon_SPOUSE 3
#define editmarriageicon_PLACE 6
#define editmarriageicon_PLACEMENU 7
#define editmarriageicon_DATE 8
#define editmarriageicon_DIVORCE 9
#define editmarriageicon_OK 1
#define editmarriageicon_CANCEL 0

#define sexmenu_M 0
#define sexmenu_F 1
#define sexmenu_U 2

static databaseelement *database=NULL;
static elementptr editingperson=none,editingmarriage=none;
static Desk_window_handle editpersonwin,editmarriagewin,edittitlewin;
static Desk_menu_ptr sexmenu;

static void Database_FreeElement(elementptr element)
{
	AJWLib_Assert(database!=NULL);
	database[element].element.freeelement.next=database[0].element.file.freeelement;
	database[0].element.file.freeelement=element;
	database[element].type=element_FREE;
	database[element].selected=Desk_FALSE;
}

void Database_Select(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	database[person].selected=Desk_TRUE;
}

void Database_DeSelect(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	database[person].selected=Desk_FALSE;
}

void Database_DeSelectAll(void)
{
	int i;
	AJWLib_Assert(database!=NULL);
	for (i=0;i<database[0].element.file.numberofelements;i++) {
		if (database[i].type==element_PERSON || database[i].type==element_MARRIAGE) {
			database[i].selected=Desk_FALSE;
		}
	}
}

elementtype Database_AnyoneSelected(void)
{
	int i;
	elementtype selected=element_NONE;
	AJWLib_Assert(database!=NULL);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		if (database[i].selected) {
			if (database[i].type==element_PERSON || database[i].type==element_MARRIAGE) {
				switch (selected) {
					case element_NONE:
						selected=database[i].type;
						break;
					case element_PERSON:
					case element_MARRIAGE:
						selected=element_SELECTION;
						return selected;
				}
			}
		}
	}
	return selected;
}

Desk_bool Database_GetSelect(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	return database[person].selected;
}

void Database_RemoveMarriage(elementptr marriage)
{
	elementptr child;
	/*Remove marriage from chain*/
	if (database[marriage].element.marriage.previous==none) {
		database[database[marriage].element.marriage.principal].element.person.marriage=database[marriage].element.marriage.next;
		if (database[marriage].element.marriage.next) {
			database[database[marriage].element.marriage.next].element.marriage.previous=none;
		}
	} else {
		database[database[marriage].element.marriage.previous].element.marriage.next=database[marriage].element.marriage.next;
		if (database[marriage].element.marriage.next) {
			database[database[marriage].element.marriage.next].element.marriage.previous=database[marriage].element.marriage.previous;
		}
	}
	/*Remove marriage from spouse*/
	database[database[marriage].element.marriage.spouse].element.person.marriage=none;
	/*Unlink any children from this marriage and their siblings*/
	child=database[marriage].element.marriage.leftchild;
	while (child) {
		int temp;
		database[child].element.person.parentsmarriage=none;
		database[child].element.person.siblingsrtol=none;
		temp=database[child].element.person.siblingsltor;
		database[child].element.person.siblingsltor=none;
		child=temp;
	}
	Database_FreeElement(marriage);
}

void Database_UnlinkSelected(layout *layout)
{
	int i;
	elementptr marriage;
	AJWLib_Assert(database!=NULL);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		switch (database[i].type) {
			case element_PERSON:
				/*Get parents marriage*/
				if ((marriage=Database_GetParentsMarriage(i))!=none) {
					/*If selection differs from parents marriage then unlink from siblings and from parents*/
					if (database[marriage].selected!=database[i].selected) {
						/*Remove from siblings chain*/
						if (database[i].element.person.siblingsltor) database[database[i].element.person.siblingsltor].element.person.siblingsrtol=database[i].element.person.siblingsrtol;
						if (database[i].element.person.siblingsrtol) database[database[i].element.person.siblingsrtol].element.person.siblingsltor=database[i].element.person.siblingsltor;
						/*Remove from parents marriage*/
						if (database[marriage].element.marriage.leftchild==i) database[marriage].element.marriage.leftchild=database[i].element.person.siblingsltor;
						if (database[marriage].element.marriage.rightchild==i) database[marriage].element.marriage.rightchild=database[i].element.person.siblingsrtol;
						/*Remove parents*/
						database[i].element.person.parentsmarriage=none;
					}
				}
				break;
			case element_MARRIAGE:
				/*Remove marriage if both spouses and the marriage are not the same selection*/
				if (database[database[i].element.marriage.principal].selected!=database[i].selected || database[database[i].element.marriage.spouse].selected!=database[i].selected) {
					Layout_RemoveMarriage(layout,i);
					Database_RemoveMarriage(i);
					Modules_ChangedStructure();
					break;
			}
		}
	}
}

void Database_DeleteSelected(layout *layout)
{
	int i;
	AJWLib_Assert(database!=NULL);
	Database_UnlinkSelected(layout);
	for (i=1;i<database[0].element.file.numberofelements;i++) {
		if (database[i].selected) {
			switch (database[i].type) {
				case element_PERSON:
					Layout_RemovePerson(layout,i);
					Database_FreeElement(i);
					Modules_ChangedStructure();
					break;
				case element_MARRIAGE:
					Layout_RemoveMarriage(layout,i);
					Database_FreeElement(i);
					Modules_ChangedStructure();
					break;
			}
		}
	}
}

persondata *Database_GetPersonData(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return NULL;
	return &(database[person].element.person.data);
}

elementptr Database_GetParentsMarriage(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.parentsmarriage;
}

elementptr Database_GetMother(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	if (database[person].element.person.parentsmarriage==none) return none;
	return database[database[person].element.person.parentsmarriage].element.marriage.spouse;
}

elementptr Database_GetFather(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	if (database[person].element.person.parentsmarriage==none) return none;
	return database[database[person].element.person.parentsmarriage].element.marriage.principal;
}

elementptr Database_GetSiblingRtoL(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.siblingsrtol;
}

elementptr Database_GetSiblingLtoR(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.siblingsltor;
}

elementptr Database_GetLeftChild(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return none;
	return database[marriage].element.marriage.leftchild;
}

elementptr Database_GetRightChild(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return none;
	return database[marriage].element.marriage.rightchild;
}

elementptr Database_GetSpouseFromMarriage(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return none;
	return database[marriage].element.marriage.spouse;
}

elementptr Database_GetPrincipalFromMarriage(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return none;
	return database[marriage].element.marriage.principal;
}

elementptr Database_GetMarriage(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.marriage;
}

marriagedata *Database_GetMarriageData(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return NULL;
	return &(database[marriage].element.marriage.data);
}

Desk_bool Database_IsFirstMarriage(elementptr marriage)
{
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return Desk_FALSE;
	if (database[marriage].element.marriage.previous==none) return Desk_TRUE;
	return Desk_FALSE;
}

elementptr Database_GetMarriageLtoR(elementptr person)
{
	elementptr marriage;
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	marriage=Database_GetMarriage(person);
	if (marriage==none) return none;
	if (person!=database[marriage].element.marriage.principal) marriage=database[marriage].element.marriage.next;
	if (marriage==none) return none;
	return database[marriage].element.marriage.spouse;
}

elementptr Database_GetMarriageRtoL(elementptr person)
{
	elementptr marriage,marriage2;
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	marriage=Database_GetMarriage(person);
	if (marriage==none) return none;
	if (person==database[marriage].element.marriage.principal) return none;
	marriage2=database[marriage].element.marriage.previous;
	if (marriage2==none) return database[marriage].element.marriage.principal;
	return database[marriage2].element.marriage.spouse;
}

char *Database_GetName(elementptr person)
{
	static char result[256];
	AJWLib_Assert(database!=NULL);
	strcpy(result,"");
	if (strlen(database[person].element.person.data.forename)) {
		strcat(result,database[person].element.person.data.forename);
		strcat(result," ");
	}
	if (strlen(database[person].element.person.data.surname)) {
		strcat(result,database[person].element.person.data.surname);
	}
	return result;
}

char *Database_GetFullName(elementptr person)
{
	static char result[256];
	AJWLib_Assert(database!=NULL);
	strcpy(result,"");
	if (strlen(database[person].element.person.data.forename)) {
		strcat(result,database[person].element.person.data.forename);
		strcat(result," ");
	}
	if (strlen(database[person].element.person.data.middlenames)) {
		strcat(result,database[person].element.person.data.middlenames);
		strcat(result," ");
	}
	if (strlen(database[person].element.person.data.surname)) {
		strcat(result,database[person].element.person.data.surname);
	}
	return result;
}

static elementptr Database_GetFreeElement(void)
{
	elementptr newelement=none;
	AJWLib_Assert(database!=NULL);
	if ((newelement=database[0].element.file.freeelement)==none) {
		AJWLib_Flex_Extend((flex_ptr)&database,sizeof(databaseelement)*(database[0].element.file.numberofelements+1));
		newelement=database[0].element.file.numberofelements++;
	} else {
		database[0].element.file.freeelement=database[newelement].element.freeelement.next;
	}
	return newelement;
}

elementptr Database_Marry(elementptr linked,elementptr unlinked)
{
	elementptr marriage,marriage2,marriage3;
	AJWLib_Assert(database!=NULL);
	/*Check that these two people are not already married together*/
	if ((marriage=Database_GetMarriage(linked))==none) marriage=Database_GetMarriage(unlinked);
	marriage2=marriage;
	while (marriage2) {
		if ((database[marriage2].element.marriage.spouse==linked && database[marriage2].element.marriage.principal==unlinked) || (database[marriage2].element.marriage.spouse==unlinked && database[marriage2].element.marriage.principal==linked)) {
			AJWLib_Error2_HandleMsgs("Error.Married:These two people are already married to each other.");
		}
		marriage2=database[marriage2].element.marriage.previous;
	}
	marriage2=marriage;
	while (marriage2) {
		if ((database[marriage2].element.marriage.spouse==linked && database[marriage2].element.marriage.principal==unlinked) || (database[marriage2].element.marriage.spouse==unlinked && database[marriage2].element.marriage.principal==linked)) {
			AJWLib_Error2_HandleMsgs("Error.Married:These two people are already married to each other.");
		}
		marriage2=database[marriage2].element.marriage.next;
	}
	marriage=Database_GetFreeElement(); /*An error is ok, as we haven't altered the structure yet*/
	database[marriage].type=element_MARRIAGE;
	database[marriage].selected=Desk_FALSE;
	/*Link new marriage into marriage chain*/
	if ((marriage2=Database_GetMarriage(linked))==none) {
		if ((marriage3=Database_GetMarriage(unlinked))==none) {
			/*Neither people are married already*/
			database[marriage].element.marriage.previous=none;
			database[marriage].element.marriage.next=none;
		} else {
			/*unlinked is married already*/
			database[marriage].element.marriage.next=marriage3;
			database[marriage].element.marriage.previous=database[marriage3].element.marriage.previous;
			database[marriage3].element.marriage.previous=marriage;
			if (database[marriage].element.marriage.previous) database[database[marriage].element.marriage.previous].element.marriage.next=marriage;
		}
	} else {
		if ((marriage3=Database_GetMarriage(unlinked))==none) {
			/*linked is married already*/
			database[marriage].element.marriage.next=marriage2;
			database[marriage].element.marriage.previous=database[marriage2].element.marriage.previous;
			database[marriage2].element.marriage.previous=marriage;
			if (database[marriage].element.marriage.previous) database[database[marriage].element.marriage.previous].element.marriage.next=marriage;
		} else {
			/*Both are married already*/
			while (database[marriage2].element.marriage.next!=none) marriage2=database[marriage2].element.marriage.next;
			while (database[marriage3].element.marriage.previous!=none) marriage3=database[marriage3].element.marriage.previous;
			database[marriage2].element.marriage.next=marriage;
			database[marriage].element.marriage.previous=marriage2;
			database[marriage3].element.marriage.previous=marriage;
			database[marriage].element.marriage.next=marriage3;
		}
	}
	database[linked].element.person.marriage=marriage;
	database[unlinked].element.person.marriage=marriage;
	database[marriage].element.marriage.principal=linked;
	database[marriage].element.marriage.spouse=unlinked;
	database[marriage].element.marriage.leftchild=none;
	database[marriage].element.marriage.rightchild=none;
	strcpy(database[marriage].element.marriage.data.place,"");
	strcpy(database[marriage].element.marriage.data.date,"");
	strcpy(database[marriage].element.marriage.data.divorce,"");
	Modules_ChangedStructure();
	return marriage;
}

void Database_AddChild(elementptr marriage,elementptr child)
{
	AJWLib_Assert(database!=NULL);
	database[child].element.person.siblingsltor=none;
	database[child].element.person.siblingsrtol=database[marriage].element.marriage.rightchild;
	database[marriage].element.marriage.rightchild=child;
	if (database[marriage].element.marriage.leftchild==none) database[marriage].element.marriage.leftchild=child;
	if (database[child].element.person.siblingsrtol!=none) database[database[child].element.person.siblingsrtol].element.person.siblingsltor=child;
	database[child].element.person.parentsmarriage=marriage;
	Modules_ChangedStructure();
}

static void Database_EditPerson(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	Desk_Icon_SetText(editpersonwin,editpersonicon_SURNAME,database[person].element.person.data.surname);
	Desk_Icon_SetText(editpersonwin,editpersonicon_FORENAME,database[person].element.person.data.forename);
	Desk_Icon_SetText(editpersonwin,editpersonicon_MIDDLENAMES,database[person].element.person.data.middlenames);
/*	Desk_Icon_SetText(editpersonwin,editpersonicon_TITLE,database[person].element.person.data.title);*/
	switch (database[person].element.person.data.sex) {
		case sex_MALE:
			AJWLib_Msgs_SetText(editpersonwin,editpersonicon_SEX,"Sex.M:");
			break;
		case sex_FEMALE:
			AJWLib_Msgs_SetText(editpersonwin,editpersonicon_SEX,"Sex.F:");
			break;
		case sex_UNKNOWN:
			AJWLib_Msgs_SetText(editpersonwin,editpersonicon_SEX,"Sex.U:");
			break;
	}
	Desk_Icon_SetText(editpersonwin,editpersonicon_DOB,database[person].element.person.data.dob);
	Desk_Icon_SetText(editpersonwin,editpersonicon_DOD,database[person].element.person.data.dod);
	Desk_Icon_SetText(editpersonwin,editpersonicon_BIRTHPLACE,database[person].element.person.data.placeofbirth);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USER1,database[person].element.person.data.userdata[0]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USER2,database[person].element.person.data.userdata[1]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USER3,database[person].element.person.data.userdata[2]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USERDESC1,database[0].element.file.userdesc[0]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USERDESC2,database[0].element.file.userdesc[1]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USERDESC3,database[0].element.file.userdesc[2]);
	Desk_Window_Show(editpersonwin,editingperson ? Desk_open_WHEREVER : Desk_open_CENTERED);
	Desk_Icon_SetCaret(editpersonwin,editpersonicon_SURNAME);
	editingperson=person;
}

char *Database_GetTitle(void)
{
	AJWLib_Assert(database!=NULL);
	return database[0].element.file.filetitle;
}

void Database_EditTitle(void)
{
	Desk_Icon_SetText(edittitlewin,edittitleicon_TEXT,database[0].element.file.filetitle);
	Desk_Window_Show(edittitlewin,Desk_open_CENTEREDUNDERPOINTER);
	Desk_Icon_SetCaret(edittitlewin,edittitleicon_TEXT);
}

static Desk_bool Database_OkEditTitleWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	Desk_Icon_GetText(edittitlewin,edittitleicon_TEXT,database[0].element.file.filetitle);
	if (block->data.mouse.button.data.select) Desk_Window_Hide(edittitlewin);
	Modules_ChangedData(none);
	return Desk_TRUE;
}

static Desk_bool Database_CancelEditTitleWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	Desk_Window_Hide(edittitlewin);
	return Desk_TRUE;
}

static Desk_bool Database_CancelEditWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	editingperson=none;
	Desk_Window_Hide(editpersonwin);
	return Desk_TRUE;
}

static Desk_bool Database_OkEditWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (block->data.mouse.button.data.menu || editingperson==none) return Desk_FALSE;
	Desk_Icon_GetText(editpersonwin,editpersonicon_SURNAME,database[editingperson].element.person.data.surname);
	Desk_Icon_GetText(editpersonwin,editpersonicon_FORENAME,database[editingperson].element.person.data.forename);
	Desk_Icon_GetText(editpersonwin,editpersonicon_MIDDLENAMES,database[editingperson].element.person.data.middlenames);
/*	Desk_Icon_GetText(editpersonwin,editpersonicon_TITLE,database[editingperson].element.person.data.title);*/
	if (!strcmp(Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_SEX),AJWLib_Msgs_TempLookup("Sex.M:"))) database[editingperson].element.person.data.sex=sex_MALE;
	else if (!strcmp(Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_SEX),AJWLib_Msgs_TempLookup("Sex.F:"))) database[editingperson].element.person.data.sex=sex_FEMALE;
	else database[editingperson].element.person.data.sex=sex_UNKNOWN;
	Desk_Icon_GetText(editpersonwin,editpersonicon_DOB,database[editingperson].element.person.data.dob);
	Desk_Icon_GetText(editpersonwin,editpersonicon_DOD,database[editingperson].element.person.data.dod);
	Desk_Icon_GetText(editpersonwin,editpersonicon_BIRTHPLACE,database[editingperson].element.person.data.placeofbirth);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USER1,database[editingperson].element.person.data.userdata[0]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USER2,database[editingperson].element.person.data.userdata[1]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USER3,database[editingperson].element.person.data.userdata[2]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USERDESC1,database[0].element.file.userdesc[0]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USERDESC2,database[0].element.file.userdesc[1]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USERDESC3,database[0].element.file.userdesc[2]);
	Modules_ChangedData(editingperson);
	editingperson=none;
	if (block->data.mouse.button.data.select) Desk_Window_Hide(editpersonwin);
	return Desk_TRUE;
}

static Desk_bool Database_CancelEditMarriageWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	editingmarriage=none;
	Desk_Window_Hide(editmarriagewin);
	return Desk_TRUE;
}

static Desk_bool Database_OkEditMarriageWindow(Desk_event_pollblock *block,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	if (block->data.mouse.button.data.menu || editingmarriage==none) return Desk_FALSE;
	Desk_Icon_GetText(editmarriagewin,editmarriageicon_PLACE,database[editingmarriage].element.marriage.data.place);
	Desk_Icon_GetText(editmarriagewin,editmarriageicon_DATE,database[editingmarriage].element.marriage.data.date);
	Desk_Icon_GetText(editmarriagewin,editmarriageicon_DIVORCE,database[editingmarriage].element.marriage.data.divorce);
	Modules_ChangedData(editingmarriage);
	editingmarriage=none;
	if (block->data.mouse.button.data.select) Desk_Window_Hide(editmarriagewin);
	return Desk_TRUE;
}

static void Database_EditMarriage(elementptr marriage)
{
	char name[256];
	AJWLib_Assert(database!=NULL);
	if (marriage==none) return;
	sprintf(name,"%s %s",database[database[marriage].element.marriage.principal].element.person.data.forename,database[database[marriage].element.marriage.principal].element.person.data.surname);
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_PRINCIPAL,name);
	sprintf(name,"%s %s",database[database[marriage].element.marriage.spouse].element.person.data.forename,database[database[marriage].element.marriage.spouse].element.person.data.surname);
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_SPOUSE,name);
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_PLACE,database[marriage].element.marriage.data.place);
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_DATE,database[marriage].element.marriage.data.date);
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_DIVORCE,database[marriage].element.marriage.data.divorce);
	Desk_Window_Show(editmarriagewin,editingmarriage ? Desk_open_WHEREVER : Desk_open_CENTERED);
	Desk_Icon_SetCaret(editmarriagewin,editmarriageicon_PLACE);
	editingmarriage=marriage;
}

void Database_Edit(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(person!=none);
	switch (database[person].type) {
		case element_PERSON:
			Database_EditPerson(person);
			break;
		case element_MARRIAGE:
			Database_EditMarriage(person);
			break;
	}
}

void Database_Delete(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	Database_FreeElement(person);
	Modules_ChangedStructure();
}

elementptr Database_Add(void)
{
	elementptr newperson;
	AJWLib_Assert(database!=NULL);
	newperson=Database_GetFreeElement(); /*Ok if an error is thrown*/
	database[newperson].type=element_PERSON;
	database[newperson].element.person.parentsmarriage=none;
	database[newperson].element.person.siblingsrtol=none;
	database[newperson].element.person.siblingsltor=none;
	database[newperson].element.person.marriage=none;
	sprintf(database[newperson].element.person.data.surname,"NewPerson%d",database[0].element.file.newpersonnumber++);
	strcpy(database[newperson].element.person.data.forename,"");
	strcpy(database[newperson].element.person.data.middlenames,"");
	strcpy(database[newperson].element.person.data.title,"");
	database[newperson].element.person.data.sex=sex_UNKNOWN;
	strcpy(database[newperson].element.person.data.dob,"");
	strcpy(database[newperson].element.person.data.dod,"");
	strcpy(database[newperson].element.person.data.placeofbirth,"");
	strcpy(database[newperson].element.person.data.userdata[0],"");
	strcpy(database[newperson].element.person.data.userdata[1],"");
	strcpy(database[newperson].element.person.data.userdata[2],"");
	Modules_ChangedStructure();
	return newperson;
}

static void Database_SexMenuClick(int entry,void *ref)
{
	Desk_UNUSED(ref);
	AJWLib_Assert(database!=NULL);
	Desk_Icon_SetText(editpersonwin,editpersonicon_SEX,Desk_Menu_GetText(sexmenu,entry));
}

int Database_GetNumPeople(void)
{
	int numpeople=0,i;
	AJWLib_Assert(database!=NULL);
	for (i=0;i<database[0].element.file.numberofelements;i++) {
		if (database[i].type==element_PERSON) numpeople++;
	}
	return numpeople;
}

int Database_GetSize(void)
{
	if (database) return sizeof(tag)+sizeof(int)+database[0].element.file.numberofelements*sizeof(databaseelement);
	return 0;
}

void Database_Save(FILE *file)
{
	tag tag=tag_DATABASE;
	int size;
	AJWLib_Assert(database!=NULL);
	AJWLib_Assert(file!=NULL);
/*Remove free elements?*/
/*Consistency check?*/
	size=Database_GetSize();
	AJWLib_File_fwrite(&tag,sizeof(tag),1,file);
	AJWLib_File_fwrite(&size,sizeof(int),1,file);
	AJWLib_File_fwrite(database,sizeof(databaseelement),database[0].element.file.numberofelements,file);
}

void Database_Load(FILE *file)
{
	AJWLib_Assert(database==NULL);
	AJWLib_Assert(file!=NULL);
/*Remove free elements?*/
/*Consistency check?*/
	AJWLib_Flex_Alloc((flex_ptr)&database,sizeof(databaseelement));
	AJWLib_File_fread(database,sizeof(databaseelement),1,file);
	AJWLib_Flex_Extend((flex_ptr)&database,sizeof(databaseelement)*database[0].element.file.numberofelements);
	AJWLib_File_fread(database+1,sizeof(databaseelement),database[0].element.file.numberofelements-1,file);
	Modules_ChangedStructure();
}

char *Database_GetUserDesc(int num)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<3);
	return database[0].element.file.userdesc[num];
}

void Database_SetUserDesc(int num,char *desc)
{
	AJWLib_Assert(num>=0);
	AJWLib_Assert(num<3);
	strcpy(database[0].element.file.userdesc[num],desc);
}

void Database_New(void)
{
	AJWLib_Assert(database==NULL);
	AJWLib_Flex_Alloc((flex_ptr)&database,sizeof(databaseelement));
	database[0].type=element_FILE;
	database[0].element.file.numberofelements=1;
	database[0].element.file.newpersonnumber=1;
	database[0].element.file.reserved0=0;
	database[0].element.file.reserved1=0;
	database[0].element.file.freeelement=0;
	strcpy(database[0].element.file.filetitle,AJWLib_Msgs_TempLookup("Tree.Title:Title"));
	strcpy(database[0].element.file.userdesc[0],AJWLib_Msgs_TempLookup("User.Desc1:"));
	strcpy(database[0].element.file.userdesc[1],AJWLib_Msgs_TempLookup("User.Desc2:"));
	strcpy(database[0].element.file.userdesc[2],AJWLib_Msgs_TempLookup("User.Desc3:"));
}

void Database_StopEditing(void)
{
	Desk_Window_Hide(edittitlewin);
	Desk_Window_Hide(editpersonwin);
	Desk_Window_Hide(editmarriagewin);
	editingperson=none;
	editingmarriage=none;
}

void Database_Remove(void)
{
	AJWLib_AssertWarning(database!=NULL);
	if (database) AJWLib_Flex_Free((flex_ptr)&database);
	Database_StopEditing();
}

void Database_Init(void)
{
	AJWLib_Assert(database==NULL);
	editpersonwin=Desk_Window_Create("EditPerson",Desk_template_TITLEMIN);
	editmarriagewin=Desk_Window_Create("EditMarriage",Desk_template_TITLEMIN);
	edittitlewin=Desk_Window_Create("EditTitle",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,editpersonwin,editpersonicon_OK,Database_OkEditWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editpersonwin,editpersonicon_CANCEL,Database_CancelEditWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editmarriagewin,editmarriageicon_OK,Database_OkEditMarriageWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editmarriagewin,editmarriageicon_CANCEL,Database_CancelEditMarriageWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,edittitlewin,edittitleicon_OK,Database_OkEditTitleWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,edittitlewin,edittitleicon_CANCEL,Database_CancelEditTitleWindow,NULL);
	AJWLib_Window_KeyHandler(edittitlewin,edittitleicon_OK,Database_OkEditTitleWindow,edittitleicon_CANCEL,Database_CancelEditTitleWindow,NULL);
	AJWLib_Window_KeyHandler(editpersonwin,editpersonicon_OK,Database_OkEditWindow, editpersonicon_CANCEL,Database_CancelEditWindow,NULL);
	AJWLib_Window_KeyHandler(editmarriagewin,editmarriageicon_OK,Database_OkEditMarriageWindow,editmarriageicon_CANCEL,Database_CancelEditMarriageWindow,NULL);
	sexmenu=AJWLib_Menu_CreateFromMsgs("Title.Sex:","Menu.Sex:M,F,U",Database_SexMenuClick,NULL);
	AJWLib_Menu_AttachPopup(editpersonwin,editpersonicon_SEXMENU,editpersonicon_SEX,sexmenu,Desk_button_MENU | Desk_button_SELECT);
}
