#pragma once

#include "stdafx.h"


/* ~~ constante indiquant la r�ussite d'une fonction ~~ */
#define STATUS_SUCCESS  ((DWORD)0)


// Fonction g�n�rant et affichant un message d'erreur ad�quat
//  et complet suite � l'�chec d'une fonction syst�me.
// Param�tre :
// - fmtMsg : message arbitraire indiquant les cons�quences
//            de l'erreur ; doit contenir un marqueur "%u"
//            pour placer le num�ro d'erreur syst�me.
// Valeur de retour :
// - le code de l'erreur trouv�e et affich�e (r�cup�r�
//   via la fonction GetLastError() de l'API Win32).
DWORD
MsgErreurSys(const WCHAR* fmtMsg);


