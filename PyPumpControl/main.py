import pycom
from machine import Pin
from machine import SPI
from machine import I2C
from ssd1306 import SSD1306
from ssd1306 import SSD1306_I2C
from ssd1306 import SSD1306_SPI
from gfx import GFX
import machine
import time
import uos
from encoder import Encoder
import network
from pump import Pump
from gui import GUI
from button import Button
from machine import Timer


# the following is for SPI OLED. SPI pins: SCLK=P10 MOSI=P11, MISO unused in this case
dc = Pin('P7')
res = Pin('P9')
cs = Pin('P8')
oledSPI = SPI(0, mode=SPI.MASTER)
oled = SSD1306_SPI(oledSPI, dc, res, cs, width=128, height=64)


# the following is for I2C OLED. I2C pins: SDA=P9 SCL=P10
# res = Pin("P8")
# oledI2C = I2C(0)
# oledI2C.init(I2C.MASTER, baudrate=400000)
# print(oledI2C.scan())
# oled = SSD1306_I2C(128, 64, oledI2C, res, addr=0x3D, external_vcc=True)


graphics = GFX(
    oled.pixel, hline=oled.hline, vline=oled.vline, width=128, height=64
)

gui = GUI(device=oled, graphics=graphics, width=128, height=64)

pump = Pump(pin_tach='P23', pin_out='P3', interval=1000)
pump.setStrokeRate(0)

enc = Encoder(pin_x='P4', pin_y='P5',  min=-100, max=100)
btn = Button(pin_btn='P6')
enc.setScale(0.01)
enc.setMin(0)
enc.setMax(1000)

setPoint = 0
# draw border and top left title.
gui.window("gal/day")
while True:
    # draw the big numbers
    gui.bigSensor(1, pump.getStrokeRate)
    # gui.bigSensor(1, lambda: 1)
    # while True:
    #     pass

    if (btn.presses() == 1):
        btn.reset()
        # the chrono object helps us time out the "set" menu in case user forgot to click.
        chrono = Timer.Chrono()
        chrono.start()
        chrono.reset()
        # the change flag prevents the timeout if it is true
        change = False
        while True:
            temp = gui.encoderInput(2, "Set", enc.position)
            change = True if setPoint != temp else False
            setPoint = temp
            # this is when we exit the loop
            if btn.presses() == 1 or (chrono.read() > 5 and not change):
                btn.reset()
                pump.setStrokeRate(setPoint)
                gui.clearIndex(2)
                gui.clearLabel(2)
                break
            if change:
                chrono.reset()
