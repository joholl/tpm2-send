// SPDX-FileCopyrightText: 2023 Infineon Technologies AG
//
// SPDX-License-Identifier: BSD-2-Clause

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <windows.h>
#include <tbs.h>


#define TPM2_MAX_COMMAND_SIZE 4096
#define TPM2_HEADER_SIZE 10
#define TPM2_RESPONSE_SIZE_OFFSET 2

#define ARG_IN "-i"
#define ARG_IN_LONG "--in"
#define ARG_OUT "-o"
#define ARG_OUT_LONG "--out"
#define ARG_HEX "-x"
#define ARG_HEX_LONG "--hex"
#define PATH_DEFAULT "-"

#define BUF_TO_UINT32(buffer) \
    ( \
        ((uint32_t)((buffer)[0]) << 24) | \
        ((uint32_t)((buffer)[1]) << 16) | \
        ((uint32_t)((buffer)[2]) << 8) | \
        ((uint32_t)(buffer)[3]) \
    )



int parse_args(int argc, char* argv[], char **file_in_path, char **file_out_path, int *hex) {
    char *arg;

    /* set defaults */
    *file_in_path = PATH_DEFAULT;
    *file_out_path = PATH_DEFAULT;
    *hex = 0;

    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        if (strcmp(arg, ARG_IN) == 0 || strcmp(arg, ARG_IN_LONG) == 0) {
            if (i + 1 >= argc) {
                goto end_invalid_number_of_args;
            }
            *file_in_path = argv[i + 1];
            i++;
        } else if (strcmp(arg, ARG_OUT) == 0 || strcmp(arg, ARG_OUT_LONG) == 0) {
            if (i + 1 >= argc) {
                goto end_invalid_number_of_args;
            }
            *file_out_path = argv[i + 1];
            i++;
        } else if (strcmp(arg, ARG_HEX) == 0 || strcmp(arg, ARG_HEX_LONG) == 0) {
            *hex = 1;
        } else {
            fprintf(stderr, "Invalid argument: %s\n", arg);
            goto end_print_usage;
        }
    }

    return 0;

end_invalid_number_of_args:
    fprintf(stderr, "Invalid number of arguments.\n");
end_print_usage:
    fprintf(stderr, "\nUsage:\n  %s [-x] [" ARG_IN " <input file>] [" ARG_OUT " <output file>]\n", argv[0]);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  " ARG_IN "/" ARG_IN_LONG  " <input file>     File commands are read from (default stdin)\n");
    fprintf(stderr, "  " ARG_OUT "/" ARG_OUT_LONG " <output file>   File responses are written to (default: stderr)\n");
    fprintf(stderr, "  " ARG_HEX "/" ARG_HEX_LONG "                 Print as hex stream (instead of binary)\n");
    return -1;
}

