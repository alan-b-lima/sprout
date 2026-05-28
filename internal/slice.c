#include "slice.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct slice {
    void  *ptr;
    size_t len;
    size_t cap;
    bool   view;
};

#define alignof(type) _Alignof(type)

static_assert(sizeof(slice_h) == sizeof(struct slice));
static_assert(alignof(slice_h) == alignof(struct slice));

#define htos__(s) (*(struct slice*)(&s))
#define stoh__(h) (*(slice_h*)(&h))

static size_t slice_grow__(size_t cap);

slice_h slice_make(size_t size, size_t len, size_t cap) {
    if (cap == 0) {
        if (len == 0) {
            struct slice h = (struct slice){
                .ptr  = NULL,
                .len  = 0,
                .cap  = 0,
                .view = false,
            };

            return stoh__(h);
        }

        cap = len;
    }

    if (cap < len) {
        fprintf(stderr, "cap should be zero or greater than len");
        abort();
    }

    void *ptr = calloc(cap, size);
    if (ptr == NULL) {
        fprintf(stderr, "bad alloc");
        abort();
    }

    struct slice h = (struct slice){
        .ptr  = ptr,
        .len  = len,
        .cap  = cap,
        .view = false,
    };

    return stoh__(h);
}

slice_h slice_append(slice_h s, size_t size, void *elem) {
    struct slice h = htos__(s);
    size_t newlen = h.len + 1;

    if (newlen > h.cap) {
        slice_h ns = slice_make(size, 0, slice_grow__(h.cap));
        struct slice nh = htos__(ns);

        memcpy(nh.ptr, h.ptr, size * h.len);
        slice_release(s);
        h = nh;
    }

    memcpy(h.ptr + size * (newlen - 1), elem, size);
    h.len = newlen;

    return stoh__(h);
}

size_t slice_len(const slice_h s) {
    return htos__(s).len;
}

size_t slice_cap(const slice_h s) {
    return htos__(s).cap;
}

void *slice_at(slice_h s, size_t size, size_t at) {
    struct slice h = htos__(s);

#ifdef SPROUT_SLICE_BOUNDS_CHECK
    if (at >= h.len) {
        fprintf(stderr, "index [%zu] out of bounds for %zu\n", at, h.len);
        abort();
    }
#endif

    return h.ptr + size * at;
}

size_t slice_copy(slice_h dst, slice_h src, size_t size) {
    struct slice dsth = htos__(dst);
    struct slice srch = htos__(src);

    size_t min = dsth.len;
    if (min > srch.len) {
        min = srch.len;
    }

    memcpy(dsth.ptr, srch.ptr, size * min);
    return min;
}

slice_h slice_reslice(slice_h s, size_t size, size_t start, size_t end) {
    struct slice h = htos__(s);

#ifdef SPROUT_SLICE_BOUNDS_CHECK
    if (start > end) {
        fprintf(stderr, "invalid reslicing [%zu:%zu]\n", start, end);
        abort();
    }

    if (start > h.len || end > h.len) {
        fprintf(stderr, "reslicing [%zu:%zu] out of bounds for %zu\n", start, end, h.len);
        abort();
    }
#endif

    struct slice nh = (struct slice){
        .ptr = h.ptr + size * start,
        .len = end - start,
        .cap = h.cap - start,
        .view = true,
    };

    return stoh__(nh);
}

void slice_release(slice_h s) {
    struct slice h = htos__(s);

    if (!h.view) {
        free(h.ptr);
    }
}

#define SPROUT_MIN_CAP   16
#define SPROUT_THRES_CAP 512

size_t slice_grow__(size_t cap) {
    if (cap > SPROUT_THRES_CAP) {
        return cap + (cap + 3 * SPROUT_THRES_CAP) / 4;
    }

    int newcap = cap * 2;
    if (newcap < SPROUT_MIN_CAP) {
        return SPROUT_MIN_CAP;
    }

    return newcap;
}