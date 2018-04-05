/*
** Copyright (C) 1999-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2004-2005 David Viens <davidv@plogue.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "config.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"
#include "wavlike.h"

#define WAV_BEXT_MIN_CHUNK_SIZE (602)
#define WAV_BEXT_MAX_CHUNK_SIZE (10 * 1024)

#define WAV_CART_MIN_CHUNK_SIZE (2048)
#define WAV_CART_MAX_CHUNK_SIZE (0xffffffff)

static int exif_subchunk_parse(SF_PRIVATE *psf, uint32_t length);

// Known WAVEFORMATEXTENSIBLE GUIDS.
static const EXT_SUBFORMAT MSGUID_SUBTYPE_PCM = {
    0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

static const EXT_SUBFORMAT MSGUID_SUBTYPE_MS_ADPCM = {
    0x00000002, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

static const EXT_SUBFORMAT MSGUID_SUBTYPE_IEEE_FLOAT = {
    0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

static const EXT_SUBFORMAT MSGUID_SUBTYPE_ALAW = {
    0x00000006, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

static const EXT_SUBFORMAT MSGUID_SUBTYPE_MULAW = {
    0x00000007, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

/*
 * the next two are from
 * http://dream.cs.bath.ac.uk/researchdev/wave-ex/bformat.html
 */

static const EXT_SUBFORMAT MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_PCM = {
    0x00000001, 0x0721, 0x11d3, {0x86, 0x44, 0xc8, 0xc1, 0xca, 0x00, 0x00, 0x00}};

static const EXT_SUBFORMAT MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT = {
    0x00000003, 0x0721, 0x11d3, {0x86, 0x44, 0xc8, 0xc1, 0xca, 0x00, 0x00, 0x00}};

#if 0
/* maybe interesting one day to read the following through sf_read_raw */
/* http://www.bath.ac.uk/~masrwd/pvocex/pvocex.html */
static const EXT_SUBFORMAT MSGUID_SUBTYPE_PVOCEX =
{
	0x8312b9c2, 0x2e6e, 0x11d4, { 0xa8, 0x24, 0xde, 0x5b, 0x96, 0xc3, 0xab, 0x21 }
};
#endif

/* This stores which bit in dwChannelMask maps to which channel */
static const struct chanmap_s
{
    int id;
    const char *name;
} channel_mask_bits[] = {
    /* WAVEFORMATEXTENSIBLE doesn't distuingish FRONT_LEFT from LEFT */
    {SF_CHANNEL_MAP_LEFT, "L"},
    {SF_CHANNEL_MAP_RIGHT, "R"},
    {SF_CHANNEL_MAP_CENTER, "C"},
    {SF_CHANNEL_MAP_LFE, "LFE"},
    {SF_CHANNEL_MAP_REAR_LEFT, "Ls"},
    {SF_CHANNEL_MAP_REAR_RIGHT, "Rs"},
    {SF_CHANNEL_MAP_FRONT_LEFT_OF_CENTER, "Lc"},
    {SF_CHANNEL_MAP_FRONT_RIGHT_OF_CENTER, "Rc"},
    {SF_CHANNEL_MAP_REAR_CENTER, "Cs"},
    {SF_CHANNEL_MAP_SIDE_LEFT, "Sl"},
    {SF_CHANNEL_MAP_SIDE_RIGHT, "Sr"},
    {SF_CHANNEL_MAP_TOP_CENTER, "Tc"},
    {SF_CHANNEL_MAP_TOP_FRONT_LEFT, "Tfl"},
    {SF_CHANNEL_MAP_TOP_FRONT_CENTER, "Tfc"},
    {SF_CHANNEL_MAP_TOP_FRONT_RIGHT, "Tfr"},
    {SF_CHANNEL_MAP_TOP_REAR_LEFT, "Trl"},
    {SF_CHANNEL_MAP_TOP_REAR_CENTER, "Trc"},
    {SF_CHANNEL_MAP_TOP_REAR_RIGHT, "Trr"},
};

static int wavex_guid_equal(const EXT_SUBFORMAT *first, const EXT_SUBFORMAT *second)
{
    return !memcmp(first, second, sizeof(EXT_SUBFORMAT));
}

