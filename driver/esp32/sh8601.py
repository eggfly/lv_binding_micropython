import machine
import espidf as esp
import lvgl as lv
import lv_utils
import micropython
import time
import uasyncio

from micropython import const

micropython.alloc_emergency_exception_buf(256)

class SH8601:
    def __init__(self, spi_host=esp.HSPI_HOST, cs=13, rst=1, sclk=7, io0=9, io1=8, io2=5, io3=6, mhz=40, double_buffer=True):
        self.asynchronous = True
        self.swap_rgb565_bytes = False
        if not lv.is_initialized():
            lv.init()
        self.width = 368
        self.height = 448
        # TODO: USE PSRAM
        self.factor = 3
        self.color_format = lv.COLOR_FORMAT.RGB565
        self.pixel_size = lv.color_format_get_size(self.color_format)
        self.buf_size = (self.width * self.height * self.pixel_size) // self.factor
        print("buf_size", self.buf_size)
        self.buf1 = esp.heap_caps_malloc(self.buf_size, esp.MALLOC_CAP.DMA)
        if not self.buf1:
            free = esp.heap_caps_get_largest_free_block(esp.MALLOC_CAP.DMA)
            raise RuntimeError(
                f"No enough memory for display buffer. Needed: {self.buf_size} bytes, largest free block: {free} bytes")
        double_buffer = False
        self.buf2 = esp.heap_caps_malloc(
            self.buf_size, esp.MALLOC_CAP.DMA) if double_buffer else None
        self.spi_host = spi_host
        self.rst = rst
        self.cs = cs
        self.sclk = sclk
        self.io0 = io0
        self.io1 = io1
        self.io2 = io2
        self.io3 = io3
        
        self.disp_drv = lv.display_create(self.width, self.height)
        self.disp_drv.set_color_format(self.color_format)
        self.disp_drv.set_buffers(
            self.buf1, self.buf2, self.buf_size, lv.DISPLAY_RENDER_MODE.PARTIAL)
        self.disp_drv.set_flush_cb(esp.sh8601_flush)
        self.disp_drv.set_driver_data({
            'swap_rgb565_bytes': self.swap_rgb565_bytes,
#             'rst': self.rst,
#             'cs': self.cs,
#             'sclk': self.sclk,
#             'io0': self.io0,
#             'io1': self.io1,
#             'io2': self.io2,
#             'io3': self.io3,
        })
        esp.sh8601_init()
        if not lv_utils.event_loop.is_running():
            print("run event_loop()")
            self.event_loop = lv_utils.event_loop(asynchronous=self.asynchronous)
        print("__init__() done")
        # pass

    def deinit(self):
        print("deinit")
        # Prevent callbacks to lvgl, which refer to the buffers we are about to delete
        if lv_utils.event_loop.is_running():
            self.event_loop.deinit()

        if self.disp_drv:
            self.disp_drv.delete()

        if self.buf1:
            esp.heap_caps_free(self.buf1)
            self.buf1 = None

        if self.buf2:
            esp.heap_caps_free(self.buf2)
            self.buf2 = None

# start
sh8601 = SH8601()

scr = lv.obj()

mystyle = lv.style_t()
# mystyle.body.main_color = lv.color_hex(0xFF0000)
# mystyle.body.grad_color = lv.color_hex(0x00FF00)
# mystyle.text.color = lv.color_hex(0x0000ff)
# scr.set_style(mystyle)
# lv.color_hex3(0x000)
scr.set_style_bg_color(lv.color_hex(0xFFCCCC), lv.PART.MAIN)


btn = lv.button(scr)
lbl = lv.label(btn)
lbl.set_text("============= \n Now U CAN'T \n PRESS ME \n Yet!!!")
btn.center()
# btn.add_event(lambda event: print('Button clicked!'), lv.EVENT.CLICKED, None)

from img_res import image_data

image = lv.image(scr)
image.align(lv.ALIGN.TOP_MID, 0, 0)
image_dsc = lv.image_dsc_t(
    {
        "header": {"w": 100, "h": 75, "cf": lv.COLOR_FORMAT.ARGB8888},
        "data_size": len(image_data),
        "data": image_data,
    }
)

image.set_src(image_dsc)

lv.screen_load(scr)
print("main done")
uasyncio.Loop.run_forever()
print("Can't reach here: run_forever")

# sh8601.deinit()
