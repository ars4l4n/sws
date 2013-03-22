/******************************************************************************
/ SnM_Util.cpp
/
/ Copyright (c) 2012-2013 Jeffos
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

#include "stdafx.h"
#include "SnM.h"
#include "../reaper/localize.h"
#include "../../WDL/projectcontext.h"


///////////////////////////////////////////////////////////////////////////////
// File util
///////////////////////////////////////////////////////////////////////////////

const char* GetFileRelativePath(const char* _fn)
{
	if (const char* p = strrchr(_fn, PATH_SLASH_CHAR))
		return p+1;
	return _fn;
}

const char* GetFileExtension(const char* _fn)
{
	if (const char* p = strrchr(_fn, '.'))
		return p+1;
	return "";
}

void GetFilenameNoExt(const char* _fullFn, char* _fn, int _fnSz)
{
	const char* p = strrchr(_fullFn, PATH_SLASH_CHAR);
	if (p) p++;
	else p = _fullFn;
	lstrcpyn(_fn, p, _fnSz);
	_fn = strrchr(_fn, '.');
	if (_fn) *_fn = '\0';
}

const char* GetFilenameWithExt(const char* _fullFn)
{
	const char* p = strrchr(_fullFn, PATH_SLASH_CHAR);
	if (p) p++;
	else p = _fullFn;
	return p;
}

// check the most retrictive OS forbidden chars (so that filenames are crossplatform)
void Filenamize(char* _fnInOut, int _fnInOutSz)
{
	if (_fnInOut)
	{
		int i=0, j=0;
		while (_fnInOut[i] && i < _fnInOutSz)
		{
			if (_fnInOut[i] != ':' && _fnInOut[i] != '/' && _fnInOut[i] != '\\' &&
				_fnInOut[i] != '*' && _fnInOut[i] != '?' && _fnInOut[i] != '\"' &&
				_fnInOut[i] != '>' && _fnInOut[i] != '<' && _fnInOut[i] != '\'' && 
				_fnInOut[i] != '|' && _fnInOut[i] != '^' /* ^ is forbidden on FAT */)
				_fnInOut[j++] = _fnInOut[i];
			i++;
		}
		if (j>=_fnInOutSz)
			j = _fnInOutSz-1;
		_fnInOut[j] = '\0';
	}
}

// check the most retrictive OS forbidden chars (so that filenames are crossplatform)
bool IsValidFilenameErrMsg(const char* _fn, bool _errMsg)
{
	bool ko = (!_fn || !*_fn
		|| strchr(_fn, ':') || strchr(_fn, '/') || strchr(_fn, '\\')
		|| strchr(_fn, '*') || strchr(_fn, '?') || strchr(_fn, '\"')
		|| strchr(_fn, '>') || strchr(_fn, '<') || strchr(_fn, '\'')
		|| strchr(_fn, '|') || strchr(_fn, '^'));

	if (ko && _errMsg)
	{
		char buf[SNM_MAX_PATH] = "";
		lstrcpyn(buf, __LOCALIZE("Empty filename!","sws_mbox"), sizeof(buf));
		if (_fn && *_fn)
			_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Invalid filename: %s\nFilenames cannot contain any of the following characters: / \\ * ? \" < > ' | :","sws_mbox"), _fn);
		MessageBox(GetMainHwnd(), buf, __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
	}
	return !ko;
}

// the API function file_exists() is a bit different, it returns false for folder paths
bool FileOrDirExists(const char* _fn)
{
	if (_fn && *_fn && *_fn!='.') // valid absolute path (1/2)?
	{
		if (const char* p = strrchr(_fn, PATH_SLASH_CHAR)) // valid absolute path (2/2)?
		{
			WDL_FastString fn;
			fn.Set(_fn, *(p+1)? 0 : (int)(p-_fn)); // // bug fix for directories, skip last PATH_SLASH_CHAR if needed
			struct stat s;
#ifdef _WIN32
			return (statUTF8(fn.Get(), &s) == 0);
#else
			return (stat(fn.Get(), &s) == 0);
#endif
		}
	}
	return false;
}

// see above remark..
bool FileOrDirExistsErrMsg(const char* _fn, bool _errMsg)
{
	bool exists = FileOrDirExists(_fn);
	if (!exists && _errMsg)
	{
		char buf[SNM_MAX_PATH] = "";
		lstrcpyn(buf, __LOCALIZE("Empty filename!","sws_mbox"), sizeof(buf));
		if (_fn && *_fn)
			_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("File or directory not found: %s","sws_mbox"), _fn);
		MessageBox(GetMainHwnd(), buf, __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
	}
	return exists;
}

