#ifndef CHAR_PTR_VECTOR_H
#define CHAR_PTR_VECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    char ***data;    // Array of `char**` elements
    size_t size;     // Number of elements
    size_t capacity; // Allocated capacity
    int natts;
} CharPtrVector;

// Function prototypes
void char_ptr_vector_init(CharPtrVector *vec);
void char_ptr_vector_push(CharPtrVector *vec, char **array);
char **char_ptr_vector_get(CharPtrVector *vec, size_t index);
void char_ptr_vector_free(CharPtrVector *vec);

typedef struct {
    char **data;    // Array of `char*` elements
    size_t size;     // Number of elements
    size_t capacity; // Allocated capacity
    int natts;
} StringVector;

// Function prototypes
void string_vector_init(StringVector *vec);
void string_vector_push(StringVector *vec, char *elem);
char *string_vector_get(StringVector *vec, size_t index);
void string_vector_free(StringVector *vec);
int string_vector_find(StringVector *vec, char *attribute);

typedef struct {
    void **data;    // Array of `char*` elements
    size_t size;     // Number of elements
    size_t capacity; // Allocated capacity
} PtrVector;

// Function prototypes
void ptr_vector_init(PtrVector *vec);
void ptr_vector_push(PtrVector *vec, void *elem);
void *ptr_vector_get(PtrVector *vec, size_t index);
void ptr_vector_free(PtrVector *vec);
void ptr_vector_replace(PtrVector *vec, size_t index, void *elem);

typedef struct {
    bool *data;    // Array of `char*` elements
    size_t size;     // Number of elements
    size_t capacity; // Allocated capacity
} BoolVector;

// Function prototypes
void bool_vector_init(BoolVector *vec);
void bool_vector_push(BoolVector *vec, bool elem);
bool bool_vector_get(BoolVector *vec, size_t index);
void bool_vector_free(BoolVector *vec);
#endif
