#ifndef SHAREWARE_H
#define SHAREWARE_H

#ifndef SHAREWARE_STANDALONE

#define UNREGISTEREDMAXPEOPLE 25

char *Shareware_GetUser(void);
void Shareware_CheckRegistered(void);
Desk_bool Shareware_GetErrorStatus(void);

#endif
#endif
