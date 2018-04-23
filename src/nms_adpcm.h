#pragma once

#define NMS_SAMPLES_PER_BLOCK 160
#define NMS_BLOCK_SHORTS_32 41
#define NMS_BLOCK_SHORTS_24 31
#define NMS_BLOCK_SHORTS_16 21

/* Variable names from ITU G.726 spec */
struct nms_adpcm_state
{
    /* Log of the step size multiplier. Operated on by codewords. */
    int yl;

    /* Quantizer step size multiplier. Generated from yl. */
    int y;

    /* Coefficents of the pole predictor */
    int a[2];

    /* Coefficents of the zero predictor  */
    int b[6];

    /* Previous quantized deltas (multiplied by 2^14) */
    int d_q[7];

    /* d_q [x] + s_ez [x], used by the pole-predictor for signs only. */
    int p[3];

    /* Previous reconstructed signal values. */
    int s_r[2];

    /* Zero predictor components of the signal estimate. */
    int s_ez;

    /* Signal estimate, (including s_ez). */
    int s_e;

    /* The most recent codeword (enc:generated, dec:inputted) */
    int Ik;

    int parity;

    /*
    ** Offset into code tables for the bitrate.
    ** 2-bit words: +0
    ** 3-bit words: +8
    ** 4-bit words: +16
    */
    int t_off;
};

enum nms_enc_type
{
    NMS16,
    NMS24,
    NMS32
};

void nms_adpcm_codec_init(struct nms_adpcm_state *s, enum nms_enc_type type);

uint8_t nms_adpcm_encode_sample(struct nms_adpcm_state *s, int16_t sl);
int16_t nms_adpcm_decode_sample(struct nms_adpcm_state *s, uint8_t code);

void nms_adpcm_block_unpack_16(const uint16_t block[], int16_t codewords[], int16_t *rms);
void nms_adpcm_block_unpack_24(const uint16_t block[], int16_t codewords[], int16_t *rms);
void nms_adpcm_block_unpack_32(const uint16_t block[], int16_t codewords[], int16_t *rms);
