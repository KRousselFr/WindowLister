//
// FonctionCommunes.cpp : fonctions utilitaires communes à tous
//                        les fichiers source de l'application.
//

#include "stdafx.h"

#include "FonctionsCommunes.h"


/*========================================================================*/
/*                               CONSTANTES                               */
/*========================================================================*/

/* tailles maximales pour les "tampons" en mémoire */
#define MAX_STRING_SZ  1024

/* messages d'erreur affichés */
static const WCHAR* MSG_SYSTEM_ERROR_MSG =
	L"\r\nMessage d'erreur système : ";


/*========================================================================*/
/*                               FONCTIONS                                */
/*========================================================================*/

/* === FONCTIONS "PUBLIQUES" === */

DWORD
MsgErreurSys(const WCHAR* fmtMsg)
{
	DWORD codeErr = GetLastError();
	WCHAR msgErr[MAX_STRING_SZ];
	swprintf(msgErr, MAX_STRING_SZ, fmtMsg, codeErr);
	/* essaie d'obtenir une description de l'erreur en question */
	LPWSTR ptrMsgSys;
	DWORD res = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
	                           | FORMAT_MESSAGE_FROM_SYSTEM,
	                           NULL,
	                           codeErr,
	                           MAKELANGID(LANG_SYSTEM_DEFAULT,
	                                      SUBLANG_SYS_DEFAULT),
	                           (LPWSTR)&ptrMsgSys,
	                           0,
	                           NULL);
	if (res != 0)
	{
		wcscat_s(msgErr, MAX_STRING_SZ, MSG_SYSTEM_ERROR_MSG);
		wcscat_s(msgErr, MAX_STRING_SZ, ptrMsgSys);
		LocalFree(ptrMsgSys);
	}
	MessageBoxW(NULL,
	            msgErr,
	            NULL,
	            MB_ICONERROR | MB_SETFOREGROUND);
	/* renvoie le code d'erreur système utilisé */
	return codeErr;
}

