#include <stdio.h>
#include <stdlib.h>
#include "../dlpatch.h"
int main(int argc, char **argv){
    unsigned limit = (unsigned)strtoul(argv[1],0,10), e, f; unsigned char a[4], b[4];
    dl_values(limit,&e,&f); dl_put32(a,e); dl_put32(b,f);
    printf("%02X%02X%02X%02X %02X%02X%02X%02X\n",a[0],a[1],a[2],a[3],b[0],b[1],b[2],b[3]); return 0; }
