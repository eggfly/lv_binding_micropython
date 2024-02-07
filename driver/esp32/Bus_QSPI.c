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
#if defined (ESP_PLATFORM)
#include <sdkconfig.h>

/// ESP32-S3をターゲットにした際にREG_SPI_BASEが定義されていなかったので応急処置 ;
#if defined ( CONFIG_IDF_TARGET_ESP32S3 )
 #define REG_SPI_BASE(i)   (DR_REG_SPI1_BASE + (((i)>1) ? (((i)* 0x1000) + 0x20000) : (((~(i)) & 1)* 0x1000 )))
#elif defined ( CONFIG_IDF_TARGET_ESP32 ) || !defined ( CONFIG_IDF_TARGET )
 #define LGFX_SPIDMA_WORKAROUND
#endif

#include "Bus_QSPI.h"

// #include "../../misc/pixelcopy.hpp"

#include <soc/dport_reg.h>
#include <driver/rtc_io.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

#if __has_include (<esp_private/periph_ctrl.h>)
 #include <esp_private/periph_ctrl.h>
#else
 #include <driver/periph_ctrl.h>
#endif

#if defined (ARDUINO) // Arduino ESP32
 #include <soc/periph_defs.h>
 #include <esp32-hal-cpu.h>
#else
 #include <driver/spi_master.h>

 #if defined ( CONFIG_IDF_TARGET_ESP32S3 )
  #if __has_include (<esp32s3/rom/gpio.h>)
    #include <esp32s3/rom/gpio.h>
  #else
    #include <rom/gpio.h>
  #endif
 #elif defined ( CONFIG_IDF_TARGET_ESP32S2 )
  #if __has_include (<esp32s2/rom/gpio.h>)
    #include <esp32s2/rom/gpio.h>
  #else
    #include <rom/gpio.h>
  #endif
 #else
  #if __has_include (<esp32/rom/gpio.h>)
    #include <esp32/rom/gpio.h>
  #else
    #include <rom/gpio.h>
  #endif
 #endif
#endif

#ifndef SPI_PIN_REG
 #define SPI_PIN_REG SPI_MISC_REG
#endif

#if defined (SOC_GDMA_SUPPORTED)  // for C3/S3
 #include <soc/gdma_channel.h>
 #include <soc/gdma_reg.h>
 #if !defined DMA_OUT_LINK_CH0_REG
  #define DMA_OUT_LINK_CH0_REG       GDMA_OUT_LINK_CH0_REG
  #define DMA_OUTFIFO_STATUS_CH0_REG GDMA_OUTFIFO_STATUS_CH0_REG
  #define DMA_OUTLINK_START_CH0      GDMA_OUTLINK_START_CH0
  #define DMA_OUTFIFO_EMPTY_CH0      GDMA_OUTFIFO_EMPTY_L3_CH0
 #endif
#endif

#include "common.h"

// #include <algorithm>


// eggfly added


#if !defined(SPI_MOSI_DLEN_REG)
  #define SPI_EXECUTE (uint32_t)(SPI_USR | SPI_UPDATE)
#define SPI_MOSI_DLEN_REG(i) (REG_SPI_BASE(i) + 0x1C)
#define SPI_MISO_DLEN_REG(i) (REG_SPI_BASE(i) + 0x1C)
#else
  #define SPI_EXECUTE (SPI_USR)
#endif

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    #define default_spi_host VSPI_HOST
    #define spi_periph_num 3
#else
    #define default_spi_host SPI2_HOST
    #define spi_periph_num SOC_SPI_PERIPH_NUM
#endif

static spi_device_handle_t _spi_dev_handle[spi_periph_num] = {};


  inline volatile uint32_t *reg(uint32_t addr) { return (volatile uint32_t *)ETS_UNCACHED_ADDR(addr); }
  inline void qspi_exec_spi(bus_qspi_t* bus) { *(bus->_spi_cmd_reg) = SPI_EXECUTE; }
  inline void qspi_wait_spi(bus_qspi_t* bus)
  {
    while (*(bus->_spi_cmd_reg) & SPI_USR)
      ;
  }
  inline void qspi_set_write_len(bus_qspi_t* bus, uint32_t bitlen) { *(bus->_spi_mosi_dlen_reg) = bitlen - 1; }
  inline void qspi_set_read_len(bus_qspi_t* bus, uint32_t bitlen) { *reg(SPI_MISO_DLEN_REG(bus->_spi_port)) = bitlen - 1; }

  void qspi_dc_control(bus_qspi_t* bus, bool flg)
  {
    volatile uint32_t * reg = bus->_gpio_reg_dc[flg];
    uint32_t mask = bus->_mask_reg_dc;
    volatile uint32_t * spi_cmd_reg = bus->_spi_cmd_reg;
#if !defined(CONFIG_IDF_TARGET) || defined(CONFIG_IDF_TARGET_ESP32)
    while (*spi_cmd_reg & SPI_USR)
    {
    } // wait SPI
#else
#if defined(SOC_GDMA_SUPPORTED)
    volatile uint32_t * dma = bus->_clear_dma_reg;
    if (dma)
    {
      bus->_clear_dma_reg = NULL;
      while (*spi_cmd_reg & SPI_USR)
      {
      } // wait SPI
      *dma = 0;
    }
    else
    {
      while (*spi_cmd_reg & SPI_USR)
      {
      } // wait SPI
    }
#else
    uint32_t* dma = bus->_spi_dma_out_link_reg;
    while (*spi_cmd_reg & SPI_USR)
    {
    } // wait SPI
    *dma = 0;
#endif
#endif
    *reg = mask;
  }



