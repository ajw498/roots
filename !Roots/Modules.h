#ifndef MODULES_H
#define MODULES_H

#ifndef DATABASE_H
#include "Database.h"
#endif

void Modules_Init(void);

void Modules_ChangedStructure(void);

void Modules_ChangedLayout(void);

void Modules_ChangedData(elementptr person);

void Modules_ReflectChanges(void);

#endif
