/*
	FT - Database
	� Alex Waugh 1999

	$Id: Database.c,v 1.19 2000/02/26 17:02:56 uid1 Exp $

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
#include "Config.h"


#define editpersonicon_SURNAME 1
#define editpersonicon_FORENAME 7
#define editpersonicon_MIDDLENAMES 8
/*#define editpersonicon_TITLE 11*/
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
/*#define editpersonicon_SURNAMEMENU 2*/
/*#define editpersonicon_TITLEMENU 13*/
#define editpersonicon_SEXMENU 22

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

#define titlemenu_Mr 0
#define titlemenu_Mrs 1
#define titlemenu_Miss 2
#define titlemenu_Ms 3
#define titlemenu_Dr 4

static databaseelement *database=NULL;
static elementptr editingperson=none,editingmarriage=none;
static Desk_window_handle editpersonwin,editmarriagewin;
Desk_menu_ptr sexmenu,titlemenu;

void Database_FudgeLinked(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	database[0].element.file.linkedpeople=person;
}

void Database_FudgeMarriageSwap(elementptr marriage)
{
	elementptr temp;
	AJWLib_Assert(database!=NULL);
	temp=database[marriage].element.marriage.principal;
	database[marriage].element.marriage.principal=database[marriage].element.marriage.spouse;
	database[marriage].element.marriage.spouse=temp;
}

Desk_bool Database_IsUnlinked(elementptr person)
{
	elementptr unlinkedpeople;
	AJWLib_Assert(database!=NULL);
	unlinkedpeople=Database_GetUnlinked(none);
	while (unlinkedpeople!=none) {
		if (unlinkedpeople==person) return Desk_TRUE;
		unlinkedpeople=Database_GetUnlinked(unlinkedpeople);
	}
	return Desk_FALSE;
}

persondata *Database_GetPersonData(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return NULL;
	return &(database[person].element.person.data);
}

elementptr Database_GetUnlinked(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return database[0].element.file.unlinkedpeople;
	return database[person].element.person.nextunlinked;
}

elementptr Database_GetLinked(void)
{
	AJWLib_Assert(database!=NULL);
	return database[0].element.file.linkedpeople;
}

elementptr Database_GetMother(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.mother;
}