// eggfly end


  void qspi_config(bus_qspi_t* bus, qspi_config_t cfg)
  {
    bus->_cfg = cfg;

    uint32_t spi_port = (uint32_t)(cfg.spi_host) + 1;  // FSPI=1  HSPI=2  VSPI=3;
    bus->_spi_port = spi_port;
    bus->_spi_w0_reg           = reg(SPI_W0_REG(          spi_port));
    bus->_spi_cmd_reg          = reg(SPI_CMD_REG(         spi_port));
    bus->_spi_user_reg         = reg(SPI_USER_REG(        spi_port));
    bus->_spi_mosi_dlen_reg    = reg(SPI_MOSI_DLEN_REG(   spi_port));
#if defined ( SOC_GDMA_SUPPORTED )
#elif defined ( SPI_DMA_STATUS_REG )
    bus->_spi_dma_out_link_reg = reg(SPI_DMA_OUT_LINK_REG(spi_port));
    bus->_spi_dma_outstatus_reg = reg(SPI_DMA_STATUS_REG(spi_port));
#else
    bus->_spi_dma_out_link_reg = reg(SPI_DMA_OUT_LINK_REG(spi_port));
    bus->_spi_dma_outstatus_reg = reg(SPI_DMA_OUTSTATUS_REG(spi_port));
#endif
    if (cfg.pin_dc < 0)
    { // D/Cピン不使用の場合はGPIOレジスタの代わりにダミーとしてmask_reg_dcのアドレスを設定しておく;
      bus->_mask_reg_dc = 0;
      bus->_gpio_reg_dc[0] = &bus->_mask_reg_dc;
      bus->_gpio_reg_dc[1] = &bus->_mask_reg_dc;
    }
    else
    {
      bus->_mask_reg_dc = (1ul << (cfg.pin_dc & 31));
      bus->_gpio_reg_dc[0] = get_gpio_lo_reg(cfg.pin_dc);
      bus->_gpio_reg_dc[1] = get_gpio_hi_reg(cfg.pin_dc);
    }
    bus->_last_freq_apb = 0;

    uint8_t spi_mode = cfg.spi_mode;
    bus->_user_reg = (spi_mode == 1 || spi_mode == 2) ? SPI_CK_OUT_EDGE | SPI_USR_MOSI : SPI_USR_MOSI;
    ESP_LOGI("LGFX","Bus_QSPI::config  spi_port:%d  dc:%0d %02x", spi_port, cfg.pin_dc, bus->_mask_reg_dc);
  }

  bool qspi_init(bus_qspi_t* bus)
  {
    ESP_LOGI("LGFX","Bus_QSPI::init");
    qspi_dc_control(bus, true);
    // eggfly mod
    // pinMode(_cfg.pin_dc, pin_mode_t::output);

    int dma_ch = bus->_cfg.dma_channel;
#if defined (ESP_IDF_VERSION)
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
    dma_ch = dma_ch ? SPI_DMA_CH_AUTO : SPI_DMA_DISABLED;
 #endif
#endif
    // TODO
    // bus->_inited = spi::initQuad(bus->_cfg.spi_host, bus->_cfg.pin_sclk, bus->_cfg.pin_io0, bus->_cfg.pin_io1, bus->_cfg.pin_io2, bus->_cfg.pin_io3, dma_ch).has_value();

#if defined ( SOC_GDMA_SUPPORTED )
    // 割当られたDMAチャネル番号を取得する

#if defined ( SOC_GDMA_TRIG_PERIPH_SPI3 )
    int peri_sel = (bus->_spi_port == 3) ? SOC_GDMA_TRIG_PERIPH_SPI3 : SOC_GDMA_TRIG_PERIPH_SPI2;
#else
    int peri_sel = SOC_GDMA_TRIG_PERIPH_SPI2;
#endif

    int assigned_dma_ch = search_dma_out_ch(peri_sel);

    if (assigned_dma_ch >= 0)
    { // DMAチャンネルが特定できたらそれを使用する;
      bus->_spi_dma_out_link_reg  = reg(DMA_OUT_LINK_CH0_REG       + assigned_dma_ch * 0xC0);
      bus->_spi_dma_outstatus_reg = reg(DMA_OUTFIFO_STATUS_CH0_REG + assigned_dma_ch * 0xC0);
    }
#elif defined ( CONFIG_IDF_TARGET_ESP32 ) || !defined ( CONFIG_IDF_TARGET )

    dma_ch = (*reg(DPORT_SPI_DMA_CHAN_SEL_REG) >> (bus->_cfg.spi_host * 2)) & 3;
    ESP_LOGE("LGFX", "SPI_HOST: %d / DMA_CH: %d", bus->_cfg.spi_host, dma_ch);

#endif

    bus->_dma_ch = dma_ch;

    return bus->_inited;
  }

  static void qspi_gpio_reset(size_t pin)
  {
    if (pin >= GPIO_NUM_MAX) return;
    gpio_reset_pin( (gpio_num_t)pin);
    gpio_matrix_out((gpio_num_t)pin, 0x100, 0, 0);
    gpio_matrix_in( (gpio_num_t)pin, 0x100, 0   );
  }

  void spi_beginTransaction(int spi_host)
  {
    if (_spi_dev_handle[spi_host]) {
      if (ESP_OK != spi_device_acquire_bus(_spi_dev_handle[spi_host], portMAX_DELAY)) {
        ESP_LOGW("LGFX", "Failed to spi_device_acquire_bus. ");
      }
#if defined ( SOC_GDMA_SUPPORTED )
      *reg(SPI_DMA_CONF_REG((spi_host + 1))) = 0; /// Clear previous transfer
#endif
    }
  }

  void spi_endTransaction(int spi_host)
  {
    if (_spi_dev_handle[spi_host]) {
      spi_device_release_bus(_spi_dev_handle[spi_host]);
    }
  }

  void spi_host_release(int spi_host)
  {
    ESP_LOGI("LGFX", "spi::release");
    if (_spi_dev_handle[spi_host] != NULL)
    {
      spi_bus_remove_device(_spi_dev_handle[spi_host]);
      spi_bus_free((spi_host_device_t)(spi_host));
      _spi_dev_handle[spi_host] = NULL;
    }
  }

  void qspi_release(bus_qspi_t* bus)
  {
    ESP_LOGI("LGFX","Bus_QSPI::release");
    if (!bus->_inited) return;
    bus->_inited = false;
    spi_host_release(bus->_cfg.spi_host);
    qspi_gpio_reset(bus->_cfg.pin_dc);
    // qspi_gpio_reset(_cfg.pin_mosi);
    // qspi_gpio_reset(_cfg.pin_miso);
    // qspi_gpio_reset(_cfg.pin_sclk);
    qspi_gpio_reset(bus->_cfg.pin_io0);
    qspi_gpio_reset(bus->_cfg.pin_io1);
    qspi_gpio_reset(bus->_cfg.pin_io2);
    qspi_gpio_reset(bus->_cfg.pin_io3);
    qspi_gpio_reset(bus->_cfg.pin_sclk);
  }

  void qspi_beginTransaction(bus_qspi_t* bus)
  {
    ESP_LOGI("LGFX", "Bus_QSPI::beginTransaction");
    uint32_t freq_apb = getApbFrequency();
    uint32_t clkdiv_write = bus->_clkdiv_write;
    if (bus->_last_freq_apb != freq_apb)
    {
      bus->_last_freq_apb = freq_apb;
      bus->_clkdiv_read = FreqToClockDiv(freq_apb, bus->_cfg.freq_read);
      clkdiv_write = FreqToClockDiv(freq_apb, bus->_cfg.freq_write);
      bus->_clkdiv_write = clkdiv_write;
      qspi_dc_control(bus, true);
      // pinMode(_cfg.pin_dc, pin_mode_t::output);
    }

    uint8_t spi_mode = bus->_cfg.spi_mode;
    uint32_t pin = (spi_mode & 2) ? SPI_CK_IDLE_EDGE : 0;

    if (bus->_cfg.use_lock) spi_beginTransaction(bus->_cfg.spi_host);

    *bus->_spi_user_reg = bus->_user_reg;
    uint8_t spi_port = bus->_spi_port;
    (void)spi_port;
    *reg(SPI_PIN_REG(spi_port)) = pin;
    *reg(SPI_CLOCK_REG(spi_port)) = clkdiv_write;
#if defined ( SPI_UPDATE )
    *bus->_spi_cmd_reg = SPI_UPDATE;
#endif
  }

  void qspi_endTransaction(bus_qspi_t* bus)
  {
    qspi_dc_control(bus, true);
#if defined ( LGFX_SPIDMA_WORKAROUND )
    if (bus->_dma_ch) { spicommon_dmaworkaround_idle(_dma_ch); }
#endif
    if (bus->_cfg.use_lock) spi_endTransaction(bus->_cfg.spi_host);
#if defined (ARDUINO) // Arduino ESP32
    *bus->_spi_user_reg = SPI_USR_MOSI | SPI_USR_MISO | SPI_DOUTDIN; // for other SPI device (e.g. SD card)
#endif
  }

  void qspi_wait(bus_qspi_t* bus)
  {
    volatile uint32_t * spi_cmd_reg = bus->_spi_cmd_reg;
    // while (*spi_cmd_reg & SPI_USR);
    uint32_t time_out = esp_timer_get_time();
    while (*spi_cmd_reg & SPI_USR) {
        if ((esp_timer_get_time() - time_out) > 20000) { break; }
    }
  }

  bool qspi_busy(bus_qspi_t* bus)
  {
    return (*bus->_spi_cmd_reg & SPI_USR);
  }

  bool qspi_writeCommand(bus_qspi_t* bus, uint32_t data, uint_fast8_t bit_length)
  {
    ESP_LOGI("LGFX", "writeCmd: %02x  len:%d   dc:%02x", data, bit_length, bus->_mask_reg_dc);
    --bit_length;
    volatile uint32_t* spi_mosi_dlen_reg = bus->_spi_mosi_dlen_reg;
    volatile uint32_t* spi_w0_reg = bus->_spi_w0_reg;
    volatile uint32_t* spi_cmd_reg = bus->_spi_cmd_reg;
    volatile uint32_t* gpio_reg_dc = bus->_gpio_reg_dc[0];
    uint32_t mask_reg_dc = bus->_mask_reg_dc;

    /* Send data in 1-bit mode */
    volatile uint32_t* spi_user_reg = bus->_spi_user_reg;
    uint32_t user = (*spi_user_reg & (~SPI_FWRITE_QUAD));

#if !defined ( CONFIG_IDF_TARGET ) || defined ( CONFIG_IDF_TARGET_ESP32 )
    while (*spi_cmd_reg & SPI_USR) {}    // wait SPI
#else
    volatile uint32_t * dma = bus->_clear_dma_reg;
    if (dma)
    {
      bus->_clear_dma_reg = NULL;
      while (*spi_cmd_reg & SPI_USR) {}    // wait SPI
      *dma = 0;
    }
    else
    {
      while (*spi_cmd_reg & SPI_USR) {}    // wait SPI
    }
#endif
    *spi_user_reg = user;
    *spi_mosi_dlen_reg = bit_length;   // set bitlength
    *spi_w0_reg = data;                // set data
    *gpio_reg_dc = mask_reg_dc;        // D/C
    *spi_cmd_reg = SPI_EXECUTE;        // exec SPI
    return true;
  }


  void qspi_writeData(bus_qspi_t* bus, uint32_t data, uint_fast8_t bit_length)
  {
    ESP_LOGI("LGFX","writeData: %02x  len:%d", data, bit_length);
    --bit_length;
    volatile uint32_t * spi_mosi_dlen_reg = bus->_spi_mosi_dlen_reg;
    volatile uint32_t * spi_w0_reg = bus->_spi_w0_reg;
    volatile uint32_t * spi_cmd_reg = bus->_spi_cmd_reg;
    volatile uint32_t * gpio_reg_dc = bus->_gpio_reg_dc[1];
    uint32_t mask_reg_dc = bus->_mask_reg_dc;

    /* Send data in 4-bit mode */
    volatile uint32_t * spi_user_reg = bus->_spi_user_reg;
    uint32_t user = (*spi_user_reg | SPI_FWRITE_QUAD);


#if !defined ( CONFIG_IDF_TARGET ) || defined ( CONFIG_IDF_TARGET_ESP32 )
    while (*spi_cmd_reg & SPI_USR) {}    // wait SPI
#else
    volatile uint32_t * dma = bus->_clear_dma_reg;
    if (dma)
    {
      bus->_clear_dma_reg = NULL;
      while (*spi_cmd_reg & SPI_USR) {}    // wait SPI
      *dma = 0;
    }
    else
    {
      while (*spi_cmd_reg & SPI_USR) {}    // wait SPI
    }
#endif
    *spi_user_reg = user;
    *spi_mosi_dlen_reg = bit_length;   // set bitlength
    *spi_w0_reg = data;                // set data
    *gpio_reg_dc = mask_reg_dc;        // D/C
    *spi_cmd_reg = SPI_EXECUTE;        // exec SPI
  }


