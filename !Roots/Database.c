/*
	FT - Database
	© Alex Waugh 1999

	$Log: Database.c,v $
	Revision 1.6  1999/10/11 19:28:59  AJW
	Changed Database_Add and _Init to return void - they now use Error2

	Revision 1.5  1999/10/11 17:56:04  AJW
	Handles Edit marriage window now

	Revision 1.4  1999/10/10 20:53:46  AJW
	Modified to use Desk

	Revision 1.3  1999/10/02 18:04:43  AJW
	Fixed Database_RemoveParents

	Revision 1.2  1999/10/02 17:49:16  AJW
	Fixed Database_RemoveSpouse

	Revision 1.1  1999/09/27 15:32:05  AJW
	Initial revision

	
*/

/*	Includes  */

#include "Desk.Window.h"
#include "Desk.Error2.h"
#include "Desk.Event.h"
#include "Desk.Template.h"
#include "Desk.File.h"
#include "Desk.Menu.h"
#include "Desk.DeskMem.h"
#include "Desk.Filing.h"

#include "AJWLib.Window.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Menu.h"

#include "AJWLib.Flex.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Database.h"
#include "Modules.h"


/*	Macros  */

#define FILEID "FT"
#define VERSIONNUM 001

#define editpersonicon_SURNAME 1
#define editpersonicon_FORENAME 9
#define editpersonicon_MIDDLENAMES 10
#define editpersonicon_TITLE 11
#define editpersonicon_SEX 12
#define editpersonicon_DOB 0
#define editpersonicon_DOD 0
#define editpersonicon_BIRTHPLACE 21
#define editpersonicon_USER1 22
#define editpersonicon_USER2 23
#define editpersonicon_USER3 24
#define editpersonicon_USERDESC1 18
#define editpersonicon_USERDESC2 19
#define editpersonicon_USERDESC3 20
#define editpersonicon_OK 4
#define editpersonicon_CANCEL 3
#define editpersonicon_SURNAMEMENU 2
#define editpersonicon_TITLEMENU 13
#define editpersonicon_SEXMENU 14

#define editmarriageicon_PRINCIPAL 2
#define editmarriageicon_SPOUSE 3
#define editmarriageicon_PLACE 6
#define editmarriageicon_PLACEMENU 7
#define editmarriageicon_DATE -99
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


static element *database;
static elementptr editingperson=none,editingmarriage=none;
static Desk_window_handle editpersonwin,editmarriagewin;
Desk_menu_ptr sexmenu,titlemenu;

void Database_FudgeLinked(elementptr person)
{
	database[0].file.linkedpeople=person;
}

void Database_FudgeMarriageSwap(elementptr marriage)
{
	elementptr temp=database[marriage].marriage.principal;
	database[marriage].marriage.principal=database[marriage].marriage.spouse;
	database[marriage].marriage.spouse=temp;
}

Desk_bool Database_IsUnlinked(elementptr person)
{
	elementptr unlinkedpeople=Database_GetUnlinked(none);
	while (unlinkedpeople!=none) {
		if (unlinkedpeople==person) return Desk_TRUE;
		unlinkedpeople=Database_GetUnlinked(unlinkedpeople);
	}
	return Desk_FALSE;
}

persondata *Database_GetPersonData(elementptr person)
{
	if (person==none) return NULL;
	return &(database[person].person.data);
}

elementptr Database_GetUnlinked(elementptr person)
{
	if (person==none) return database[0].file.unlinkedpeople;
	return database[person].person.nextunlinked;
}

elementptr Database_GetLinked(void)
{
	return database[0].file.linkedpeople;
}

elementptr Database_GetMother(elementptr person)
{
	if (person==none) return none;
	return database[person].person.mother;
}

elementptr Database_GetFather(elementptr person)
{
	if (person==none) return none;
	return database[person].person.father;
}

elementptr Database_GetSiblingRtoL(elementptr person)
{
	if (person==none) return none;
	return database[person].person.siblingsrtol;
}

elementptr Database_GetSiblingLtoR(elementptr person)
{
	if (person==none) return none;
	return database[person].person.siblingsltor;
}

