#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"

typedef struct _DecodeInfo
{
    char *stego_fname;
    FILE *fptr_stego;
    char *dest_fname;
    FILE *fptr_dest;
    char output_fname[50];
} DecodeInfo;

/* read and validate inputs */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

/* perform decoding */
Status do_decoding(DecodeInfo *decodeInfo);
Status open_stego_imag(DecodeInfo *decInfo);

/* FIXED prototypes */
Status skip_bmp_header(FILE *fptr_stego);
Status decode_magic_string(FILE *fptr_stego, char *magic_string);

char lsb_to_byte(char *buffer);
int lsb_to_size(char *buffer);

Status decode_extn_size(FILE *stego, int *ext_size);
Status decode_extn(FILE *stego, char *ext, int ext_size);
Status decode_secret_file_size(FILE *stego, int *file_size);
Status decode_sec_data(FILE *stego, FILE *fptr_dest, int file_size);

#endif
