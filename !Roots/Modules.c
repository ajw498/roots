/*
	FT - Modules
	© Alex Waugh 1999

	$Log: Modules.c,v $
	Revision 1.1  1999/09/27 15:33:18  AJW
	Initial revision


*/

/*	Includes  */

#include "DeskLib:Window.h"
#include "DeskLib:Error.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:Handler.h"
#include "DeskLib:Hourglass.h"
#include "DeskLib:Icon.h"
#include "DeskLib:Menu.h"
#include "DeskLib:Msgs.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Template.h"
#include "DeskLib:File.h"
#include "DeskLib:Filing.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:GFX.h"
#include "DeskLib:ColourTran.h"

#include "AJWLib:Window.h"
#include "AJWLib:Menu.h"
#include "AJWLib:Msgs.h"
#include "AJWLib:Misc.h"
#include "AJWLib:Handler.h"
#include "AJWLib:Error.h"
#include "AJWLib:Flex.h"
#include "AJWLib:DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Modules.h"
#include "Database.h"
#include "Graphics.h"
#include "GConfig.h"

static BOOL changedstructure=FALSE,changeddata=FALSE;

BOOL Modules_Init(void)
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
	return TRUE;
}

void Modules_ChangedStructure(void)
{
	changedstructure=TRUE;
}

void Modules_ReflectChanges(void)
{
	if (changedstructure) Graphics_Relayout();
	changedstructure=FALSE;
	/*if (changeddata) Graphics_ForceRedraw();*/
	changeddata=FALSE;
}

void Modules_ChangedData(elementptr person)
{
	changeddata=TRUE;
}
