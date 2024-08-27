#include  <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const char learn_tmpfile[] = "cmdout.tmp";

int runcmd(const char *cmd)
{
    FILE *pr = popen(cmd, "r");
    const int MAX_LEN = 128;
    int ret = 0;

    if(!pr) {
        perror("Failed to open stream, err: %w\n");
        return -1;
    }

    int fd;

    fd = open(learn_tmpfile, O_CREAT|O_WRONLY,  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("Failed to open file %m\n");
        ret = -1;
        goto done;
    }

    char outline[MAX_LEN];
    while(fgets(outline, MAX_LEN, pr) != NULL) {
        int l = strlen(outline);
        int ret = write(fd, outline, l);
        if(ret < 0) {
            perror("Write failed %m\n");
            ret = -1;
            goto done;
        }
    }

done:
    close(fd);
    pclose(pr);
    return ret;
}

#define debug_printf 
int get_page_byte(const char pages[128][128], int page_num, int page_off)
{
    const char *page = pages[page_num];
    char page_byte_str[5]={0};
    page_off *= 2;
    sprintf(page_byte_str, "0x%c%c", page[page_off], page[page_off+1]);
    //debug_printf("\t\tpage: %s\t\tpage_byte_str: %s %c %c\n", page, page_byte_str, page[page_off], page[page_off+1]);
    return strtoul(page_byte_str, 0, 16);
}

void process_va(const char pages[128][128], int pdbr, int va)
{
    const int offset_bits = 5;
    const int level2_vpn_bits = 5;
    const int vpn_bits = 10;
    int offset = va &  ((1<<offset_bits)-1);
    int vpn  = (va >>  offset_bits) & ((1<<(vpn_bits)) -1);
    int pde = (vpn >>  level2_vpn_bits);
    vpn = vpn & ((1<<(vpn_bits - level2_vpn_bits))-1);

    int page_byte = get_page_byte(pages, pdbr, pde);
    //debug_printf("\t\tpdbr: %d : offset %#X pde: %#x vpn: %#X page_byte: %#X\n", pdbr, offset, pde, vpn, page_byte);
    if (!(page_byte & 0x80)) {
        printf("%#x: PAGE FAULT\n", va);
        return;
    }
    int pte =  get_page_byte(pages, page_byte  & 0x7f, vpn);
    if (!(pte & 0x80)) {
        printf("%#x: PAGE FAULT (invalid pte)\n", va);
        return;
    }
    offset |= (pte &  0x7f) << offset_bits;
    int page = offset/32;
    int page_off = (offset % 32);
    int byte = get_page_byte(pages, page, page_off);
    printf("%#x = %#x = %#02x\n", va, offset, byte);
}

void decipher_output_smallfiles(void)
{
    FILE *fd = fopen(learn_tmpfile, "r");
    if(fd < 0) {
        perror("failed to open file %m\n");
        return;
    }

    int indx = 0, va = 0;
    int pdbr = -1;
    ssize_t readlines;
    size_t len;
    char *line = NULL;
    char pages[128][128];
    while((readlines = getline(&line, &len, fd)) != -1) {
        if(strncmp(line, "page",  4) != 0) {
            /* check for PDBR */
            if(strncmp(line, "PDBR", 4) == 0) {
                pdbr = 1;
            }
            else if(strncmp(line , "Virtual", 7) == 0) {
                va = 1;
            }
            else
                continue;
        }

        if (!va) {
            char *ptr = strstr(line, ":");
            if(ptr) {
                ++ptr;
                if(pdbr < 0) {
                    strcpy(pages[indx++], ptr);
                }
                else {
                    for(pdbr = 0, ptr++; *ptr != ' '; ptr++) pdbr = pdbr*10+(*ptr)-'0';
                }
            }
            else {
                perror("strstr failed: %m\n");
                goto done;
            }
        }
        else {
            char *ptr = strstr(line, "Address");
            ptr += strlen("Address ");
            for(va = 0; *ptr != ':'; ptr++) {
                if(*ptr >= 'a' && *ptr <= 'f')
                    va = va<<4 | (*ptr-'a'+10);
                else
                    va = va<<4 | *ptr-'0';
            }
            process_va(pages, pdbr, va);
        }
    }

done:
    fclose(fd);
}

int main(int argc, char *argv[])
{
    int s = 1;

    if(argc < 2) {
        printf("Usage:  [cmd]  args\n");
        return 0;
    }

    char cmd[128] = {0};
    for(int i = 1; i < argc; i++) {
        strcat(cmd, argv[i]);
        strcat(cmd, " ");
    }

    int ret = runcmd(cmd);
    if(ret) {
        return 0;
    }


    decipher_output_smallfiles();

#ifdef DEBUG_OUTPUT
    FILE *fd = fopen(learn_tmpfile, "r");
    if(fd < 0) {
        perror("failed to open file %m\n");
        return 0;
    }
    ssize_t readlines;
    size_t len;
    char *line = NULL;
    while((readlines = getline(&line, &len, fd)) != -1) {
        printf("%s", line);
    }
    if(line) free(line);
    fclose(fd);
#endif
    return 0;
}
