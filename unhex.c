// SPDX-FileCopyrightText: 2023 Infineon Technologies AG
//
// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdio.h>

void main(){
    int c;
    setbuf(stdout, NULL);
    while(scanf("%*1[ ]") != EOF)
        for(int i = 0; i<8; i++)
            if (scanf("%02x", &c) != EOF)
                printf("%c", (unsigned char) c);
}