elementptr Database_GetLeftChild(elementptr marriage)
{
	if (marriage==none) return none;
	return database[marriage].marriage.leftchild;
}

elementptr Database_GetRightChild(elementptr marriage)
{
	if (marriage==none) return none;
	return database[marriage].marriage.rightchild;
}

elementptr Database_GetSpouseFromMarriage(elementptr marriage)
{
	if (marriage==none) return none;
	return database[marriage].marriage.spouse;
}

elementptr Database_GetPrincipalFromMarriage(elementptr marriage)
{
	if (marriage==none) return none;
	return database[marriage].marriage.principal;
}

elementptr Database_GetMarriage(elementptr person)
{
	if (person==none) return none;
	return database[person].person.marriage;
}

marriagedata *Database_GetMarriageData(elementptr marriage)
{
	if (marriage==none) return NULL;
	return &(database[marriage].marriage.data);
}

Desk_bool Database_IsFirstMarriage(elementptr marriage)
{
	if (marriage==none) return Desk_FALSE;
	if (database[marriage].marriage.previous==none) return Desk_TRUE;
	return Desk_FALSE;
}

elementptr Database_GetMarriageLtoR(elementptr person)
{
	elementptr marriage;
	if (person==none) return none;
	marriage=Database_GetMarriage(person);
	if (marriage==none) return none;
	if (person!=database[marriage].marriage.principal) marriage=database[marriage].marriage.next;
	if (marriage==none) return none;
	return database[marriage].marriage.spouse;
}

elementptr Database_GetMarriageRtoL(elementptr person)
{
	elementptr marriage,marriage2;
	if (person==none) return none;
	marriage=Database_GetMarriage(person);
	if (marriage==none) return none;
	if (person==database[marriage].marriage.principal) return none;
	marriage2=database[marriage].marriage.previous;
	if (marriage2==none) return database[marriage].marriage.principal;
	return database[marriage2].marriage.spouse;
}

void Database_UnlinkPerson(elementptr person)
{
	database[person].person.nextunlinked=database[0].file.unlinkedpeople;
	database[0].file.unlinkedpeople=person;
	database[person].person.father=none;
	database[person].person.mother=none;
	database[person].person.marriage=none;
	database[person].person.siblingsltor=none;
	database[person].person.siblingsrtol=none;
}

void Database_RemoveSpouse(elementptr person)
{
	elementptr principal,marriage,spouse;
	marriage=Database_GetMarriage(person);
	if (marriage==none) return;
	if (Database_GetLeftChild(marriage)!=none) return;
	if (Database_GetMother(person)!=none) return;
	principal=Database_GetPrincipalFromMarriage(marriage);
	spouse=Database_GetSpouseFromMarriage(marriage);
	if (database[marriage].marriage.previous) database[database[marriage].marriage.previous].marriage.next=database[marriage].marriage.next;
	if (database[marriage].marriage.next) database[database[marriage].marriage.next].marriage.previous=database[marriage].marriage.previous;
	if (database[marriage].marriage.previous==none && database[marriage].marriage.next==none) database[principal].person.marriage=none;
	database[spouse].person.marriage=none;
	database[marriage].freeelement.next=database[0].file.freeelement;
	database[0].file.freeelement=marriage;
	if (database[0].file.linkedpeople==person) {
		if (principal==person) database[0].file.linkedpeople=spouse; else database[0].file.linkedpeople=principal;
	}
	database[person].person.marriage=none;
	database[person].person.nextunlinked=database[0].file.unlinkedpeople;
	database[0].file.unlinkedpeople=person;
	Modules_ChangedStructure();
}

