// SPDX-FileCopyrightText: 2023 Infineon Technologies AG
//
// SPDX-License-Identifier: BSD-2-Clause

#include <stdio.h>
void main() {
    int ch;
    setbuf(stdout, NULL);
    while ((ch = getchar()) != EOF)
        printf("%02x", ch);
}