int wavlike_read_fmt_chunk(SF_PRIVATE *psf, int fmtsize)
{
    WAVLIKE_PRIVATE *wpriv;
    WAV_FMT *wav_fmt;
    int bytesread, k, bytespersec = 0;

    if ((wpriv = (WAVLIKE_PRIVATE *)psf->container_data) == NULL)
        return SFE_INTERNAL;
    wav_fmt = &wpriv->wav_fmt;

    memset(wav_fmt, 0, sizeof(WAV_FMT));

    if (fmtsize < 16)
        return SFE_WAV_FMT_SHORT;

    /* assume psf->rwf_endian is already properly set */

    /* Read the minimal WAV file header here. */
    bytesread = psf->binheader_readf("224422", &(wav_fmt->format), &(wav_fmt->min.channels),
                                    &(wav_fmt->min.samplerate), &(wav_fmt->min.bytespersec),
                                    &(wav_fmt->min.blockalign), &(wav_fmt->min.bitwidth));

    psf->log_printf("  Format        : 0x%X => %s\n", wav_fmt->format,
                   wavlike_format_str(wav_fmt->format));
    psf->log_printf("  Channels      : %d\n", wav_fmt->min.channels);
    psf->log_printf("  Sample Rate   : %d\n", wav_fmt->min.samplerate);

    if (wav_fmt->format == WAVE_FORMAT_PCM && wav_fmt->min.blockalign == 0 &&
        wav_fmt->min.bitwidth > 0 && wav_fmt->min.channels > 0)
    {
        wav_fmt->min.blockalign =
            wav_fmt->min.bitwidth / 8 + (wav_fmt->min.bitwidth % 8 > 0 ? 1 : 0);
        wav_fmt->min.blockalign *= wav_fmt->min.channels;
        psf->log_printf("  Block Align   : 0 (should be %d)\n", wav_fmt->min.blockalign);
    }
    else
        psf->log_printf("  Block Align   : %d\n", wav_fmt->min.blockalign);

    if (wav_fmt->format == WAVE_FORMAT_PCM && wav_fmt->min.bitwidth == 24 &&
        wav_fmt->min.blockalign == 4 * wav_fmt->min.channels)
    {
        psf->log_printf("  Bit Width     : 24\n");

        psf->log_printf("\n"
                            "  Ambiguous information in 'fmt ' chunk. Possibile file types:\n"
                            "    0) Invalid IEEE float file generated by Syntrillium's "
                            "Cooledit!\n"
                            "    1) File generated by ALSA's arecord containing 24 bit samples "
                            "in 32 bit containers.\n"
                            "    2) 24 bit file with incorrect Block Align value.\n"
                            "\n");

        wpriv->fmt_is_broken = 1;
    }
    else if (wav_fmt->min.bitwidth == 0)
    {
        switch (wav_fmt->format)
        {
        case WAVE_FORMAT_GSM610:
        case WAVE_FORMAT_IPP_ITU_G_723_1:
            psf->log_printf("  Bit Width     : %d\n", wav_fmt->min.bitwidth);
            break;
        default:
            psf->log_printf("  Bit Width     : %d (should not be 0)\n", wav_fmt->min.bitwidth);
        }
    }
    else
    {
        switch (wav_fmt->format)
        {
        case WAVE_FORMAT_GSM610:
        case WAVE_FORMAT_IPP_ITU_G_723_1:
            psf->log_printf("  Bit Width     : %d (should be 0)\n", wav_fmt->min.bitwidth);
            break;
        default:
            psf->log_printf("  Bit Width     : %d\n", wav_fmt->min.bitwidth);
        }
    };

    psf->sf.samplerate = wav_fmt->min.samplerate;
    psf->sf.frames = 0; /* Correct this when reading data chunk. */
    psf->sf.channels = wav_fmt->min.channels;

    switch (wav_fmt->format)
    {
    case WAVE_FORMAT_PCM:
    case WAVE_FORMAT_IEEE_FLOAT:
        bytespersec = wav_fmt->min.samplerate * wav_fmt->min.blockalign;
        if (wav_fmt->min.bytespersec != (unsigned)bytespersec)
            psf->log_printf("  Bytes/sec     : %d (should be %d)\n", wav_fmt->min.bytespersec,
                           bytespersec);
        else
            psf->log_printf("  Bytes/sec     : %d\n", wav_fmt->min.bytespersec);

        psf->bytewidth = BITWIDTH2BYTES(wav_fmt->min.bitwidth);
        break;

    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:
        if (wav_fmt->min.bytespersec != wav_fmt->min.samplerate * wav_fmt->min.blockalign)
            psf->log_printf("  Bytes/sec     : %d (should be %d)\n", wav_fmt->min.bytespersec,
                           wav_fmt->min.samplerate * wav_fmt->min.blockalign);
        else
            psf->log_printf("  Bytes/sec     : %d\n", wav_fmt->min.bytespersec);

        psf->bytewidth = 1;
        if (fmtsize >= 18)
        {
            bytesread += psf->binheader_readf("2", &(wav_fmt->size20.extrabytes));
            psf->log_printf("  Extra Bytes   : %d\n", wav_fmt->size20.extrabytes);
        };
        break;

    case WAVE_FORMAT_IMA_ADPCM:
        if (wav_fmt->min.bitwidth != 4)
            return SFE_WAV_ADPCM_NOT4BIT;
        if (wav_fmt->min.channels < 1 || wav_fmt->min.channels > 2)
            return SFE_WAV_ADPCM_CHANNELS;

        bytesread += psf->binheader_readf("22", &(wav_fmt->ima.extrabytes),
                                         &(wav_fmt->ima.samplesperblock));
        psf->log_printf("  Extra Bytes   : %d\n", wav_fmt->ima.extrabytes);
        if (wav_fmt->ima.samplesperblock < 1)
        {
            psf->log_printf("  Samples/Block : %d (should be > 0)\n",
                           wav_fmt->ima.samplesperblock);
            return SFE_WAV_ADPCM_SAMPLES;
        }
        else
        {
            psf->log_printf("  Samples/Block : %d\n", wav_fmt->ima.samplesperblock);
        }

        bytespersec =
            (wav_fmt->ima.samplerate * wav_fmt->ima.blockalign) / wav_fmt->ima.samplesperblock;
        if (wav_fmt->ima.bytespersec != (unsigned)bytespersec)
            psf->log_printf("  Bytes/sec     : %d (should be %d)\n", wav_fmt->ima.bytespersec,
                           bytespersec);
        else
            psf->log_printf("  Bytes/sec     : %d\n", wav_fmt->ima.bytespersec);

        break;

    case WAVE_FORMAT_MS_ADPCM:
        if (wav_fmt->msadpcm.bitwidth != 4)
            return SFE_WAV_ADPCM_NOT4BIT;
        if (wav_fmt->msadpcm.channels < 1 || wav_fmt->msadpcm.channels > 2)
            return SFE_WAV_ADPCM_CHANNELS;

        bytesread +=
            psf->binheader_readf("222", &(wav_fmt->msadpcm.extrabytes),
                                &(wav_fmt->msadpcm.samplesperblock), &(wav_fmt->msadpcm.numcoeffs));

        psf->log_printf("  Extra Bytes   : %d\n", wav_fmt->msadpcm.extrabytes);
        if (wav_fmt->ima.samplesperblock < 1)
        {
            psf->log_printf("  Samples/Block : %d (should be > 0)\n",
                           wav_fmt->ima.samplesperblock);
            return SFE_WAV_ADPCM_SAMPLES;
        }
        else
        {
            psf->log_printf("  Samples/Block : %d\n", wav_fmt->ima.samplesperblock);
        }

        bytespersec =
            (wav_fmt->min.samplerate * wav_fmt->min.blockalign) / wav_fmt->msadpcm.samplesperblock;
        if (wav_fmt->min.bytespersec == (unsigned)bytespersec)
            psf->log_printf("  Bytes/sec     : %d\n", wav_fmt->min.bytespersec);
        else if (wav_fmt->min.bytespersec ==
                 (wav_fmt->min.samplerate / wav_fmt->msadpcm.samplesperblock) *
                     wav_fmt->min.blockalign)
            psf->log_printf("  Bytes/sec     : %d (should be %d (MS BUG!))\n",
                           wav_fmt->min.bytespersec, bytespersec);
        else
            psf->log_printf("  Bytes/sec     : %d (should be %d)\n", wav_fmt->min.bytespersec,
                           bytespersec);

        if (wav_fmt->msadpcm.numcoeffs > ARRAY_LEN(wav_fmt->msadpcm.coeffs))
        {
            psf->log_printf("  No. of Coeffs : %d (should be <= %d)\n",
                           wav_fmt->msadpcm.numcoeffs, ARRAY_LEN(wav_fmt->msadpcm.coeffs));
            wav_fmt->msadpcm.numcoeffs = ARRAY_LEN(wav_fmt->msadpcm.coeffs);
        }
        else
        {
            psf->log_printf("  No. of Coeffs : %d\n", wav_fmt->msadpcm.numcoeffs);
        }

        psf->log_printf("    Index   Coeffs1   Coeffs2\n");
        for (k = 0; k < wav_fmt->msadpcm.numcoeffs; k++)
        {
            char buffer[128];

            bytesread += psf->binheader_readf("22", &(wav_fmt->msadpcm.coeffs[k].coeff1),
                                             &(wav_fmt->msadpcm.coeffs[k].coeff2));
            snprintf(buffer, sizeof(buffer), "     %2d     %7d   %7d\n", k,
                     wav_fmt->msadpcm.coeffs[k].coeff1, wav_fmt->msadpcm.coeffs[k].coeff2);
            psf->log_printf(buffer);
        };
        break;

    case WAVE_FORMAT_GSM610:
        if (wav_fmt->gsm610.channels != 1 || wav_fmt->gsm610.blockalign != 65)
            return SFE_WAV_GSM610_FORMAT;

        bytesread += psf->binheader_readf("22", &(wav_fmt->gsm610.extrabytes),
                                         &(wav_fmt->gsm610.samplesperblock));

        if (wav_fmt->gsm610.samplesperblock != 320)
            return SFE_WAV_GSM610_FORMAT;

        bytespersec = (wav_fmt->gsm610.samplerate * wav_fmt->gsm610.blockalign) /
                      wav_fmt->gsm610.samplesperblock;
        if (wav_fmt->gsm610.bytespersec != (unsigned)bytespersec)
            psf->log_printf("  Bytes/sec     : %d (should be %d)\n",
                           wav_fmt->gsm610.bytespersec, bytespersec);
        else
            psf->log_printf("  Bytes/sec     : %d\n", wav_fmt->gsm610.bytespersec);

        psf->log_printf("  Extra Bytes   : %d\n", wav_fmt->gsm610.extrabytes);
        psf->log_printf("  Samples/Block : %d\n", wav_fmt->gsm610.samplesperblock);
        break;

    case WAVE_FORMAT_EXTENSIBLE:
        if (wav_fmt->ext.bytespersec != wav_fmt->ext.samplerate * wav_fmt->ext.blockalign)
            psf->log_printf("  Bytes/sec     : %d (should be %d)\n", wav_fmt->ext.bytespersec,
                           wav_fmt->ext.samplerate * wav_fmt->ext.blockalign);
        else
            psf->log_printf("  Bytes/sec     : %d\n", wav_fmt->ext.bytespersec);

        bytesread += psf->binheader_readf("224", &(wav_fmt->ext.extrabytes),
                                         &(wav_fmt->ext.validbits), &(wav_fmt->ext.channelmask));

        psf->log_printf("  Valid Bits    : %d\n", wav_fmt->ext.validbits);

        if (wav_fmt->ext.channelmask == 0)
        {
            psf->log_printf("  Channel Mask  : 0x0 (should not be zero)\n");
        }
        else
        {
            char buffer[512];
            size_t bit;

            wpriv->wavex_channelmask = wav_fmt->ext.channelmask;

            /* It's probably wise to ignore the channel mask if it is all zero */
            free(psf->channel_map);

            if ((psf->channel_map = (int *)calloc(psf->sf.channels, sizeof(psf->channel_map[0]))) == NULL)
                return SFE_MALLOC_FAILED;

            /* Terminate the buffer we're going to append_snprintf into. */
            buffer[0] = 0;

            for (bit = k = 0; bit < ARRAY_LEN(channel_mask_bits) && k < psf->sf.channels; bit++)
            {
                if (wav_fmt->ext.channelmask & (1 << bit))
                {
                    if (k > psf->sf.channels)
                    {
                        psf->log_printf("*** More channel map bits than "
                                            "there are channels.\n");
                        break;
                    };

                    psf->channel_map[k++] = channel_mask_bits[bit].id;
                    append_snprintf(buffer, sizeof(buffer), "%s, ", channel_mask_bits[bit].name);
                };
            };

            /* Remove trailing ", ". */
            bit = strlen(buffer);
            if (bit >= 2)
            {
                buffer[--bit] = 0;
                buffer[--bit] = 0;
            };

            if (k != psf->sf.channels)
            {
                psf->log_printf("  Channel Mask  : 0x%X\n", wav_fmt->ext.channelmask);
                psf->log_printf("*** Less channel map bits than there are channels.\n");
            }
            else
            {
                psf->log_printf("  Channel Mask  : 0x%X (%s)\n", wav_fmt->ext.channelmask,
                               buffer);
            }
        };

        bytesread +=
            psf->binheader_readf("422", &(wav_fmt->ext.esf.esf_field1),
                                &(wav_fmt->ext.esf.esf_field2), &(wav_fmt->ext.esf.esf_field3));

        /* compare the esf_fields with each known GUID? and print? */
        psf->log_printf("  Subformat\n");
        psf->log_printf("    esf_field1 : 0x%X\n", wav_fmt->ext.esf.esf_field1);
        psf->log_printf("    esf_field2 : 0x%X\n", wav_fmt->ext.esf.esf_field2);
        psf->log_printf("    esf_field3 : 0x%X\n", wav_fmt->ext.esf.esf_field3);
        psf->log_printf("    esf_field4 : ");
        for (k = 0; k < 8; k++)
        {
            bytesread += psf->binheader_readf("1", &(wav_fmt->ext.esf.esf_field4[k]));
            psf->log_printf("0x%X ", wav_fmt->ext.esf.esf_field4[k] & 0xFF);
        };
        psf->log_printf("\n");
        psf->bytewidth = BITWIDTH2BYTES(wav_fmt->ext.bitwidth);

        /* Compare GUIDs for known ones. */
        if (wavex_guid_equal(&wav_fmt->ext.esf, &MSGUID_SUBTYPE_PCM))
        {
            psf->sf.format = SF_FORMAT_WAVEX | u_bitwidth_to_subformat(psf->bytewidth * 8);
            psf->log_printf("    format : pcm\n");
        }
        else if (wavex_guid_equal(&wav_fmt->ext.esf, &MSGUID_SUBTYPE_MS_ADPCM))
        {
            psf->sf.format = (SF_FORMAT_WAVEX | SF_FORMAT_MS_ADPCM);
            psf->log_printf("    format : ms adpcm\n");
        }
        else if (wavex_guid_equal(&wav_fmt->ext.esf, &MSGUID_SUBTYPE_IEEE_FLOAT))
        {
            psf->sf.format =
                SF_FORMAT_WAVEX | ((psf->bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT);
            psf->log_printf("    format : IEEE float\n");
        }
        else if (wavex_guid_equal(&wav_fmt->ext.esf, &MSGUID_SUBTYPE_ALAW))
        {
            psf->sf.format = (SF_FORMAT_WAVEX | SF_FORMAT_ALAW);
            psf->log_printf("    format : A-law\n");
        }
        else if (wavex_guid_equal(&wav_fmt->ext.esf, &MSGUID_SUBTYPE_MULAW))
        {
            psf->sf.format = (SF_FORMAT_WAVEX | SF_FORMAT_ULAW);
            psf->log_printf("    format : u-law\n");
        }
        else if (wavex_guid_equal(&wav_fmt->ext.esf, &MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_PCM))
        {
            psf->sf.format = SF_FORMAT_WAVEX | u_bitwidth_to_subformat(psf->bytewidth * 8);
            psf->log_printf("    format : pcm (Ambisonic B)\n");
            wpriv->wavex_ambisonic = SF_AMBISONIC_B_FORMAT;
        }
        else if (wavex_guid_equal(&wav_fmt->ext.esf, &MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT))
        {
            psf->sf.format =
                SF_FORMAT_WAVEX | ((psf->bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT);
            psf->log_printf("    format : IEEE float (Ambisonic B)\n");
            wpriv->wavex_ambisonic = SF_AMBISONIC_B_FORMAT;
        }
        else
        {
            return SFE_UNIMPLEMENTED;
        }

        break;

    case WAVE_FORMAT_G721_ADPCM:
        psf->log_printf("  Bytes/sec     : %d\n", wav_fmt->g72x.bytespersec);
        if (fmtsize >= 20)
        {
            bytesread += psf->binheader_readf("22", &(wav_fmt->g72x.extrabytes),
                                             &(wav_fmt->g72x.auxblocksize));
            if (wav_fmt->g72x.extrabytes == 0)
                psf->log_printf("  Extra Bytes   : %d (should be 2)\n",
                               wav_fmt->g72x.extrabytes);
            else
                psf->log_printf("  Extra Bytes   : %d\n", wav_fmt->g72x.extrabytes);
            psf->log_printf("  Aux Blk Size  : %d\n", wav_fmt->g72x.auxblocksize);
        }
        else if (fmtsize == 18)
        {
            bytesread += psf->binheader_readf("2", &(wav_fmt->g72x.extrabytes));
            psf->log_printf("  Extra Bytes   : %d%s\n", wav_fmt->g72x.extrabytes,
                           wav_fmt->g72x.extrabytes != 0 ? " (should be 0)" : "");
        }
        else
        {
            psf->log_printf("*** 'fmt ' chunk should be bigger than this!\n");
        }
        break;

    case WAVE_FORMAT_NMS_VBXADPCM:
        if (wav_fmt->min.channels != 1 || wav_fmt->min.bitwidth < 2 || wav_fmt->min.bitwidth * 20 + 2 != wav_fmt->min.blockalign)
            return SFE_WAV_NMS_FORMAT;

        bytespersec = (wav_fmt->min.samplerate * wav_fmt->min.blockalign) / 160;
        if (wav_fmt->min.bytespersec == (unsigned)bytespersec)
            psf->log_printf("  Bytes/sec     : %d\n", wav_fmt->min.bytespersec);
        else
            psf->log_printf("  Bytes/sec     : %d (should be %d)\n", wav_fmt->min.bytespersec, bytespersec);
        if (fmtsize >= 18)
        {
            bytesread += psf->binheader_readf("2", &(wav_fmt->size20.extrabytes));
            psf->log_printf("  Extra Bytes   : %d\n", wav_fmt->size20.extrabytes);
        };
        break;

    default:
        psf->log_printf("*** No 'fmt ' chunk dumper for this format!\n");
        return SFE_WAV_BAD_FMT;
    };

    if (bytesread > fmtsize)
    {
        psf->log_printf("*** wavlike_read_fmt_chunk (bytesread > fmtsize)\n");
        return SFE_WAV_BAD_FMT;
    }
    else
		psf->binheader_seekf(fmtsize - bytesread, SF_SEEK_CUR);

    psf->blockwidth = wav_fmt->min.channels * psf->bytewidth;

    return 0;
}

void wavlike_write_guid(SF_PRIVATE *psf, const EXT_SUBFORMAT *subformat)
{
    psf->binheader_writef("422b", BHW4(subformat->esf_field1), BHW2(subformat->esf_field2),
                         BHW2(subformat->esf_field3), BHWv(subformat->esf_field4), BHWz(8));
}

int wavlike_gen_channel_mask(const int *chan_map, int channels)
{
    int chan, mask = 0, bit = -1, last_bit = -1;

    if (chan_map == NULL)
        return 0;

    for (chan = 0; chan < channels; chan++)
    {
        int k;

        for (k = bit + 1; k < ARRAY_LEN(channel_mask_bits); k++)
            if (chan_map[chan] == channel_mask_bits[k].id)
            {
                bit = k;
                break;
            };

        /* Check for bad sequence. */
        if (bit <= last_bit)
            return 0;

        mask += 1 << bit;
        last_bit = bit;
    };

    return mask;
}

void wavlike_analyze(SF_PRIVATE *psf)
{
    unsigned char buffer[4096];
	struct AUDIO_DETECT ad;
    int format = 0;

    if (psf->file.is_pipe)
    {
        psf->log_printf("*** Error : Reading from a pipe. Can't analyze "
                            "data section to figure out real data format.\n\n");
        return;
    };

    psf->log_printf("---------------------------------------------------\n"
                        "Format is known to be broken. Using detection code.\n");

    /* Code goes here. */
    ad.endianness = SF_ENDIAN_LITTLE;
    ad.channels = psf->sf.channels;

    psf->fseek(3 * 4 * 50, SEEK_SET);

    while (psf->fread(buffer, 1, sizeof(buffer)) == sizeof(buffer))
    {
        format = audio_detect(psf, &ad, buffer, sizeof(buffer));
        if (format != 0)
            break;
    };

    /* Seek to start of DATA section. */
    psf->fseek(psf->dataoffset, SEEK_SET);

    if (format == 0)
    {
        psf->log_printf("wavlike_analyze : detection failed.\n");
        return;
    };

    switch (format)
    {
    case SF_FORMAT_PCM_32:
    case SF_FORMAT_FLOAT:
        psf->log_printf("wavlike_analyze : found format : 0x%X\n", format);
        psf->sf.format = (psf->sf.format & ~SF_FORMAT_SUBMASK) + format;
        psf->bytewidth = 4;
        psf->blockwidth = psf->sf.channels * psf->bytewidth;
        break;

    case SF_FORMAT_PCM_24:
        psf->log_printf("wavlike_analyze : found format : 0x%X\n", format);
        psf->sf.format = (psf->sf.format & ~SF_FORMAT_SUBMASK) + format;
        psf->bytewidth = 3;
        psf->blockwidth = psf->sf.channels * psf->bytewidth;
        break;

    default:
        psf->log_printf("wavlike_analyze : unhandled format : 0x%X\n", format);
        break;
    };

    return;
}

typedef struct
{
    int ID;
    const char *name;
} WAV_FORMAT_DESC;

#define STR(x) #x
#define FORMAT_TYPE(x) \
    {                  \
        x, STR(x)      \
    }

static WAV_FORMAT_DESC wave_descs[] = {
    FORMAT_TYPE(WAVE_FORMAT_PCM),
    FORMAT_TYPE(WAVE_FORMAT_MS_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_IEEE_FLOAT),
    FORMAT_TYPE(WAVE_FORMAT_VSELP),
    FORMAT_TYPE(WAVE_FORMAT_IBM_CVSD),
    FORMAT_TYPE(WAVE_FORMAT_ALAW),
    FORMAT_TYPE(WAVE_FORMAT_MULAW),
    FORMAT_TYPE(WAVE_FORMAT_OKI_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_IMA_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_MEDIASPACE_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_SIERRA_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_G723_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_DIGISTD),
    FORMAT_TYPE(WAVE_FORMAT_DIGIFIX),
    FORMAT_TYPE(WAVE_FORMAT_DIALOGIC_OKI_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_MEDIAVISION_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_CU_CODEC),
    FORMAT_TYPE(WAVE_FORMAT_YAMAHA_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_SONARC),
    FORMAT_TYPE(WAVE_FORMAT_DSPGROUP_TRUESPEECH),
    FORMAT_TYPE(WAVE_FORMAT_ECHOSC1),
    FORMAT_TYPE(WAVE_FORMAT_AUDIOFILE_AF36),
    FORMAT_TYPE(WAVE_FORMAT_APTX),
    FORMAT_TYPE(WAVE_FORMAT_AUDIOFILE_AF10),
    FORMAT_TYPE(WAVE_FORMAT_PROSODY_1612),
    FORMAT_TYPE(WAVE_FORMAT_LRC),
    FORMAT_TYPE(WAVE_FORMAT_DOLBY_AC2),
    FORMAT_TYPE(WAVE_FORMAT_GSM610),
    FORMAT_TYPE(WAVE_FORMAT_MSNAUDIO),
    FORMAT_TYPE(WAVE_FORMAT_ANTEX_ADPCME),
    FORMAT_TYPE(WAVE_FORMAT_CONTROL_RES_VQLPC),
    FORMAT_TYPE(WAVE_FORMAT_DIGIREAL),
    FORMAT_TYPE(WAVE_FORMAT_DIGIADPCM),
    FORMAT_TYPE(WAVE_FORMAT_CONTROL_RES_CR10),
    FORMAT_TYPE(WAVE_FORMAT_NMS_VBXADPCM),
    FORMAT_TYPE(WAVE_FORMAT_ROLAND_RDAC),
    FORMAT_TYPE(WAVE_FORMAT_ECHOSC3),
    FORMAT_TYPE(WAVE_FORMAT_ROCKWELL_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_ROCKWELL_DIGITALK),
    FORMAT_TYPE(WAVE_FORMAT_XEBEC),
    FORMAT_TYPE(WAVE_FORMAT_G721_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_G728_CELP),
    FORMAT_TYPE(WAVE_FORMAT_MSG723),
    FORMAT_TYPE(WAVE_FORMAT_MPEG),
    FORMAT_TYPE(WAVE_FORMAT_RT24),
    FORMAT_TYPE(WAVE_FORMAT_PAC),
    FORMAT_TYPE(WAVE_FORMAT_MPEGLAYER3),
    FORMAT_TYPE(WAVE_FORMAT_LUCENT_G723),
    FORMAT_TYPE(WAVE_FORMAT_CIRRUS),
    FORMAT_TYPE(WAVE_FORMAT_ESPCM),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE),
    FORMAT_TYPE(WAVE_FORMAT_CANOPUS_ATRAC),
    FORMAT_TYPE(WAVE_FORMAT_G726_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_G722_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_DSAT),
    FORMAT_TYPE(WAVE_FORMAT_DSAT_DISPLAY),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_BYTE_ALIGNED),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_AC8),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_AC10),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_AC16),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_AC20),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_RT24),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_RT29),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_RT29HW),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_VR12),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_VR18),
    FORMAT_TYPE(WAVE_FORMAT_VOXWARE_TQ40),
    FORMAT_TYPE(WAVE_FORMAT_SOFTSOUND),
    FORMAT_TYPE(WAVE_FORMAT_VOXARE_TQ60),
    FORMAT_TYPE(WAVE_FORMAT_MSRT24),
    FORMAT_TYPE(WAVE_FORMAT_G729A),
    FORMAT_TYPE(WAVE_FORMAT_MVI_MV12),
    FORMAT_TYPE(WAVE_FORMAT_DF_G726),
    FORMAT_TYPE(WAVE_FORMAT_DF_GSM610),
    FORMAT_TYPE(WAVE_FORMAT_ONLIVE),
    FORMAT_TYPE(WAVE_FORMAT_SBC24),
    FORMAT_TYPE(WAVE_FORMAT_DOLBY_AC3_SPDIF),
    FORMAT_TYPE(WAVE_FORMAT_ZYXEL_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_PHILIPS_LPCBB),
    FORMAT_TYPE(WAVE_FORMAT_PACKED),
    FORMAT_TYPE(WAVE_FORMAT_RHETOREX_ADPCM),
    FORMAT_TYPE(IBM_FORMAT_MULAW),
    FORMAT_TYPE(IBM_FORMAT_ALAW),
    FORMAT_TYPE(IBM_FORMAT_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_VIVO_G723),
    FORMAT_TYPE(WAVE_FORMAT_VIVO_SIREN),
    FORMAT_TYPE(WAVE_FORMAT_DIGITAL_G723),
    FORMAT_TYPE(WAVE_FORMAT_CREATIVE_ADPCM),
    FORMAT_TYPE(WAVE_FORMAT_CREATIVE_FASTSPEECH8),
    FORMAT_TYPE(WAVE_FORMAT_CREATIVE_FASTSPEECH10),
    FORMAT_TYPE(WAVE_FORMAT_QUARTERDECK),
    FORMAT_TYPE(WAVE_FORMAT_FM_TOWNS_SND),
    FORMAT_TYPE(WAVE_FORMAT_BZV_DIGITAL),
    FORMAT_TYPE(WAVE_FORMAT_VME_VMPCM),
    FORMAT_TYPE(WAVE_FORMAT_OLIGSM),
    FORMAT_TYPE(WAVE_FORMAT_OLIADPCM),
    FORMAT_TYPE(WAVE_FORMAT_OLICELP),
    FORMAT_TYPE(WAVE_FORMAT_OLISBC),
    FORMAT_TYPE(WAVE_FORMAT_OLIOPR),
    FORMAT_TYPE(WAVE_FORMAT_LH_CODEC),
    FORMAT_TYPE(WAVE_FORMAT_NORRIS),
    FORMAT_TYPE(WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS),
    FORMAT_TYPE(WAVE_FORMAT_DVM),
    FORMAT_TYPE(WAVE_FORMAT_INTERWAV_VSC112),
    FORMAT_TYPE(WAVE_FORMAT_IPP_ITU_G_723_1),
    FORMAT_TYPE(WAVE_FORMAT_EXTENSIBLE),
};

