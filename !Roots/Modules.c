/*
	FT - Modules
	© Alex Waugh 1999

	$Log: Modules.c,v $
	Revision 1.5  2000/01/14 13:08:50  AJW
	Changed Graphics_ to Windows_

	Revision 1.4  2000/01/13 17:55:34  AJW
	Calls File_Modified

	Revision 1.3  1999/10/27 16:04:58  AJW
	Added Modules_ChangedLayout

	Revision 1.2  1999/10/10 20:54:23  AJW
	Modified to use Desk

	Revision 1.1  1999/09/27 15:33:18  AJW
	Initial revision


*/

/*	Includes  */

#include "Desk.Core.h"

#include "Modules.h"
#include "Database.h"
#include "Graphics.h"
#include "GConfig.h"
#include "File.h"

static Desk_bool changedstructure=Desk_FALSE,changeddata=Desk_FALSE,changedlayout=Desk_FALSE;

void Modules_Init(void)
{
	Database_Init();
	Windows_Init();
	Graphics_Init();

	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
/*	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_Add();
	Database_LinkPerson(1);
	Database_Marry(1,2);
	Database_AddChild(24,3);
	Database_AddChild(24,16);
	Database_AddChild(24,17);
	Database_Marry(3,4);
	Database_AddChild(25,21);
	Database_AddChild(25,20);
	Database_Marry(21,19);
	Database_FudgeMarriageSwap(26);
	Database_AddChild(26,22);
	Database_AddChild(26,23);
	Database_Marry(17,18);
	Database_AddChild(27,5);
	Database_Marry(5,6);
	Database_AddChild(28,10);
	Database_AddChild(28,11);
*//*	Database_AddParents(6,12,13);
*//*	Database_AddParents(12,14,15);
*//*	Database_AddChild(29,7);
	Database_Marry(7,8);
	Database_AddChild(31,9);
*//*	Database_FudgeLinked(5);*/
/*    Database_Marry(1,7);
    Database_Marry(16,15);
    Database_AddChild(30,14);
    Database_Marry(14,13);
    Database_FudgeLinked(13);
*/
}

void Modules_ChangedStructure(void)
{
	changedstructure=Desk_TRUE;
	File_Modified();
}

void Modules_ChangedLayout(void)
{
	changedlayout=Desk_TRUE;
	File_Modified();
}

void Modules_ReflectChanges(void)
{
	if (changedstructure) Windows_Relayout();
	/*if (changeddata) Windows_ForceRedraw();*/
	if (changedlayout) Windows_ChangedLayout();
	changedstructure=Desk_FALSE;
	changeddata=Desk_FALSE;
	changedlayout=Desk_FALSE;
}

void Modules_ChangedData(elementptr person)
{
	changeddata=Desk_TRUE;
	File_Modified();
}