bool SNM_DeleteFile(const char* _filename, bool _recycleBin)
{
	bool ok = false;
	if (_filename && *_filename) 
	{
#ifdef _WIN32
		if (_recycleBin)
			return (SendFileToRecycleBin(_filename) ? false : true); // avoid warning C4800
#endif
		ok = (DeleteFile(_filename) ? true : false); // avoid warning C4800
	}
	return ok;
}

bool SNM_CopyFile(const char* _destFn, const char* _srcFn)
{
	bool ok = false;
	if (_destFn && _srcFn) {
		if (WDL_HeapBuf* hb = LoadBin(_srcFn)) {
			ok = SaveBin(_destFn, hb);
			DELETE_NULL(hb);
		}
	}
	return ok;
}

// browse + return short resource filename (if possible and if _wantFullPath == false)
// returns false if cancelled
bool BrowseResourcePath(const char* _title, const char* _resSubDir, const char* _fileFilters, char* _fn, int _fnSize, bool _wantFullPath)
{
	bool ok = false;
	char defaultPath[SNM_MAX_PATH] = "";
	if (_snprintfStrict(defaultPath, sizeof(defaultPath), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir) < 0)
		*defaultPath = '\0';
	if (char* fn = BrowseForFiles(_title, defaultPath, NULL, false, _fileFilters)) 
	{
		if(!_wantFullPath)
			GetShortResourcePath(_resSubDir, fn, _fn, _fnSize);
		else
			lstrcpyn(_fn, fn, _fnSize);
		free(fn);
		ok = true;
	}
	return ok;
}

// get a short filename from a full resource path
// ex: C:\Documents and Settings\<user>\Application Data\REAPER\FXChains\EQ\JS\test.RfxChain -> EQ\JS\test.RfxChain
// notes: 
// - *must* work with non existing files (just some string processing here)
// - *must* be no-op for non resource paths (c:\temp\test.RfxChain -> c:\temp\test.RfxChain)
// - *must* be no-op for short resource paths 
void GetShortResourcePath(const char* _resSubDir, const char* _fullFn, char* _shortFn, int _shortFnSize)
{
	if (_resSubDir && *_resSubDir && _fullFn && *_fullFn)
	{
		char defaultPath[SNM_MAX_PATH] = "";
		if (_snprintfStrict(defaultPath, sizeof(defaultPath), "%s%c%s%c", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir, PATH_SLASH_CHAR) < 0)
			*defaultPath = '\0';
		if (strstr(_fullFn, defaultPath) == _fullFn) // no stristr: osx + utf-8
			lstrcpyn(_shortFn, (char*)(_fullFn + strlen(defaultPath)), _shortFnSize);
		else
			lstrcpyn(_shortFn, _fullFn, _shortFnSize);
	}
	else if (_shortFn)
		*_shortFn = '\0';
}

