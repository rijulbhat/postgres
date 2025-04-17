#include "lib/my_vector.h"

#define INITIAL_CAPACITY 4

// Initialize the vector
void char_ptr_vector_init(CharPtrVector *vec) {
    vec->data = (char***) malloc(INITIAL_CAPACITY * sizeof(char **));
    vec->size = 0;
    vec->capacity = INITIAL_CAPACITY;
}

// Push an already allocated `char**` into the vector
void char_ptr_vector_push(CharPtrVector *vec, char **array) {
    // Resize if needed
    if (vec->size == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity * sizeof(char **));
    }
    // Store the pointer
    vec->data[vec->size++] = array;
}

// Get a `char**` from the vector
char **char_ptr_vector_get(CharPtrVector *vec, size_t index) {
    if (index >= vec->size) return NULL;
    return vec->data[index];
}

// Free the vector (does not free stored `char**` elements)
void char_ptr_vector_free(CharPtrVector *vec) {
    free(vec->data);  // Free only the vector storage
}

// Initialize the vector
void header_init(Header *vec) {
    vec->data = (char**) malloc(INITIAL_CAPACITY * sizeof(char *));
    vec->size = 0;
    vec->capacity = INITIAL_CAPACITY;
}

// Push an already allocated `char*` into the vector
void header_push(Header *vec, char *elem) {
    // Resize if needed
    if (vec->size == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity * sizeof(char *));
    }
    // Store the pointer
    vec->data[vec->size++] = elem;
}

// Get a `char*` from the vector
char *header_get(Header *vec, size_t index) {
    if (index >= vec->size) return NULL;
    return vec->data[index];
}

// Free the vector (does not free stored `char*` elements)
void header_free(Header *vec) {
    free(vec->data);  // Free only the vector storage
}