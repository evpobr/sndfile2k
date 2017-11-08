#include <windows.h>

LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_AUS
1 VERSIONINFO
	FILEVERSION		@VER_FILEVERSION@
	PRODUCTVERSION	@VER_PRODUCTVERSION@
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
			VALUE "FileVersion", "@VER_FILEVERSION_STR@\0"
			VALUE "InternalName", "sndfile2k"
			VALUE "LegalCopyright", "Copyright (C) 2017, Licensed LGPL"
			VALUE "OriginalFilename", "@VER_ORIGINALFILENAME_STR@"
			VALUE "ProductName", "SndFile2K"
			VALUE "ProductVersion", "@VER_PRODUCTVERSION_STR@\0"
			VALUE "Language", "Language Neutral"
		}
	}
	BLOCK "VarFileInfo"
	{
		VALUE "Translation", 0x0409, 0x04E4
	}
}
