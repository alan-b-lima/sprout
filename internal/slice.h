#ifndef SPROUT_SLICE_H
#define SPROUT_SLICE_H

#include <stddef.h>

#define SPROUT_SLICE_SIZE__ sizeof(struct { void *_0; size_t _1; size_t _2; _Bool _3;})

typedef struct slice_h { size_t __data[SPROUT_SLICE_SIZE__ / sizeof(size_t)]; } slice_h;
#define slice(type) slice_h

#define SPROUT_SLICE_BOUNDS_CHECK

#define make(type, len, cap)             slice_make(sizeof(type), (len), (cap))
#define append(type, slice, elem)        slice_append(slice, sizeof(type), &(elem))
#define len(type, slice)                 slice_len(slice)
#define cap(type, slice)                 slice_cap(slice)
#define at(type, slice, at)              (*(typeof(type)*)slice_at(slice, sizeof(type), (at)))
#define copy(type, dst, src)             slice_copy(dst, src, sizeof(type))
#define reslice(type, slice, start, end) slice_reslice(slice, sizeof(type), (start), (end))
#define release(type, slice)             slice_release(slice);

slice_h slice_make(size_t size, size_t len, size_t cap);
slice_h slice_append(slice_h slice, size_t size, void *elem);
size_t  slice_len(const slice_h slice);
size_t  slice_cap(const slice_h slice);
void   *slice_at(slice_h slice, size_t size, size_t at);
size_t  slice_copy(slice_h dst, slice_h src, size_t size);
slice_h slice_reslice(slice_h slice, size_t size, size_t start, size_t end);
void    slice_release(slice_h slice);

#endif