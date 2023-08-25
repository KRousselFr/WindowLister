#pragma once

#include "stdafx.h"


/* ~~ constante indiquant la réussite d'une fonction ~~ */
#define STATUS_SUCCESS  ((DWORD)0)


// Fonction générant et affichant un message d'erreur adéquat
//  et complet suite à l'échec d'une fonction système.
// Paramètre :
// - fmtMsg : message arbitraire indiquant les conséquences
//            de l'erreur ; doit contenir un marqueur "%u"
//            pour placer le numéro d'erreur système.
// Valeur de retour :
// - le code de l'erreur trouvée et affichée (récupéré
//   via la fonction GetLastError() de l'API Win32).
DWORD
MsgErreurSys(const WCHAR* fmtMsg);