elementptr Database_GetFather(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (person==none) return none;
	return database[person].element.person.father;
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

char *Database_GetTitledName(elementptr person)
{
	static char result[256];
	AJWLib_Assert(database!=NULL);
	strcpy(result,"");
	if (strlen(database[person].element.person.data.title)) {
		strcat(result,database[person].element.person.data.title);
		strcat(result," ");
	}
	if (strlen(database[person].element.person.data.forename)) {
		strcat(result,database[person].element.person.data.forename);
		strcat(result," ");
	}
	if (strlen(database[person].element.person.data.surname)) {
		strcat(result,database[person].element.person.data.surname);
	}
	return result;
}

char *Database_GetTitledFullName(elementptr person)
{
	static char result[256];
	AJWLib_Assert(database!=NULL);
	strcpy(result,"");
	if (strlen(database[person].element.person.data.title)) {
		strcat(result,database[person].element.person.data.title);
		strcat(result," ");
	}
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

elementptr Database_GetFreeElement(void)
{
	elementptr newelement=none;
	AJWLib_Assert(database!=NULL);
	if ((newelement=database[0].element.file.freeelement)==none) {
		AJWLib_Flex_Extend((flex_ptr)&database,sizeof(databaseelement)*(database[0].element.file.numberofelements+1));
		/*Errors?*/
		newelement=database[0].element.file.numberofelements++;
	} else {
		database[0].element.file.freeelement=database[newelement].element.freeelement.next;
	}
	return newelement;
}

void Database_FreeElement(elementptr element)
{
	AJWLib_Assert(database!=NULL);
	database[element].element.freeelement.next=database[0].element.file.freeelement;
	database[0].element.file.freeelement=element;
	database[element].type=element_FREE;
}

void Database_UnlinkPerson(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	database[person].element.person.nextunlinked=database[0].element.file.unlinkedpeople;
	database[0].element.file.unlinkedpeople=person;
	database[person].element.person.father=none;
	database[person].element.person.mother=none;
	database[person].element.person.marriage=none;
	database[person].element.person.siblingsltor=none;
	database[person].element.person.siblingsrtol=none;
}

void Database_RemoveSpouse(elementptr person)
{
	elementptr principal,marriage,spouse;
	AJWLib_Assert(database!=NULL);
	marriage=Database_GetMarriage(person);
	if (marriage==none) return;
	if (Database_GetLeftChild(marriage)!=none) return;
	if (Database_GetMother(person)!=none) return;
	principal=Database_GetPrincipalFromMarriage(marriage);
	spouse=Database_GetSpouseFromMarriage(marriage);
	if (database[marriage].element.marriage.previous) database[database[marriage].element.marriage.previous].element.marriage.next=database[marriage].element.marriage.next;
	if (database[marriage].element.marriage.next) database[database[marriage].element.marriage.next].element.marriage.previous=database[marriage].element.marriage.previous;
	if (database[marriage].element.marriage.previous==none && database[marriage].element.marriage.next==none) database[principal].element.person.marriage=none;
	database[spouse].element.person.marriage=none;
	Database_FreeElement(marriage);
	if (database[0].element.file.linkedpeople==person) {
		if (principal==person) database[0].element.file.linkedpeople=spouse; else database[0].element.file.linkedpeople=principal;
	}
	database[person].element.person.marriage=none;
	Database_UnlinkPerson(person);
	Modules_ChangedStructure();
}

void Database_RemoveParents(elementptr person)
{
	elementptr spouse,principal,marriage,child;
	AJWLib_Assert(database!=NULL);
	marriage=Database_GetMarriage(person);
	if (marriage==none) return;
	if (database[marriage].element.marriage.next!=none) return;
	if (database[marriage].element.marriage.previous!=none) return;
	spouse=Database_GetSpouseFromMarriage(marriage);
	principal=Database_GetPrincipalFromMarriage(marriage);
	child=Database_GetLeftChild(marriage);
	if (child==none) return;
	if (Database_GetMother(principal)!=none) return;
	if (Database_GetMother(spouse)!=none) return;
	if (Database_GetSiblingLtoR(child)!=none) return;
    Database_FreeElement(marriage);
	database[child].element.person.father=none;
	database[child].element.person.mother=none;
	Database_UnlinkPerson(principal);
	Database_UnlinkPerson(spouse);
	if (database[0].element.file.linkedpeople==principal) database[0].element.file.linkedpeople=child;
	if (database[0].element.file.linkedpeople==spouse) database[0].element.file.linkedpeople=child;
	Modules_ChangedStructure();
}

void Database_RemoveChild(elementptr child)
{
	elementptr leftsibling,rightsibling,marriage;
	AJWLib_Assert(database!=NULL);
	if (child==none) return;
	if (Database_GetMarriage(child)) return;
	leftsibling=Database_GetSiblingRtoL(child);
	rightsibling=Database_GetSiblingLtoR(child);
	marriage=Database_GetMarriage(Database_GetMother(child));
	if (marriage) {
		if (rightsibling==none) database[marriage].element.marriage.rightchild=leftsibling; else database[rightsibling].element.person.siblingsrtol=leftsibling;
		if (leftsibling==none) database[marriage].element.marriage.leftchild=rightsibling; else database[leftsibling].element.person.siblingsltor=rightsibling;
	}
	Database_UnlinkPerson(child);
	if (database[0].element.file.linkedpeople==child) database[0].element.file.linkedpeople=Database_GetMother(child);
	Modules_ChangedStructure();
}

void Database_UnlinkUnlinkedPerson(elementptr person)
{
	int i;
	AJWLib_Assert(database!=NULL);
	i=database[0].element.file.unlinkedpeople;
	database[person].element.person.father=none;
	database[person].element.person.mother=none;
	database[person].element.person.siblingsrtol=none;
	database[person].element.person.siblingsltor=none;
	database[person].element.person.marriage=none;
	if (i==person) {
		database[0].element.file.unlinkedpeople=database[person].element.person.nextunlinked;
	} else {
		while (i!=none && database[i].element.person.nextunlinked!=person) i=database[i].element.person.nextunlinked;
		if (i==none) return;
		database[i].element.person.nextunlinked=database[person].element.person.nextunlinked;
	}
}

void Database_Marry(elementptr linked,elementptr unlinked)
{
	elementptr marriage;
	AJWLib_Assert(database!=NULL);
	if (Database_IsUnlinked(linked)) return;
	if (!Database_IsUnlinked(unlinked)) return;
	if (Database_GetMarriage(linked) && Database_GetPrincipalFromMarriage(Database_GetMarriage(linked))!=linked) {
		Desk_Error2_HandleText("You are not marrying the principal person.");
		/*swap principal and spouse if only one marriage*/
		return;
	}
	marriage=Database_GetFreeElement();
	database[marriage].type=element_MARRIAGE;
	Database_UnlinkUnlinkedPerson(unlinked);
	if (Database_GetMarriage(linked)==none) {
		database[linked].element.person.marriage=marriage;
		database[marriage].element.marriage.previous=none;
	} else {
		elementptr next;
		next=Database_GetMarriage(linked);
		while (database[next].element.marriage.next!=none) next=database[next].element.marriage.next;
		database[next].element.marriage.next=marriage;
		database[marriage].element.marriage.previous=next;
	}
	database[unlinked].element.person.marriage=marriage;
	database[marriage].element.marriage.principal=linked;
	database[marriage].element.marriage.spouse=unlinked;
	database[marriage].element.marriage.leftchild=none;
	database[marriage].element.marriage.rightchild=none;
	strcpy(database[marriage].element.marriage.data.place,"");
	strcpy(database[marriage].element.marriage.data.date,"");
	strcpy(database[marriage].element.marriage.data.divorce,"");
	database[marriage].element.marriage.next=none;
	Modules_ChangedStructure();
}

void Database_AddParents(elementptr child,elementptr mother,elementptr father)
{
	elementptr marriage;
	AJWLib_Assert(database!=NULL);
	Database_UnlinkUnlinkedPerson(mother);
	Database_Marry(mother,father);
	marriage=Database_GetMarriage(mother);
	database[marriage].element.marriage.leftchild=child;
	database[marriage].element.marriage.rightchild=child;
	database[child].element.person.mother=Database_GetSpouseFromMarriage(marriage);
	database[child].element.person.father=Database_GetPrincipalFromMarriage(marriage);
	Modules_ChangedStructure();
}

void Database_AddChild(elementptr marriage,elementptr child)
{
	AJWLib_Assert(database!=NULL);
	Database_UnlinkUnlinkedPerson(child);
	database[child].element.person.siblingsltor=none;
	database[child].element.person.siblingsrtol=database[marriage].element.marriage.rightchild;
	database[marriage].element.marriage.rightchild=child;
	if (database[marriage].element.marriage.leftchild==none) database[marriage].element.marriage.leftchild=child;
	if (database[child].element.person.siblingsrtol!=none) database[database[child].element.person.siblingsrtol].element.person.siblingsltor=child;
	database[child].element.person.mother=database[marriage].element.marriage.spouse;
	database[child].element.person.father=database[marriage].element.marriage.principal;
	Modules_ChangedStructure();
}

void Database_LinkPerson(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	if (database[0].element.file.linkedpeople!=none) return;
	Database_UnlinkUnlinkedPerson(person);
	database[0].element.file.linkedpeople=person;
	Modules_ChangedStructure();
}

void Database_EditPerson(elementptr person)
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

Desk_bool Database_CancelEditWindow(Desk_event_pollblock *block,void *ref)
{
	AJWLib_Assert(database!=NULL);
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	editingperson=none;
	Desk_Window_Hide(editpersonwin);
	return Desk_TRUE;
}

Desk_bool Database_OkEditWindow(Desk_event_pollblock *block,void *ref)
{
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
	editingperson=none;
	if (block->data.mouse.button.data.select) Desk_Window_Hide(editpersonwin);
	return Desk_TRUE;
}

Desk_bool Database_CancelEditMarriageWindow(Desk_event_pollblock *block,void *ref)
{
	AJWLib_Assert(database!=NULL);
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	editingmarriage=none;
	Desk_Window_Hide(editmarriagewin);
	return Desk_TRUE;
}

Desk_bool Database_OkEditMarriageWindow(Desk_event_pollblock *block,void *ref)
{
	AJWLib_Assert(database!=NULL);
	if (block->data.mouse.button.data.menu || editingmarriage==none) return Desk_FALSE;
	Desk_Icon_GetText(editmarriagewin,editmarriageicon_PLACE,database[editingmarriage].element.marriage.data.place);
	Desk_Icon_GetText(editmarriagewin,editmarriageicon_DATE,database[editingmarriage].element.marriage.data.date);
	Desk_Icon_GetText(editmarriagewin,editmarriageicon_DIVORCE,database[editingmarriage].element.marriage.data.divorce);
	editingmarriage=none;
	if (block->data.mouse.button.data.select) Desk_Window_Hide(editmarriagewin);
	return Desk_TRUE;
}

void Database_EditMarriage(elementptr marriage)
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

void Database_Delete(elementptr person)
{
	AJWLib_Assert(database!=NULL);
	Database_UnlinkUnlinkedPerson(person);
	Database_FreeElement(person);
	Modules_ChangedStructure();
}

void Database_Add(void)
{
	elementptr newperson;
	AJWLib_Assert(database!=NULL);
	newperson=Database_GetFreeElement();
	database[newperson].type=element_PERSON;
	database[newperson].element.person.nextunlinked=database[0].element.file.unlinkedpeople;
	database[0].element.file.unlinkedpeople=newperson;
	database[newperson].element.person.mother=none;
	database[newperson].element.person.father=none;
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
}

void Database_SexMenuClick(int entry,void *ref)
{
	AJWLib_Assert(database!=NULL);
	Desk_Icon_SetText(editpersonwin,editpersonicon_SEX,Desk_Menu_GetText(sexmenu,entry));
}

/*void Database_TitleMenuClick(int entry,void *ref)
{
	AJWLib_Assert(database!=NULL);
	Desk_Icon_SetText(editpersonwin,editpersonicon_TITLE,Desk_Menu_GetText(titlemenu,entry));
} */

int Database_GetNumPeople(void)
{
	int numpeople=0,i;
	AJWLib_Assert(database!=NULL);
	for (i=0;i<database[0].element.file.numberofelements;i++) {
		if (database[i].type==element_PERSON) numpeople++;
	}
	return numpeople;
}

char *Database_GetTitle(void)
{
	AJWLib_Assert(database!=NULL);
	return database[0].element.file.filetitle;
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

void Database_New(void)
{
	AJWLib_Assert(database==NULL);
	AJWLib_Flex_Alloc((flex_ptr)&database,sizeof(databaseelement));
	database[0].type=element_FILE;
	database[0].element.file.numberofelements=1;
	database[0].element.file.newpersonnumber=1;
	database[0].element.file.unlinkedpeople=0;
	database[0].element.file.linkedpeople=0;
	database[0].element.file.freeelement=0;
	strcpy(database[0].element.file.filetitle,AJWLib_Msgs_TempLookup("Tree.Title:"));
	strcpy(database[0].element.file.userdesc[0],Config_UserDesc(1));
	strcpy(database[0].element.file.userdesc[1],Config_UserDesc(2));
	strcpy(database[0].element.file.userdesc[2],Config_UserDesc(3));
}

void Database_Remove(void)
{
	AJWLib_Assert(database!=NULL);
	AJWLib_Flex_Free((flex_ptr)&database);
	/*Close any windows if open?*/
}

void Database_Init(void)
{
	AJWLib_Assert(database==NULL);
	editpersonwin=Desk_Window_Create("EditPerson",Desk_template_TITLEMIN);
	editmarriagewin=Desk_Window_Create("EditMarriage",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,editpersonwin,editpersonicon_OK,Database_OkEditWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editpersonwin,editpersonicon_CANCEL,Database_CancelEditWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editmarriagewin,editmarriageicon_OK,Database_OkEditMarriageWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editmarriagewin,editmarriageicon_CANCEL,Database_CancelEditMarriageWindow,NULL);
	/*Register handlers*/
	sexmenu=AJWLib_Menu_CreateFromMsgs("Title.Sex:","Menu.Sex:M,F,U",Database_SexMenuClick,NULL);
	AJWLib_Menu_AttachPopup(editpersonwin,editpersonicon_SEXMENU,editpersonicon_SEX,sexmenu,Desk_button_MENU | Desk_button_SELECT);
/*	titlemenu=AJWLib_Menu_CreateFromMsgs("Title.Title:","Menu.Title:",Database_TitleMenuClick,NULL);
	AJWLib_Menu_AttachPopup(editpersonwin,editpersonicon_TITLEMENU,editpersonicon_TITLE,titlemenu,Desk_button_MENU | Desk_button_SELECT);*/
}