//   void Bus_QSPI::writeDataRepeat(uint32_t data, uint_fast8_t bit_length, uint32_t count)
//   {
//     auto spi_mosi_dlen_reg = _spi_mosi_dlen_reg;
//     auto spi_w0_reg = _spi_w0_reg;
//     auto spi_cmd_reg = _spi_cmd_reg;
//     auto gpio_reg_dc = _gpio_reg_dc[1];
//     auto mask_reg_dc = _mask_reg_dc;

//     /* Send data in 4-bit mode */
//     auto spi_user_reg = _spi_user_reg;
//     uint32_t user = (*spi_user_reg | SPI_FWRITE_QUAD);
    
// #if defined ( CONFIG_IDF_TARGET ) && !defined ( CONFIG_IDF_TARGET_ESP32 )
//     auto dma = bus->_clear_dma_reg;
//     if (dma) { _clear_dma_reg = NULL; }
// #endif
//     if (1 == count)
//     {
//       --bit_length;
//       while (*spi_cmd_reg & SPI_USR);    // wait SPI
// #if defined ( CONFIG_IDF_TARGET ) && !defined ( CONFIG_IDF_TARGET_ESP32 )
//       if (dma) { *dma = 0; }
// #endif
//       *spi_user_reg = user;
//       *gpio_reg_dc = mask_reg_dc;        // D/C high (data)
//       *spi_mosi_dlen_reg = bit_length;   // set bitlength
//       *spi_w0_reg = data;                // set data
//       *spi_cmd_reg = SPI_EXECUTE;        // exec SPI
//       return;
//     }

