/*
	FT - Modules
	© Alex Waugh 1999

	$Log: Modules.c,v $
	Revision 1.2  1999/10/10 20:54:23  AJW
	Modified to use Desk

	Revision 1.1  1999/09/27 15:33:18  AJW
	Initial revision


*/

/*	Includes  */

#include "Desk.Window.h"
#include "Desk.Error2.h"
#include "Desk.Event.h"
#include "Desk.EventMsg.h"
#include "Desk.Handler.h"
#include "Desk.Hourglass.h"
#include "Desk.Icon.h"
#include "Desk.Menu.h"
#include "Desk.Msgs.h"
#include "Desk.Resource.h"
#include "Desk.Screen.h"
#include "Desk.Template.h"
#include "Desk.File.h"
#include "Desk.Filing.h"
#include "Desk.Sprite.h"
#include "Desk.GFX.h"
#include "Desk.ColourTran.h"

#include "AJWLib.Window.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Handler.h"
#include "AJWLib.Flex.h"
#include "AJWLib.DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Modules.h"
#include "Database.h"
#include "Graphics.h"
#include "GConfig.h"

static Desk_bool changedstructure=Desk_FALSE,changeddata=Desk_FALSE;

Desk_bool Modules_Init(void)
{
	Database_Init(); /*errors*/
	Graphics_Init();
	Graphics_Init2();

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
/*	Database_AddParents(6,12,13);
*//*	Database_AddParents(12,14,15);
*//*	Database_AddChild(29,7);
	Database_Marry(7,8);
	Database_AddChild(31,9);
*//*	Database_FudgeLinked(5);*/
    Database_Marry(1,7);
	return Desk_TRUE;
}

void Modules_ChangedStructure(void)
{
	changedstructure=Desk_TRUE;
}

void Modules_ReflectChanges(void)
{
	if (changedstructure) Graphics_Relayout();
	changedstructure=Desk_FALSE;
	/*if (changeddata) Graphics_ForceRedraw();*/
	changeddata=Desk_FALSE;
}

void Modules_ChangedData(elementptr person)
{
	changeddata=Desk_TRUE;
}
