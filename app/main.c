#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

enum decoded_value_types {
    string_type = 1,
    integer_type = 2
};

typedef struct{
    enum decoded_value_types type;
    char* decoded_str;
} decoded_value;

decoded_value decode_bencode(const char* bencoded_value) {
    if (is_digit(bencoded_value[0])) {
        // decode strings
        int len = atoi(bencoded_value);
        const char* colon_index = strchr(bencoded_value, ':');
        if (colon_index == NULL) {
            fprintf(stderr, "Invalid encoded string: %s\n", bencoded_value);
            exit(1);
        }
        const char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(len + 1);
        strncpy(decoded_str, start, len);
        decoded_str[len] = '\0';
        decoded_value result = {string_type, decoded_str};
        return result;
    }
    if (bencoded_value[0] == 'i') {
        // decode integers
        int len = strlen(bencoded_value)-2;
        if (len <= 0 || bencoded_value[len+1] != 'e') {
            fprintf(stderr, "Invalid encoded int: %s\n", bencoded_value);
            exit(1);
        }
        char* decoded_str = (char*)malloc(len+1);
        strncpy(decoded_str, bencoded_value+1, len);
        decoded_str[len] = '\0';
        decoded_value result = {integer_type, decoded_str};
        return result;
    }
    fprintf(stderr, "Not supported\n");
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
        return 1;
    }

    const char* command = argv[1];

    if (strcmp(command, "decode") == 0) {
    	// You can use print statements as follows for debugging, they'll be visible when running tests.
        // printf("Logs from your program will appear here!\n");
            
        // Uncomment this block to pass the first stage
        const char* encoded_str = argv[2];
        decoded_value v = decode_bencode(encoded_str);
        if (v.type == string_type) {
            printf("\"%s\"\n", v.decoded_str);
        } else {
            printf("%s\n", v.decoded_str);
        }
        free(v.decoded_str);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
