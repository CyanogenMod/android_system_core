#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include "mincrypt/sha.h"
#include "bootimg.h"

#define MT65XX_IDENTITY     "KERNEL"

typedef unsigned char byte;

int read_padding(FILE* f, unsigned itemsize, int pagesize)
{
    byte* buf = (byte*)malloc(sizeof(byte) * pagesize);
    unsigned pagemask = pagesize - 1;
    unsigned count;

    if((itemsize & pagemask) == 0) {
        free(buf);
        return 0;
    }

    count = pagesize - (itemsize & pagemask);

    fread(buf, count, 1, f);
    free(buf);
    return count;
}

void write_string_to_file(char* file, char* string)
{
    FILE* f = fopen(file, "w");
    fwrite(string, strlen(string), 1, f);
    fwrite("\n", 1, 1, f);
    fclose(f);
}

int usage() {
    printf("usage: unpackbootimg\n");
    printf("\t-i|--input boot.img\n");
    printf("\t[ -o|--output output_directory]\n");
    printf("\t[ -p|--pagesize <size-in-hexadecimal> ]\n");
    return 0;
}

int main(int argc, char** argv)
{
    char tmp[PATH_MAX];
    char* directory = "./";
    char* filename = NULL;
    int pagesize = 0;
    char check_mt65xx[15];
    int mt65xx_kernel = 0;

    argc--;
    argv++;
    while(argc > 0){
        char *arg = argv[0];
        char *val = argv[1];
        argc -= 2;
        argv += 2;
        if(!strcmp(arg, "--input") || !strcmp(arg, "-i")) {
            filename = val;
        } else if(!strcmp(arg, "--output") || !strcmp(arg, "-o")) {
            directory = val;
        } else if(!strcmp(arg, "--pagesize") || !strcmp(arg, "-p")) {
            pagesize = strtoul(val, 0, 16);
        } else {
            return usage();
        }
    }
    
    if (filename == NULL) {
        return usage();
    }
    
    int total_read = 0;
    FILE* f = fopen(filename, "rb");
    boot_img_hdr header;

    //printf("Reading header...\n");
    int i;
    for (i = 0; i <= 512; i++) {
        fseek(f, i, SEEK_SET);
        fread(tmp, BOOT_MAGIC_SIZE, 1, f);
        if (memcmp(tmp, BOOT_MAGIC, BOOT_MAGIC_SIZE) == 0)
            break;
    }
    total_read = i;
    if (i > 512) {
        printf("Android boot magic not found.\n");
        return 1;
    }
    fseek(f, i, SEEK_SET);
    // printf("Android magic found at: %d\n", i);

    fread(&header, sizeof(header), 1, f);
    /* printf("BOARD_KERNEL_CMDLINE %s\n", header.cmdline);
    printf("BOARD_KERNEL_BASE %08x\n", header.kernel_addr - 0x00008000);
    printf("BOARD_PAGE_SIZE %d\n", header.page_size); */
    
    if (pagesize == 0) {
        pagesize = header.page_size;
    }
    
    //printf("cmdline...\n");
    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-cmdline");
    write_string_to_file(tmp, header.cmdline);
    
    //printf("base...\n");
    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-base");
    char basetmp[200];
    sprintf(basetmp, "%08x", header.kernel_addr - 0x00008000);
    write_string_to_file(tmp, basetmp);

    //printf("pagesize...\n");
    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-pagesize");
    char pagesizetmp[200];
    sprintf(pagesizetmp, "%d", header.page_size);
    write_string_to_file(tmp, pagesizetmp);
    
    total_read += sizeof(header);
    //printf("total read: %d\n", total_read);
    total_read += read_padding(f, sizeof(header), pagesize);

    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-zImage");
    FILE *k = fopen(tmp, "wb");
    // Check whether this is a mt65xx kernel
    memset(check_mt65xx, 0, sizeof(check_mt65xx));
    fread(check_mt65xx, 14, 1, f);
    if (!strncasecmp(check_mt65xx + 8, MT65XX_IDENTITY, 6)) {
        // MT65xx kernel, skip 512 bytes from the page boundary
        fseek(f, total_read + 512, SEEK_SET);
        byte* kernel = (byte*)malloc(header.kernel_size - 512);
        fread(kernel, header.kernel_size - 512, 1, f);
        fwrite(kernel, header.kernel_size - 512, 1, k);

	printf("MT65xx kernel found. Will chop kernel & ramdisk.\n");
        mt65xx_kernel = 1;
    } else {
        fseek(f, total_read, SEEK_SET);
        byte* kernel = (byte*)malloc(header.kernel_size);
        fread(kernel, header.kernel_size, 1, f);
        fwrite(kernel, header.kernel_size, 1, k);
    }
    total_read += header.kernel_size;
    fclose(k);

    //printf("total read: %d\n", header.kernel_size);
    total_read += read_padding(f, header.kernel_size, pagesize);

    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-ramdisk.gz");
    FILE *r = fopen(tmp, "wb");
    if (mt65xx_kernel) {
        fseek(f, total_read + 512, SEEK_SET);
        byte* ramdisk = (byte*)malloc(header.ramdisk_size - 512);
        fread(ramdisk, header.ramdisk_size - 512, 1, f);
        fwrite(ramdisk, header.ramdisk_size - 512, 1, r);
    } else {
        byte* ramdisk = (byte*)malloc(header.ramdisk_size);
        fread(ramdisk, header.ramdisk_size, 1, f);
        fwrite(ramdisk, header.ramdisk_size, 1, r);
    }
    total_read += header.ramdisk_size;
    fclose(r);
    
    fclose(f);
    
    //printf("Total Read: %d\n", total_read);
    return 0;
}
