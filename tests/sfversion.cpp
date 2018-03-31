/*
** Copyright (C) 1999-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2018 evpobr <evpobr@github.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "config.h"

#include "sndfile2k/sndfile2k.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#define BUFFER_SIZE (256)

int main(void)
{
    vector<char> strbuffer(BUFFER_SIZE);
    int ver1_length = sf_command(NULL, SFC_GET_LIB_VERSION, strbuffer.data(), sizeof(strbuffer));
	string ver1(strbuffer.begin(), strbuffer.end());
	ver1.resize(ver1_length);
	string ver2 = sf_version_string();

    if (ver1 == ver2)
    {
		cout << "Version: '" << ver1 << "'" << endl;
		return 0;
    }
	else
	{
		cerr << "Version mismatch: '" << ver1 << "' != '" << ver2 << "'" << endl;
		return 1;
	}
}
