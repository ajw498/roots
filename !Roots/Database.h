#ifndef DATABASE_H
#define DATABASE_H

#include "DatabaseStruct.h"
#include "LayoutStruct.h"

#define fieldconfig_OK 2

extern Desk_window_handle fieldconfigwin;

extern struct fieldconfigicons fieldconfigpersonicons[];
extern struct fieldconfigicons fieldconfigmarriageicons[];


void Database_OpenFileConfig(void);
Desk_bool Database_ElementValid(elementptr person);
Desk_bool Database_Married(elementptr person1,elementptr person2);
char *Database_GetField(elementptr element,char *fieldname);
elementptr Database_Link(layout *layout,elementptr start,elementptr end);
Desk_bool Database_LinkValid(layout *layout,elementptr start,elementptr end);
elementtype Database_GetElementType(elementptr element);
elementptr Database_GetLinked(int *index);
elementptr Database_GetLinkedMarriages(int *index);
void Database_SetFlag(elementptr person);
void Database_UnsetFlag(elementptr person);
void Database_UnsetAllFlags(void);
Desk_bool Database_GetFlag(elementptr person);
elementtype Database_AnyoneFlagged(void);
void Database_UnlinkFromSiblingsAndParents(elementptr person,elementptr marriage);
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
void Database_RemoveElement(layout *layout,elementptr element);
void Database_New(void);
void Database_Remove(void);
void Database_Init(void);
char *Database_GetTitle(void);
elementptr Database_Marry(elementptr linked,elementptr unlinked);
void Database_AddChild(elementptr marriage,elementptr child);
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
char *Database_GetMarriageFieldList(int num);
void Database_SetMarriageFieldList(int num,char *desc);
int Database_GetMarriageFieldType(int num);
void Database_SetMarriageFieldType(int num,int type);
char *Database_GetPersonFieldList(int num);
void Database_SetPersonFieldList(int num,char *desc);
int Database_GetPersonFieldType(int num);
void Database_SetPersonFieldType(int num,int type);
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
char *Database_GetPersonUserField(elementptr person,int num);
char *Database_GetMarriageUserField(elementptr marriage,int num);
char *Database_GetInitialedMiddleName(elementptr person);
char *Database_GetInitialedName(elementptr person);

#endif

