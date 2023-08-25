//
// WindowFinder.cpp : fichier source principal de l'application.
//

#include "stdafx.h"
#include "Resource.h"
#include "FonctionsCommunes.h"


/*========================================================================*/
/*                               CONSTANTES                               */
/*========================================================================*/

/* tailles maximales pour les "tampons" en mémoire */
#define MAX_STRING_SZ  1024
#define MAX_NUMSTR_SZ  16

/* messages affichés et chaînes de format */
static const WCHAR* TITLE_CRIT_ERR = L"ERREUR CRITIQUE !";
static const WCHAR* TITLE_ABOUT = L"À propos de";
static const WCHAR* MSG_CRIT_ERR_OUT_OF_MEMORY =
	L"Le système n'a plus assez de mémoire disponible pour ce programme !\r\n"
	L"Sauvegardez vos travaux en urgence, et redémarrez votre ordinateur !";
static const WCHAR* MSG_FATAL_ERR_REGISTERCLASSEX_FAIL =
	L"ERREUR FATALE (n° %u) à l'exécution de RegisterClassEx() !";
static const WCHAR* MSG_FATAL_ERR_CREATEWINDOW_FAIL =
	L"ERREUR FATALE (n° %u) à l'exécution de CreateWindow() !";
static const WCHAR* MSG_FATAL_ERR_DESTROYWINDOW_FAIL =
	L"ERREUR FATALE (n° %u) à l'exécution de DestroyWindow() !";
static const WCHAR* MSG_FATAL_ERR_MOVEWINDOW_FAIL =
	L"ERREUR FATALE (n° %u) à l'exécution de MoveWindow() !";
static const WCHAR* MSG_FATAL_ERR_GETMESSAGE_FAIL =
	L"ERREUR FATALE (n° %u) à l'exécution de GetMessage() !";
static const WCHAR* MSG_FATAL_ERR_ENUMWINDOWS_FAIL =
	L"ERREUR FATALE (n° %u) à l'exécution de EnumWindows() !";
static const WCHAR* MSG_ERR_DIALOGBOX_FAIL =
	L"ERREUR : impossible d'ouvrir la boîte de dialogue"
	L" de lancement de processus, échec de DialogBox (erreur %u) !";
static const WCHAR* MSG_ERR_GETDLGITEM_FAIL =
	L"ERREUR : impossible d'accéder à un contrôle de boîte de dialogue,"
	L" échec de GetDlgItem (erreur %u) !";
static const WCHAR* MSG_ERR_GETCLASSNAME_FAIL =
	L"ERREUR : impossible de retrouver la classe d'une fenêtre,"
	L" échec de GetClassName (erreur %u) !";
static const WCHAR* MSG_ERR_GETWINDOWRECT_FAIL =
	L"ERREUR : impossible de retrouver les coordonnées d'une fenêtre,"
	L" échec de GetWindowRect (erreur %u) !";
static const WCHAR* MSG_ERR_GETFILEVERSIONINFO_FAIL =
	L"ERREUR : impossible de lire les caractéristiques"
	L" du programme, échec de GetFileVersionInfo (erreur %u) !";
static const WCHAR* MSG_ERR_VERQUERYVALUE_FAIL =
	L"ERREUR : impossible de lire les caractéristiques"
	L" du programme, échec de VerQueryValue (erreur %u) !";
static const WCHAR* MSG_FMT_DATETIME =
	L" du %02u/%02u/%04u à %02u:%02u";
static const WCHAR* MSG_FMT_ABOUTBOX =
	L"%s (version %s%s)\r\n"
	L"%s\r\n"
	L"%s";


/*========================================================================*/
/*                          DÉFINITIONS DE TYPES                          */
/*========================================================================*/

//
// relie un "handle" de fenêtre recensée à un "item" du "treeview"
//
typedef struct _refWndTree {
	HWND hWnd;
	HTREEITEM hTreeItem;
	struct _refWndTree* next;
} LinkWndTreeitem, *PLinkWndTreeItem;


