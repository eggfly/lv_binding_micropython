#include "common.h"

struct _FlipBuffer
{
  uint8_t *_buffer[2]; // = { nullptr, nullptr };
  size_t _length[2];   // = { 0, 0 };
  uint8_t _flip;       // = false;
};

typedef struct _FlipBuffer FlipBuffer;

void FlipBuffer_deleteBuffer(FlipBuffer *buf)
{
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

uint8_t *FlipBuffer_getBuffer(FlipBuffer *buf, size_t length)
{
  length = (length + 3) & ~3;
  buf->_flip = !_flip;

  if (buf->_length[_flip] < length || buf->_length[buf->_flip] > length + 64)
  {
    if (buf->_buffer[_flip])
    {
      heap_free(buf->_buffer[buf->_flip]);
    }
    buf->_buffer[_flip] = (uint8_t *)heap_alloc_dma(length);
    buf->_length[_flip] = buf->_buffer[_flip] ? length : 0;
  }
  return buf->_buffer[_flip];
}


  void pinMode(int_fast16_t pin, pin_mode_t mode)
  {
    if (pin < 0) return;

    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_DISABLE);


#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
    if (rtc_gpio_is_valid_gpio((gpio_num_t)pin)) rtc_gpio_deinit((gpio_num_t)pin);
#endif
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (std::uint64_t)1 << pin;
    switch (mode)
    {
    case pin_mode_t::output:
      io_conf.mode = GPIO_MODE_OUTPUT;
      break;
    default:
      io_conf.mode = GPIO_MODE_INPUT;
      break;
    }
    io_conf.mode         = (mode == pin_mode_t::output) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    io_conf.pull_down_en = (mode == pin_mode_t::input_pulldown) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = (mode == pin_mode_t::input_pullup  ) ? GPIO_PULLUP_ENABLE   : GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

  }