void Database_RemoveParents(elementptr person)
{
	elementptr spouse,principal,marriage,child;
	marriage=Database_GetMarriage(person);
	if (marriage==none) return;
	if (database[marriage].marriage.next!=none) return;
	if (database[marriage].marriage.previous!=none) return;
	spouse=Database_GetSpouseFromMarriage(marriage);
	principal=Database_GetPrincipalFromMarriage(marriage);
	child=Database_GetLeftChild(marriage);
	if (child==none) return;
	if (Database_GetMother(principal)!=none) return;
	if (Database_GetMother(spouse)!=none) return;
	if (Database_GetSiblingLtoR(child)!=none) return;
	database[marriage].freeelement.next=database[0].file.freeelement;
	database[0].file.freeelement=marriage;
	database[child].person.father=none;
	database[child].person.mother=none;
	Database_UnlinkPerson(principal);
	Database_UnlinkPerson(spouse);
	if (database[0].file.linkedpeople==principal) database[0].file.linkedpeople=child;
	if (database[0].file.linkedpeople==spouse) database[0].file.linkedpeople=child;
	Modules_ChangedStructure();
}

void Database_RemoveChild(elementptr child)
{
	elementptr leftsibling,rightsibling,marriage;
	if (child==none) return;
	if (Database_GetMarriage(child)) return;
	leftsibling=Database_GetSiblingRtoL(child);
	rightsibling=Database_GetSiblingLtoR(child);
	marriage=Database_GetMarriage(Database_GetMother(child));
	if (marriage) {
		if (rightsibling==none) database[marriage].marriage.rightchild=leftsibling; else database[rightsibling].person.siblingsrtol=leftsibling;
		if (leftsibling==none) database[marriage].marriage.leftchild=rightsibling; else database[leftsibling].person.siblingsltor=rightsibling;
	}
	database[child].person.nextunlinked=database[0].file.unlinkedpeople;
	database[0].file.unlinkedpeople=child;
	if (database[0].file.linkedpeople==child) database[0].file.linkedpeople=Database_GetMother(child);
	Modules_ChangedStructure();
}

void Database_UnlinkUnlinkedPerson(elementptr person)
{
	int i;
	i=database[0].file.unlinkedpeople;
	database[person].person.father=none;
	database[person].person.mother=none;
	database[person].person.siblingsrtol=none;
	database[person].person.siblingsltor=none;
	database[person].person.marriage=none;
	if (i==person) {
		database[0].file.unlinkedpeople=database[person].person.nextunlinked;
	} else {
		while (i!=none && database[i].person.nextunlinked!=person) i=database[i].person.nextunlinked;
		if (i==none) return;
		database[i].person.nextunlinked=database[person].person.nextunlinked;
	}
}

void Database_Marry(elementptr linked,elementptr unlinked)
{
	elementptr marriage;
	if (Database_IsUnlinked(linked)) return;
	if (!Database_IsUnlinked(unlinked)) return;
	if (Database_GetMarriage(linked) && Database_GetPrincipalFromMarriage(Database_GetMarriage(linked))!=linked) {
		Desk_Error2_HandleText("You are not marrying the principal person.");
		/*swap principal and spouse if only one marriage*/
		return;
	}
	if ((marriage=database[0].file.freeelement)==none) {
		if (!AJWLib_Flex_Extend((flex_ptr)&database,sizeof(element)*(database[0].file.numberofelements+1))) Desk_Error2_HandleText("Error.NoMem:Out of memory");
		marriage=database[0].file.numberofelements++;
	} else {
		database[0].file.freeelement=database[marriage].freeelement.next;
	}
	Database_UnlinkUnlinkedPerson(unlinked);
	if (Database_GetMarriage(linked)==none) {
		database[linked].person.marriage=marriage;
		database[marriage].marriage.previous=none;
	} else {
		elementptr next;
		next=Database_GetMarriage(linked);
		while (database[next].marriage.next!=none) next=database[next].marriage.next;
		database[next].marriage.next=marriage;
		database[marriage].marriage.previous=next;
	}
	database[unlinked].person.marriage=marriage;
	database[marriage].marriage.principal=linked;
	database[marriage].marriage.spouse=unlinked;
	database[marriage].marriage.leftchild=none;
	database[marriage].marriage.rightchild=none;
	strcpy(database[marriage].marriage.data.place,"");
/*	database[marriage].marriage.data.date=??;
	database[marriage].marriage.data.end=??;*/
	database[marriage].marriage.next=none;
	Modules_ChangedStructure();
}

