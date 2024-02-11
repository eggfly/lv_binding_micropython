
#include "monica/disp/hal_disp.hpp"

// Put C++ linking here.
LGFX_Monica *disp;

extern "C" {

// C linking headers starts here.
#include "py/runtime.h"
#include "py_sh8601.h"
#include "sh8601.hpp"
#include "espidf.h"

#include "lvgl/lvgl.h"

void sh8601_init() {
  mp_printf(&mp_plat_print, "sh8601_init() called.\n");
  disp = new LGFX_Monica;
  mp_printf(&mp_plat_print, "sh8601_init() new LGFX_Monica called.\n");
  disp->init();
  mp_printf(&mp_plat_print, "sh8601_init() after disp->init().\n");
  disp->setColorDepth(16);
  mp_printf(&mp_plat_print, "sh8601_init() after disp->setColorDepth().\n");
  disp->setBrightness(200);
  mp_printf(&mp_plat_print, "sh8601_init() after disp->setBrightness().\n");

  // flush
  uint32_t x1 = 1;
  uint32_t x2 = 256;
  uint32_t y1 = 1;
  uint32_t y2 = 256;
  uint32_t w = x2 - x1 + 1;
  uint32_t h = y2 - y1 + 1;
  disp->startWrite();
  disp->setWindow(x1, y1, x2, y2);
  void *p = malloc(w * h * 2);
  disp->pushPixels(p, w * h, true);
  // _disp->pushPixelsDMA(&color_p->full, w * h, true);
  disp->endWrite();
  free(p);
  mp_printf(&mp_plat_print,
            "sh8601_init() after disp->pushPixels() and disp->endWrite().\n");
}

void sh8601_flush(void *_disp_drv, const void *_area, void *_color_p) {
  mp_printf(&mp_plat_print, "1234\n\n");
  mp_printf(&mp_plat_print, "5678\n\n");

  lv_display_t *disp_drv = (lv_display_t*)_disp_drv;
  const lv_area_t *area = (const lv_area_t *)_area;
  lv_color_t *color_p = (lv_color_t *)_color_p;

  void *driver_data = lv_display_get_driver_data(disp_drv);

  // We use disp_drv->driver_data to pass data from MP to C

  int x1 = area->x1;
  int x2 = area->x2;
  int y1 = area->y1;
  int y2 = area->y2;

  size_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
  // uint8_t color_size = 2;

  bool swap_rgb565_bytes = mp_obj_get_int(
      mp_obj_dict_get(driver_data, MP_OBJ_NEW_QSTR(MP_QSTR_swap_rgb565_bytes)));
  if (swap_rgb565_bytes == true) {
    lv_draw_sw_rgb565_swap(color_p, size);
  }

  uint32_t w = x2 - x1 + 1;
  uint32_t h = y2 - y1 + 1;
  disp->startWrite();
  disp->setWindow(x1, y1, x2, y2);
  disp->pushPixels(color_p, w * h, true);
  // _disp->pushPixelsDMA()
  disp->endWrite();
  mp_printf(&mp_plat_print, "sh8601_flush() done.\n");
}
}
