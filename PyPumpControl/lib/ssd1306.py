# MicroPython SSD1306 OLED driver, I2C and SPI interfaces

from micropython import const
import time
import framebuf


# register definitions
SET_CONTRAST = const(0x81)
SET_ENTIRE_ON = const(0xa4)
SET_NORM_INV = const(0xa6)
SET_DISP = const(0xae)
SET_MEM_ADDR = const(0x20)
SET_COL_ADDR = const(0x21)
SET_PAGE_ADDR = const(0x22)
SET_DISP_START_LINE = const(0x40)
SET_SEG_REMAP = const(0xa0)
SET_MUX_RATIO = const(0xa8)
SET_COM_OUT_DIR = const(0xc0)
SET_DISP_OFFSET = const(0xd3)
SET_COM_PIN_CFG = const(0xda)
SET_DISP_CLK_DIV = const(0xd5)
SET_PRECHARGE = const(0xd9)
SET_VCOM_DESEL = const(0xdb)
SET_CHARGE_PUMP = const(0x8d)

'''
oled
import the class corresponding to the interface you want to use. 

'''


class SSD1306:
    def __init__(self, width, height, external_vcc):
        self.width = width
        self.height = height
        self.external_vcc = external_vcc
        self.pages = self.height // 8
        self.buffer = bytearray(self.pages * self.width)
        self.framebuf = framebuf.FrameBuffer1(
            self.buffer, self.width, self.height)
        self.poweron()
        self.init_display()

    def init_display(self):
        for cmd in (
            SET_DISP | 0x00,  # off
            # address setting
            SET_MEM_ADDR, 0x00,  # horizontal
            # resolution and layout
            SET_DISP_START_LINE | 0x00,
            SET_SEG_REMAP | 0x01,  # column addr 127 mapped to SEG0
            SET_MUX_RATIO, self.height - 1,
            SET_COM_OUT_DIR | 0x08,  # scan from COM[N] to COM0
            SET_DISP_OFFSET, 0x00,
            SET_COM_PIN_CFG, 0x02 if self.height == 32 else 0x12,
            # timing and driving scheme
            SET_DISP_CLK_DIV, 0x80,
            SET_PRECHARGE, 0x22 if self.external_vcc else 0xf1,
            SET_VCOM_DESEL, 0x30,  # 0.83*Vcc
            # display
            SET_CONTRAST, 0xff,  # maximum
            SET_ENTIRE_ON,  # output follows RAM contents
            SET_NORM_INV,  # not inverted
            # charge pump
            SET_CHARGE_PUMP, 0x10 if self.external_vcc else 0x14,
                SET_DISP | 0x01):  # on
            self.write_cmd(cmd)
        self.fill(0)
        self.show()

    def poweroff(self):
        self.write_cmd(SET_DISP | 0x00)

    def contrast(self, contrast):
        self.write_cmd(SET_CONTRAST)
        self.write_cmd(contrast)

    def invert(self, invert):
        self.write_cmd(SET_NORM_INV | (invert & 1))

    def show(self):
        x0 = 0
        x1 = self.width - 1
        if self.width == 64:
            # displays with width of 64 pixels are shifted by 32
            x0 += 32
            x1 += 32
        self.write_cmd(SET_COL_ADDR)
        self.write_cmd(x0)
        self.write_cmd(x1)
        self.write_cmd(SET_PAGE_ADDR)
        self.write_cmd(0)
        self.write_cmd(self.pages - 1)
        self.write_data(self.buffer)

    # all of these are device independent. The framebuf object is given to us by uPy
    def fill(self, col):
        self.framebuf.fill(col)

    def pixel(self, x, y, col):
        self.framebuf.pixel(x, y, col)

    def hline(self, x, y, w, col):
        self.framebuf.hline(x, y, w, col)

    def vline(self, x, y, h, col):
        self.framebuf.vline(x, y, h, col)

    def scroll(self, dx, dy):
        self.framebuf.scroll(dx, dy)

    def line(self, x1, y1, x2, y2, col):
        self.framebuf.line(x1, y1, x2, y2, col)

    def text(self, string, x, y, col=1):
        self.framebuf.text(string, x, y, col)


class SSD1306_I2C(SSD1306):
    def __init__(self, width, height, i2c, res, addr=0x3D, external_vcc=False):
        self.i2c = i2c
        self.addr = addr
        self.temp = bytearray(2)
        self.res = res
        super().__init__(width, height, external_vcc)

    '''
    I could not figure out write_cmd and write_data for I2C. There's conflicting/unclear
    information about Co and D/C bits in the data sheets and the order of xmission
    
    Good news: The adafruit panel is discovered on the I2C bus.
    Bad news: The chinese panel is not discovered on the I2C bus.

    The original methods are commented below. 
    The issue with them is that Pycom's micropython does not provide the primitive I2C methods:
        i2c.start() i2c.stop() i2c.write()

    '''

    def write_cmd(self, cmd):
        self.temp[0] = 0x80  # Co=1, D/C#=0
        self.temp[1] = cmd
        self.i2c.writeto(self.addr, self.temp)
        # self.i2c.writeto_mem(self.addr, 0x80, cmd)

    def write_data(self, buf):
        # self.temp[1] = 0x40  # Co=0, D/C#=1
        t1 = bytearray(1)
        t1[0] = 0x40
        self.i2c.writeto(self.addr, t1 + buf)
        # self.i2c.writeto_mem(self.addr, 0x40, buf)
    '''
        def write_cmd(self, cmd):
            self.temp[0] = 0x80  # Co=1, D/C#=0
            self.temp[1] = cmd
            self.i2c.writeto(self.addr, self.temp)

        def write_data(self, buf):
            self.temp[0] = self.addr << 1
            self.temp[1] = 0x40  # Co=0, D/C#=1
            self.i2c.start()
            self.i2c.write(self.temp)
            self.i2c.write(buf)
            self.i2c.stop()
    '''

    def poweron(self):
        self.res.value(1)
        time.sleep_ms(1)
        self.res.value(0)
        time.sleep_ms(10)
        self.res.value(1)


class SSD1306_SPI(SSD1306):
    def __init__(self, spi, dc, res, cs, width=128, height=64, external_vcc=False):
        self.rate = 10 * 1024 * 1024
        dc.init(dc.OUT, value=0)
        res.init(res.OUT, value=0)
        cs.init(cs.OUT, value=1)
        self.spi = spi
        self.dc = dc
        self.res = res
        self.cs = cs
        super().__init__(width, height, external_vcc)

    def write_cmd(self, cmd):
        self.spi.init(baudrate=self.rate, polarity=0, phase=0)
        self.cs.value(1)
        self.dc.value(0)
        self.cs.value(0)
        self.spi.write(bytearray([cmd]))
        self.cs.value(1)

    def write_data(self, buf):
        self.spi.init(baudrate=self.rate, polarity=0, phase=0)
        self.cs.value(1)
        self.dc.value(1)
        self.cs.value(0)
        self.spi.write(buf)
        self.cs.value(1)

    def poweron(self):
        self.res.value(1)
        time.sleep_ms(1)
        self.res.value(0)
        time.sleep_ms(10)
        self.res.value(1)