void Database_AddParents(elementptr child,elementptr mother,elementptr father)
{
	elementptr marriage;
	Database_UnlinkUnlinkedPerson(mother);
	Database_Marry(mother,father);
	marriage=Database_GetMarriage(mother);
	database[marriage].marriage.leftchild=child;
	database[marriage].marriage.rightchild=child;
	database[child].person.mother=Database_GetSpouseFromMarriage(marriage);
	database[child].person.father=Database_GetPrincipalFromMarriage(marriage);
	Modules_ChangedStructure();
}

void Database_AddChild(elementptr marriage,elementptr child)
{
	Database_UnlinkUnlinkedPerson(child);
	database[child].person.siblingsltor=none;
	database[child].person.siblingsrtol=database[marriage].marriage.rightchild;
	database[marriage].marriage.rightchild=child;
	if (database[marriage].marriage.leftchild==none) database[marriage].marriage.leftchild=child;
	if (database[child].person.siblingsrtol!=none) database[database[child].person.siblingsrtol].person.siblingsltor=child;
	database[child].person.mother=database[marriage].marriage.spouse;
	database[child].person.father=database[marriage].marriage.principal;
	Modules_ChangedStructure();
}

void Database_LinkPerson(elementptr person)
{
	if (database[0].file.linkedpeople!=none) return;
	Database_UnlinkUnlinkedPerson(person);
	database[0].file.linkedpeople=person;
	Modules_ChangedStructure();
}

void Database_EditPerson(elementptr person)
{
	Desk_Icon_SetText(editpersonwin,editpersonicon_SURNAME,database[person].person.data.surname);
	Desk_Icon_SetText(editpersonwin,editpersonicon_FORENAME,database[person].person.data.forename);
	Desk_Icon_SetText(editpersonwin,editpersonicon_MIDDLENAMES,database[person].person.data.middlenames);
	Desk_Icon_SetText(editpersonwin,editpersonicon_TITLE,database[person].person.data.title);
	switch (database[person].person.data.sex) {
		case male:
			AJWLib_Msgs_SetText(editpersonwin,editpersonicon_SEX,"Sex.M:");
			break;
		case female:
			AJWLib_Msgs_SetText(editpersonwin,editpersonicon_SEX,"Sex.F:");
			break;
		case unknown:
			AJWLib_Msgs_SetText(editpersonwin,editpersonicon_SEX,"Sex.U:");
			break;
	}
/*	Desk_Icon_SetText(editpersonwin,editpersonicon_DOB,database[person].person.data.);
	Desk_Icon_SetText(editpersonwin,editpersonicon_DOD,database[person].person.data.);
*/	Desk_Icon_SetText(editpersonwin,editpersonicon_BIRTHPLACE,database[person].person.data.placeofbirth);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USER1,database[person].person.data.userdata[0]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USER2,database[person].person.data.userdata[1]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USER3,database[person].person.data.userdata[2]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USERDESC1,database[0].file.userdesc[0]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USERDESC2,database[0].file.userdesc[1]);
	Desk_Icon_SetText(editpersonwin,editpersonicon_USERDESC3,database[0].file.userdesc[2]);
	Desk_Window_Show(editpersonwin,editingperson ? Desk_open_WHEREVER : Desk_open_CENTERED);
	editingperson=person;
}

Desk_bool Database_CancelEditWindow(Desk_event_pollblock *block,void *ref)
{
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	editingperson=none;
	Desk_Window_Hide(editpersonwin);
	return Desk_TRUE;
}

Desk_bool Database_OkEditWindow(Desk_event_pollblock *block,void *ref)
{
	if (block->data.mouse.button.data.menu || editingperson==none) return Desk_FALSE;
	Desk_Icon_GetText(editpersonwin,editpersonicon_SURNAME,database[editingperson].person.data.surname);
	Desk_Icon_GetText(editpersonwin,editpersonicon_FORENAME,database[editingperson].person.data.forename);
	Desk_Icon_GetText(editpersonwin,editpersonicon_MIDDLENAMES,database[editingperson].person.data.middlenames);
	Desk_Icon_GetText(editpersonwin,editpersonicon_TITLE,database[editingperson].person.data.title);
	if (!strcmp(Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_SEX),AJWLib_Msgs_TempLookup("Sex.M:"))) database[editingperson].person.data.sex=male;
	else if (!strcmp(Desk_Icon_GetTextPtr(editpersonwin,editpersonicon_SEX),AJWLib_Msgs_TempLookup("Sex.F:"))) database[editingperson].person.data.sex=female;
	else database[editingperson].person.data.sex=unknown;