// get a full resource path from a short filename
// ex: EQ\JS\test.RfxChain -> C:\Documents and Settings\<user>\Application Data\REAPER\FXChains\EQ\JS\test.RfxChain
// notes: 
// - work with non existing files //JFB!!! humm.. looks like it fails with non existing files?
// - no-op for non resource paths (c:\temp\test.RfxChain -> c:\temp\test.RfxChain)
// - no-op for full resource paths 
void GetFullResourcePath(const char* _resSubDir, const char* _shortFn, char* _fullFn, int _fullFnSize)
{
	if (_shortFn && _fullFn) 
	{
		if (*_shortFn == '\0') {
			*_fullFn = '\0';
			return;
		}
		if (!strstr(_shortFn, GetResourcePath())) // no stristr: osx + utf-8
		{
			char resFn[SNM_MAX_PATH] = "", resDir[SNM_MAX_PATH];
			if (_snprintfStrict(resFn, sizeof(resFn), "%s%c%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir, PATH_SLASH_CHAR, _shortFn) > 0)
			{
				lstrcpyn(resDir, resFn, sizeof(resDir));
				if (char* p = strrchr(resDir, PATH_SLASH_CHAR)) *p = '\0';
				if (FileOrDirExists(resDir)) {
					lstrcpyn(_fullFn, resFn, _fullFnSize);
					return;
				}
			}
		}
		lstrcpyn(_fullFn, _shortFn, _fullFnSize);
	}
	else if (_fullFn)
		*_fullFn = '\0';
}

// _trim: if true, remove empty lines + left trim
// _approxMaxlen: 0=full file, n=truncate arround n bytes (well, a bit more)
// note: WDL's ProjectCreateFileRead() not used: seems horribly slow..
bool LoadChunk(const char* _fn, WDL_FastString* _chunkOut, bool _trim, int _approxMaxlen)
{
	if (_chunkOut && _fn && *_fn)
	{
		_chunkOut->Set("");
		if (FILE* f = fopenUTF8(_fn, "r"))
		{
			char line[SNM_MAX_CHUNK_LINE_LENGTH]="";
			while(fgets(line, sizeof(line), f) && *line)
			{
				//JFB!!! useless with SNM_ChunkParserPatcher v2
				if (_trim) 
				{
					char* p = line;
					while(*p && (*p == ' ' || *p == '\t')) p++;
					if (*p != '\n' && *p != '\r') // the !*p case is managed in Append()
						_chunkOut->Append(p);
				}
				else
					_chunkOut->Append(line);

				if (_approxMaxlen && _chunkOut->GetLength() > _approxMaxlen)
					break;
			}
			fclose(f);
			return true;
		}
	}
	return false;
}

// note: WDL's ProjectCreateFileWrite() not used: seems horribly slow..
bool SaveChunk(const char* _fn, WDL_FastString* _chunk, bool _indent)
{
	if (_fn && *_fn && _chunk)
	{
		SNM_ChunkIndenter p(_chunk, false); // no auto-commit, see trick below
		if (_indent) p.Indent();
		if (FILE* f = fopenUTF8(_fn, "w"))
		{
			fputs(p.GetUpdates() ? p.GetChunk()->Get() : _chunk->Get(), f); // avoids p.Commit(), faster
			fclose(f);
			return true;
		}
	}
	return false;
}

// returns NULL if failed, otherwise it's up to the caller to free the returned buffer
WDL_HeapBuf* LoadBin(const char* _fn)
{
	WDL_HeapBuf* hb = NULL;
	if (FILE* f = fopenUTF8(_fn , "rb"))
	{
		long lSize;
		fseek(f, 0, SEEK_END);
		lSize = ftell(f);
		rewind(f);

		hb = new WDL_HeapBuf();
		void* p = hb->Resize(lSize,false);
		if (p && hb->GetSize() == lSize) {
			size_t result = fread(p,1,lSize,f);
			if (result != lSize)
				DELETE_NULL(hb);
		}
		else
			DELETE_NULL(hb);
		fclose(f);
	}
	return hb;
}

bool SaveBin(const char* _fn, const WDL_HeapBuf* _hb)
{
	bool ok = false;
	if (_hb && _hb->GetSize())
		if (FILE* f = fopenUTF8(_fn , "wb")) {
			ok = (fwrite(_hb->Get(), 1, _hb->GetSize(), f) == _hb->GetSize());
			fclose(f);
		}
	return ok;
}

#ifdef _SNM_MISC
bool TranscodeFileToFile64(const char* _outFn, const char* _inFn)
{
	bool ok = false;
	WDL_HeapBuf* hb = LoadBin(_inFn); 
	if (hb && hb->GetSize())
	{
		ProjectStateContext* ctx = ProjectCreateFileWrite(_outFn);
		cfg_encode_binary(ctx, hb->Get(), hb->GetSize());
		delete ctx;
		ok = FileOrDirExists(_outFn);
	}
	delete hb;
	return ok;
}
#endif

// returns NULL if failed, otherwise it's up to the caller to free the returned buffer
WDL_HeapBuf* TranscodeStr64ToHeapBuf(const char* _str64)
{
	WDL_HeapBuf* hb = NULL;
	long len = strlen(_str64);
	WDL_HeapBuf hbTmp;
	void* p = hbTmp.Resize(len, false);
	if (p && hbTmp.GetSize() == len)
	{
		memcpy(p, _str64, len);
		ProjectStateContext* ctx = ProjectCreateMemCtx(&hbTmp);
		hb = new WDL_HeapBuf();
		cfg_decode_binary(ctx, hb);
		delete ctx;
	}
	return hb;
}

// use Filenamize() first!
bool GenerateFilename(const char* _dir, const char* _name, const char* _ext, char* _outFn, int _outSz)
{
	if (_dir && _name && _ext && _outFn && *_dir)
	{
		char fn[SNM_MAX_PATH] = "";
		bool slash = _dir[strlen(_dir)-1] == PATH_SLASH_CHAR;
		if (slash) {
			if (_snprintfStrict(fn, sizeof(fn), "%s%s.%s", _dir, _name, _ext) <= 0)
				return false;
		} else {
			if (_snprintfStrict(fn, sizeof(fn), "%s%c%s.%s", _dir, PATH_SLASH_CHAR, _name, _ext) <= 0)
				return false;
		}

		int i=0;
		while(FileOrDirExists(fn))
		{
			if (slash) {
				if (_snprintfStrict(fn, sizeof(fn), "%s%s_%03d.%s", _dir, _name, ++i, _ext) <= 0)
					return false;
			} else {
				if (_snprintfStrict(fn, sizeof(fn), "%s%c%s_%03d.%s", _dir, PATH_SLASH_CHAR, _name, ++i, _ext) <= 0)
					return false;
			}
		}
		lstrcpyn(_outFn, fn, _outSz);
		return true;
	}
	return false;
}

// fills a list of filenames for the desired extension
// if _ext == NULL or '\0', look for media files
// note: it is up to the caller to free _files (WDL_PtrList_DeleteOnDestroy)
void ScanFiles(WDL_PtrList<WDL_String>* _files, const char* _initDir, const char* _ext, bool _subdirs)
{
	WDL_DirScan ds;
	if (_files && _initDir && !ds.First(_initDir))
	{
		bool lookForMedias = (!_ext || !*_ext);
		do 
		{
			if (!strcmp(ds.GetCurrentFN(), ".") || !strcmp(ds.GetCurrentFN(), "..")) 
				continue;

			WDL_String* foundFn = new WDL_String();
			ds.GetCurrentFullFN(foundFn);

			if (ds.GetCurrentIsDirectory())
			{
				if (_subdirs) ScanFiles(_files, foundFn->Get(), _ext, true);
				delete foundFn;
			}
			else
			{
				const char* ext = GetFileExtension(foundFn->Get());
				if ((lookForMedias && IsMediaExtension(ext, false)) || (!lookForMedias && !_stricmp(ext, _ext)))
					_files->Add(foundFn);
				else
					delete foundFn;
			}
		}
		while(!ds.Next());
	}
}

void StringToExtensionConfig(WDL_FastString* _str, ProjectStateContext* _ctx)
{
	if (_str && _ctx)
	{
		const char* pEOL = _str->Get()-1;
		char curLine[SNM_MAX_CHUNK_LINE_LENGTH] = "";
		for(;;) 
		{
			const char* pLine = pEOL+1;
			pEOL = strchr(pLine, '\n');
			if (!pEOL)
				break;
			int curLineLen = (int)(pEOL-pLine);
			if (curLineLen >= SNM_MAX_CHUNK_LINE_LENGTH)
				curLineLen = SNM_MAX_CHUNK_LINE_LENGTH-1; // trim long lines
			memcpy(curLine, pLine, curLineLen);
			curLine[curLineLen] = '\0';
			_ctx->AddLine("%s", curLine); // "%s" needed, see http://code.google.com/p/sws-extension/issues/detail?id=358
		}
	}
}

void ExtensionConfigToString(WDL_FastString* _str, ProjectStateContext* _ctx)
{
	if (_str && _ctx)
	{
		LineParser lp(false);
		char linebuf[SNM_MAX_CHUNK_LINE_LENGTH] = "";
		for(;;) 
		{
			if (!_ctx->GetLine(linebuf, sizeof(linebuf)) && !lp.parse(linebuf))
			{
				_str->Append(linebuf);
				_str->Append("\n");
				if (lp.gettoken_str(0)[0] == '>')
					break;
			}
			else
				break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Ini file helpers
///////////////////////////////////////////////////////////////////////////////

// write a full ini file section in one go
// "the data in the buffer pointed to by the lpString parameter consists 
// of one or more null-terminated strings, followed by a final null character"
void SaveIniSection(const char* _iniSectionName, WDL_FastString* _iniSection, const char* _iniFn)
{
	if (_iniSectionName && _iniSection && _iniFn)
	{
		if (char* buf = (char*)calloc(_iniSection->GetLength()+1, sizeof(char)))  // +1 for dbl-null termination
		{
			memcpy(buf, _iniSection->Get(), _iniSection->GetLength());
			for (int j=0; j < _iniSection->GetLength(); j++)
				if (buf[j] == '\n') buf[j] = '\0';
			WritePrivateProfileStruct(_iniSectionName, NULL, NULL, 0, _iniFn); // flush section
			WritePrivateProfileSection(_iniSectionName, buf, _iniFn);
			free(buf);
		}
	}
}

void UpdatePrivateProfileSection(const char* _oldAppName, const char* _newAppName, const char* _iniFn, const char* _newIniFn)
{
	char buf[SNM_MAX_INI_SECTION]="";
	int sectionSz = GetPrivateProfileSection(_oldAppName, buf, SNM_MAX_INI_SECTION, _iniFn);
	WritePrivateProfileStruct(_oldAppName, NULL, NULL, 0, _iniFn); // flush section
	if (sectionSz)
		WritePrivateProfileSection(_newAppName, buf, _newIniFn ? _newIniFn : _iniFn);
}

void UpdatePrivateProfileString(const char* _appName, const char* _oldKey, const char* _newKey, const char* _iniFn, const char* _newIniFn)
{
	char buf[SNM_MAX_PATH] = "";
	GetPrivateProfileString(_appName, _oldKey, "", buf, sizeof(buf), _iniFn);
	WritePrivateProfileString(_appName, _oldKey, NULL, _iniFn); // remove key
	if (*buf)
		WritePrivateProfileString(_appName, _newKey, buf, _newIniFn ? _newIniFn : _iniFn);
}

void SNM_UpgradeIniFiles()
{
	g_SNM_IniVersion = GetPrivateProfileInt("General", "IniFileUpgrade", 0, g_SNM_IniFn.Get());

	if (g_SNM_IniVersion < 1) // < v2.1.0 #18
	{
		// upgrade deprecated section names 
		UpdatePrivateProfileSection("FXCHAIN", "FXChains", g_SNM_IniFn.Get());
		UpdatePrivateProfileSection("FXCHAIN_VIEW", "RESOURCE_VIEW", g_SNM_IniFn.Get());
		// upgrade deprecated key names (automatically generated now)
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClick_Type", "DblClickFXChains", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClick_Type_Tr_Template", "DblClickTrackTemplates", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClick_Type_Prj_Template", "DblClickProjectTemplates", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirFXChain", "AutoSaveDirFXChains", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirFXChain", "AutoFillDirFXChains", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirTrTemplate", "AutoSaveDirTrackTemplates", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirTrTemplate", "AutoFillDirTrackTemplates", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirPrjTemplate", "AutoSaveDirProjectTemplates", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirPrjTemplate", "AutoFillDirProjectTemplates", g_SNM_IniFn.Get());
	}
	if (g_SNM_IniVersion < 2) // < v2.1.0 #21
	{
		// move cycle actions to a new dedicated file (+ make backup if it already exists)
		WDL_FastString fn;
		fn.SetFormatted(SNM_MAX_PATH, SNM_CYCLACTION_BAK_FILE, GetResourcePath());
		if (FileOrDirExists(g_SNM_CyclIniFn.Get()))
			MoveFile(g_SNM_CyclIniFn.Get(), fn.Get());
		UpdatePrivateProfileSection("MAIN_CYCLACTIONS", "Main_Cyclactions", g_SNM_IniFn.Get(), g_SNM_CyclIniFn.Get());
		UpdatePrivateProfileSection("ME_LIST_CYCLACTIONS", "ME_List_Cyclactions", g_SNM_IniFn.Get(), g_SNM_CyclIniFn.Get());
		UpdatePrivateProfileSection("ME_PIANO_CYCLACTIONS", "ME_Piano_Cyclactions", g_SNM_IniFn.Get(), g_SNM_CyclIniFn.Get());
	}
	if (g_SNM_IniVersion < 3) // < v2.1.0 #22
	{
		WritePrivateProfileString("RESOURCE_VIEW", "DblClick_To", NULL, g_SNM_IniFn.Get()); // remove key
		UpdatePrivateProfileString("RESOURCE_VIEW", "FilterByPath", "Filter", g_SNM_IniFn.Get());
	}
	if (g_SNM_IniVersion < 4) // < v2.2.0
	{
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoSaveTrTemplateWithItems", "AutoSaveTrTemplateFlags", g_SNM_IniFn.Get());
#ifdef _WIN32
		UpdatePrivateProfileSection("data\\track_icons", "Track_icons", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirdata\\track_icons", "AutoFillDirTrack_icons", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "TiedActionsdata\\track_icons", "TiedActionsTrack_icons", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClickdata\\track_icons", "DblClickTrack_icons", g_SNM_IniFn.Get());
#else
		UpdatePrivateProfileSection("data/track_icons", "Track_icons", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirdata/track_icons", "AutoFillDirTrack_icons", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "TiedActionsdata/track_icons", "TiedActionsTrack_icons", g_SNM_IniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClickdata/track_icons", "DblClickTrack_icons", g_SNM_IniFn.Get());
#endif
	}
	if (g_SNM_IniVersion < 5) // < v2.2.0 #3
		UpdatePrivateProfileSection("LAST_CUEBUS", "CueBuss1", g_SNM_IniFn.Get());
	if (g_SNM_IniVersion < 6) // < v2.2.0 #6
		WritePrivateProfileStruct("RegionPlaylist", NULL, NULL, 0, g_SNM_IniFn.Get()); // flush section
	if (g_SNM_IniVersion < 7) // < v2.2.0 #16
	{
		UpdatePrivateProfileSection("RESOURCE_VIEW", "Resources", g_SNM_IniFn.Get());
		UpdatePrivateProfileSection("NOTES_HELP_VIEW", "Notes", g_SNM_IniFn.Get());
		UpdatePrivateProfileSection("FIND_VIEW", "Find", g_SNM_IniFn.Get());
		UpdatePrivateProfileSection("MIDI_EDITOR", "MidiEditor", g_SNM_IniFn.Get());
		WritePrivateProfileStruct("LIVE_CONFIGS", NULL, NULL, 0, g_SNM_IniFn.Get()); // flush section, everything moved to rpp

		// update nb of default actions (SNM_LIVECFG_NB_CONFIGS changed)
		WritePrivateProfileString("NbOfActions", "S&M_TOGGLE_LIVE_CFG", STR(SNM_LIVECFG_NB_CONFIGS), g_SNM_IniFn.Get());
		WritePrivateProfileString("NbOfActions", "S&M_NEXT_LIVE_CFG", STR(SNM_LIVECFG_NB_CONFIGS), g_SNM_IniFn.Get());
		WritePrivateProfileString("NbOfActions", "S&M_PREVIOUS_LIVE_CFG", STR(SNM_LIVECFG_NB_CONFIGS), g_SNM_IniFn.Get());
	}

	g_SNM_IniVersion = SNM_INI_FILE_VERSION;
}


///////////////////////////////////////////////////////////////////////////////
// String helpers
///////////////////////////////////////////////////////////////////////////////

// a _snprintf that ensures the string is always null terminated (but truncated if needed)
// also see _snprintfStrict()
// note: WDL_snprintf's return value cannot be trusted, see wdlcstring.h
//       (using it could break other sws members' code)
int _snprintfSafe(char* _buf, size_t _n, const char* _fmt, ...)
{
	va_list va;
	va_start(va, _fmt);
	*_buf='\0';
#if defined(_WIN32) && defined(_MSC_VER)
	int l = _vsnprintf(_buf, _n, _fmt, va);
	if (l < 0 || l >= (int)_n) {
		_buf[_n-1]=0;
		l = strlen(_buf);
	}
#else
	// vsnprintf() on non-win32, always null terminates
	int l = vsnprintf(_buf, _n, _fmt, va);
	if (l>=(int)_n)
		l=_n-1;
#endif
	va_end(va);
	return l;
}

// a _snprintf that returns >=0 when the string is null terminated and not truncated
// => callers must check the returned value 
// also see _snprintfSafe()
// note: WDL_snprintf's return value cannot be trusted, see wdlcstring.h
//       (using it could break other sws members' code)
int _snprintfStrict(char* _buf, size_t _n, const char* _fmt, ...)
{
	va_list va;
	va_start(va, _fmt);
	*_buf='\0';
#if defined(_WIN32) && defined(_MSC_VER)
	int l = _vsnprintf(_buf, _n, _fmt, va);
	if (l < 0 || l >= (int)_n) {
		_buf[_n-1]=0;
		l = -1;
	}
#else
	// vsnprintf() on non-win32, always null terminates
	int l = vsnprintf(_buf, _n, _fmt, va);
	if (l>=(int)_n)
		l = -1;
#endif
	va_end(va);
	return l;
}

bool GetStringWithRN(const char* _bufIn, char* _bufOut, int _bufOutSz)
{
	if (!_bufOut || !_bufIn)
		return false;

	int i=0, j=0;
	while (_bufIn[i] && j < _bufOutSz)
	{
		if (_bufIn[i] == '\n') {
			_bufOut[j++] = '\r';
			_bufOut[j++] = '\n';
		}
		else if (_bufIn[i] != '\r')
			_bufOut[j++] = _bufIn[i];
		i++;
	}

	if (j < _bufOutSz)
		_bufOut[j] = '\0';
	else
		_bufOut[_bufOutSz-1] = '\0'; // truncates, best effort..

	return (j < _bufOutSz);
}

const char* FindFirstRN(const char* _str) {
	if (_str) {
		const char* p = strchr(_str, '\r');
		if (!p) p = strchr(_str, '\n');
		return p;
	}
	return NULL;
}

char* ShortenStringToFirstRN(char* _str) {
	if (char* p = (char*)FindFirstRN(_str)) {
		*p = '\0'; 
		return p;
	}
	return NULL;
}

// replace "%02d " with _replaceCh in _str
void Replace02d(char* _str, char _replaceCh) {
	if (_str && *_str)
		if (char* p = strstr(_str, "%02d")) {
			p[0] = _replaceCh;
			if (p[4]) memmove((char*)(p+1), p+4, strlen(p+4)+1);
			else p[1] = '\0';
		}
}


///////////////////////////////////////////////////////////////////////////////
// Time/play helpers
///////////////////////////////////////////////////////////////////////////////

// note: the API's SnapToGrid() requires snap options to be enabled..
int SNM_SnapToMeasure(double _pos)
{
	int meas;
	if (TimeMap2_timeToBeats(NULL, _pos, &meas, NULL, NULL, NULL) > SNM_FUDGE_FACTOR) {
		double measPosA = TimeMap2_beatsToTime(NULL, 0.0, &meas); meas++;
		double measPosB = TimeMap2_beatsToTime(NULL, 0.0, &meas);
		return ((_pos-measPosA) > (measPosB-_pos) ? meas : meas-1);
	}
	return meas;
}

void TranslatePos(double _pos, int* _h, int* _m, int* _s, int* _ms)
{
	if (_h) *_h = int(_pos/3600);
	if (_m) *_m = int((_pos - 3600*int(_pos/3600)) / 60);
	if (_s) *_s = int(_pos - 3600*int(_pos/3600) - 60*int((_pos-3600*int(_pos/3600))/60));
	if (_ms) *_ms = int(1000*(_pos-int(_pos)) + 0.5); // rounding
}

double SeekPlay(double _pos, bool _moveView)
{
	double cursorpos = GetCursorPositionEx(NULL);
	if (PreventUIRefresh) PreventUIRefresh(1);
	SetEditCurPos2(NULL, _pos, _moveView, true);
	if ((GetPlayStateEx(NULL)&1) != 1) OnPlayButton();
	SetEditCurPos2(NULL, cursorpos, false, false);
	if (PreventUIRefresh) PreventUIRefresh(-1);
#ifdef _SNM_DEBUG
	char dbg[256] = "";
	_snprintfSafe(dbg, sizeof(dbg), "SeekPlay() - Pos: %f\n", _pos);
	OutputDebugString(dbg);
#endif
	return cursorpos;
}


///////////////////////////////////////////////////////////////////////////////
// Action helpers
// - API LIMITATION: most of the funcs here struggle to work but they do work
// - Corner-case: "Custom: " can be removed tweaked by hand, this is not 
//   managed and leads to native problems anyway
///////////////////////////////////////////////////////////////////////////////

// overrides the API's NamedCommandLookup() which can return an id although the 
// related action is not registered yet, ex: at init time, when an action used in a mouse modifier)
int SNM_NamedCommandLookup(const char* _custId, KbdSectionInfo* _section)
{
	if (_custId && *_custId && 
		!_section) //JFB API LIMITATION..
	{
		if (int cmdId = NamedCommandLookup(_custId)) 
			if (const char* desc = kbd_getTextFromCmd(cmdId, _section))
				if (*desc)
					return cmdId;
	}
	return 0;
}

// overrides the API's GetToggleCommandState() 
// in order to skip the reentrance test for sws actions
int SNM_GetToggleCommandState(int _cmdId, KbdSectionInfo* _section)
{
	if (_cmdId>0)
	{
		// SWS action => skip the reentrance test
		if (!_section) // API LIMITATION: extensions like sws can only register actions in the main section ATM
			if (COMMAND_T* cmd = SWSGetCommandByID(_cmdId))
				if (cmd->accel.accel.cmd==_cmdId && cmd->doCommand!=SWS_NOOP)
					return cmd->getEnabled ? cmd->getEnabled(cmd) : -1;

		if (GetToggleCommandState2)
			return GetToggleCommandState2(_section, _cmdId);
		 //JFB!!! removeme > 4.33pre
		else if (!_section)
			return GetToggleCommandState(_cmdId);
	}
	return -1;
}

bool LoadKbIni(WDL_PtrList<WDL_FastString>* _out)
{
	char buf[SNM_MAX_PATH] = "";
	if (_out && _snprintfStrict(buf, sizeof(buf), SNM_KB_INI_FILE, GetResourcePath()) > 0)
	{
		_out->Empty(true);
		if (FILE* f = fopenUTF8(buf, "r"))
		{
			while(fgets(buf, sizeof(buf), f) && *buf)
				if (!_strnicmp(buf,"ACT",3) || !_strnicmp(buf,"SRC",3))
					_out->Add(new WDL_FastString(buf));
			fclose(f);
			return true;
		}
	}
	return false;
}

// returns 1 for a macro, 2 for a script, 0 if not found
// _customId: custom id (both formats are allowed: "bla" and "_bla")
// _inMacroScripts: to optimize accesses to reaper-kb.ini
// _outCmds: optionnal, if any it is up to the caller to unalloc items
// note: no cache here (as the user can create new macros)
int GetMacroOrScript(const char* _customId, int _sectionUniqueId, WDL_PtrList<WDL_FastString>* _inMacroScripts, WDL_PtrList<WDL_FastString>* _outCmds)
{
	if (!_customId || !_inMacroScripts)
		return 0;

	if (*_customId == '_')
		_customId++; // custom ids in reaper-kb.ini do not start with '_'

	if (_outCmds)
		_outCmds->Empty(true);

	int found = 0;
	for (int i=0; i<_inMacroScripts->GetSize(); i++)
	{
		if (const char* cmd = _inMacroScripts->Get(i) ? _inMacroScripts->Get(i)->Get() : "")
		{
			LineParser lp(false);
			if (!lp.parse(cmd) && 
				lp.getnumtokens()>5 && // indirectly exlude key shortcuts, etc..
				!_stricmp(lp.gettoken_str(3), _customId))
			{
				int success, iniSecId = lp.gettoken_int(2, &success);
				if (success && iniSecId==_sectionUniqueId)
				{
					found = !_stricmp(lp.gettoken_str(0), "ACT") ? 1 : !_stricmp(lp.gettoken_str(0), "SRC") ? 2 : 0;
					if (_outCmds && found)
					{
						for (int i=5; i<lp.getnumtokens(); i++)
						{
							WDL_FastString* cmd = new WDL_FastString;
							const char* p = FindFirstRN(lp.gettoken_str(i)); // there are some "\r\n" sometimes
							cmd->Set(lp.gettoken_str(i), p ? (int)(p-lp.gettoken_str(i)) : 0);
							_outCmds->Add(new WDL_FastString(cmd));
						}
					}
					break;
				}
			}
		}
	}
	return found;
}

// API LIMITATION: a bit hacky but we definitely need this..
bool IsMacroOrScript(const char* _cmd, bool _cmdIsName)
{
	if (_cmd && *_cmd)
	{
		// _cmd is an action name?
		if (_cmdIsName)
		{
			static const char* custom = __localizeFunc("Custom","actions",0);
			static int customlen = strlen(custom);
			return (!_strnicmp(_cmd, custom, customlen) && _cmd[customlen] == ':');
		}
		// _cmd is a custom id
		else
		{
			int len=0;
			if (*_cmd=='_') _cmd++;
			while (*_cmd)
			{
				if ((*_cmd>='0' && *_cmd<='9') || 
//					(*_cmd>='A' && *_cmd<='F') || 
					(*_cmd>='a' && *_cmd<='f'))
				{
					_cmd++; len++;
				}
				else
					return false;
			}
			return len==SNM_MACRO_CUSTID_LEN;
		}
	}
	return false;
}

bool GetSectionURL(bool _alr, const char* _section, char* _sectionURL, int _sectionURLSize)
{
	if (!_section || !_sectionURL)
		return false;

	if (_alr)
	{
		if (!_stricmp(_section, __localizeFunc("Main","accel_sec",0)) || !strcmp(_section, __localizeFunc("Main (alt recording)","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_Main", _sectionURLSize);
		else if (!_stricmp(_section, __localizeFunc("Media explorer","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_MediaExplorer", _sectionURLSize);
		else if (!_stricmp(_section, __localizeFunc("MIDI Editor","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_MIDIEditor", _sectionURLSize);
		else if (!_stricmp(_section, __localizeFunc("MIDI Event List Editor","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_MIDIEvtList", _sectionURLSize);
		else if (!_stricmp(_section, __localizeFunc("MIDI Inline Editor","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_MIDIInline", _sectionURLSize);
		else
			return false;
	}
	else
		lstrcpyn(_sectionURL, _section, _sectionURLSize);
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Other util funcs
///////////////////////////////////////////////////////////////////////////////

#ifdef _SNM_MISC

#ifdef _WIN32
#define FNV64_IV ((WDL_UINT64)(0xCBF29CE484222325i64))
#else
#define FNV64_IV ((WDL_UINT64)(0xCBF29CE484222325LL))
#endif

WDL_UINT64 FNV64(WDL_UINT64 h, const unsigned char* data, int sz)
{
	int i;
	for (i=0; i < sz; ++i)
	{
#ifdef _WIN32
		h *= (WDL_UINT64)0x00000100000001B3i64;
#else
		h *= (WDL_UINT64)0x00000100000001B3LL;
#endif
		h ^= data[i];
	}
	return h;
}

// _strOut[65] by definition..
bool FNV64(const char* _strIn, char* _strOut)
{
	WDL_UINT64 h = FNV64_IV;
	const char *p=_strIn;
	while (*p)
	{
		char c = *p++;
		if (c == '\\')
		{
			if (*p == '\\'||*p == '"' || *p == '\'') h=FNV64(h,(unsigned char *)p,1);
			else if (*p == 'n') h=FNV64(h,(unsigned char *)"\n",1);
			else if (*p == 'r') h=FNV64(h,(unsigned char *)"\r",1);
			else if (*p == 't') h=FNV64(h,(unsigned char *)"\t",1);
			else if (*p == '0') h=FNV64(h,(unsigned char *)"",1);
			else return false;
			p++;
		}
		else
			h=FNV64(h,(unsigned char *)&c,1);
	}
	h=FNV64(h,(unsigned char *)"",1);
	return (_snprintfStrict(_strOut, 65, "%08X%08X",(int)(h>>32),(int)(h&0xffffffff)) > 0);
}

#endif

