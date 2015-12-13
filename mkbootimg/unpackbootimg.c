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

    // does directory exist? 
    if (access(directory, F_OK) == -1) {
        if(errno == ENOENT) {
            printf("directory does not exist: '%s'\n", directory);
        } else if (errno == ENOTDIR) {
            printf("file is not a directory: '%s'\n", directory);
        } else {
            printf("could not access directory: '%s'\n", directory);
        }
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
    printf("Android magic found at: %d\n", i);

    fread(&header, sizeof(header), 1, f);
    printf("BOARD_KERNEL_CMDLINE %s\n", header.cmdline);
    printf("BOARD_KERNEL_BASE %08x\n", header.kernel_addr - 0x00008000);
    printf("BOARD_RAMDISK_OFFSET %08x\n", header.ramdisk_addr - header.kernel_addr + 0x00008000);
    printf("BOARD_SECOND_OFFSET %08x\n", header.second_addr - header.kernel_addr + 0x00008000);
    printf("BOARD_TAGS_OFFSET %08x\n",header.tags_addr - header.kernel_addr + 0x00008000);
    printf("BOARD_PAGE_SIZE %d\n", header.page_size);
    printf("BOARD_SECOND_SIZE %d\n", header.second_size);
    printf("BOARD_DT_SIZE %d\n", header.dt_size);
    
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

    //printf("ramdisk_offset...\n");
    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-ramdisk_offset");
    char ramdisktmp[200];
    sprintf(ramdisktmp, "%08x", header.ramdisk_addr - header.kernel_addr + 0x00008000);
    write_string_to_file(tmp, ramdisktmp);

    //printf("second_offset...\n");
    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-second_offset");
    char secondtmp[200];
    sprintf(secondtmp, "%08x", header.second_addr - header.kernel_addr + 0x00008000);
    write_string_to_file(tmp, secondtmp);

    //printf("tags_offset...\n");
    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-tags_offset");
    char tagstmp[200];
    sprintf(tagstmp, "%08x", header.tags_addr - header.kernel_addr + 0x00008000);
    write_string_to_file(tmp, tagstmp);

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
    byte* kernel = (byte*)malloc(header.kernel_size);
    //printf("Reading kernel...\n");
    fread(kernel, header.kernel_size, 1, f);
    total_read += header.kernel_size;
    fwrite(kernel, header.kernel_size, 1, k);
    fclose(k);

    //printf("total read: %d\n", header.kernel_size);
    total_read += read_padding(f, header.kernel_size, pagesize);


    byte* ramdisk = (byte*)malloc(header.ramdisk_size);
    //printf("Reading ramdisk...\n");
    fread(ramdisk, header.ramdisk_size, 1, f);
    total_read += header.ramdisk_size;
    sprintf(tmp, "%s/%s", directory, basename(filename));
    if(ramdisk[0] == 0x02 && ramdisk[1]== 0x21)
        strcat(tmp, "-ramdisk.lz4");
    else
        strcat(tmp, "-ramdisk.gz");
    FILE *r = fopen(tmp, "wb");
    fwrite(ramdisk, header.ramdisk_size, 1, r);
    fclose(r);

    total_read += read_padding(f, header.ramdisk_size, pagesize);

    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-second");
    FILE *s = fopen(tmp, "wb");
    byte* second = (byte*)malloc(header.second_size);
    //printf("Reading second...\n");
    fread(second, header.second_size, 1, f);
    total_read += header.second_size;
    fwrite(second, header.second_size, 1, r);
    fclose(s);

    total_read += read_padding(f, header.second_size, pagesize);

    sprintf(tmp, "%s/%s", directory, basename(filename));
    strcat(tmp, "-dt");
    FILE *d = fopen(tmp, "wb");
    byte* dt = (byte*)malloc(header.dt_size);
    //printf("Reading dt...\n");
    fread(dt, header.dt_size, 1, f);
    total_read += header.dt_size;
    fwrite(dt, header.dt_size, 1, r);
    fclose(d);
    
    fclose(f);
    
    //printf("Total Read: %d\n", total_read);
    return 0;
}
