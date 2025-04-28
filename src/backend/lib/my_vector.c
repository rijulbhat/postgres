#include "lib/my_vector.h"
#include <string.h>

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
void string_vector_init(StringVector *vec) {
    vec->data = (char**) malloc(INITIAL_CAPACITY * sizeof(char *));
    vec->size = 0;
    vec->capacity = INITIAL_CAPACITY;
}

// Push an already allocated `char*` into the vector
void string_vector_push(StringVector *vec, char *elem) {
    // Resize if needed
    if (vec->size == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity * sizeof(char *));
    }
    // Store the pointer
    vec->data[vec->size++] = elem;
}

// Get a `char*` from the vector
char *string_vector_get(StringVector *vec, size_t index) {
    if (index >= vec->size) return NULL;
    return vec->data[index];
}

// Find the index of a `char*` in the vector, or -1 if not found
int string_vector_find(StringVector *vec, const char *attribute) {
    for (size_t i = 0; i < vec->size; i++) {
        if (strcmp(vec->data[i], attribute) == 0) {
            return i;  // Found, return index
        }
    }
    return -1;  // Not found
}

// Free the vector (does not free stored `char*` elements)
void string_vector_free(StringVector *vec) {
    free(vec->data);  // Free only the vector storage
}

// Initialize the vector
void ptr_vector_init(PtrVector *vec) {
    vec->data = (void**) malloc(INITIAL_CAPACITY * sizeof(void *));
    vec->size = 0;
    vec->capacity = INITIAL_CAPACITY;
}

// Push an already allocated `char*` into the vector
void ptr_vector_push(PtrVector *vec, void *elem) {
    // Resize if needed
    if (vec->size == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity * sizeof(void *));
    }
    // Store the pointer
    vec->data[vec->size++] = elem;
}

// Replace an element at a specific index in the vector
void ptr_vector_replace(PtrVector *vec, size_t index, void *elem) {
    if (index < vec->size) {
        vec->data[index] = elem;  // Replace the element
    }
}

// Get a `char*` from the vector
void *ptr_vector_get(PtrVector *vec, size_t index) {
    if (index >= vec->size) return NULL;
    return vec->data[index];
}

// Free the vector (does not free stored `char*` elements)
void ptr_vector_free(PtrVector *vec) {
    free(vec->data);  // Free only the vector storage
}

// Initialize the vector
void bool_vector_init(BoolVector *vec) {
    vec->data = (bool*) malloc(INITIAL_CAPACITY * sizeof(bool));
    vec->size = 0;
    vec->capacity = INITIAL_CAPACITY;
}

// Push an already allocated `char*` into the vector
void bool_vector_push(BoolVector *vec, bool elem) {
    // Resize if needed
    if (vec->size == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity * sizeof(bool));
    }
    // Store the pointer
    vec->data[vec->size++] = elem;
}

// Get a `char*` from the vector
bool bool_vector_get(BoolVector *vec, size_t index) {
    if (index >= vec->size) return NULL;
    return vec->data[index];
}

// Free the vector (does not free stored `char*` elements)
void bool_vector_free(BoolVector *vec) {
    free(vec->data);  // Free only the vector storage
}

// Initialize the vector
void int_vector_init(IntVector *vec) {
    vec->data = (int*) malloc(INITIAL_CAPACITY * sizeof(int));
    vec->size = 0;
    vec->capacity = INITIAL_CAPACITY;
}

// Push an integer into the vector
void int_vector_push(IntVector *vec, int elem) {
    // Resize if needed
    if (vec->size == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity * sizeof(int));
    }
    // Store the element
    vec->data[vec->size++] = elem;
}

// Get an integer from the vector
int int_vector_get(IntVector *vec, size_t index) {
    if (index >= vec->size) return 0;  // Return 0 if index is out of bounds
    return vec->data[index];
}

// Replace an element at a specific index in the vector
void int_vector_replace(IntVector *vec, size_t index, int elem) {
    if (index < vec->size) {
        vec->data[index] = elem;  // Replace the element
    }
}

// Free the vector
void int_vector_free(IntVector *vec) {
    free(vec->data);  // Free only the vector storage
}
