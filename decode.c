#include <stdio.h>
#include "decode.h"
#include "types.h"
#include <string.h>
#include "common.h"
#include <stdint.h>


//this function is used for opening the base image file i.e., stego image for decoding
Status open_stego_imag(DecodeInfo *decInfo)
{
    decInfo->fptr_stego = fopen(decInfo->stego_fname, "rb");//opening the file in read mode

    if (decInfo->fptr_stego == NULL)//if the file doesnt have anything then give error
    {
    	perror("fopen");//this function is used for checking error at fopen function
    	fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->stego_fname );

    	return e_failure;//if file not opening or null then return failure
     }
    
    return e_success;//this function succeed then returning success 
}


//from the user will get some input based on that we have to start decoding for that 
//we have to validate whether the user is passsing valid input or not the content we required
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
            //inputs:./a.out[0] -d[1] stego.bmp[2] default[3]

    // check stego.bmp
    if(argv[2] == NULL)
    {
        printf(".bmp  file is not passed");//not passed
        return e_failure;
    }

    if((strstr(argv[2],".bmp")) == NULL)
    {
        printf("Invalid image file name");//file is not bmp
        return e_failure;
    }

    decInfo->stego_fname = argv[2];//if passed then copy to structure
    
    if(argv[3] == NULL)
    {
        //store default name for output file to default
        decInfo->dest_fname = "output";
        return e_success;
    }

    else
    {
        if(strstr(argv[3],".") == NULL)
    {
        printf("invalid secret file name");//check . present or not 
        return e_failure;
    }
        decInfo->dest_fname = argv[3];//store the deafult in the dest_fname
    }

    return e_success;//function work was success
}



Status do_decoding(DecodeInfo *decInfo)
{
    char magic_array[50];
    int ret = open_stego_imag(decInfo);
    if(ret == e_failure)
    {
        printf("Error opening stego file\n");
        return e_failure;
    }

    printf("Stego image opened successfully\n");

    /* Skip BMP header */
    ret = skip_bmp_header(decInfo->fptr_stego);
    if(ret == e_failure)
    {
        printf("Error skipping BMP header\n");
        return e_failure;
    }

    printf("BMP header skipped successfully\n");

    /* Decode magic string */
    char magic_string[50];
    ret = decode_magic_string(decInfo->fptr_stego, magic_string);
    magic_string[2] = '\0';
    if(ret == e_failure)
        return e_failure;

    printf("Magic string decoded\n");

    /* Validate magic string */
    char user_magic_string[10];
    printf("Enter magic string: ");
    scanf("%s", user_magic_string);

    if(strcmp(user_magic_string, magic_string) != 0)
    {
       
        printf("Invalid magic string!!\n");
        return e_failure;
    }
    else
    {
        printf("proceed with next step\n");
    }

    /* Decode extension size */
    int ext_size;
    ret = decode_extn_size(decInfo->fptr_stego, &ext_size);
    if(ret == e_failure)
        return e_failure;

    /* Decode extension */
    char extn[20];
    printf("extn = %d\n", ext_size);
    ret = decode_extn(decInfo->fptr_stego, extn, ext_size);
    if(ret == e_failure)
        return e_failure;

    /* FIXED member name */
    strcpy(decInfo->output_fname, decInfo->dest_fname);
    strcat(decInfo->output_fname, extn);

    decInfo->fptr_dest = fopen(decInfo->output_fname, "w");
    if(decInfo->fptr_dest == NULL)
    {
        perror("fopen");
        return e_failure;
    }

    printf("Output file created successfully\n");

    /* Decode secret file size */
    int file_size;
    ret = decode_secret_file_size(decInfo->fptr_stego, &file_size);
    if(ret == e_failure)
        return e_failure;

    /* Decode secret data */
    ret = decode_sec_data(decInfo->fptr_stego, decInfo->fptr_dest, file_size);
    if(ret == e_failure)
        return e_failure;

    fclose(decInfo->fptr_stego);
    fclose(decInfo->fptr_dest);

    return e_success;
}

Status skip_bmp_header(FILE *fptr_stego)
{
     fseek(fptr_stego,54,SEEK_SET);
     printf("offset position :  %ld\n",ftell(fptr_stego));
     return e_success;
}

char lsb_to_byte(char *buffer)
{
    char ch = 0; 
    for (int i = 0; i < 8; i++) 
    { 
        ch = (ch << 1) | (buffer[i] & 1); 
    } 
    return ch;
}

int lsb_to_size(char *buffer)
{
    int size = 0; 
    for (int i = 0; i < 32; i++) 
    { 
        size = (size << 1) | (buffer[i] & 1); 
    } 
    return size;
}


Status decode_magic_string(FILE * fptr_stego, char * magic_string)

{
    int len = strlen(MAGIC_STRING);
    

    for (int i = 0; i < len; i++)
    {
        char temp[8];
        if (fread(temp, 1, 8, fptr_stego) != 8)
        {
            printf("Error reading magic string\n");
            return e_failure;
        }

        magic_string[i] = lsb_to_byte(temp);
    }

    magic_string[len] = '\0';
    printf("Decoded magic string:%s\n", magic_string);
    if(strcmp(magic_string, MAGIC_STRING) != 0)
{
    printf("Magic string mismatch!\n");
    return e_failure;
}

    printf("Pointer before magic decode = %ld\n", ftell(fptr_stego));

    return e_success;
}

Status decode_extn_size(FILE *stego, int *ext_size)
{
    char temp[32];
    fread(temp, 32, 1, stego);
    *ext_size = lsb_to_size(temp);
    return e_success;
}



Status decode_extn(FILE *stego, char *ext, int ext_size)
{
    for(int i = 0; i < ext_size; i++)
    {
        char temp[8];
        fread(temp, 1, 8, stego);
        ext[i] = lsb_to_byte(temp);
    }
    ext[ext_size] = '\0';
    printf("Decoded extension: %s\n", ext);
    return e_success;
}

Status decode_secret_file_size(FILE *stego, int *file_size)
{
    char temp[32];
    fread(temp, 1, 32, stego);
    *file_size = lsb_to_size(temp);
    return e_success;
}

Status decode_sec_data(FILE *stego, FILE *fptr_dest, int file_size)
{
    char temp[8];
    for(int i = 0; i < file_size; i++)
    {
        fread(temp, 1, 8, stego);
        char ch = lsb_to_byte(temp);
        fputc(ch, fptr_dest);
    }
    return e_success;
}