/*========================================================================*/
/*                           VARIABLES GLOBALES                           */
/*========================================================================*/

// instance actuelle du programme
HINSTANCE hInst;

// chemin vers l'exécutable du présent programme
static WCHAR exePath[MAX_PATH];

// contrôle occupant l'espace client de la fenêtre,
// et montrant l'arborescence des fenêtres recensées
static HWND hWndTreeview;

// faut-il lister toutes les fenêtres
// ou seulement celles du bureau courant ?
static BOOL allWindows;

// lien vers la liste des fenêtres-enfants en cours de recensement
static PLinkWndTreeItem lstChildWindows;

// propriétés de la fenêtre sélectionnée
static WCHAR wndText[MAX_STRING_SZ];
static WCHAR wndHandle[MAX_NUMSTR_SZ];
static WCHAR wndClass[MAX_STRING_SZ];
static WCHAR wndTop[MAX_NUMSTR_SZ], wndLeft[MAX_NUMSTR_SZ],
             wndBottom[MAX_NUMSTR_SZ], wndRight[MAX_NUMSTR_SZ];
static WCHAR wndPID[MAX_NUMSTR_SZ], wndTID[MAX_NUMSTR_SZ];


/*========================================================================*/
/*                               FONCTIONS                                */
/*========================================================================*/

/* === MACROS === */

#define CRIT_ERR_OUT_OF_MEMORY { \
			MessageBoxW(NULL, \
			            MSG_CRIT_ERR_OUT_OF_MEMORY, \
			            TITLE_CRIT_ERR, \
			            MB_ICONERROR | MB_SYSTEMMODAL); \
			ExitProcess((UINT)-1); \
		}


/* === FONCTIONS UTILITAIRES === */

static void
EmptyChildWindowsList(void)
{
	// se place au début de la liste des fenêtres-enfants
	PLinkWndTreeItem current = lstChildWindows;
	// supprime chaque élément de la liste l'un après l'autre
	while (current != NULL)
	{
		PLinkWndTreeItem next = current->next;
		free(current);
		current = next;
	}
	// RAZ du pointeur sur la liste
	lstChildWindows = NULL;
}

static PLinkWndTreeItem
AddChildWindowLink(HWND childHwnd,
                   HTREEITEM hTreeNode)
{
	// alloue la mémoire pour le nouvel élément de la liste
	PLinkWndTreeItem newLink =
			(PLinkWndTreeItem)(malloc(sizeof(LinkWndTreeitem)));
	if (newLink == NULL) CRIT_ERR_OUT_OF_MEMORY;
	// remplit le nouvel élément ainsi alloué
	newLink->hTreeItem = hTreeNode;
	newLink->hWnd = childHwnd;
	// place le nouvel élément en début de liste
	newLink->next = lstChildWindows;
	lstChildWindows = newLink;
	// renvoie le pointeur sur ce nouvel élément
	return newLink;
}

static HTREEITEM
FindTreeItemForHwnd(HWND wantedHwnd)
{
	// se place en début de liste
	PLinkWndTreeItem current = lstChildWindows;
	// recherche le HWND voulu dans chaque élément
	while (current != NULL)
	{
		// trouvé ?
		if (current->hWnd == wantedHwnd) return current->hTreeItem;
		// sinon, passe à l'élément suivant
		current = current->next;
	}
	// si on arrive ici, le HWND n'est pas recensé dans la liste
	return NULL;
}


/* ~~ fonctions "callback" ~~ */

