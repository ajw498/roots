/*
	FT - Modules
	Interaction between different modules
	© Alex Waugh 1999

	$Id: Modules.c,v 1.9 2000/02/26 18:54:29 uid1 Exp $

*/

#include "Desk.Core.h"

#include "Modules.h"
#include "Database.h"
#include "Graphics.h"
#include "Windows.h"
#include "File.h"

static Desk_bool changedstructure=Desk_FALSE,changeddata=Desk_FALSE,changedlayout=Desk_FALSE;

void Modules_Init(void)
{
	Database_Init();
	Windows_Init();
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
	if (changeddata) Windows_ForceRedraw();
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
