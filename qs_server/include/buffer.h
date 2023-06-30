#ifndef QS_BUFFER
#define QS_BUFFER

typedef struct {
    void* data;
    unsigned int len,
                 size,
                 bytes;
} Buffer;

#define BUFFER_ALLOC(buf, bsize) \
    do { \
        buf.data  = alloc(bsize); \
        buf.len   = 0; \
        buf.size  = 0; \
        buf.bytes = bsize; \
    } while (false)

#define B_ZERO(buf) memset(buf.data, 0, buf.size)
#define B_DATA(buf) buf.data
#define B_LEN(buf) buf.len
#define B_RESET(buf) \
    do { \
        B_ZERO(buf); \
        buf.len  = 0; \
        buf.size = 0; \
    } while(false)

#endif /* QS_BUFFER */