char const *wavlike_format_str(int k)
{
    int lower, upper, mid;

    lower = -1;
    upper = sizeof(wave_descs) / sizeof(WAV_FORMAT_DESC);

    /* binary search */
    if ((wave_descs[0].ID <= k) && (k <= wave_descs[upper - 1].ID))
    {
        while (lower + 1 < upper)
        {
            mid = (upper + lower) / 2;

            if (k == wave_descs[mid].ID)
                return wave_descs[mid].name;
            if (k < wave_descs[mid].ID)
                upper = mid;
            else
                lower = mid;
        };
    };

    return "Unknown format";
}

int wavlike_srate2blocksize(int srate_chan_product)
{
    if (srate_chan_product < 12000)
        return 256;
    if (srate_chan_product < 23000)
        return 512;
    if (srate_chan_product < 44000)
        return 1024;
    return 2048;
}

int wavlike_subchunk_parse(SF_PRIVATE *psf, int chunk, uint32_t chunk_length)
{
    sf_count_t current_pos;
    char buffer[2048];
    uint32_t chunk_size, bytesread = 0;

    current_pos = psf->fseek(0, SEEK_CUR);

    if (chunk_length <= 8)
    {
        /* This case is for broken files generated by PEAK. */
        psf->log_printf("%M : %u (weird length)\n", chunk, chunk_length);
        psf->binheader_readf("m", &chunk);
		psf->binheader_seekf(chunk_length - 4, SF_SEEK_CUR);
        psf->log_printf("  %M\n", chunk);
        return 0;
    };

    if (current_pos + chunk_length > psf->filelength)
    {
        psf->log_printf("%M : %u (should be %d)\n", chunk, chunk_length,
                       (int)(psf->filelength - current_pos));
        chunk_length = psf->filelength - current_pos;
    }
    else
    {
        psf->log_printf("%M : %u\n", chunk, chunk_length);
    }

    while (bytesread < chunk_length)
    {
        uint32_t thisread;

        if ((thisread = psf->binheader_readf("m", &chunk)) == 0)
            break;
        bytesread += thisread;

        switch (chunk)
        {
        case adtl_MARKER:
        case INFO_MARKER:
            /* These markers don't contain anything, not even a chunk lebgth. */
            psf->log_printf("  %M\n", chunk);
            continue;

        case exif_MARKER:
            psf->log_printf("  %M\n", chunk);
            if (chunk_length > bytesread)
                bytesread += exif_subchunk_parse(psf, chunk_length - bytesread);
            continue;

        case data_MARKER:
            psf->log_printf("  %M inside a LIST block??? Backing out.\n", chunk);
            /* Jump back four bytes and return to caller. */
			psf->binheader_seekf(-4, SF_SEEK_CUR);
            return 0;

        case 0:
            /*
			**	Four zero bytes where a marker was expected. Assume this means
			**	the rest of the chunk is garbage.
			*/
            psf->log_printf("    *** Found weird-ass zero marker. Jumping "
                                "to end of chunk.\n");
            if (bytesread < chunk_length)
                psf->binheader_seekf(chunk_length - bytesread, SF_SEEK_CUR);
            psf->log_printf("    *** Offset is now : 0x%X\n", psf->fseek(0, SEEK_CUR));
            return 0;

        default:
            break;
        };

        switch (chunk)
        {
        case ISFT_MARKER:
        case ICOP_MARKER:
        case IARL_MARKER:
        case IART_MARKER:
        case ICMT_MARKER:
        case ICRD_MARKER:
        case IENG_MARKER:
        case IGNR_MARKER:
        case INAM_MARKER:
        case IPRD_MARKER:
        case ISBJ_MARKER:
        case ISRC_MARKER:
        case IAUT_MARKER:
        case ITRK_MARKER:
            bytesread += psf->binheader_readf("4", &chunk_size);
            chunk_size += (chunk_size & 1);
            if (chunk_size >= SIGNED_SIZEOF(buffer) || chunk_size >= chunk_length)
            {
                psf->log_printf("  *** %M : %u (too big)\n", chunk, chunk_size);
                goto cleanup_subchunk_parse;
            };

            bytesread += psf->binheader_readf("b", buffer, chunk_size);
            buffer[chunk_size] = 0;
            psf->log_printf("    %M : %s\n", chunk, buffer);
            break;

        case labl_MARKER:
        {
            int mark_id;

            bytesread += psf->binheader_readf("44", &chunk_size, &mark_id);
            chunk_size -= 4;
            chunk_size += (chunk_size & 1);
            if (chunk_size < 1 || chunk_size >= SIGNED_SIZEOF(buffer) || chunk_size >= chunk_length)
            {
                psf->log_printf("  *** %M : %u (too big)\n", chunk, chunk_size);
                goto cleanup_subchunk_parse;
            };

            bytesread += psf->binheader_readf("b", buffer, chunk_size);
            buffer[chunk_size] = 0;
            psf->log_printf("    %M : %u : %s\n", chunk, mark_id, buffer);
        };
        break;

        case DISP_MARKER:
        case ltxt_MARKER:
        case note_MARKER:
            bytesread += psf->binheader_readf("4", &chunk_size);
            chunk_size += (chunk_size & 1);
            if (chunk_size >= SIGNED_SIZEOF(buffer) || chunk_size >= chunk_length)
            {
                psf->log_printf("  *** %M : %u (too big)\n", chunk, chunk_size);
                goto cleanup_subchunk_parse;
            };

            psf->log_printf("    %M : %u\n", chunk, chunk_size);
            goto cleanup_subchunk_parse;

        default:
            bytesread += psf->binheader_readf("4", &chunk_size);
            chunk_size += (chunk_size & 1);
            psf->log_printf("    *** %M : %u\n", chunk, chunk_size);
            if (bytesread + chunk_size > chunk_length)
            {
				psf->binheader_seekf(chunk_length - bytesread + 4, SF_SEEK_CUR);
				bytesread += chunk_length - bytesread + 4;
                continue;
            }
			else
			{
				psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
				bytesread += chunk_size;
			}

            if (chunk_size >= chunk_length)
                return 0;
            break;
        };

        switch (chunk)
        {
        case ISFT_MARKER:
            psf_store_string(psf, SF_STR_SOFTWARE, buffer);
            break;
        case ICOP_MARKER:
            psf_store_string(psf, SF_STR_COPYRIGHT, buffer);
            break;
        case INAM_MARKER:
            psf_store_string(psf, SF_STR_TITLE, buffer);
            break;
        case IART_MARKER:
            psf_store_string(psf, SF_STR_ARTIST, buffer);
            break;
        case ICMT_MARKER:
            psf_store_string(psf, SF_STR_COMMENT, buffer);
            break;
        case ICRD_MARKER:
            psf_store_string(psf, SF_STR_DATE, buffer);
            break;
        case IGNR_MARKER:
            psf_store_string(psf, SF_STR_GENRE, buffer);
            break;
        case IPRD_MARKER:
            psf_store_string(psf, SF_STR_ALBUM, buffer);
            break;
        case ITRK_MARKER:
            psf_store_string(psf, SF_STR_TRACKNUMBER, buffer);
            break;
        };
    };

cleanup_subchunk_parse:

	if (chunk_length > bytesread)
	{
		psf->binheader_seekf(chunk_length - bytesread, SF_SEEK_CUR);
		bytesread += chunk_length - bytesread;
	}

    return 0;
}