BOOL CALLBACK
ChildWindowEnumerated(HWND hChildWnd,
                      LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	// retrouve le libellé de la fenêtre
	int len = GetWindowTextLengthW(hChildWnd);
	if (len > (MAX_STRING_SZ / 2)) len = MAX_STRING_SZ / 2;
	if (len < 4) len = 4;
	WCHAR* strTxt = (WCHAR*)(calloc(len + 1, sizeof(WCHAR)));
	if (strTxt == NULL) CRIT_ERR_OUT_OF_MEMORY;
	GetWindowTextW(hChildWnd, strTxt, len + 1);
	if (wcslen(strTxt) == 0) swprintf(strTxt, len, L"\"\"");

	// retrouve la classe de la fenêtre
	len = MAX_STRING_SZ / 4;
	WCHAR* strCls = (WCHAR*)(calloc(len + 1, sizeof(WCHAR)));
	if (strCls == NULL) CRIT_ERR_OUT_OF_MEMORY;
	GetClassNameW(hChildWnd, strCls, len + 1);

	// texte du nouveau noeud pour le TreeView
	WCHAR txt[MAX_STRING_SZ];
	swprintf(txt, MAX_STRING_SZ,
	         L"%s — [%s] — (0x%08X)",
	         strTxt, strCls,
	         hChildWnd);
	free(strTxt);
	free(strCls);

	// retrouve le parent (direct) du noeud à créer
	HWND hwndParent = GetParent(hChildWnd);
	if (hwndParent == NULL) return FALSE;
	HTREEITEM hItemParent = FindTreeItemForHwnd(hwndParent);
	if (hItemParent == NULL) return FALSE;

	// création et insertion du nouveau noeud
	TVINSERTSTRUCTW treeInsert;
	treeInsert.hParent = hItemParent;
	treeInsert.hInsertAfter = TVI_LAST;
	treeInsert.item.mask = TVIF_TEXT | TVIF_PARAM;
	treeInsert.item.pszText = txt;
	treeInsert.item.lParam = (LPARAM)hChildWnd;
	HTREEITEM hNewItem = TreeView_InsertItem(hWndTreeview, &treeInsert);
	AddChildWindowLink(hChildWnd, hNewItem);

	// fenêtre suivante
	return TRUE;
}

BOOL CALLBACK
MainWindowEnumerated(HWND hWnd,
                     LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	// retrouve le libellé de la fenêtre
	int len = GetWindowTextLengthW(hWnd);
	if (len > (MAX_STRING_SZ / 2)) len = MAX_STRING_SZ / 2;
	if (len < 4) len = 4;
	WCHAR* strTxt = (WCHAR*)(calloc(len + 1, sizeof(WCHAR)));
	if (strTxt == NULL) CRIT_ERR_OUT_OF_MEMORY;
	GetWindowTextW(hWnd, strTxt, len + 1);
	if (wcslen(strTxt) == 0) swprintf(strTxt, len, L"\"\"");

	// retrouve la classe de la fenêtre
	len = MAX_STRING_SZ / 4;
	WCHAR* strCls = (WCHAR*)(calloc(len + 1, sizeof(WCHAR)));
	if (strCls == NULL) CRIT_ERR_OUT_OF_MEMORY;
	GetClassNameW(hWnd, strCls, len + 1);

	// texte du nouveau noeud pour le TreeView
	WCHAR txt[MAX_STRING_SZ];
	swprintf(txt, MAX_STRING_SZ,
	         L"%s — [%s] — (0x%08X)",
	         strTxt, strCls,
	         hWnd);
	free(strTxt);
	free(strCls);

	// création et insertion du nouveau noeud
	TVINSERTSTRUCTW treeInsert;
	treeInsert.hParent = NULL;
	treeInsert.hInsertAfter = TVI_LAST;
	treeInsert.item.mask = TVIF_TEXT | TVIF_PARAM;
	treeInsert.item.pszText = txt;
	treeInsert.item.lParam = (LPARAM)hWnd;
	HTREEITEM hNewItem = TreeView_InsertItem(hWndTreeview, &treeInsert);
	AddChildWindowLink(hWnd, hNewItem);

	// recense les fenêtres filles de celle-ci
	EnumChildWindows(hWnd, ChildWindowEnumerated, NULL);
	EmptyChildWindowsList();

	// fenêtre suivante
	return TRUE;
}


/* ~~ fonction effectrice ~~ */

