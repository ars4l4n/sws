/******************************************************************************
/ Menus.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS)
/ http://www.standingwaterstudios.com/reaper
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/ 
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/ 
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

// Utility functions for manipulating menus, as well as the main menu creation function
// for the SWS extension.

#include "stdafx.h"

// *************************** UTILITY FUNCTIONS ***************************

void AddToMenu(HMENU hMenu, const char* text, int id, int iInsertAfter, bool bPos, UINT uiSate)
{
	if (!text)
		return;

	int iPos = GetMenuItemCount(hMenu);
	if (bPos)
		iPos = iInsertAfter;
	else
	{
		if (iInsertAfter < 0)
			iPos += iInsertAfter + 1;
		else
		{
			HMENU h = FindMenuItem(hMenu, iInsertAfter, &iPos);
			if (h)
			{
				hMenu = h;
				iPos++;
			}
		}
	}
	
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	if (strcmp(text, SWS_SEPARATOR) == 0)
	{
		mi.fType = MFT_SEPARATOR;
		mi.fMask = MIIM_TYPE;
		InsertMenuItem(hMenu, iPos, true, &mi);
	}
	else
	{
		mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		mi.fState = uiSate;
		mi.fType = MFT_STRING;
		mi.dwTypeData = (char*)text;
		mi.wID = id;
		InsertMenuItem(hMenu, iPos, true, &mi);
	}
}

void AddSubMenu(HMENU hMenu, HMENU subMenu, const char* text, int iInsertAfter, UINT uiSate)
{
	int iPos = GetMenuItemCount(hMenu);
	if (iInsertAfter < 0)
	{
		iPos += iInsertAfter + 1;
		if (iPos < 0)
			iPos = 0;
	}
	else
	{
		HMENU h = FindMenuItem(hMenu, iInsertAfter, &iPos);
		if (h)
		{
			hMenu = h;
			iPos++;
		}
	}

	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
	mi.fState = uiSate;
	mi.fType = MFT_STRING;
	mi.hSubMenu = subMenu;
	mi.dwTypeData = (LPSTR)text;
	InsertMenuItem(hMenu, iPos, true, &mi);
}

HMENU FindMenuItem(HMENU hMenu, int iCmd, int* iPos)
{
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_ID | MIIM_SUBMENU;
	for (int i = 0; i < GetMenuItemCount(hMenu); i++)
	{
		GetMenuItemInfo(hMenu, i, true, &mi);
		if (mi.hSubMenu)
		{
			HMENU hSubMenu = FindMenuItem(mi.hSubMenu, iCmd, iPos);
			if (hSubMenu)
				return hSubMenu;
		}
		if (mi.wID == iCmd)
		{
			*iPos = i;
			return hMenu;
		}
	}
	return NULL;
}

void SWSSetMenuText(HMENU hMenu, int iCmd, const char* cText)
{
	int iPos;
	hMenu = FindMenuItem(hMenu, iCmd, &iPos);
	if (hMenu)
	{
		MENUITEMINFO mi={sizeof(MENUITEMINFO),};
		mi.fMask = MIIM_TYPE;
		mi.fType = MFT_STRING;
		mi.dwTypeData = (char*)cText;
		SetMenuItemInfo(hMenu, iPos, true, &mi);
	}
}

int SWSGetMenuPosFromID(HMENU hMenu, UINT id)
{	// Replacement for deprecated windows func GetMenuPosFromID
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_ID;
	for (int i = 0; i < GetMenuItemCount(hMenu); i++)
	{
		GetMenuItemInfo(hMenu, i, true, &mi);
		if (mi.wID == id)
			return i;
	}
	return -1;
}

// *************************** MENU CREATION ***************************

void SWSCreateExtensionsMenu(HMENU hMenu)
{
	// Create the common "Extensions" menu 
	AddToMenu(hMenu, "About SWS Extensions", NamedCommandLookup("_SWS_ABOUT"));
	AddToMenu(hMenu, "Auto Color/Icon", NamedCommandLookup("_SWSAUTOCOLOR_OPEN"));

	HMENU hAutoRenderSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hAutoRenderSubMenu, "Autorender");
	AddToMenu(hAutoRenderSubMenu, "Batch render regions", NamedCommandLookup("_AUTORENDER"));
	AddToMenu(hAutoRenderSubMenu, "Edit project metadata", NamedCommandLookup("_AUTORENDER_METADATA"));
	AddToMenu(hAutoRenderSubMenu, "Open Render Path", NamedCommandLookup("_AUTORENDER_OPEN_RENDER_PATH"));
	AddToMenu(hAutoRenderSubMenu, "Show Instructions", NamedCommandLookup("_AUTORENDER_HELP"));
	AddToMenu(hAutoRenderSubMenu, "Global Preferences", NamedCommandLookup("_AUTORENDER_PREFERENCES"));

	AddToMenu(hMenu, "Cue Buss Generator", NamedCommandLookup("_S&M_SENDS4"));
	AddToMenu(hMenu, "Envelope Processor...", NamedCommandLookup("_PADRE_ENVPROC"));
	AddToMenu(hMenu, "Fill gaps...", NamedCommandLookup("_SWS_AWFILLGAPSADV"));
	AddToMenu(hMenu, "Find", NamedCommandLookup("_S&M_SHOWFIND"));
	AddToMenu(hMenu, "Groove Tool...", NamedCommandLookup("_FNG_GROOVE_TOOL"));
/*JFB: commented: tested some => crash
	HMENU hItemTkSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hItemTkSubMenu, "Item/Take");
	AddToMenu(hItemTkSubMenu, "Repeat Paste...", NamedCommandLookup("_XENAKIOS_REPEATPASTE"));
	AddToMenu(hItemTkSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hItemTkSubMenu, "Randomize item positions...", NamedCommandLookup("_XENAKIOS_RANDOMIZE_ITEMPOS"));
	AddToMenu(hItemTkSubMenu, "Move selected items to edit cursor", NamedCommandLookup("_XENAKIOS_MOVEITEMSTOEDITCURSOR"));
	AddToMenu(hItemTkSubMenu, "Move selected items left by item length", NamedCommandLookup("_XENAKIOS_MOVEITEMSLEFTBYLEN"));
	AddToMenu(hItemTkSubMenu, "Trim/untrim item left edge to edit cursor", NamedCommandLookup("_XENAKIOS_TRIM_LEFTEDGETO_EDCURSOR"));
	AddToMenu(hItemTkSubMenu, "Trim/untrim item right edge to edit cursor", NamedCommandLookup("_XENAKIOS_TRIM_RIGHTEDGETO_EDCURSOR"));
	AddToMenu(hItemTkSubMenu, "Reposition selected items...", NamedCommandLookup("_XENAKIOS_REPOSITION_ITEMS"));
	AddToMenu(hItemTkSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hItemTkSubMenu, "Rename selected takes...", NamedCommandLookup("_XENAKIOS_RENAMEMULTIPLETAKES"));
	AddToMenu(hItemTkSubMenu, "Auto-rename selected takes...", NamedCommandLookup("_XENAKIOS_AUTORENAMETAKES"));
	AddToMenu(hItemTkSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hItemTkSubMenu, "Invert item selection", NamedCommandLookup("_XENAKIOS_INVERTITEMSELECTION"));
	AddToMenu(hItemTkSubMenu, "Select items to start of track", NamedCommandLookup("_XENAKIOS_SELITEMSTOSTARTOFTRACK"));
	AddToMenu(hItemTkSubMenu, "Select items to end of track", NamedCommandLookup("_XENAKIOS_SELITEMSTOENDOFTRACK"));
	AddToMenu(hItemTkSubMenu, "Select first items of selected tracks", NamedCommandLookup("_XENAKIOS_SELFIRSTITEMSOFTRACKS"));
*/
	AddToMenu(hMenu, "LFO Generator...", NamedCommandLookup("_PADRE_ENVLFO"));
	AddToMenu(hMenu, "Live Configs", NamedCommandLookup("_S&M_SHOWMIDILIVE"));
	AddToMenu(hMenu, "MarkerList", NamedCommandLookup("_SWSMARKERLIST1"));

	HMENU hMarkerSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hMarkerSubMenu, "Marker utilites");
	AddToMenu(hMarkerSubMenu, "Load marker set...", NamedCommandLookup("_SWSMARKERLIST2"));
	AddToMenu(hMarkerSubMenu, "Save marker set...", NamedCommandLookup("_SWSMARKERLIST3"));
	AddToMenu(hMarkerSubMenu, "Delete marker set...", NamedCommandLookup("_SWSMARKERLIST4"));
	AddToMenu(hMarkerSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMarkerSubMenu, "Copy marker set to clipboard", NamedCommandLookup("_SWSMARKERLIST5"));
	AddToMenu(hMarkerSubMenu, "Paste marker set from clipboard", NamedCommandLookup("_SWSMARKERLIST6"));
	AddToMenu(hMarkerSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMarkerSubMenu, "Reorder marker IDs", NamedCommandLookup("_SWSMARKERLIST7"));
	AddToMenu(hMarkerSubMenu, "Reorder region IDs", NamedCommandLookup("_SWSMARKERLIST8"));
	AddToMenu(hMarkerSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMarkerSubMenu, "Select next region", NamedCommandLookup("_SWS_SELNEXTREG"));
	AddToMenu(hMarkerSubMenu, "Select prev region", NamedCommandLookup("_SWS_SELPREVREG"));
	AddToMenu(hMarkerSubMenu, "Delete all markers", NamedCommandLookup("_SWSMARKERLIST9"));
	AddToMenu(hMarkerSubMenu, "Delete all regions", NamedCommandLookup("_SWSMARKERLIST10"));

	AddToMenu(hMenu, "Notes/Help", NamedCommandLookup("_S&M_SHOWNOTESHELP"));
	AddToMenu(hMenu, "Project List", NamedCommandLookup("_SWS_PROJLIST_OPEN"));

	HMENU hPrjMgmtSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hPrjMgmtSubMenu, "Project Management");
	AddToMenu(hPrjMgmtSubMenu, "Open projects from list...", NamedCommandLookup("_SWS_PROJLISTSOPEN"));
	AddToMenu(hPrjMgmtSubMenu, "Save list of open projects...", NamedCommandLookup("_SWS_PROJLISTSAVE"));
	AddToMenu(hPrjMgmtSubMenu, "Add related project(s)...", NamedCommandLookup("_SWS_ADDRELATEDPROJ"));
	AddToMenu(hPrjMgmtSubMenu, "Delete related project...", NamedCommandLookup("_SWS_DELRELATEDPROJ"));
	AddToMenu(hPrjMgmtSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hPrjMgmtSubMenu, "(related projects list)", NamedCommandLookup("_SWS_OPENRELATED1"));

	AddToMenu(hMenu, "ReaConsole...", NamedCommandLookup("_SWSCONSOLE"));
	AddToMenu(hMenu, "Resources", NamedCommandLookup("_S&M_SHOWFXCHAINSLOTS"));
	AddToMenu(hMenu, "Snapshots", NamedCommandLookup("_SWSSNAPSHOT_OPEN"));