void wavlike_write_strings(SF_PRIVATE *psf, int location)
{
    int k, prev_head_index, saved_head_index;

    if (psf_location_string_count(psf, location) == 0)
        return;

    prev_head_index = psf->header.indx + 4;

    psf->binheader_writef("m4m", BHWm(LIST_MARKER), BHW4(0xBADBAD), BHWm(INFO_MARKER));

    for (k = 0; k < SF_MAX_STRINGS; k++)
    {
        if (psf->strings.data[k].type == 0)
            break;
        if (psf->strings.data[k].type < 0 || psf->strings.data[k].flags != location)
            continue;

        switch (psf->strings.data[k].type)
        {
        case SF_STR_SOFTWARE:
            psf->binheader_writef("ms", BHWm(ISFT_MARKER),
                                 BHWs(psf->strings.storage + psf->strings.data[k].offset));
            break;

        case SF_STR_TITLE:
            psf->binheader_writef("ms", BHWm(INAM_MARKER),
                                 BHWs(psf->strings.storage + psf->strings.data[k].offset));
            break;

        case SF_STR_COPYRIGHT:
            psf->binheader_writef("ms", BHWm(ICOP_MARKER),
                                 BHWs(psf->strings.storage + psf->strings.data[k].offset));
            break;

        case SF_STR_ARTIST:
            psf->binheader_writef("ms", BHWm(IART_MARKER),
                                 BHWs(psf->strings.storage + psf->strings.data[k].offset));
            break;

        case SF_STR_COMMENT:
            psf->binheader_writef("ms", BHWm(ICMT_MARKER),
                                 BHWs(psf->strings.storage + psf->strings.data[k].offset));
            break;

        case SF_STR_DATE:
            psf->binheader_writef("ms", BHWm(ICRD_MARKER),
                                 BHWs(psf->strings.storage + psf->strings.data[k].offset));
            break;

        case SF_STR_GENRE:
            psf->binheader_writef("ms", BHWm(IGNR_MARKER),
                                 BHWs(psf->strings.storage + psf->strings.data[k].offset));
            break;

        case SF_STR_ALBUM:
            psf->binheader_writef("ms", BHWm(IPRD_MARKER),
                                 BHWs(psf->strings.storage + psf->strings.data[k].offset));
            break;

        case SF_STR_TRACKNUMBER:
            psf->binheader_writef("ms", BHWm(ITRK_MARKER),
                                 BHWs(psf->strings.storage + psf->strings.data[k].offset));
            break;

        default:
            break;
        };
    };

    saved_head_index = psf->header.indx;
    psf->header.indx = prev_head_index;
    psf->binheader_writef("4", BHW4(saved_head_index - prev_head_index - 4));
    psf->header.indx = saved_head_index;
}

