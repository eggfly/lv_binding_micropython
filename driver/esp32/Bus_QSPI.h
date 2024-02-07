/*----------------------------------------------------------------------------/
  Lovyan GFX - Graphics library for embedded devices.

Original Source:
 https://github.com/lovyan03/LovyanGFX/

Licence:
 [FreeBSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)

Contributors:
 [ciniml](https://github.com/ciniml)
 [mongonta0716](https://github.com/mongonta0716)
 [tobozo](https://github.com/tobozo)
/----------------------------------------------------------------------------*/


#include <string.h>

#if __has_include(<rom/lldesc.h>)
#include <rom/lldesc.h>
#else
#include <esp32/rom/lldesc.h>
#endif

#if __has_include(<esp_private/spi_common_internal.h>)
// ESP-IDF v5
#include <esp_private/spi_common_internal.h>
#elif __has_include(<driver/spi_common_internal.h>)
// ESP-IDF v4
#include <driver/spi_common_internal.h>
#endif

#include <driver/spi_common.h>
#include <soc/spi_reg.h>

#if __has_include(<esp_idf_version.h>)
#include <esp_idf_version.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
#define LGFX_ESP32_SPI_DMA_CH SPI_DMA_CH_AUTO
#endif
#endif

#ifndef LGFX_ESP32_SPI_DMA_CH
#define LGFX_ESP32_SPI_DMA_CH 0
#endif

// eggfly remove
// #include "../../Bus.hpp"
#include "common.h"

struct _qspi_config_t // renamed from cpp config_t
{
  // max80MHz , 40MHz , 26.67MHz , 20MHz , 16MHz , and more ...
  uint32_t freq_write; // = 16000000;
  uint32_t freq_read; // = 8000000;
  int16_t pin_sclk; // = -1;
  int16_t pin_io0; // = -1;
  int16_t pin_io1; // = -1;
  int16_t pin_io2; // = -1;
  int16_t pin_io3; // = -1;
  int16_t pin_dc; // = -1;
  uint8_t spi_mode; // = 0;
  uint8_t spi_3wire; // = true;
  uint8_t use_lock; // = true;
  uint8_t dma_channel; // = LGFX_ESP32_SPI_DMA_CH;
  spi_host_device_t spi_host; // = VSPI_HOST;
};

typedef struct _qspi_config_t qspi_config_t;

struct _bus_qspi_t
{
  qspi_config_t _cfg;
  FlipBuffer _flip_buffer;
  volatile uint32_t *_gpio_reg_dc[2];// = {nullptr, nullptr};
  volatile uint32_t *_spi_mosi_dlen_reg;// = nullptr;
  volatile uint32_t *_spi_w0_reg;// = nullptr;
  volatile uint32_t *_spi_cmd_reg;//= nullptr;
  volatile uint32_t *_spi_user_reg;// = nullptr;
  volatile uint32_t *_spi_dma_out_link_reg;// = nullptr;
  volatile uint32_t *_spi_dma_outstatus_reg;// = nullptr;
  volatile uint32_t *_clear_dma_reg;// = nullptr;
  uint32_t _last_freq_apb;// = 0;
  uint32_t _clkdiv_write;// = 0;
  uint32_t _clkdiv_read;// = 0;
  uint32_t _user_reg;// = 0;
  uint32_t _mask_reg_dc;// = 0;
  uint32_t _dma_queue_bytes;// = 0;
  lldesc_t *_dmadesc;// = nullptr;
  uint32_t _dmadesc_size;// = 0;
  lldesc_t *_dma_queue;// = nullptr;
  uint32_t _dma_queue_size;// = 0;
  uint32_t _dma_queue_capacity;// = 0;
  uint8_t _spi_port;// = 0;
  uint8_t _dma_ch;// = 0;
  bool _inited;// = false;
};

typedef struct _bus_qspi_t bus_qspi_t;
  // constexpr Bus_QSPI(void) = default;

  // const config_t &config(void) const { return _cfg; }

  // void config(const config_t &config);

  // bus_type_t busType(void) const override { return bus_type_t::bus_spi; }

  bool qspi_init(bus_qspi_t *bus);
  void qspi_release(bus_qspi_t * bus);

  void qspi_beginTransaction(bus_qspi_t *bus);
  void qspi_endTransaction(bus_qspi_t * bus);
  void qspi_wait(bus_qspi_t * bus);
  bool qspi_busy(bus_qspi_t * bus);
  // uint32_t qspi_getClock(void) const override { return _cfg.freq_write; }
  // void setClock(uint32_t freq) override
  // {
  //   if (_cfg.freq_write != freq)
  //   {
  //     _cfg.freq_write = freq;
  //     _last_freq_apb = 0;
  //   }
  // }
  // uint32_t getReadClock(void) const override { return _cfg.freq_read; }
  // void setReadClock(uint32_t freq) override
  // {
  //   if (_cfg.freq_read != freq)
  //   {
  //     _cfg.freq_read = freq;
  //     _last_freq_apb = 0;
  //   }
  // }

  // void flush(void) override {}
  bool qspi_writeCommand(bus_qspi_t * bus, uint32_t data, uint_fast8_t bit_length);
  void qspi_writeData(bus_qspi_t * bus, uint32_t data, uint_fast8_t bit_length);
  // void writeDataRepeat(uint32_t data, uint_fast8_t bit_length, uint32_t count) override;
  // void writePixels(bus_qspi_t * bus, pixelcopy_t *pc, uint32_t length);
  // void writeBytes(const uint8_t *data, uint32_t length, bool dc, bool use_dma) override;

  // void initDMA(void) override {}
  void qspi_addDMAQueue(bus_qspi_t * bus, const uint8_t *data, uint32_t length);
  void qspi_execDMAQueue(bus_qspi_t * bus);
  // uint8_t *qspi_getDMABuffer(bus_qspi_t * bus, uint32_t length) override { return _flip_buffer.getBuffer(length); }



  // void _alloc_dmadesc(size_t len);
  // void _spi_dma_reset(void);
  void qspi__setup_dma_desc_links(bus_qspi_t * bus, const uint8_t *data, int32_t len);


