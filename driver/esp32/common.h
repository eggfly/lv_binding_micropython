#pragma once

#include <soc/soc.h>

#include <esp_log.h>

#include <driver/rtc_io.h>
#include <soc/rtc.h>

#include <soc/apb_ctrl_reg.h>

#include <driver/gpio.h>
#include <soc/spi_reg.h>
#include <soc/gpio_struct.h>

#define SWAP(x, y, T) do { T _TMP = x; x = y; y = _TMP; } while (0)

struct _FlipBuffer {
    uint8_t* _buffer[2];// = { nullptr, nullptr };
    size_t _length[2];// = { 0, 0 };
    uint8_t _flip;// = false;
};

typedef struct _FlipBuffer FlipBuffer;

void FlipBuffer_deleteBuffer(FlipBuffer * buf);


    uint8_t* FlipBuffer_getBuffer(FlipBuffer * buf, size_t length);

  uint32_t getApbFrequency(void)
  {
    rtc_cpu_freq_config_t conf;
    rtc_clk_cpu_freq_get_config(&conf);
    if (conf.freq_mhz >= 80){
      return 80 * 1000000;
    }
    return (conf.source_freq_mhz * 1000000) / conf.div;
  }

  uint32_t FreqToClockDiv(uint32_t fapb, uint32_t hz)
  {
    if (fapb <= hz) return SPI_CLK_EQU_SYSCLK;
    uint32_t div_num = fapb / (1 + hz);
    uint32_t pre = div_num / 64u;
    div_num = div_num / (pre+1);
    return div_num << 12 | ((div_num-1)>>1) << 6 | div_num | pre << 18;
  }


#if defined ( CONFIG_IDF_TARGET_ESP32C3 )
  static inline volatile uint32_t* get_gpio_hi_reg(int_fast8_t pin) { return &GPIO.out_w1ts.val; }
  static inline volatile uint32_t* get_gpio_lo_reg(int_fast8_t pin) { return &GPIO.out_w1tc.val; }
  static inline bool gpio_in(int_fast8_t pin) { return GPIO.in.val & (1 << (pin & 31)); }
#else
  static inline volatile uint32_t* get_gpio_hi_reg(int_fast8_t pin) { return (pin & 32) ? &GPIO.out1_w1ts.val : &GPIO.out_w1ts; }
//static inline volatile uint32_t* get_gpio_hi_reg(int_fast8_t pin) { return (volatile uint32_t*)((pin & 32) ? 0x60004014 : 0x60004008) ; } // workaround Eratta
  static inline volatile uint32_t* get_gpio_lo_reg(int_fast8_t pin) { return (pin & 32) ? &GPIO.out1_w1tc.val : &GPIO.out_w1tc; }
//static inline volatile uint32_t* get_gpio_lo_reg(int_fast8_t pin) { return (volatile uint32_t*)((pin & 32) ? 0x60004018 : 0x6000400C) ; }
  static inline bool gpio_in(int_fast8_t pin) { return ((pin & 32) ? GPIO.in1.data : GPIO.in) & (1 << (pin & 31)); }
#endif
  static inline void gpio_hi(int_fast8_t pin) { if (pin >= 0) *get_gpio_hi_reg(pin) = 1 << (pin & 31); } // ESP_LOGI("LGFX", "gpio_hi: %d", pin); }
  static inline void gpio_lo(int_fast8_t pin) { if (pin >= 0) *get_gpio_lo_reg(pin) = 1 << (pin & 31); } // ESP_LOGI("LGFX", "gpio_lo: %d", pin); }
  
static inline void heap_free(void* buf) { heap_caps_free(buf); }


// Find GDMA assigned to a peripheral;
int32_t search_dma_out_ch(int peripheral_select);