int wavlike_read_peak_chunk(SF_PRIVATE *psf, size_t chunk_size)
{
    char buffer[256];
    uint32_t uk;

    if (chunk_size != WAVLIKE_PEAK_CHUNK_SIZE(psf->sf.channels))
    {
		psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
        psf->log_printf(
                       "*** File PEAK chunk size doesn't fit with number "
                       "of channels (%d).\n",
                       psf->sf.channels);
        return SFE_WAV_BAD_PEAK;
    };

    if ((psf->peak_info = peak_info_calloc(psf->sf.channels)) == NULL)
        return SFE_MALLOC_FAILED;

    /* read in rest of PEAK chunk. */
    psf->binheader_readf("44", &(psf->peak_info->version), &(psf->peak_info->timestamp));

    if (psf->peak_info->version != 1)
        psf->log_printf("  version    : %d *** (should be version 1)\n",
                       psf->peak_info->version);
    else
        psf->log_printf("  version    : %d\n", psf->peak_info->version);

    psf->log_printf("  time stamp : %d\n", psf->peak_info->timestamp);
    psf->log_printf("    Ch   Position       Value\n");

    for (uk = 0; uk < (uint32_t)psf->sf.channels; uk++)
    {
        float value;
        uint32_t position;

        psf->binheader_readf("f4", &value, &position);
        psf->peak_info->peaks[uk].value = value;
        psf->peak_info->peaks[uk].position = position;

        snprintf(buffer, sizeof(buffer), "    %2d   %-12" PRId64 "   %g\n", uk,
                 psf->peak_info->peaks[uk].position, psf->peak_info->peaks[uk].value);
        buffer[sizeof(buffer) - 1] = 0;
        psf->log_printf("%s", buffer);
    };

    return 0;
}

