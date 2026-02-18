#include <stdio.h>
#include "encode.h"
#include "types.h"
#include "common.h"
#include <string.h>

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

    	return e_failure;
    }
    else{
        printf("Image opened successfully\n");
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

    	return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

    	return e_failure;
    }

    // No failure return e_success
    return e_success;
}

Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    
        if(argv[2] == NULL)
        {
            printf(".bmp file not passed\n");
            return e_failure;
        }
        if(strstr(argv[2], ".bmp") == NULL)
        {
            printf("invalid image file name\n");
            return e_failure;
        }
        encInfo->src_image_fname = argv[2];
        if(argv[3] == NULL)
        {
            printf(".txt file not passed\n");
            return e_failure;
        }
        if(strstr(argv[3], ".txt") == NULL)
        {
            printf("invalid sec file name");
            return e_failure;
        }
        encInfo->secret_fname = argv[3];
        if(argv[4] == NULL)
        {
            encInfo->stego_image_fname = "stego.bmp";
        }
        else
        {
            if(strstr(argv[4], ".bmp") == NULL)
            {
                 printf("invalid stego file name\n");
                 return e_failure;
            }
            encInfo->stego_image_fname = argv[4];
        }
       
        char *chr = strchr(encInfo->secret_fname, '.');
        strcpy(encInfo->extn_secret_file, chr);
        return e_success;
}

Status do_encoding(EncodeInfo *encInfo)
{
    
        int file_ret = open_files(encInfo);
        if(file_ret == e_failure)
        {
            printf("open file failed"); 
            return e_failure;
        }
        int ret = check_capacity(encInfo);
        if(ret == e_failure)
        {
           return e_failure;
        }     
        ret = copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image);
        //print offset
        printf("offset postion: %ld\n",ftell(encInfo->fptr_stego_image));
        if(ret == e_failure)
        return e_failure;
        ret = encode_magic_string(MAGIC_STRING,encInfo);
        printf("offset postion: %ld\n",ftell(encInfo->fptr_stego_image));
        if(ret == e_failure)
        return e_failure;
        encode_secret_file_extn_size(strlen(encInfo->extn_secret_file), encInfo);
        printf("offset postion: %ld\n",ftell(encInfo->fptr_stego_image));
        encode_secret_file_extn(encInfo->extn_secret_file, encInfo);
        printf("offset postion: %ld\n",ftell(encInfo->fptr_stego_image));

        /* 9. calculate secret file size and reset offset */
        fseek(encInfo->fptr_secret, 0, SEEK_END);
        long sec_file_size = ftell(encInfo->fptr_secret);
        rewind(encInfo->fptr_secret);

       /* 10. encode secret file size */
       encode_secret_file_size(sec_file_size, encInfo);
       printf("offset postion: %ld\n",ftell(encInfo->fptr_stego_image));

       /* 11. encode secret file data */
       encode_secret_file_data(encInfo);
       printf("offset postion: %ld\n",ftell(encInfo->fptr_stego_image));

       /* 12. copy remaining image data */
       copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image);
       printf("offset postion: %ld\n",ftell(encInfo->fptr_stego_image));

       /* 13. close files */
       fclose(encInfo->fptr_src_image);
       fclose(encInfo->fptr_secret);
       fclose(encInfo->fptr_stego_image);

    return e_success;
}
        
        
Status check_capacity(EncodeInfo *encInfo)
{
    int magic_len = strlen(MAGIC_STRING);
    int extn_len  = strlen(encInfo->extn_secret_file);

    fseek(encInfo->fptr_secret, 0, SEEK_END);
    encInfo->size_secret_file = ftell(encInfo->fptr_secret);
    rewind(encInfo->fptr_secret);

    int image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);


    int total_bytes = magic_len + sizeof(int) + extn_len + sizeof(int) + encInfo->size_secret_file;  

    int total_bits_required = (total_bytes * 8) + 54;

    printf("Total_bits_required = %d\n",total_bits_required);

    if (total_bits_required <= image_capacity)
    return e_success;
    
    
    return e_failure;
   
}

        
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char temp[54];
    rewind(fptr_src_image);
    /* Read 54 bytes of BMP header from source image */
    if (fread(temp, 1, 54, fptr_src_image) != 54)
    {
        printf("ERROR: Unable to read BMP header\n");
        return e_failure;
    }

    /* Write 54 bytes of BMP header to destination image */
    if (fwrite(temp, 1, 54, fptr_dest_image) != 54)
    {
        printf("ERROR: Unable to write BMP header\n");
        return e_failure;
    }

    return e_success;
}


