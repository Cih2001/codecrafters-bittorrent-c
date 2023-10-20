#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "list.h"

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

enum bencode_type {
    invalid = 0,
    string = 1,
    integer = 2,
    list = 3
};

enum bencode_type get_bencode_type(const char* bencoded_value) {
    char c = bencoded_value[0];
    if (is_digit(c)) return string;
    if (c == 'i') return integer;
    if (c == 'l') return list;
    
    return invalid;
}

typedef struct{
    enum bencode_type type;
    char* decoded_str;
    int size;
} bencode;

void free_bencode(void* ptr) {
    bencode* v = (bencode*)ptr;
    if (v->decoded_str != NULL) free(v->decoded_str);
    free(v);
}

node* decode_string_bencode(const char* bencoded_value) {
    bencode* result = malloc(sizeof(bencode));
    int len = atoi(bencoded_value);
    const char* colon_index = strchr(bencoded_value, ':');
    if (colon_index == NULL) {
        result->size = 0;
        result->type = invalid;
        result->decoded_str = NULL;
        return node_create(result, free_bencode);
    }
    const char* start = colon_index + 1;
    char* decoded_str = (char*)malloc(len + 1);
    strncpy(decoded_str, start, len);
    decoded_str[len] = '\0';
    result->size = (colon_index-bencoded_value+len+1);
    result->type = string;
    result->decoded_str = decoded_str;
    return node_create(result, free_bencode);
}

node* decode_integer_bencode(const char* bencoded_value) {
    bencode* result = malloc(sizeof(bencode));
    const char* e_index = strchr(bencoded_value, 'e');
    int len = e_index - bencoded_value - 1;
    if (len <= 0 || bencoded_value[len+1] != 'e') {
        result->size = 0;
        result->type = invalid;
        result->decoded_str = NULL;
        return node_create(result, free_bencode);
    }
    char* decoded_str = (char*)malloc(len+1);
    strncpy(decoded_str, bencoded_value+1, len);
    decoded_str[len] = '\0';
    result->size = len+2;
    result->type = integer;
    result->decoded_str = decoded_str;
    return node_create(result, free_bencode);
}

node* decode_list_bencode(const char* bencoded_value) {
    bencode* result = malloc(sizeof(bencode));
    int len = strlen(bencoded_value)-2;
    if (len <= 0 || bencoded_value[len+1] != 'e') {
        result->type = invalid;
        result->decoded_str = NULL;
        return node_create(result, free_bencode);
    }

    result->type = list;
    result->decoded_str = NULL;
    node* head = node_create(result, free_bencode);
    node* n;

    const char* cur = bencoded_value+1;
    while (cur - bencoded_value < strlen(bencoded_value)-1) {
        switch (get_bencode_type(cur)) {
            case string:
                n = decode_string_bencode(cur);
                head = node_append(head, n);
                cur += ((bencode*)(n->data))->size;
                break;
            case integer:
                n = decode_integer_bencode(cur);
                head = node_append(head, n);
                cur += ((bencode*)(n->data))->size;
                break;
            default:
                result->size = 0;
                result->type = invalid;
                result->decoded_str = NULL;
                return node_create(result, free_bencode);
        }
    }

    return head;
}

node* decode_bencode(const char* bencoded_value) {
    enum bencode_type t = get_bencode_type(bencoded_value);
    if (t == string) return decode_string_bencode(bencoded_value);
    if (t == integer) return decode_integer_bencode(bencoded_value);
    if (t == list) return decode_list_bencode(bencoded_value);

    fprintf(stderr, "Not supported\n");
    exit(1);
}

void print_bencode(bencode* b) {
    switch (b->type) {
        case string:
            printf("\"%s\"", b->decoded_str);
            break;
        case integer:
            printf("%s", b->decoded_str);
            break;
        default:
            return;
    }
}

int start(int argc, char* argv[]) {
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
        node* n = decode_bencode(encoded_str);
        bencode* b = n->data;
        if (b->type != list) {
            print_bencode(b);
        } else {
            printf("[");
            for (node* cur=n->next; cur != NULL; cur = cur->next) {
                print_bencode(cur->data);
                if (cur->next != NULL) printf(",");
            } 
            printf("]");
        }

        printf("\n");
        node_remove(n);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}


int main(int argc, char* argv[]) {
    start(argc, argv);
    // bencode_node* b1 = (bencode_node*) malloc(sizeof(bencode_node));
    // b1->decoded_str = strdup("hello");

    // bencode_node* b2 = (bencode_node*) malloc(sizeof(bencode_node));
    // b2->decoded_str = strdup("world");

    // node* head = node_add(NULL, (void*)b1, free_decoded_value);
    // head = node_add(head, (void *)b2, free_decoded_value);
    // node_remove(head);
}
