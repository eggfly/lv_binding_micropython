
import lvgl as lv
from machine import I2C, Pin

class ft3168:
    # [52, 56, 81, 104, 119]
    # 0x51 -> PCF8563
    # 0x34 -> 
    # 0x38 -> FT3168
    # 0x68 MPU
    # 0x77 (0x76) BMP280
    # ['0x34', 0x38, '0x51', '0x68', '0x77']
    def __init__(self, i2c_dev=1, sda=11, scl=10, freq=100000, addr=0x38, width=-1, height=-1, 
                 inv_x=False, inv_y=False, swap_xy=False):
        self.width, self.height = width, height
        self.inv_x, self.inv_y, self.swap_xy = inv_x, inv_y, swap_xy
        self.i2c = I2C(i2c_dev, sda=Pin(sda), scl=Pin(scl), freq=freq)
        self.addr = addr
        # 关闭 monitor mode, always I2C on!
        self.i2c.writeto_mem(self.addr, 0x86, b'\x00')
        print(self.i2c.readfrom_mem(self.addr, 0x86, 1))
        # 触摸阈值
        self.i2c.writeto_mem(self.addr, 0x80, b'\x01')
        print(self.i2c.readfrom_mem(self.addr, 0x80, 1))
        # ID_G_PERIODACTIVE -> sampling rate
        self.i2c.writeto_mem(self.addr, 0x88, b'\x14')
        print(self.i2c.readfrom_mem(self.addr, 0x88, 1))
        try:
            print("FT3168 touch IC ready (fw id 0x{0:X} rel {1:d}, lib {2:X})".format( \
                int.from_bytes(self.i2c.readfrom_mem(self.addr, 0xA6, 1), "big"), \
                int.from_bytes(self.i2c.readfrom_mem(self.addr, 0xAF, 1), "big"), \
                int.from_bytes(self.i2c.readfrom_mem(self.addr, 0xA1, 2), "big") \
            ))
        except Exception as e: 
            import sys
            sys.print_exception(e)
            print("FT3168 touch IC not responding")
            return
    def init_for_lvgl(self):
        if not lv.is_initialized():
            lv.init()
        self.point = lv.point_t( {'x': 0, 'y': 0} )
        self.points = [lv.point_t( {'x': 0, 'y': 0} ), lv.point_t( {'x': 0, 'y': 0} )]
        self.state = lv.INDEV_STATE.RELEASED
        self.indev_drv = lv.indev_create()
        self.indev_drv.set_type(lv.INDEV_TYPE.POINTER)
        self.indev_drv.set_read_cb(self.callback)
    def get_point(self):
        sensorbytes = self.i2c.readfrom_mem(self.addr, 0x00, 7)
        # print(sensorbytes)
        event = (sensorbytes[3] >> 6) & 0x03;
        x = ((sensorbytes[3] & 0x0F) << 8) | sensorbytes[4]
        y = ((sensorbytes[5] & 0x0F) << 8) | sensorbytes[6]
        if (self.width != -1 and x >= self.width) or (self.height != -1 and y >= self.height):
            raise ValueError
        # x = self.width - x - 1 if self.inv_x else x
        # y = self.height - y - 1 if self.inv_y else y
        # (x, y) = (y, x) if self.swap_xy else (x, y)
        print(event, x, y)
        return { 'x': x, 'y': y }
    def callback(self, driver, data):
        print("callback")
        def get_point():
            print("get_point()")
            print(sensorbytes)
            event = (sensorbytes[3] >> 6) & 0x03;
            x = ((sensorbytes[3] & 0x0F) << 8) | sensorbytes[4]
            y = ((sensorbytes[5] & 0x0F) << 8) | sensorbytes[6]
            if (self.width != -1 and x >= self.width) or (self.height != -1 and y >= self.height):
                raise ValueError
            # x = self.width - x - 1 if self.inv_x else x
            # y = self.height - y - 1 if self.inv_y else y
            # (x, y) = (y, x) if self.swap_xy else (x, y)
            print(event, x, y)
            return { 'x': x, 'y': y }

        data.point = self.points[0]
        data.state = self.state
        sensorbytes = self.i2c.readfrom_mem(self.addr, 0x00, 7)
        self.presses = sensorbytes[2]
        if self.presses > 2:
            return
        try:
            if self.presses:
                self.points[0] = get_point()
            if self.presses == 2:
                self.points[1] = get_point()
        except ValueError:
            return
        # if sensorbytes[3] >> 4:
        #    self.points[0], self.points[1] = self.points[1], self.points[0]
        data.point = self.points[0]
        data.state = self.state = lv.INDEV_STATE.PRESSED if self.presses else lv.INDEV_STATE.RELEASED
        # print("data", data)
if __name__ == '__main__':
    import time
    tp = ft3168()
    while True:
        time.sleep(0.5)
        tp.get_point()