/*	Desk_Icon_GetText(editpersonwin,editpersonicon_DOB,database[editingperson].person.data.);
	Desk_Icon_GetText(editpersonwin,editpersonicon_DOD,database[editingperson].person.data.);
*/	Desk_Icon_GetText(editpersonwin,editpersonicon_BIRTHPLACE,database[editingperson].person.data.placeofbirth);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USER1,database[editingperson].person.data.userdata[0]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USER2,database[editingperson].person.data.userdata[1]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USER3,database[editingperson].person.data.userdata[2]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USERDESC1,database[0].file.userdesc[0]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USERDESC2,database[0].file.userdesc[1]);
	Desk_Icon_GetText(editpersonwin,editpersonicon_USERDESC3,database[0].file.userdesc[2]);
	editingperson=none;
	if (block->data.mouse.button.data.select) Desk_Window_Hide(editpersonwin);
	return Desk_TRUE;
}

Desk_bool Database_CancelEditMarriageWindow(Desk_event_pollblock *block,void *ref)
{
	if (!block->data.mouse.button.data.select) return Desk_FALSE;
	editingmarriage=none;
	Desk_Window_Hide(editmarriagewin);
	return Desk_TRUE;
}

Desk_bool Database_OkEditMarriageWindow(Desk_event_pollblock *block,void *ref)
{
	if (block->data.mouse.button.data.menu || editingmarriage==none) return Desk_FALSE;
	Desk_Icon_GetText(editmarriagewin,editmarriageicon_PLACE,database[editingmarriage].marriage.data.place);
/*	Desk_Icon_GetText(editmarriagewin,editmarriageicon_DATE,database[editingmarriage].marriage.data.date);*/
	editingmarriage=none;
	if (block->data.mouse.button.data.select) Desk_Window_Hide(editmarriagewin);
	return Desk_TRUE;
}

void Database_EditMarriage(elementptr marriage)
{
	char name[256];
	if (marriage==none) return;
	sprintf(name,"%s %s",database[database[marriage].marriage.principal].person.data.forename,database[database[marriage].marriage.principal].person.data.surname);
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_PRINCIPAL,name);
	sprintf(name,"%s %s",database[database[marriage].marriage.spouse].person.data.forename,database[database[marriage].marriage.spouse].person.data.surname);
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_SPOUSE,name);
	Desk_Icon_SetText(editmarriagewin,editmarriageicon_PLACE,database[marriage].marriage.data.place);
/*	Desk_Icon_SetText(editmarriagewin,editmarriageicon_DATE,database[marriage].marriage.data.date);*/
	Desk_Window_Show(editmarriagewin,editingmarriage ? Desk_open_WHEREVER : Desk_open_CENTERED);
	editingmarriage=marriage;
}

void Database_Delete(elementptr person)
{
	Database_UnlinkUnlinkedPerson(person);
	database[person].freeelement.next=database[0].file.freeelement;
	database[0].file.freeelement=person;
	Modules_ChangedStructure();
}

void Database_Add(void)
{
	elementptr newperson;
	static int newpersonnumber=1;
	if ((newperson=database[0].file.freeelement)==none) {
		if (!AJWLib_Flex_Extend((flex_ptr)&database,sizeof(element)*(database[0].file.numberofelements+1))) Desk_Error2_HandleText("Error.NoMem:Out of memory");
		newperson=database[0].file.numberofelements++;
	} else {
		database[0].file.freeelement=database[newperson].freeelement.next;
	}
	database[newperson].person.nextunlinked=database[0].file.unlinkedpeople;
	database[0].file.unlinkedpeople=newperson;
	database[newperson].person.mother=none;
	database[newperson].person.father=none;
	database[newperson].person.siblingsrtol=none;
	database[newperson].person.siblingsltor=none;
	database[newperson].person.marriage=none;
	sprintf(database[newperson].person.data.surname,"NewPerson%d",newpersonnumber++);
	strcpy(database[newperson].person.data.forename,"");
	strcpy(database[newperson].person.data.middlenames,"");
	strcpy(database[newperson].person.data.title,"");
	database[newperson].person.data.sex=unknown;
	strcpy(database[newperson].person.data.dob.data.day,"");
	strcpy(database[newperson].person.data.dob.data.month,"");
	strcpy(database[newperson].person.data.dob.data.year,"");
	database[newperson].person.data.dob.data.zero='\0';
	strcpy(database[newperson].person.data.placeofbirth,"");
	strcpy(database[newperson].person.data.userdata[0],"");
	strcpy(database[newperson].person.data.userdata[1],"");
	strcpy(database[newperson].person.data.userdata[2],"");
	strcpy(database[newperson].person.data.notes,"");
	Modules_ChangedStructure();
}

