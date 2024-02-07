#include "../include/common.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "mphalport.h"
#include "espidf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "soc/cpu.h"

#include "sh8601_qspi.h"

#include "lvgl/lvgl.h"

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} color24_t;

#define DISPLAY_TYPE_ILI9341 1
#define DISPLAY_TYPE_ILI9488 2
#define DISPLAY_TYPE_GC9A01  3
#define DISPLAY_TYPE_ST7789  4
#define DISPLAY_TYPE_ST7735  5


DMA_ATTR static uint8_t dma_buf[4] = {0};
DMA_ATTR static spi_transaction_t spi_trans = {0};


void sh8601_flush(void *_disp_drv, const void *_area, void *_color_p)
{
    lv_display_t *disp_drv = _disp_drv;
    const lv_area_t *area = _area;
    lv_color_t *color_p = _color_p;
    int start_x = 0;
    int start_y = 0;

    void *driver_data = lv_display_get_driver_data(disp_drv);

    // We use disp_drv->driver_data to pass data from MP to C
    // The following lines extract dc and spi

    int dc = mp_obj_get_int(mp_obj_dict_get(driver_data, MP_OBJ_NEW_QSTR(MP_QSTR_dc)));
    int dt = mp_obj_get_int(mp_obj_dict_get(driver_data, MP_OBJ_NEW_QSTR(MP_QSTR_dt)));
    mp_buffer_info_t buffer_info;
    mp_get_buffer_raise(mp_obj_dict_get(driver_data, MP_OBJ_NEW_QSTR(MP_QSTR_spi)), &buffer_info, MP_BUFFER_READ);
    spi_device_handle_t *spi_ptr = buffer_info.buf;

    // Some devices may need a start_x and start_y offset for displays with LCD screens smaller
    // than the devices built in frame buffer.

    start_x = mp_obj_get_int(mp_obj_dict_get(driver_data, MP_OBJ_NEW_QSTR(MP_QSTR_start_x)));
    start_y = mp_obj_get_int(mp_obj_dict_get(driver_data, MP_OBJ_NEW_QSTR(MP_QSTR_start_y)));

    int x1 = area->x1 + start_x;
    int x2 = area->x2 + start_x;
    int y1 = area->y1 + start_y;
    int y2 = area->y2 + start_y;

    // Column addresses

    // ili9xxx_send_cmd(0x2A, dc, *spi_ptr);
    dma_buf[0] = (x1 >> 8) & 0xFF;
    dma_buf[1] = x1 & 0xFF;
    dma_buf[2] = (x2 >> 8) & 0xFF;
    dma_buf[3] = x2 & 0xFF;

    // ili9xxx_send_data(dma_buf, dc, *spi_ptr);

    // Page addresses

    // ili9xxx_send_cmd(0x2B, dc, *spi_ptr);
    dma_buf[0] = (y1 >> 8) & 0xFF;
    dma_buf[1] = y1 & 0xFF;
    dma_buf[2] = (y2 >> 8) & 0xFF;
    dma_buf[3] = y2 & 0xFF;

    // ili9xxx_send_data(dma_buf, dc, *spi_ptr);

    // Memory write by DMA, disp_flush_ready when finished

    // ili9xxx_send_cmd(0x2C, dc, *spi_ptr);

    size_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    uint8_t color_size = 2;

    bool swap_rgb565_bytes = mp_obj_get_int(mp_obj_dict_get(driver_data, MP_OBJ_NEW_QSTR(MP_QSTR_swap_rgb565_bytes)));
    if ( swap_rgb565_bytes == true ) {
        lv_draw_sw_rgb565_swap(color_p, size);
    }

    if ( dt == DISPLAY_TYPE_ILI9488 ) {
        color_size = 3;
        /*Convert ARGB to RGB is required (cut off A-byte)*/
        size_t i;
        lv_color32_t* tmp32 = (lv_color32_t*) color_p;
        color24_t* tmp24 = (color24_t*) color_p;

        for(i=0; i < size; i++) {
            tmp24[i].red = tmp32[i].red;
            tmp24[i].green = tmp32[i].green;
            tmp24[i].blue = tmp32[i].blue;
        }
    }

    // ili9xxx_send_data_dma(disp_drv, color_p, size * color_size, dc, *spi_ptr);
}