// TODO remove
void dump(char *s, BYTE *buf, size_t len) {
    fprintf(stderr, "%s[%ld]: ", s, len);
    for (size_t i = 0; i < len; i++) {
        fprintf(stderr, "%02x", buf[i]);
        if (i < len - 1) {
            fprintf(stderr, " ");
        }
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

int input_init(FILE **file_in, char *file_in_path) {
    int result;

    if (strcmp(file_in_path, PATH_DEFAULT) == 0) {
        *file_in = stdin;
    } else {
        result = fopen_s(file_in, file_in_path, "rb");
        if (result != 0) {
            perror("Error opening input file");
            return result;
        }
    }

    return 0;
}

void input_cleanup(FILE *file_in) {
    if (file_in != stdout) {
        fclose(file_in);
    }
}

int nibble_char_to_uint(BYTE c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    fprintf(stderr, "Invalid hexadecimal digit: %c\n", c);
    return -1;
}

/*
 * Turns hex string to binary. buf_hexstr and buf_bin might be the same buffer.
 */
int hex_to_bin(BYTE* buf_hexstr, int *buf_hexstr_len, BYTE* buf_bin) {
    int len = strlen((char*) buf_hexstr);

    if (*buf_hexstr_len % 2 != 0) {
        fprintf(stderr, "Hex-string does not have even length\n");
        return -1;
    }

    for (int i = 0; i < len; i += 2) {
        int upper_nibble = nibble_char_to_uint(buf_hexstr[i]);
        int lower_nibble = nibble_char_to_uint(buf_hexstr[i + 1]);
        if (upper_nibble < 0 || lower_nibble < 0) {
            return -1;
        }

        buf_bin[i / 2] = (BYTE) (upper_nibble << 4 | lower_nibble);
    }

    *buf_hexstr_len /= 2;
    return 0;
}

int input_read(BYTE *tx_buf, UINT32 *tx_buf_len, FILE *file_in, int hex) {
    int result;
    int offset;
    int cmd_rsp_size;
    int read_size = TPM2_HEADER_SIZE;

    if (hex) {
        read_size *= 2;
    }

    offset = fread(tx_buf, 1, read_size, file_in);
    if (feof(file_in)) {
        /* regular exit point when input pipe is closed */
        result = -1;
        goto cleanup_file;
    }
    if (ferror(file_in)) {
        perror("Error reading from input file");
        result = -1;
        goto cleanup_file;
    }
    if (hex) {
        result = hex_to_bin(tx_buf, &offset, tx_buf);
        if (result) {
            return -1;
        }
    }

    cmd_rsp_size = BUF_TO_UINT32(tx_buf + TPM2_RESPONSE_SIZE_OFFSET);
    read_size = cmd_rsp_size - TPM2_HEADER_SIZE;
    if (hex) {
        read_size *= 2;
    }

    read_size = fread(tx_buf + offset, 1, read_size, file_in);
    if (feof(file_in)) {
        fprintf(stderr, "Unexpected EOF when reading input file\n");
        result = -1;
        goto cleanup_file;
    }
    if (ferror(file_in)) {
        perror("Error reading from input file");
        result = -1;
        goto cleanup_file;
    }
    if (hex) {
        result = hex_to_bin(tx_buf + offset, &read_size, tx_buf + offset);
        if (result) {
            return -1;
        }
    }
    offset += read_size;

    *tx_buf_len = offset;
    result = 0;

cleanup_file:
    if (file_in != stdin) {
        fclose(file_in);
    }
    return result;
}

int output_init(FILE **file_out, char *file_out_path) {
    int result;

    if (strcmp(file_out_path, PATH_DEFAULT) == 0) {
        *file_out = stdout;
    } else {
        result = fopen_s(file_out, file_out_path, "wb");
        if (result != 0) {
            perror("Error opening output file");
            return result;
        }
    }

    return 0;
}

void output_cleanup(FILE *file_out) {
    if (file_out != stdout) {
        fclose(file_out);
    }
}

int output_write(FILE *file_out, BYTE *buf, UINT32 buf_len, int hex) {
    if (!hex) {
        fwrite(buf, sizeof(BYTE), buf_len, file_out);
        if (ferror(file_out)) {
            perror("Error writing to output file");
            return -1;
        }
    } else {
        for (size_t i = 0; i < buf_len; i++) {
            fprintf(file_out, "%02x", buf[i]);
            if (ferror(file_out)) {
                perror("Error writing to output file");
                return -1;
            }
        }
    }

    fflush(file_out);

    return 0;
}

int transceive(BYTE *tx_buf, UINT32 tx_buf_len, BYTE *rx_buf, UINT32 *rx_buf_len) {
    TBS_RESULT result;

    TBS_HCONTEXT handle_ctx;
    TBS_CONTEXT_PARAMS2 ctx_params = {
        .version = TPM_VERSION_20,
        .includeTpm12 = 0,
        .includeTpm20 = 1,
    };
    result = Tbsi_Context_Create((PCTBS_CONTEXT_PARAMS) &ctx_params, &handle_ctx);
    if (result != TBS_SUCCESS) {
        fprintf(stderr, "Failed when attempting to create TBS context: %04x\n", result);
        return result;
    }

    result = Tbsip_Submit_Command(
        handle_ctx,
        TBS_COMMAND_LOCALITY_ZERO,
        TBS_COMMAND_PRIORITY_NORMAL,
        tx_buf,
        tx_buf_len,
        rx_buf,
        rx_buf_len);
    if (result != TBS_SUCCESS) {
        fprintf(stderr, "Failed when attempting to submit TBS context: %04x\n", result);
    }

    Tbsip_Context_Close(handle_ctx);

    return result;
}


int main(int argc, char **argv) {
    int result;

    int hex;
    char *file_in_path;
    char *file_out_path;
    FILE *file_in;
    FILE *file_out;

    /* we need double-sized buffer in case hex strings are used */
    BYTE tx_buf[TPM2_MAX_COMMAND_SIZE * 2] = {0};
    UINT32 tx_buf_len;
    BYTE rx_buf[TPM2_MAX_COMMAND_SIZE * 2] = {0};
    UINT32 rx_buf_len = sizeof(rx_buf);

    result = parse_args(argc, argv, &file_in_path, &file_out_path, &hex);
    if (result != 0) {
        goto end;
    }

    result = input_init(&file_in, file_in_path);
    if (result != 0) {
        goto end;
    }

    result = output_init(&file_out, file_out_path);
    if (result != 0) {
        goto cleanup_input;
    }

    while (1) {
        /* send first byte to output buffer to unstuck receiving process */
        result = output_write(file_out, (BYTE *) "\x80", 1, hex);
        if (result != 0) {
            goto end;
        }

        result = input_read(tx_buf, &tx_buf_len, file_in, hex);
        if (result != 0) {
            goto end;
        }

        result = transceive(tx_buf, tx_buf_len, rx_buf, &rx_buf_len);
        if (result != 0) {
            goto cleanup_output;
        }

        /* send response (except first byte which we sent already) */
        result = output_write(file_out, rx_buf + 1, rx_buf_len - 1, hex);
        if (result != 0) {
            goto end;
        }
    }

cleanup_input:
    input_cleanup(file_in);
cleanup_output:
    output_cleanup(file_out);

end:
    return result;
}