//     uint32_t regbuf0 = data | data << bit_length;
//     uint32_t regbuf1;
//     uint32_t regbuf2;
//     // make 12Bytes data.
//     bool bits24 = (bit_length == 24);
//     if (bits24) {
//       regbuf1 = regbuf0 >> 8 | regbuf0 << 16;
//       regbuf2 = regbuf0 >>16 | regbuf0 <<  8;
//     } else {
//       if (bit_length == 8) { regbuf0 |= regbuf0 << 16; }
//       regbuf1 = regbuf0;
//       regbuf2 = regbuf0;
//     }

//     uint32_t length = bit_length * count;          // convert to bitlength.
//     uint32_t len = (length <= 96u) ? length : (length <= 144u) ? 48u : 96u; // 1st send length = max 12Byte (96bit).

//     length -= len;
//     --len;

//     while (*spi_cmd_reg & SPI_USR) {}  // wait SPI
// #if defined ( CONFIG_IDF_TARGET ) && !defined ( CONFIG_IDF_TARGET_ESP32 )
//     if (dma) { *dma = 0; }
// #endif
//     *spi_user_reg = user;
//     *gpio_reg_dc = mask_reg_dc;      // D/C high (data)
//     *spi_mosi_dlen_reg = len;
//     // copy to SPI buffer register
//     spi_w0_reg[0] = regbuf0;
//     spi_w0_reg[1] = regbuf1;
//     spi_w0_reg[2] = regbuf2;
//     *spi_cmd_reg = SPI_EXECUTE;      // exec SPI
//     if (0 == length) return;