void wavlike_write_peak_chunk(SF_PRIVATE *psf)
{
    int k;

    if (psf->peak_info == NULL)
        return;

    psf->binheader_writef("m4", BHWm(PEAK_MARKER),
                         BHW4(WAVLIKE_PEAK_CHUNK_SIZE(psf->sf.channels)));
    psf->binheader_writef("44", BHW4(1), BHW4(time(NULL)));
    for (k = 0; k < psf->sf.channels; k++)
        psf->binheader_writef("ft8", BHWf(psf->peak_info->peaks[k].value),
                             BHW8(psf->peak_info->peaks[k].position));
}

static int exif_fill_and_sink(SF_PRIVATE *psf, char *buf, size_t bufsz, size_t toread)
{
    size_t bytesread = 0;

    buf[0] = 0;
    bufsz -= 1;
    if (toread < bufsz)
        bufsz = toread;
    bytesread = psf->binheader_readf("b", buf, bufsz);
    buf[bufsz] = 0;

	if (bytesread == bufsz && toread > bufsz)
	{
		psf->binheader_seekf(toread - bufsz, SF_SEEK_CUR);
		bytesread += toread - bufsz;
	}

    return bytesread;
}

/*
 * Exif specification for audio files, at JEITA CP-3451 Exif 2.2 section 5
 * (Exif Audio File Specification) http://www.exif.org/Exif2-2.PDF
 */
