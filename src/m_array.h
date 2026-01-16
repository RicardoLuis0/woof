//
// Copyright(C) 2024 Roman Fomin
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Growable arrays of homogeneous values of any type, similar to a std::vector
// in C++. The array must be initialised to NULL. Inspired by
// https://github.com/nothings/stb/blob/master/stb_ds.h
//
// array_push(), array_grow(), array_resize(), array_copy() and array_free() may
// change the buffer pointer, and any previously-taken pointers should be
// considered invalidated.

#ifndef M_ARRAY_H
#define M_ARRAY_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "i_system.h"

#ifndef M_ARRAY_INIT_CAPACITY
#  define M_ARRAY_INIT_CAPACITY 8
#endif

#ifndef M_ARRAY_MALLOC
#  define M_ARRAY_MALLOC(size) malloc(size)
#endif

#ifndef M_ARRAY_REALLOC
#  define M_ARRAY_REALLOC(ptr, size) I_Realloc(ptr, size)
#endif

#ifndef M_ARRAY_FREE
#  define M_ARRAY_FREE(ptr) free(ptr)
#endif

typedef struct
{
    int capacity;
    int size;
    char buffer[];
} m_array_buffer_t;

inline static m_array_buffer_t *array_ptr(const void *v)
{
    return (m_array_buffer_t *)((char *)v - offsetof(m_array_buffer_t, buffer));
}

inline static int array_size(const void *v)
{
    return v ? array_ptr(v)->size : 0;
}

inline static int array_capacity(const void *v)
{
    return v ? array_ptr(v)->capacity : 0;
}

inline static void array_clear(void *v)
{
    if (v)
    {
        array_ptr(v)->size = 0;
    }
}

inline static void *M_ArrayGrow(void *v, size_t esize, int n)
{
    m_array_buffer_t *p;

    if (v)
    {
        p = array_ptr(v);
        p = M_ARRAY_REALLOC(p, sizeof(m_array_buffer_t)
                                   + (p->capacity + n) * esize);
		
		if(!p) return NULL;
		
        p->capacity += n;
    }
    else
    {
        p = M_ARRAY_MALLOC(sizeof(m_array_buffer_t) + n * esize);
		
		if(!p) return NULL;
		
        p->capacity = n;
        p->size = 0;
    }

    return p->buffer;
}

#define array_grow(v, n) ((v) = M_ArrayGrow((void*)(v), sizeof(*(v)), n))

// Appends an element to the end of the array.
#define array_push(v, e)                                       \
    do                                                         \
    {                                                          \
        if (!(v))                                              \
        {                                                      \
            array_grow(v, M_ARRAY_INIT_CAPACITY);              \
        }                                                      \
        else if (array_ptr(v)->size == array_ptr(v)->capacity) \
        {                                                      \
            array_grow(v, array_ptr(v)->capacity);             \
        }                                                      \
        (v)[array_ptr((void*)(v))->size++] = (e);              \
    } while (0)

// follows same rules as push,
// increases size to N, if not possible, double array capacity
// (or to N, whichever is largest, but at minimum M_ARRAY_INIT_CAPACITY)
// (also zeroes memory for the grown range)
#define array_grow_size(v, n) ((v) = M_ArrayGrowSize((void*)(v), sizeof(*(v)), n))

// reduces capacity to size
#define array_shrink_to_fit(v, e) ((v) = M_ArrayResize((void*)(v), sizeof(*(v)), array_size(v)))

// Removes and returns the last element of the array.
// The array must not be empty.
#define array_pop(v) ((v)[--array_ptr((void*)(v))->size])

// Deletes 'n' elements from the array starting at index 'i'.
#define array_delete_n(v, i, n)                                                        \
    do                                                                                 \
    {                                                                                  \
        if (v)                                                                         \
        {                                                                              \
            int to_delete = (n);                                                       \
            int index = (i);                                                           \
            memmove((void*)&(v)[index], (void*)&(v)[index + to_delete],                \
                    sizeof(*(v)) * (array_ptr((void*)(v))->size - to_delete - index)); \
            array_ptr((void*)(v))->size -= to_delete;                                  \
        }                                                                              \
    } while (0)