void Database_SexMenuClick(int entry,void *ref)
{
	Desk_Icon_SetText(editpersonwin,editpersonicon_SEX,Desk_Menu_GetText(sexmenu,entry));
}

void Database_TitleMenuClick(int entry,void *ref)
{
	Desk_Icon_SetText(editpersonwin,editpersonicon_TITLE,Desk_Menu_GetText(titlemenu,entry));
}

void Database_Save(char *filename)
{
/*Remove free elements?*/
	Desk_file_handle savefile;
	savefile=Desk_File_Open(filename,Desk_file_WRITE);
	Desk_File_WriteBytes(savefile,database,(database[0].file.numberofelements)*sizeof(element));
	Desk_File_Close(savefile);
}

void Database_Load(char *filename)
{
	Desk_file_handle loadfile;
	int size;
	size=Desk_File_Size(filename);
	AJWLib_Flex_Extend((flex_ptr)&database,size);
	loadfile=Desk_File_Open(filename,Desk_file_READ);
	/*Check file has correct ID*/
	Desk_File_ReadBytes(loadfile,database,size);
	Desk_File_Close(loadfile);
	Modules_ChangedStructure();
}

void Database_Init(void)
{
	if (!AJWLib_Flex_Alloc((flex_ptr)&database,sizeof(element))) Desk_Error2_HandleText("Error:NoMem:Out of memory");
	strcpy(database[0].file.fileidentifier,FILEID);
	database[0].file.versionnumber=VERSIONNUM;
	strcpy(database[0].file.filetitle,"<Untitled>");
	strcpy(database[0].file.filedescription,"");
	database[0].file.numberofelements=1;
	database[0].file.unlinkedpeople=0;
	database[0].file.linkedpeople=0;
	database[0].file.freeelement=0;
	strcpy(database[0].file.userdesc[0],"Other1");
	strcpy(database[0].file.userdesc[1],"Other2");
	strcpy(database[0].file.userdesc[2],"Other3");

	editpersonwin=Desk_Window_Create("EditPerson",Desk_template_TITLEMIN);
	editmarriagewin=Desk_Window_Create("EditMarriage",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,editpersonwin,editpersonicon_OK,Database_OkEditWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editpersonwin,editpersonicon_CANCEL,Database_CancelEditWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editmarriagewin,editmarriageicon_OK,Database_OkEditMarriageWindow,NULL);
	Desk_Event_Claim(Desk_event_CLICK,editmarriagewin,editmarriageicon_CANCEL,Database_CancelEditMarriageWindow,NULL);
	/*Register handlers*/
	sexmenu=AJWLib_Menu_CreateFromMsgs("Title.Sex:","Menu.Sex:M,F,U",Database_SexMenuClick,NULL);
	AJWLib_Menu_AttachPopup(editpersonwin,editpersonicon_SEXMENU,editpersonicon_SEX,sexmenu,Desk_button_MENU | Desk_button_SELECT);
	titlemenu=AJWLib_Menu_CreateFromMsgs("Title.Title:","Menu.Title:",Database_TitleMenuClick,NULL);
	AJWLib_Menu_AttachPopup(editpersonwin,editpersonicon_TITLEMENU,editpersonicon_TITLE,titlemenu,Desk_button_MENU | Desk_button_SELECT);
}
