#include <windows.h>

LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_AUS
1 VERSIONINFO
	FILEVERSION		@WIN_RC_VERSION@,0
	PRODUCTVERSION	@WIN_RC_VERSION@,0
	FILEOS			VOS__WINDOWS32
	FILETYPE		VFT_DLL
	FILESUBTYPE		VFT2_UNKNOWN
	FILEFLAGSMASK	0x00000000
	FILEFLAGS		0x00000000
{
	BLOCK "StringFileInfo"
	{
		BLOCK "040904e4"
		{
			VALUE "FileDescription", "A library for reading and writing audio files."
			VALUE "FileVersion", "@CLEAN_VERSION@.0\0"
			VALUE "Full Version", "@PACKAGE_VERSION@"
			VALUE "InternalName", "libsndfile"
			VALUE "LegalCopyright", "Copyright (C) 1999-2012, Licensed LGPL"
			VALUE "OriginalFilename", "libsndfile-1.dll"
			VALUE "ProductName", "libsndfile-1 DLL"
			VALUE "ProductVersion", "@CLEAN_VERSION@.0\0"
			VALUE "Language", "Language Neutral"
		}
	}
	BLOCK "VarFileInfo"
	{
		VALUE "Translation", 0x0409, 0x04E4
	}
}