#define array_delete(v, i) array_delete_n(v, i, 1)

#define array_free(v)                                    \
    do                                                   \
    {                                                    \
        if (v)                                           \
        {                                                \
            M_ARRAY_FREE((void*)array_ptr((void*)(v)));  \
            (v) = NULL;                                  \
        }                                                \
    } while (0)

#define array_end(v) ((v) ? (v) + array_ptr((void*)(v))->size : (v))

#define array_foreach(ptr, v) for (ptr = (v); ptr != array_end(v); ++ptr)

#define array_foreach_type(ptr, v, type)                                   \
    for (type *ptr = (v), *m_array_end = array_end(v); ptr != m_array_end; \
         ++ptr)

// If n > current size, new elements are zero-initialized.
// If n < current size, array is truncated.
#define array_resize(v, n)                                    \
    do                                                        \
    {                                                         \
        int new_size = (n);                                   \
        int old_size = array_size(v);                         \
        if (new_size > array_capacity(v))                     \
        {                                                     \
            int new_capacity = M_ARRAY_INIT_CAPACITY;         \
            while (new_capacity < new_size)                   \
            {                                                 \
                new_capacity *= 2;                            \
            }                                                 \
            array_grow(v, new_capacity - array_capacity(v));  \
        }                                                     \
        if (v)                                                \
        {                                                     \
            if (new_size > old_size)                          \
            {                                                 \
                memset(&(v)[old_size], 0,                     \
                       sizeof(*(v)) * (new_size - old_size)); \
            }                                                 \
            array_ptr((void*)(v))->size = new_size;           \
        }                                                     \
    } while (0)

#define array_copy(dst, src)                             \
    do                                                   \
    {                                                    \
        int size = array_size(src);                      \
        if (!size)                                       \
        {                                                \
            array_clear(dst);                            \
        }                                                \
        else                                             \
        {                                                \
            array_resize(dst, size);                     \
            if (dst)                                     \
            {                                            \
                memcpy(dst, src, sizeof(*(src)) * size); \
            }                                            \
        }                                                \
    } while (0)

inline static void *M_ArrayGrowSize(void *v, size_t esize, int n)
{
    m_array_buffer_t *p;

    int growsize = MAX(n, M_ARRAY_INIT_CAPACITY);
    int oldsize = 0;

    if (v)
    {
        // if the array exists, we may not need to grow
        p = array_ptr(v);
        oldsize = p->size;
        if(n <= p->size)
        {
            p->size = n;
            return p->buffer;
        }
        else
        {
            growsize = MAX(p->capacity + p->capacity, growsize); // double capacity, or grow to n, whichever is greatest
        }
    }

    v = M_ArrayGrow(v, esize, growsize);
    p = array_ptr(v);
    p->size = n;

    assert(oldsize >= 0);
    assert(n >= oldsize);
    
    int delta = n - oldsize;

    if(delta)
    {
        //zero out newly created elements
        memset(p->buffer + (oldsize * esize), 0, delta * esize);
    }
    
    return p->buffer;
}

inline static void *M_ArrayResize(void *v, size_t esize, int n)
{
    m_array_buffer_t *p;

    if (v)
    {
        if(p->capacity != n)
        { // don't realloc if we're already sized right
            p = array_ptr(v);
            p = M_ARRAY_REALLOC(p, sizeof(m_array_buffer_t) + (n * esize));
            p->capacity = n;
        }
    }
    else
    {
        p = M_ARRAY_MALLOC(sizeof(m_array_buffer_t) + (n * esize));
        p->capacity = n;
        p->size = 0;
    }

    return p->buffer;
}

#endif // M_ARRAY_H