static void
FillWindowTree(void)
{
	// supprime tous les éléments pré-existants
	TreeView_DeleteAllItems(hWndTreeview);
	// demande au système d'énumérer les fenêtres
	BOOL ok;
	if (allWindows)
	{
		ok = EnumWindows(MainWindowEnumerated, NULL);
	} else {
		ok = EnumDesktopWindows(NULL, MainWindowEnumerated, NULL);
	}
	if (!ok)
	{
		DWORD errCode = MsgErreurSys(MSG_FATAL_ERR_ENUMWINDOWS_FAIL);
		// erreur fatale, on quitte le programme
		ExitProcess(errCode);
	}
}


/* ~~ fonctions de gestion des fenêtres ~~ */

//
// Gestionnaire de messages pour la boîte de dialogue
// donnant les propriétés d'une fenêtre recensée.
//
INT_PTR CALLBACK
WindowPropertiesDialogProc(HWND hDlg,
                           UINT message,
                           WPARAM wParam,
                           LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	HWND hEdit, hBtn;

	switch (message)
	{
	case WM_INITDIALOG:
		// remplit les champs comportant les caractéristiques données
		// à l'initialisation de la boîte de dialogue
		// - titre de la fenêtre
		hEdit = GetDlgItem(hDlg, IDC_EDIT_WND_TEXT);
		if (hEdit == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetWindowTextW(hEdit, wndText);
		// - handle de la fenêtre
		hEdit = GetDlgItem(hDlg, IDC_EDIT_WND_HANDLE);
		if (hEdit == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetWindowTextW(hEdit, wndHandle);
		// - classe de la fenêtre
		hEdit = GetDlgItem(hDlg, IDC_EDIT_WND_CLASS);
		if (hEdit == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetWindowTextW(hEdit, wndClass);
		// - coordonnées de la fenêtre
		hEdit = GetDlgItem(hDlg, IDC_EDIT_WND_TOP);
		if (hEdit == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetWindowTextW(hEdit, wndTop);
		hEdit = GetDlgItem(hDlg, IDC_EDIT_WND_LEFT);
		if (hEdit == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetWindowTextW(hEdit, wndLeft);
		hEdit = GetDlgItem(hDlg, IDC_EDIT_WND_BOTTOM);
		if (hEdit == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetWindowTextW(hEdit, wndBottom);
		hEdit = GetDlgItem(hDlg, IDC_EDIT_WND_RIGHT);
		if (hEdit == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetWindowTextW(hEdit, wndRight);
		// - PID et TID du "thread" propriétaire de la fenêtre
		hEdit = GetDlgItem(hDlg, IDC_EDIT_WND_PID);
		if (hEdit == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetWindowTextW(hEdit, wndPID);
		hEdit = GetDlgItem(hDlg, IDC_EDIT_WND_TID);
		if (hEdit == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetWindowTextW(hEdit, wndTID);
		// OK, terminé
		return (INT_PTR)TRUE;

	case WM_SHOWWINDOW:
		// à l'ouverture de la boîte de dialogue,
		// focus par défaut sur le bouton de fermeture
		hBtn = GetDlgItem(hDlg, IDCANCEL);
		if (hBtn == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_ERR_GETDLGITEM_FAIL);
			return (INT_PTR)FALSE;
		}
		SetFocus(hBtn);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			// ferme la fenêtre à la pression de l'unique bouton 'Fermer'
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


//
//  Traite les messages pour la fenêtre principale.
//
LRESULT CALLBACK
MainWndProc(HWND hMainWnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam)
{
	int dlgRes;
	BOOL ok;
	HWND hClient;
	POINT pt;
	HMENU hMenu;
	LPNMHDR pNotify;

	switch (message)
	{
	case WM_CREATE:
		// retrouve le chamin de l'exécutable courant
		GetModuleFileNameW(NULL, exePath, MAX_PATH);
		exePath[MAX_PATH - 1] = L'\0';
		// créé le contrôle "treeview" central
		hWndTreeview = CreateWindowW(WC_TREEVIEWW,
		                             L"Window_Lister_TreeView",
		                             WS_VISIBLE | WS_CHILD | WS_BORDER
		                             | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
		                             0, 0,
		                             0, 0,
		                             hMainWnd,
		                             NULL,
		                             hInst,
		                             NULL);
		if (hWndTreeview == NULL)
		{
			DWORD errCode = MsgErreurSys(MSG_FATAL_ERR_CREATEWINDOW_FAIL);
			ExitProcess(errCode);
		}
		FillWindowTree();
		break;

	case WM_SETFOCUS:
		// seul le "treeview" central doit recevoir le focus
		SetFocus(hWndTreeview);
		break;

	case WM_SIZE:
		// redimensionne le "treeview" central avec la fenêtre
		ok = MoveWindow(hWndTreeview,
		                0, 0,
		                LOWORD(lParam), HIWORD(lParam),
		                TRUE);
		if (!ok)
		{
			DWORD errCode = MsgErreurSys(MSG_FATAL_ERR_MOVEWINDOW_FAIL);
			ExitProcess(errCode);
		}
		break;

	case WM_CONTEXTMENU:
		// ouvre le menu contextuel défini pour le "treeview" central
		// et uniquement pour lui
		hClient = (HWND)wParam;
		if (hClient != hWndTreeview) break;
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDC_WINTREEVIEW));
		hMenu = GetSubMenu(hMenu, 0);
		TrackPopupMenu(hMenu,
		               TPM_RIGHTBUTTON,
		               pt.x,
		               pt.y,
		               0,
		               hMainWnd,
		               NULL);
		break;

	case WM_NOTIFY:
		// désactive le déroulement/enroulement des noeuds
		// lors d'un double-clic dans le "treeview" central
		pNotify = (LPNMHDR)lParam;
		if (pNotify->hwndFrom != hWndTreeview) break;
		switch (pNotify->code)
		{
		case NM_DBLCLK:
			PostMessageW(hMainWnd, WM_COMMAND, IDM_PROPERTIES, NULL);
			/* retourne un nombre non-nul pour désactiver
			   le comportement par défaut du TreeView pour le double-clic */
			return 1;
		}
		break;

	case WM_COMMAND:
		{
			HTREEITEM hItem;
			TVITEMW tvItem;
			HWND selHwnd;
			int res;
			RECT wndPos;
			DWORD pid, tid;

			BYTE verBuf[4096];
			WCHAR msg[256];
			UINT size;
			WCHAR* prodName; WCHAR* fileVer; WCHAR* fileDescr; WCHAR* copr;
			WCHAR dtHrFich[32];
			HANDLE hExe;

			int wmId    = LOWORD(wParam);
			int wmEvent = HIWORD(wParam);
			// Analyse les sélections de menu :
			switch (wmId)
			{
			case IDM_EXIT:
				DestroyWindow(hMainWnd);
				break;

			case IDM_REFRESH:
				FillWindowTree();
				break;

			case IDM_ALLWINDOWS:
				allWindows = !allWindows;
				hMenu = GetMenu(hMainWnd);
				CheckMenuItem(hMenu,
				              IDM_ALLWINDOWS,
				              (allWindows ? MF_CHECKED : MF_UNCHECKED));
				FillWindowTree();
				break;

			case IDM_PROPERTIES:
				/* retrouve la fenêtre sélectionnée */
				hItem = TreeView_GetSelection(hWndTreeview);
				ZeroMemory(&tvItem, sizeof(TVITEMW));
				tvItem.mask = TVIF_TEXT | TVIF_PARAM;
				tvItem.hItem = hItem;
				TreeView_GetItem(hWndTreeview, &tvItem);
				selHwnd = (HWND)(tvItem.lParam);
				if (selHwnd == NULL) break;
				/* retrouve les propriétés de la fenêtre sélectionnée */
				swprintf(wndHandle, MAX_NUMSTR_SZ, L"0x%08X", selHwnd);
				GetWindowTextW(selHwnd, wndText, MAX_STRING_SZ);
				res = GetClassNameW(selHwnd, wndClass, MAX_STRING_SZ);
				if (res == 0)
				{
					DWORD errCode = MsgErreurSys(MSG_ERR_GETCLASSNAME_FAIL);
					return errCode;
				}
				ok = GetWindowRect(selHwnd, &wndPos);
				if (!ok)
				{
					DWORD errCode = MsgErreurSys(MSG_ERR_GETWINDOWRECT_FAIL);
					return errCode;
				}
				swprintf(wndTop,    MAX_NUMSTR_SZ, L"%d", wndPos.top);
				swprintf(wndLeft,   MAX_NUMSTR_SZ, L"%d", wndPos.left);
				swprintf(wndBottom, MAX_NUMSTR_SZ, L"%d", wndPos.bottom);
				swprintf(wndRight,  MAX_NUMSTR_SZ, L"%d", wndPos.right);
				tid = GetWindowThreadProcessId(selHwnd, &pid);
				swprintf(wndPID, MAX_NUMSTR_SZ, L"%u", pid);
				swprintf(wndTID, MAX_NUMSTR_SZ, L"%u", tid);
				/* affiche la boîte de dialogue idoine */
				dlgRes = DialogBoxW(hInst,
				                    MAKEINTRESOURCEW(IDD_WINDOW_PROPERTIES),
				                    hMainWnd,
				                    WindowPropertiesDialogProc);
				if (dlgRes < 0)
				{
					DWORD errCode = MsgErreurSys(MSG_ERR_DIALOGBOX_FAIL);
					return errCode;
				}
				break;

			case IDM_ABOUT:
				// retrouve les éléments du présent exécutables
				// à afficher dans le message 'à propos'
				ok = GetFileVersionInfoW(exePath,
				                         NULL,
				                         4096U,
				                         verBuf);
				if (!ok) 
				{
					MsgErreurSys(MSG_ERR_GETFILEVERSIONINFO_FAIL);
					break;
				}
				ZeroMemory(msg, 256U);
				ok = VerQueryValueW(verBuf,
				                    L"\\StringFileInfo\\040C04B0\\ProductName",
				                    (LPVOID*)&prodName,
				                    &size);
				if (!ok)
				{
					MsgErreurSys(MSG_ERR_VERQUERYVALUE_FAIL);
					break;
				}
				ok = VerQueryValueW(verBuf,
				                    L"\\StringFileInfo\\040C04B0\\FileVersion",
				                    (LPVOID*)&fileVer,
				                    &size);
				if (!ok)
				{
					MsgErreurSys(MSG_ERR_VERQUERYVALUE_FAIL);
					break;
				}
				ok = VerQueryValueW(verBuf,
				                    L"\\StringFileInfo\\040C04B0\\FileDescription",
				                    (LPVOID*)&fileDescr,
				                    &size);
				if (!ok)
				{
					MsgErreurSys(MSG_ERR_VERQUERYVALUE_FAIL);
					break;
				}
				ok = VerQueryValueW(verBuf,
				                    L"\\StringFileInfo\\040C04B0\\LegalCopyright",
				                    (LPVOID*)&copr,
				                    &size);
				if (!ok)
				{
					MsgErreurSys(MSG_ERR_VERQUERYVALUE_FAIL);
					break;
				}
				// retrouve la date et heure de modification de l'exécutable
				ZeroMemory(dtHrFich, 32U);
				hExe = CreateFileW(exePath,
				                   GENERIC_READ,
				                   FILE_SHARE_READ,
				                   NULL,
				                   OPEN_EXISTING,
				                   0,
				                   NULL);
				if (hExe != INVALID_HANDLE_VALUE)
				{
					FILETIME ftCreation, ftAcces, ftModif;
					ok = GetFileTime(hExe,
					                 &ftCreation,
					                 &ftAcces,
					                 &ftModif);
					if (ok)
					{
						SYSTEMTIME stDateUTC, stDateFich;
						ok = FileTimeToSystemTime(&ftModif, &stDateUTC);
						if (ok)
						{
							ok = SystemTimeToTzSpecificLocalTime(NULL,
							                                     &stDateUTC,
							                                     &stDateFich);
							if (ok)
							{
								swprintf(dtHrFich, 32U,
								         MSG_FMT_DATETIME,
								         stDateFich.wDay,
								         stDateFich.wMonth,
								         stDateFich.wYear,
								         stDateFich.wHour,
								         stDateFich.wMinute);
							}
						}
					}
					CloseHandle(hExe);
				}
				// enfin, affiche le message 'à propos'
				swprintf(msg, 256U,
				         MSG_FMT_ABOUTBOX,
				         prodName,
				         fileVer,
				         dtHrFich,
				         fileDescr,
				         copr);
				MessageBoxW(hMainWnd,
				            msg,
				            TITLE_ABOUT,
				            MB_ICONINFORMATION);
				break;

			default:
				return DefWindowProcW(hMainWnd, message, wParam, lParam);
			}
			break;
		}

	case WM_DESTROY:
		// quitte le message à la fermeture de la fenêtre principale
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProcW(hMainWnd, message, wParam, lParam);
	}

	return 0;
}


/* === POINT D'ENTRÉE DU PROGRAMME === */

int APIENTRY
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR    lpCmdLine,
         int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// Variables locales
	WCHAR szTitle[MAX_STRING_SZ];
	WCHAR szWindowClass[MAX_STRING_SZ];
	WNDCLASSEXW wcex;
	ATOM mainWinClass;
	HWND hMainWin;
	MSG msg;
	HACCEL hAccelTable;
	int res;

	// Initialise les chaînes globales
	LoadStringW(hInstance,
	            IDS_APP_TITLE,
	            szTitle,
	            MAX_STRING_SZ);
	LoadStringW(hInstance,
	            IDC_WINDOWLISTER,
	            szWindowClass,
	            MAX_STRING_SZ);

	// Enregistre la classe de la fenêtre principale
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style         = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc   = MainWndProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = hInstance;
	wcex.hIcon         = LoadIcon(hInstance,
	                              MAKEINTRESOURCE(IDI_WINDOWLISTER));
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName  = MAKEINTRESOURCE(IDC_WINDOWLISTER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm       = LoadIcon(wcex.hInstance,
	                              MAKEINTRESOURCE(IDI_SMALL));
	mainWinClass = RegisterClassExW(&wcex);
	if (mainWinClass == 0)
	{
		DWORD errCode = MsgErreurSys(MSG_FATAL_ERR_REGISTERCLASSEX_FAIL);
		return errCode;
	}

	// Stocke le handle d'instance dans la variable globale
	hInst = hInstance;

	// Créé la fenêtre principale de l'application
	hMainWin = CreateWindowW(szWindowClass,
	                         szTitle,
	                         WS_OVERLAPPEDWINDOW,
	                         CW_USEDEFAULT,
	                         CW_USEDEFAULT,
	                         CW_USEDEFAULT,
	                         CW_USEDEFAULT,
	                         NULL,
	                         NULL,
	                         hInstance,
	                         NULL);
	if (hMainWin == NULL)
	{
		DWORD errCode = MsgErreurSys(MSG_FATAL_ERR_CREATEWINDOW_FAIL);
		return errCode;
	}

	ShowWindow(hMainWin, nCmdShow);
	UpdateWindow(hMainWin);

	hAccelTable = LoadAcceleratorsW(hInstance,
	                                MAKEINTRESOURCE(IDA_WINDOWLISTER));

	// Boucle de messages principale
	while ((res = GetMessageW(&msg, NULL, 0, 0)) > 0)
	{
		if (!TranslateAcceleratorW(hMainWin, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	// Echec de GetMessage() ?
	if (res < 0)
	{
		DWORD errCode = MsgErreurSys(MSG_FATAL_ERR_GETMESSAGE_FAIL);
		return errCode;
	}

	// Sinon : fin normale du programme
	return 0;
}