//     uint32_t regbuf[7] = { regbuf0, regbuf1, regbuf2, regbuf0, regbuf1, regbuf2, regbuf0 } ;

//     // copy to SPI buffer register
//     memcpy((void*)&spi_w0_reg[3], regbuf, 24);
//     memcpy((void*)&spi_w0_reg[9], regbuf, 28);

//     // limit = 64Byte / depth_bytes;
//     // When 24bit color, 504 bits out of 512bit buffer are used.
//     // When 16bit color, it uses exactly 512 bytes. but, it behaves like a ring buffer, can specify a larger size.
//     uint32_t limit;
//     if (bits24)
//     {
//       limit = 504;
//       len = length % limit;
//     }
//     else
//     {
// #if defined ( CONFIG_IDF_TARGET_ESP32 )
//       limit = (1 << 11);
// #else
//       limit = (1 << 9);
// #endif
//       len = length & (limit - 1);
//     }

//     if (len)
//     {
//       length -= len;
//       --len;
//       while (*spi_cmd_reg & SPI_USR);
//       *spi_mosi_dlen_reg = len;
//       *spi_cmd_reg = SPI_EXECUTE;
//       if (0 == length) return;
//     }

//     while (*spi_cmd_reg & SPI_USR);
//     *spi_mosi_dlen_reg = limit - 1;
//     *spi_cmd_reg = SPI_EXECUTE;
//     while (length -= limit)
//     {
//       while (*spi_cmd_reg & SPI_USR);
//       *spi_cmd_reg = SPI_EXECUTE;
//     }
//   }


//   void qspi_writePixels(pixelcopy_t* param, uint32_t length)
//   {
//     /* Send data in 4-bit mode */
//     auto spi_user_reg = _spi_user_reg;
//     uint32_t user = (*spi_user_reg | SPI_FWRITE_QUAD);
//     *spi_user_reg = user;

//     const uint8_t bytes = param->dst_bits >> 3;
//     if (_cfg.dma_channel)
//     {
//       uint32_t limit = (bytes == 2) ? 32 : 24;
//       uint32_t len;
//       do
//       {
//         len = (limit << 1) <= length ? limit : length;
//         if (limit <= 256) limit <<= 1;
//         auto dmabuf = _flip_buffer.getBuffer(len * bytes);
//         param->fp_copy(dmabuf, 0, len, param);
//         writeBytes(dmabuf, len * bytes, true, true);
//       } while (length -= len);
//       return;
//     }

// /// ESP32-C3 で HIGHPART を使用すると異常動作するため分岐する;
// #if defined ( SPI_UPDATE )  // for C3/S3

//     const uint32_t limit = (bytes == 2) ? 32 : 21;
//     uint32_t l = (length - 1) / limit;
//     uint32_t len = length - (l * limit);
//     length = l;
//     uint32_t regbuf[16];
//     param->fp_copy(regbuf, 0, len, param);

//     auto spi_w0_reg = _spi_w0_reg;

//     dc_control(true);
//     set_write_len(len * bytes << 3);

//     memcpy((void*)spi_w0_reg, regbuf, (len * bytes + 3) & (~3));

//     exec_spi();
//     if (0 == length) return;


//     param->fp_copy(regbuf, 0, limit, param);
//     wait_spi();
//     set_write_len(limit * bytes << 3);
//     memcpy((void*)spi_w0_reg, regbuf, limit * bytes);
//     exec_spi();


//     while (--length)
//     {
//       param->fp_copy(regbuf, 0, limit, param);
//       wait_spi();
//       memcpy((void*)spi_w0_reg, regbuf, limit * bytes);
//       exec_spi();
//     }

// #else

//     const uint32_t limit = (bytes == 2) ? 16 : 10; //  limit = 32/bytes (bytes==2 is 16   bytes==3 is 10)
//     uint32_t len = (length - 1) / limit;
//     uint32_t highpart = (len & 1) << 3;
//     len = length - (len * limit);
//     uint32_t regbuf[8];
//     param->fp_copy(regbuf, 0, len, param);

//     auto spi_w0_reg = _spi_w0_reg;

//     uint32_t user_reg = *_spi_user_reg;

//     dc_control(true);
//     set_write_len(len * bytes << 3);

