#include "common.h"

struct _FlipBuffer {
    uint8_t* _buffer[2];// = { nullptr, nullptr };
    size_t _length[2];// = { 0, 0 };
    uint8_t _flip;// = false;
};

typedef struct _FlipBuffer FlipBuffer;

void FlipBuffer_deleteBuffer(FlipBuffer * buf){
    for (size_t i = 0; i < 2; i++)
    {
    buf->_length[i] = 0;
    if (buf->_buffer[i])
    {
        heap_free(buf->_buffer[i]);
        buf->_buffer[i] = NULL;
    }
    }
}


    uint8_t* FlipBuffer_getBuffer(FlipBuffer * buf, size_t length)
    {
      length = (length + 3) & ~3;
      buf->_flip = !_flip;

      if (buf->_length[_flip] < length || buf->_length[buf->_flip] > length + 64)
      {
        if (buf->_buffer[_flip]) { heap_free(buf->_buffer[buf->_flip]); }
        buf->_buffer[_flip] = (uint8_t*)heap_alloc_dma(length);
        buf->_length[_flip] = buf->_buffer[_flip] ? length : 0;
      }
      return buf->_buffer[_flip];
    }
