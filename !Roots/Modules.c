/*
	Roots - Modules
	Interaction between different modules
	� Alex Waugh 1999

	$Id: Modules.c,v 1.18 2004/01/05 22:43:00 AJW Exp $

*/

#include "Desk/Core.h"
#include "Desk/Wimp.h"
#include "Desk/Coord.h"

#include <stdio.h>

#include "Modules.h"
#include "Database.h"
#include "Graphics.h"
#include "Windows.h"
#include "File.h"
#include "Config.h"
#include "Print.h"
#include "Layout.h"
#include "EditGraphics.h"



static Desk_bool changedstructure=Desk_FALSE,changeddata=Desk_FALSE,changedlayout=Desk_FALSE;

void Modules_Init(void)
{
	Database_Init();
	Windows_Init();
	Config_Init();
	Print_Init();
	Layout_Init();
	EditGraphics_Init();
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
	if (changedstructure) {
		Windows_Relayout();
		Windows_CloseNewView();
		Database_StopEditing();
	}
	if (changeddata) Windows_ForceRedraw();
	if (changedlayout) Windows_ChangedLayout();
	changedstructure=Desk_FALSE;
	changeddata=Desk_FALSE;
	changedlayout=Desk_FALSE;
}

void Modules_ChangedData(elementptr person)
{
	Desk_UNUSED(person);
	changeddata=Desk_TRUE;
	File_Modified();
}
