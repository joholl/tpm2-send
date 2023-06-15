// SPDX-FileCopyrightText: 2023 Infineon Technologies AG
//
// SPDX-License-Identifier: BSD-2-Clause

#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <tbs.h>

#define ARG_IN "-i"
#define ARG_OUT "-o"
#define PATH_DEFAULT "-"


int parse_args(int argc, char* argv[], char **file_in_path, char **file_out_path) {
    char *arg;

    for (int i = 1; i < argc; i += 2) {
        arg = argv[i];
        if (strcmp(arg, ARG_IN) == 0) {
            if (i + 1 >= argc) {
                goto end_invalid_number_of_args;
            }
            *file_in_path = argv[i + 1];
        } else if (strcmp(arg, ARG_OUT) == 0) {
            if (i + 1 >= argc) {
                goto end_invalid_number_of_args;
            }
            *file_out_path = argv[i + 1];
        } else {
            fprintf(stderr, "Invalid argument: %s\n", arg);
            goto end_print_usage;
        }
    }

    return 0;

end_invalid_number_of_args:
    fprintf(stderr, "Invalid number of arguments.\n");
end_print_usage:
    fprintf(stderr, "\nUsage:\n  %s [" ARG_IN " <input file>] [" ARG_OUT " <output file>]\n", argv[0]);
    return -1;
}

int input_init(BYTE **tx_buf, UINT32 *tx_buf_len, char *file_in_path) {
    int result;
    FILE *file_in;

    if (strcmp(file_in_path, PATH_DEFAULT) == 0) {
        file_in = stdin;
    } else {
        result = fopen_s(&file_in, file_in_path, "rb");
        if (result != 0) {
            perror("Error opening input file");
            return result;
        }
    }

    result = fseek(file_in, 0, SEEK_END);
    if (result != 0) {
        fprintf(stderr, "Failed when calling fseek() on input file\n");
        goto cleanup_file;
    }
    *tx_buf_len = ftell(file_in);
    if (*tx_buf_len == -1L) {
        perror("Failed when calling ftell() on input file");
        result = -1;
        goto cleanup_file;
    }
    rewind(file_in);

    *tx_buf = (BYTE *) malloc(*tx_buf_len * sizeof(BYTE));
    if (*tx_buf == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        result = -1;
        goto cleanup_file;
    }
    fread(*tx_buf, *tx_buf_len, 1, file_in);
    if (ferror(file_in)) {
        perror("Error reading from input file");
        free(*tx_buf);
        goto cleanup_file;
    }

cleanup_file:
    if (file_in != stdin) {
        fclose(file_in);
    }
    return result;
}

void input_cleanup(BYTE *tx_buf) {
    free(tx_buf);
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

    char *file_in_path = PATH_DEFAULT;
    char *file_out_path = PATH_DEFAULT;
    FILE *file_out = stdout;

    BYTE *tx_buf;
    UINT32 tx_buf_len;
    BYTE rx_buf[4096] = {0};
    UINT32 rx_buf_len = sizeof(rx_buf);

    result = parse_args(argc, argv, &file_in_path, &file_out_path);
    if (result != 0) {
        goto end;
    }

    result = input_init(&tx_buf, &tx_buf_len, file_in_path);
    if (result != 0) {
        goto end;
    }

    result = output_init(&file_out, file_out_path);
    if (result != 0) {
        goto cleanup_input;
    }

    result = transceive(tx_buf, tx_buf_len, rx_buf, &rx_buf_len);
    if (result != 0) {
        goto cleanup_output;
    }

    fwrite(rx_buf, sizeof(BYTE), rx_buf_len, file_out);

cleanup_input:
    input_cleanup(tx_buf);
cleanup_output:
    output_cleanup(file_out);
end:
    return result;
}