//     memcpy((void*)&spi_w0_reg[highpart], regbuf, (len * bytes + 3) & (~3));
//     if (highpart) *_spi_user_reg = user_reg | SPI_USR_MOSI_HIGHPART;
//     exec_spi();
//     if (0 == (length -= len)) return;

//     for (; length; length -= limit)
//     {
//       param->fp_copy(regbuf, 0, limit, param);
//       memcpy((void*)&spi_w0_reg[highpart ^= 0x08], regbuf, limit * bytes);
//       uint32_t user = user_reg;
//       if (highpart) user |= SPI_USR_MOSI_HIGHPART;
//       if (len != limit)
//       {
//         len = limit;
//         wait_spi();
//         set_write_len(limit * bytes << 3);
//         *_spi_user_reg = user;
//         exec_spi();
//       }
//       else
//       {
//         wait_spi();
//         *_spi_user_reg = user;
//         exec_spi();
//       }
//     }

// #endif

//   }


  void qspi_writeBytes(bus_qspi_t *bus, const uint8_t* data, uint32_t length, bool dc, bool use_dma)
  {
    /* Send data in 4-bit mode */
    volatile uint32_t * spi_user_reg = bus->_spi_user_reg;
    uint32_t user = (*spi_user_reg | SPI_FWRITE_QUAD);
    *spi_user_reg = user;

    if (length <= 64)
    {
      volatile uint32_t * spi_w0_reg = bus->_spi_w0_reg;
      size_t aligned_len = (length + 3) & (~3);
      length <<= 3;
      qspi_dc_control(bus, dc);
      qspi_set_write_len(bus, length);
      memcpy((void*)spi_w0_reg, data, aligned_len);
      qspi_exec_spi(bus);
      return;
    }

    if (bus->_cfg.dma_channel)
    {
      if (false == use_dma && length < 1024)
      {
        use_dma = true;
        uint8_t *buf = FlipBuffer_getBuffer(&bus->_flip_buffer, length);
        memcpy(buf, data, length);
        data = buf;
      }
      if (use_dma)
      {
        volatile uint32_t * spi_dma_out_link_reg = bus->_spi_dma_out_link_reg;
        volatile uint32_t * cmd = bus->_spi_cmd_reg;
        while (*cmd & SPI_USR) {}
        *spi_dma_out_link_reg = 0;
        qspi__setup_dma_desc_links(bus, data, length);
#if defined ( SOC_GDMA_SUPPORTED )
        volatile uint32_t * dma = reg(SPI_DMA_CONF_REG(bus->_spi_port));
        *dma = 0; /// Clear previous transfer
        uint32_t len = ((length - 1) & ((SPI_MS_DATA_BITLEN)>>3)) + 1;
        *spi_dma_out_link_reg = DMA_OUTLINK_START_CH0 | ((int)(&bus->_dmadesc[0]) & 0xFFFFF);
        *dma = SPI_DMA_TX_ENA;
        bus->_clear_dma_reg = dma;
#else
        volatile uint32_t* dma_conf_reg = reg(SPI_DMA_CONF_REG(bus->_spi_port));
        uint32_t dma_conf = *dma_conf_reg & ~(SPI_OUT_DATA_BURST_EN | SPI_AHBM_RST | SPI_AHBM_FIFO_RST | SPI_OUT_RST);
        *dma_conf_reg = dma_conf | SPI_AHBM_RST | SPI_AHBM_FIFO_RST | SPI_OUT_RST;

        // 送信長が4の倍数の場合のみバーストモードを使用する
        // ※ 以下の3つの条件が揃うと、DMA転送の末尾付近でデータが化ける現象が起きる。
        //    1.送信クロック80MHz (APBクロックと1:1)
        //    2.DMAバースト読出し有効
        //    3.送信データ長が4の倍数ではない (1Byte~3Byteの端数がある場合)
        dma_conf |= (length & 3) ? (SPI_OUTDSCR_BURST_EN) : (SPI_OUTDSCR_BURST_EN | SPI_OUT_DATA_BURST_EN);

        *dma_conf_reg = dma_conf;
        uint32_t len = length;
        *spi_dma_out_link_reg = SPI_OUTLINK_START | ((int)(&_dmadesc[0]) & 0xFFFFF);
        bus->_clear_dma_reg = spi_dma_out_link_reg;
#endif
        qspi_set_write_len(bus, len << 3);
        *bus->_gpio_reg_dc[dc] = bus->_mask_reg_dc;

        // DMA準備完了待ち;
#if defined ( SOC_GDMA_SUPPORTED )
        while (*bus->_spi_dma_outstatus_reg & DMA_OUTFIFO_EMPTY_CH0 ) {}
#elif defined (SPI_DMA_OUTFIFO_EMPTY)
        while (*bus->_spi_dma_outstatus_reg & SPI_DMA_OUTFIFO_EMPTY ) {}
#else
 #if defined ( LGFX_SPIDMA_WORKAROUND )
        if (bus->_dma_ch) { spicommon_dmaworkaround_transfer_active(bus->_dma_ch); }
 #endif
#endif
        qspi_exec_spi(bus);

#if defined ( SOC_GDMA_SUPPORTED )
        if (length -= len)
        {
          while (*cmd & SPI_USR) {}
          qspi_set_write_len(bus, SPI_MS_DATA_BITLEN + 1);
          goto label_start;
          do
          {
            while (*cmd & SPI_USR) {}
label_start:
            qspi_exec_spi(bus);
          } while (length -= ((SPI_MS_DATA_BITLEN + 1) >> 3));
        }
#endif
        return;
      }
    }

    volatile uint32_t * spi_w0_reg = bus->_spi_w0_reg;

/// ESP32-C3 で HIGHPART を使用すると異常動作するため分岐する;
#if defined ( SPI_UPDATE )  // for C3/S3

    uint32_t regbuf[16];
    const uint32_t limit = 64;
    uint32_t len = ((length - 1) & 0x3F) + 1;

    memcpy(regbuf, data, len);
    qspi_dc_control(bus, dc);
    qspi_set_write_len(bus, len << 3);

    memcpy((void*)spi_w0_reg, regbuf, (len + 3) & (~3));
    qspi_exec_spi(bus);
    if (0 == (length -= len)) return;

    data += len;
    memcpy(regbuf, data, limit);
    qspi_wait_spi(bus);
    qspi_set_write_len(bus, limit << 3);
    memcpy((void*)spi_w0_reg, regbuf, limit);
    qspi_exec_spi(bus);
    if (0 == (length -= limit)) return;

    do
    {
      data += limit;
      memcpy(regbuf, data, limit);
      qspi_wait_spi(bus);
      memcpy((void*)spi_w0_reg, regbuf, limit);
      qspi_exec_spi(bus);
    } while (0 != (length -= limit));

#else

    const uint32_t limit = 32;
    uint32_t len = ((length - 1) & 0x1F) + 1;
    uint32_t highpart = ((length - 1) & limit) >> 2; // 8 or 0

    uint32_t user_reg = bus->_user_reg;
    user_reg = user_reg | SPI_FWRITE_QUAD;

    qspi_dc_control(bus, dc);
    qspi_set_write_len(len << 3);

    memcpy((void*)&spi_w0_reg[highpart], data, (len + 3) & (~3));
    if (highpart) *_spi_user_reg = user_reg | SPI_USR_MOSI_HIGHPART;
    qspi_exec_spi(bus);
    if (0 == (length -= len)) return;

    for (; length; length -= limit)
    {
      data += len;
      memcpy((void*)&spi_w0_reg[highpart ^= 0x08], data, limit);
      uint32_t user = user_reg;
      if (highpart) user |= SPI_USR_MOSI_HIGHPART;
      if (len != limit)
      {
        len = limit;
        qspi_wait_spi(bus);
        qspi_set_write_len(limit << 3);
        *bus->_spi_user_reg = user;
        qspi_exec_spi(bus);
      }
      else
      {
        qspi_wait_spi(bus);
        *_spi_user_reg = user;
        qspi_exec_spi(bus);
      }
    }

#endif

  }




  void qspi_addDMAQueue(bus_qspi_t *bus, const uint8_t* data, uint32_t length)
  {
    if (!bus->_cfg.dma_channel)
    {
      qspi_writeBytes(bus, data, length, true, true);
      return;
    }

    bus->_dma_queue_bytes += length;
    size_t index = bus->_dma_queue_size;
    size_t new_size = index + ((length-1) / SPI_MAX_DMA_LEN) + 1;

    if (bus->_dma_queue_capacity < new_size)
    {
      bus->_dma_queue_capacity = new_size + 8;
      void* new_queue = (lldesc_t*)heap_caps_malloc(sizeof(lldesc_t) * bus->_dma_queue_capacity, MALLOC_CAP_DMA);
      if (index)
      {
        memcpy(new_queue, bus->_dma_queue, sizeof(lldesc_t) * index);
      }
      if (bus->_dma_queue != NULL) { heap_free(bus->_dma_queue); }
      bus->_dma_queue = new_queue;
    }
    bus->_dma_queue_size = new_size;
    lldesc_t *dmadesc = &bus->_dma_queue[index];

    while (length > SPI_MAX_DMA_LEN)
    {
      *(uint32_t*)dmadesc = SPI_MAX_DMA_LEN | SPI_MAX_DMA_LEN << 12 | 0x80000000;
      dmadesc->buf = (uint8_t*)(data);
      dmadesc++;
      data += SPI_MAX_DMA_LEN;
      length -= SPI_MAX_DMA_LEN;
    }
    *(uint32_t*)dmadesc = ((length + 3) & ( ~3 )) | length << 12 | 0x80000000;
    dmadesc->buf = (uint8_t*)(data);
  }


  void qspi_execDMAQueue(bus_qspi_t *bus)
  {
    if (0 == bus->_dma_queue_size) return;

    /* Send data in 4-bit mode */
    volatile uint32_t * spi_user_reg = bus->_spi_user_reg;
    uint32_t user = (*spi_user_reg | SPI_FWRITE_QUAD);
    *spi_user_reg = user;

    int index = bus->_dma_queue_size - 1;
    bus->_dma_queue_size = 0;
    bus->_dma_queue[index].eof = 1;
    bus->_dma_queue[index].qe.stqe_next = NULL;
    while (--index >= 0)
    {
      bus->_dma_queue[index].qe.stqe_next = &bus->_dma_queue[index + 1];
    }
    SWAP(bus->_dmadesc, bus->_dma_queue, lldesc_t *);
    SWAP(bus->_dmadesc_size, bus->_dma_queue_capacity, uint32_t);

    qspi_dc_control(bus, true);
    *bus->_spi_dma_out_link_reg = 0;

#if defined ( SOC_GDMA_SUPPORTED )
    *bus->_spi_dma_out_link_reg = DMA_OUTLINK_START_CH0 | ((int)(&bus->_dmadesc[0]) & 0xFFFFF);
    volatile uint32_t * dma = reg(SPI_DMA_CONF_REG(bus->_spi_port));
    *dma = SPI_DMA_TX_ENA;
    bus->_clear_dma_reg = dma;
    uint32_t len = ((bus->_dma_queue_bytes - 1) & ((SPI_MS_DATA_BITLEN)>>3)) + 1;
#else
    volatile uint32_t * dma_conf_reg = reg(SPI_DMA_CONF_REG(bus->_spi_port));
    uint32_t dma_conf = *dma_conf_reg & ~(SPI_OUT_DATA_BURST_EN | SPI_AHBM_RST | SPI_AHBM_FIFO_RST | SPI_OUT_RST);
    dma_conf |= SPI_OUTDSCR_BURST_EN;
    *dma_conf_reg = dma_conf | SPI_AHBM_RST | SPI_AHBM_FIFO_RST | SPI_OUT_RST;
    *dma_conf_reg = dma_conf;

    *bus->_spi_dma_out_link_reg = SPI_OUTLINK_START | ((int)(&bus->_dmadesc[0]) & 0xFFFFF);
    bus->_clear_dma_reg = bus->_spi_dma_out_link_reg;
    uint32_t len = bus->_dma_queue_bytes;
    bus->_dma_queue_bytes = 0;
#endif

    qspi_set_write_len(bus, len << 3);
    // DMA準備完了待ち;
#if defined ( SOC_GDMA_SUPPORTED )
    while (*bus->_spi_dma_outstatus_reg & DMA_OUTFIFO_EMPTY_CH0 ) {}
#elif defined (SPI_DMA_OUTFIFO_EMPTY)
    while (*bus->_spi_dma_outstatus_reg & SPI_DMA_OUTFIFO_EMPTY ) {}
#else
 #if defined ( LGFX_SPIDMA_WORKAROUND )
    if (bus->_dma_ch) { spicommon_dmaworkaround_transfer_active(bus->_dma_ch); }
 #endif
#endif
    qspi_exec_spi(bus);

#if defined ( SOC_GDMA_SUPPORTED )
    uint32_t length = bus->_dma_queue_bytes - len;
    bus->_dma_queue_bytes = 0;
    if (length)
    {
      qspi_wait_spi(bus);
      qspi_set_write_len(bus, SPI_MS_DATA_BITLEN + 1);
      goto label_start;
      do
      {
        qspi_wait_spi(bus);
label_start:
        qspi_exec_spi(bus);
      } while (length -= ((SPI_MS_DATA_BITLEN + 1) >> 3));
    }
#endif
  }



  void qspi__alloc_dmadesc(bus_qspi_t* bus, size_t len)
  {
    if (bus->_dmadesc) heap_caps_free(bus->_dmadesc);
    bus->_dmadesc_size = len;
    bus->_dmadesc = (lldesc_t*)heap_caps_malloc(sizeof(lldesc_t) * len, MALLOC_CAP_DMA);
  }

  void qspi__spi_dma_reset(bus_qspi_t* bus)
  {
#if defined( SOC_GDMA_SUPPORTED )  // for C3/S3

#elif defined( CONFIG_IDF_TARGET_ESP32S2 )
    if (bus->_cfg.spi_host == SPI2_HOST)
    {
      periph_module_reset( PERIPH_SPI2_DMA_MODULE );
    }
    else if (bus->_cfg.spi_host == SPI3_HOST)
    {
      periph_module_reset( PERIPH_SPI3_DMA_MODULE );
    }
#else
    periph_module_reset( PERIPH_SPI_DMA_MODULE );
#endif
  }

  void qspi__setup_dma_desc_links(bus_qspi_t *bus, const uint8_t *data, int32_t len)
  {          //spicommon_setup_dma_desc_links
    if (!bus->_cfg.dma_channel) return;

    if (bus->_dmadesc_size * SPI_MAX_DMA_LEN < len)
    {
      qspi__alloc_dmadesc(bus, len / SPI_MAX_DMA_LEN + 1);
    }
    lldesc_t *dmadesc = bus->_dmadesc;

    while (len > SPI_MAX_DMA_LEN)
    {
      len -= SPI_MAX_DMA_LEN;
      dmadesc->buf = (uint8_t *)data;
      data += SPI_MAX_DMA_LEN;
      *(uint32_t*)dmadesc = SPI_MAX_DMA_LEN | SPI_MAX_DMA_LEN << 12 | 0x80000000;
      dmadesc->qe.stqe_next = dmadesc + 1;
      dmadesc++;
    }
    *(uint32_t*)dmadesc = ((len + 3) & ( ~3 )) | len << 12 | 0xC0000000;
    dmadesc->buf = (uint8_t *)data;
    dmadesc->qe.stqe_next = NULL;
  }


#endif
