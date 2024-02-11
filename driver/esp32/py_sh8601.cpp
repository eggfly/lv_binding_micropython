
#include "monica/disp/hal_disp.hpp"

// Put C++ linking here.
LGFX_Monica *disp = nullptr;

extern "C" {

// C linking headers starts here.

#include "py_sh8601.h"

#include "../include/common.h"
#include "esp_system.h"
#include "espidf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mphalport.h"
#include "py/gc.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "soc/cpu.h"

#include "lvgl/lvgl.h"

static inline void *heap_alloc_psram(size_t length) {
  return heap_caps_malloc((length + 3) & ~3,
                          MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void sh8601_init() {
  mp_printf(&mp_plat_print, "sh8601_init() called.\n");
  if (!disp) {
    disp = new LGFX_Monica;
    mp_printf(&mp_plat_print, "sh8601_init() new LGFX_Monica called.\n");
    disp->init();
    mp_printf(&mp_plat_print, "sh8601_init() disp->init() called.\n");
    disp->setColorDepth(16);
    mp_printf(&mp_plat_print, "sh8601_init() after disp->setColorDepth().\n");
  }
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
  size_t malloc_size = w * 2;
  void *real_p = get_sp();
  void *p = malloc(malloc_size);
  if (p) {
    real_p = p;
  }
  mp_printf(&mp_plat_print, "sh8601_init() p=%x\n", p);

  memset(p, 0xff, malloc_size);
  mp_printf(&mp_plat_print, "sh8601_init() free_size=%d\n",
            esp_get_free_heap_size());
  disp->pushPixels(real_p, w * h, true);
  // _disp->pushPixelsDMA(&color_p->full, w * h, true);
  disp->endWrite();
  if (p) {
    free(p);
    mp_printf(&mp_plat_print, "sh8601_init() free(p)\n");
  } else {
    mp_printf(&mp_plat_print, "sh8601_init() not free(p)\n");
  }
  mp_printf(&mp_plat_print, "sh8601_init() free_size=%d\n",
            esp_get_free_heap_size());
  mp_printf(&mp_plat_print, "sh8601_init() after disp->pushPixels().\n");
}

void sh8601_flush(void *_disp_drv, const void *_area, void *_color_p) {
  mp_printf(&mp_plat_print, "sh8601_flush() START free_size=%d\n",
            esp_get_free_heap_size());

  lv_display_t *disp_drv = (lv_display_t *)_disp_drv;
  // TODO: PSRAM Not OK..
  void *psram = heap_alloc_psram(512 * 1024);
  mp_printf(&mp_plat_print, "sh8601_flush() allocpsram p=%x\n", psram);
  const lv_area_t *area = (const lv_area_t *)_area;
  lv_color_t *color_p = (lv_color_t *)_color_p;

  void *driver_data = lv_display_get_driver_data(disp_drv);

  // We use disp_drv->driver_data to pass data from MP to C

  int x1 = area->x1;
  int x2 = area->x2;
  int y1 = area->y1;
  int y2 = area->y2;

  uint32_t w = x2 - x1 + 1;
  uint32_t h = y2 - y1 + 1;

  size_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
  // uint8_t color_size = 2;

  mp_printf(&mp_plat_print,
            "sh8601_flush() x1=%d,x2=%d,y1=%d,y2=%d,w=%d,h=%d,size=%d\n", x1,
            x2, y1, y2, w, h, size);

  bool swap_rgb565_bytes = mp_obj_get_int(
      mp_obj_dict_get(driver_data, MP_OBJ_NEW_QSTR(MP_QSTR_swap_rgb565_bytes)));
  mp_printf(&mp_plat_print, "sh8601_flush() swap_rgb565_bytes=%d\n",
            swap_rgb565_bytes);
  if (swap_rgb565_bytes) {
    lv_draw_sw_rgb565_swap(color_p, size);
    mp_printf(&mp_plat_print,
              "sh8601_flush() lv_draw_sw_rgb565_swap() done.\n");
  }

  disp->startWrite();
  disp->setWindow(x1, y1, x2, y2);
  uint16_t *data = (uint16_t *)color_p;
  disp->pushPixels(data, w * h, true);
  // _disp->pushPixelsDMA()
  disp->endWrite();

  // IMPORTANT!!!
  // Inform the graphics library that you are ready with the flushing.
  lv_disp_flush_ready(disp_drv);
  mp_printf(&mp_plat_print, "sh8601_flush() 2 free_size=%d\n",
            esp_get_free_heap_size());
  mp_printf(&mp_plat_print, "sh8601_flush() done.\n");
}
}
