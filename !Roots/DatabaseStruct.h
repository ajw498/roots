#ifndef DATABASE_H
#define DATABASE_H

#include "Desk.Window.h"
#include "Layout.h"

#define none 0
#define FIELDSIZE 20
#define NUMBERPERSONUSERFIELDS 10
#define NUMBERMARRIAGEUSERFIELDS 5

#ifndef ELEMENTPTR
#define ELEMENTPTR

typedef int elementptr;

#endif

typedef enum elementtype {
	element_NONE,
	element_PERSON,
	element_MARRIAGE,
	element_SELECTION,
	element_FREE,
	element_FILE,
	element_TITLE
} elementtype;

typedef enum sextype {
	sex_MALE='M',
	sex_FEMALE='F',
	sex_UNKNOWN='U',
	sex_ANY='A'
} sextype;

elementptr Database_GetLinked(int *index);
void Database_Select(elementptr person);
void Database_DeSelect(elementptr person);
void Database_DeSelectAll(void);
Desk_bool Database_GetSelect(elementptr person);
elementtype Database_AnyoneSelected(void);
void Database_UnlinkSelected(layout *layout);
void Database_DeleteSelected(layout *layout);
elementptr Database_GetMarriage(elementptr person);
elementptr Database_GetMarriageLtoR(elementptr person);
elementptr Database_GetMarriageRtoL(elementptr person);
char *Database_GetName(elementptr person);
char *Database_GetFullName(elementptr person);
elementptr Database_GetMother(elementptr person);
elementptr Database_GetFather(elementptr person);
elementptr Database_GetParentsMarriage(elementptr person);
elementptr Database_GetSiblingLtoR(elementptr person);
elementptr Database_GetSiblingRtoL(elementptr person);
elementptr Database_GetLeftChild(elementptr person);
elementptr Database_GetRightChild(elementptr person);
elementptr Database_GetSpouseFromMarriage(elementptr marriage);
elementptr Database_GetPrincipalFromMarriage(elementptr marriage);
Desk_bool Database_IsFirstMarriage(elementptr marriage);
void Database_RemoveMarriage(elementptr marriage);
void Database_New(void);
void Database_Remove(void);
void Database_Init(void);
char *Database_GetTitle(void);
elementptr Database_Marry(elementptr linked,elementptr unlinked);
void Database_AddChild(elementptr marriage,elementptr child);
void Database_EditTitle(void);
void Database_Edit(elementptr person);
elementptr Database_Add(void);
void Database_Delete(elementptr person);
int Database_GetNumPeople(void);
void Database_StopEditing(void);
char *Database_GetPersonUserDesc(int num);
void Database_SetPersonUserDesc(int num,char *desc);
char *Database_GetPersonGEDCOMDesc(int num);
void Database_SetPersonGEDCOMDesc(int num,char *desc);
char *Database_GetMarriageUserDesc(int num);
void Database_SetMarriageUserDesc(int num,char *desc);
char *Database_GetMarriageGEDCOMDesc(int num);
void Database_SetMarriageGEDCOMDesc(int num,char *desc);
sextype Database_GetSex(elementptr person);
void Database_SaveGEDCOM(FILE *file,Desk_bool plainGEDCOM);
void Database_SetTitle(char *title);
void Database_SetNextNewPerson(int personnumber);
void Database_SetForename(elementptr person,char *name);
void Database_SetMiddleNames(elementptr person,char *name);
void Database_SetSurname(elementptr person,char *name);
void Database_SetSex(elementptr person,sextype sexchar);
void Database_SetMarriage(elementptr person,elementptr marriage);
void Database_SetParentsMarriage(elementptr person,elementptr marriage);
void Database_SetPrincipal(elementptr marriage,elementptr person);
void Database_SetSpouse(elementptr marriage,elementptr person);
elementptr Database_AddMarriage(void);
void Database_CheckConsistency(void);
void Database_LinkAllChildren(void);
void Database_SetPersonUser(int num,elementptr person,char *str);
void Database_SetMarriageUser(int num,elementptr marriage,char *str);
char *Database_GetForename(elementptr person);
char *Database_GetMiddleNames(elementptr person);
char *Database_GetSurname(elementptr person);
Desk_bool Database_Loaded(void);
void Database_LinkAllMarriages(void);

#endif