static int exif_subchunk_parse(SF_PRIVATE *psf, uint32_t length)
{
    uint32_t marker, dword, vmajor = -1, vminor = -1, bytesread = 0;
    char buf[4096];
    int thisread;

    while (bytesread < length)
    {
        if ((thisread = psf->binheader_readf("m", &marker)) == 0)
            break;
        bytesread += thisread;

        switch (marker)
        {
        case 0: /* camera padding? */
            break;

        case ever_MARKER:
			psf->binheader_seekf(4, SF_SEEK_CUR);
			bytesread += 4;
            bytesread += psf->binheader_readf("4", &dword);
            vmajor = 10 * (((dword >> 24) & 0xff) - '0') + (((dword >> 16) & 0xff) - '0');
            vminor = 10 * (((dword >> 8) & 0xff) - '0') + ((dword & 0xff) - '0');
            psf->log_printf("    EXIF Version : %u.%02u\n", vmajor, vminor);
            break;

        case olym_MARKER:
            bytesread += psf->binheader_readf("4", &dword);
            psf->log_printf("%M : %u\n", marker, dword);
            if (dword > length || bytesread + dword > length)
                break;
            dword += (dword & 1);
			psf->binheader_seekf(dword, SF_SEEK_CUR);
            bytesread += dword;
            break;

        case emnt_MARKER: /* design information: null-terminated string */
        case emdl_MARKER: /* model name ; null-terminated string */
        case ecor_MARKER: /* manufacturer: null-terminated string */
        case etim_MARKER: /* creation time: null-terminated string in the format "hour:minute:second.subsecond" */
        case erel_MARKER: /* relation info: null-terminated string (filename) */
        case eucm_MARKER: /* user comment: 4-byte size follows, then possibly unicode data */
            bytesread += psf->binheader_readf("4", &dword);
            bytesread += sizeof(dword);
            dword += (dword & 1);

            if (dword >= sizeof(buf))
            {
                psf->log_printf("*** Marker '%M' is too big %u\n\n", marker, dword);
                return bytesread;
            };

            bytesread += exif_fill_and_sink(psf, buf, sizeof(buf), dword);

            /* BAD - don't know what's going on here -- maybe a bug in the camera */
            /* field should be NULL-terminated but there's no room for it with the reported number */
            /*  example output:     emdl : 8 (EX-Z1050) */
            if (marker == emdl_MARKER && dword == strlen(buf) /* should be >= strlen+1*/)
            {
                psf->log_printf("    *** field size too small for string "
                                    "(sinking 2 bytes)\n");
				psf->binheader_seekf(2, SF_SEEK_CUR);
                bytesread += 2;
            };

            psf->log_printf("    %M : %u (%s)\n", marker, dword, buf);
            if (dword > length)
                return bytesread;
            break;

        default:
            psf->log_printf("    *** %M (%u): -- ignored --\n", marker, marker);
            break;
        };
    };

    return bytesread;
}

void wavlike_write_custom_chunks(SF_PRIVATE *psf)
{
    uint32_t k;

    for (k = 0; k < psf->wchunks.used; k++)
        psf->binheader_writef("m4b", BHWm(psf->wchunks.chunks[k].mark32),
                             BHW4(psf->wchunks.chunks[k].len), BHWv(psf->wchunks.chunks[k].data),
                             BHWz(psf->wchunks.chunks[k].len));
}