Status encode_size_to_lsb(int data, char *image_buffer)
{
    for(int i=0;i<32;i++)
    {
        char mask = 1<<(31-i);
        char bit = data & mask;
        image_buffer[i]=image_buffer[i] & 0xfe;
        bit =bit >> (31-i);
        image_buffer[i] = image_buffer[i]|bit;
    }
    return e_success;
}
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for(int i = 0; i < 8; i++)
    {
        char bit = (data >> (7 - i)) & 1;
        image_buffer[i] = (image_buffer[i] & 0xFE) | bit;
    }
    return e_success;
}

Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    for(int i=0; i<2;i++)
    {
        char temp[8];
        fread(temp, 8, 1, encInfo->fptr_src_image);
        printf("Encoding character: %c\n", magic_string[i]);
        encode_byte_to_lsb(magic_string[i], temp);
        fwrite(temp, 8, 1, encInfo->fptr_stego_image);
        
    }
    printf("Magic string encoding success\n");
}

Status encode_secret_file_extn_size(int file_extn_size, EncodeInfo *encInfo)
{
    char temp_buffer[32];
    printf("extn = %d\n", file_extn_size);
    /* 1. read 32 bytes buff from src file */
    fread(temp_buffer, 32, 1, encInfo->fptr_src_image);
    
    /* 2. call size_to_lsb(file_extn_size, temp_buffer) */
    encode_size_to_lsb(file_extn_size, temp_buffer);
    
    /* 3. write temp_buffer to stego file */
    fwrite(temp_buffer, 32, 1, encInfo->fptr_stego_image);
    printf("encoded secret file extension size successfully\n");
    return e_success;
}

Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    int len = strlen(file_extn);

    for (int i = 0; i < len; i++)
    {
        char temp[8];

        /* 2. read 8 bytes of buffer from src file */
        fread(temp, 8, 1, encInfo->fptr_src_image);
       
        /* 3. call byte_to_lsb(file_extn[i], temp) */
        encode_byte_to_lsb(file_extn[i], temp);

        /* 4. write 8 bytes temp to stego file */
        fwrite(temp, 8, 1, encInfo->fptr_stego_image);
    }
    printf("encoded secret file extension successfully\n");
    return e_success;
}

Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    char temp[32];

    /* 2. read 32 bytes of buff from src file */
    fread(temp, 32, 1, encInfo->fptr_src_image);

    /* 3. call size_to_lsb(file_size, temp) */
    encode_size_to_lsb(file_size, temp);

    /* 4. write temp to stego file */
    fwrite(temp, 32, 1, encInfo->fptr_stego_image);
    printf("secret file size: %ld\n", file_size);
    printf("encoded secret file size successfully\n");
    return e_success;
}

Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char ch;

    while (fread(&ch, 1, 1, encInfo->fptr_secret) == 1)
    {
        char temp[8];

        /* 2. read 8 bytes buff from src file */
        fread(temp, 8, 1, encInfo->fptr_src_image);
        
        /* 3. call byte_to_lsb(ch, temp) */
        encode_byte_to_lsb(ch, temp);
       
        /* 4. write temp to stego file */
        fwrite(temp, 8, 1, encInfo->fptr_stego_image);
    }
   
    printf("encoded secret file data successfully\n");
    return e_success;
}

 Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char ch;

    while (fread(&ch, 1, 1, fptr_src) == 1)
    {
        fwrite(&ch, 1, 1, fptr_dest);
    }

    return e_success;
}