/*JFB "Xen's Track" sub-menu todo?
In case someone is motivated:
item_68=_XENAKIOS_RESETTRACKVOLANDPAN1 Reset volume and pan of selected tracks
item_69=_XENAKIOS_RESETTRACKVOL1 Set volume of selected tracks to 0.0 dB
item_70=_XENAKIOS_SYMMETRICAL_CHANPANSLTOR Pan selected tracks symmetrically, left to right
item_71=_XENAKIOS_SYMMETRICAL_CHANPANSRTOL Pan selected tracks symmetrically, right to left
item_72=_XENAKIOS_PANTRACKSRANDOM Pan selected tracks randomly
item_73=_XENAKIOS_PANTRACKSCENTER Pan selected tracks to center
item_74=_XENAKIOS_PANTRACKSLEFT Pan selected tracks to left
item_75=_XENAKIOS_PANTRACKSRIGHT Pan selected tracks to right
item_76=_XENAKIOS_TOGTRAXVISMIXER Toggle selected tracks visible in mixer
item_77=_XENAKIOS_SELTRAX_RECARMED Set selected tracks record armed
item_78=_XENAKIOS_SELTRAX_RECUNARMED Set selected tracks record unarmed
item_79=_XENAKIOS_RENAMETRAXDLG Rename selected tracks...
*/
	AddToMenu(hMenu, "Tracklist", NamedCommandLookup("_SWSTL_OPEN"));
	AddToMenu(hMenu, "Zoom preferences", NamedCommandLookup("_SWS_ZOOMPREFS"));

	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	HMENU hOptionsSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hOptionsSubMenu, "SWS Options");
	AddToMenu(hOptionsSubMenu, "Enable marker actions", NamedCommandLookup("_SWSMA_TOGGLE"));
	AddToMenu(hOptionsSubMenu, "Enable red ruler while recording", NamedCommandLookup("_SWS_RECREDRULER"));
	AddToMenu(hOptionsSubMenu, "Enable auto coloring", NamedCommandLookup("_SWSAUTOCOLOR_ENABLE"));
	AddToMenu(hOptionsSubMenu, "Enable auto icon", NamedCommandLookup("_S&MAUTOICON_ENABLE"));

}