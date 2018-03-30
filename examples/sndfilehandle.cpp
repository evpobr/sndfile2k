/*
** Copyright (C) 2007-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#include "sndfile2k/sndfile2k.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace sf;

#include <cstdio>

#define BUFFER_LEN (1024)

static void create_file(const string &fname, int format)
{
    vector<short> buffer;

    SndfileHandle file;
    int channels = 2;
    int srate = 48000;

	cout << endl;
	cout << "Creating file named '" << fname << '\'' << endl;

    file = SndfileHandle(fname, SFM_WRITE, format, channels, srate);

	buffer.resize(BUFFER_LEN);

    file.write(buffer.data(), BUFFER_LEN);

	cout << endl;
    /*
	 * The SndfileHandle object will automatically close the file and
	 * release all allocated memory when the object goes out of scope.
	 * This is the Resource Acquisition Is Initailization idom.
	 * See : http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization
	 */
}

static void read_file(const string &fname)
{
    vector<short> buffer;

    SndfileHandle file;

    file = SndfileHandle(fname);

	cout << "Opened file '" << fname << "'" << endl;
	cout << "    Sample rate : " << file.samplerate() << endl;
	cout << "    Channels    : " << file.channels() << endl;

	buffer.resize(BUFFER_LEN);

    file.read(buffer.data(), BUFFER_LEN);

	cout << endl;

    /* RAII takes care of destroying SndfileHandle object. */
}

int main(void)
{
    const string fname("test.wav");

	cout << endl;
    cout << "Simple example showing usage of the C++ SndfileHandle object.";
	cout << endl;

    create_file(fname, SF_FORMAT_WAV | SF_FORMAT_PCM_16);

    read_file(fname);

	remove(fname.c_str());

    cout << "Done" << endl << endl;

	return 0;
}
