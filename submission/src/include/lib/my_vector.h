#ifndef CHAR_PTR_VECTOR_H
#define CHAR_PTR_VECTOR_H

#include <stdio.h>
#include <stdlib.h>

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
} Header;

// Function prototypes
void header_init(Header *vec);
void header_push(Header *vec, char *elem);
char **header_get(Header *vec, size_t index);
void header_free(Header *vec);
#endif
