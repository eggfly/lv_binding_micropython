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
#pragma once

// #include "Panel_Device.hpp"
#include "Bus_QSPI.h"

struct panel_config_t
{
  /// CS ピン番号;
  /// Number of CS pin
  int16_t pin_cs;// = -1;

  /// RST ピン番号;
  /// Number of RST pin
  int16_t pin_rst;// = -1;

  /// BUSY ピン番号;
  /// Number of BUSY pin
  int16_t pin_busy;// = -1;

  /// LCDドライバが扱える画像の最大幅;
  /// The maximum width of an image that the LCD driver can handle.
  uint16_t memory_width;// = 240;

  /// LCDドライバが扱える画像の最大高さ;
  /// The maximum height of an image that the LCD driver can handle.
  uint16_t memory_height;// = 240;

  /// 実際に表示できる幅;
  /// Actual width of the display.
  uint16_t panel_width;// = 240;

  /// 実際に表示できる高さ;
  /// Actual height of the display.
  uint16_t panel_height;// = 240;

  /// パネルのX方向オフセット量;
  /// Number of offset pixels in the X direction.
  uint16_t offset_x;// = 0;

  /// パネルのY方向オフセット量;
  /// Number of offset pixels in the Y direction.
  uint16_t offset_y;// = 0;

  /// 回転方向のオフセット 0~7 (4~7は上下反転);
  /// Offset value in the direction of rotation. 0~7 (4~7 is upside down)
  uint8_t offset_rotation;// = 0;

  /// ピクセル読出し前のダミーリードのビット数;
  /// Number of bits in dummy read before pixel readout.
  uint8_t dummy_read_pixel;// = 8;

  /// データ読出し前のダミーリードのビット数;
  /// Number of bits in dummy read before data readout.
  uint8_t dummy_read_bits;// = 1;

  /// データ読出し終了時のウェイト(ST7796で必要);
  uint16_t end_read_delay_us;// = 0;

  /// データ読出しが可能か否か;
  /// Whether the data is readable or not.
  bool readable;// = true;

  /// 明暗の反転 (IPSパネルはtrueに設定);
  /// brightness inversion (e.g. IPS panel)
  bool invert;// = false;

  /// RGB=true / BGR=false パネルの赤と青が入れ替わってしまう場合 trueに設定;
  /// Set the RGB/BGR color order.
  bool rgb_order;// = false;

  /// 送信データの16bitアライメント データ長を16bit単位で送信するパネルの場合 trueに設定;
  /// 16-bit alignment of transmitted data
  bool dlen_16bit;// = false;

  /// SD等のファイルシステムとのバス共有の有無 (trueに設定するとdrawJpgFile等でバス制御が行われる);
  /// Whether or not to share the bus with the file system (if set to true, drawJpgFile etc. will control the bus)
  bool bus_shared;// = true;
};

struct _panel_sh8601 {
  bus_qspi_t _bus;
  struct panel_config_t _cfg;
};

typedef struct _panel_sh8601 panel_sh8601;


void panel_device_init_cs(panel_sh8601 *panel);

// struct Panel_SH8601Z : public Panel_Device
// {
//   public:
//       Panel_SH8601Z(void) {}

//       bool init(bool use_reset) override;
//       void beginTransaction(void) override;
//       void endTransaction(void) override;

//       color_depth_t setColorDepth(color_depth_t depth) override;
//       void setRotation(uint_fast8_t r) override;
//       void setInvert(bool invert) override;
//       void setSleep(bool flg) override;
//       void setPowerSave(bool flg) override;

//       void waitDisplay(void) override;
//       bool displayBusy(void) override;

//       void writePixels(pixelcopy_t* param, uint32_t len, bool use_dma) override;
//       void writeBlock(uint32_t rawcolor, uint32_t len) override;

//       void setWindow(uint_fast16_t xs, uint_fast16_t ys, uint_fast16_t xe, uint_fast16_t ye) override;
//       void drawPixelPreclipped(uint_fast16_t x, uint_fast16_t y, uint32_t rawcolor) override;
//       void writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor) override;
//       void writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, pixelcopy_t* param, bool use_dma) override;

//       uint32_t readCommand(uint_fast16_t cmd, uint_fast8_t index, uint_fast8_t len) override;
//       uint32_t readData(uint_fast8_t index, uint_fast8_t len) override;
//       void readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* dst, pixelcopy_t* param) override;

//       /* Override */
//       void setBrightness(uint8_t brightness) override;


// protected:
//       bool _in_transaction = false;

//       void write_cmd(uint8_t cmd);
//       void start_qspi();
//       void end_qspi();
//       void write_bytes(const uint8_t* data, uint32_t len, bool use_dma);
// };
