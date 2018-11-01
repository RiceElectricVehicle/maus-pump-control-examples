from gfx import GFX
import ssd1306
from machine import Pin
from machine import SPI
from machine import I2C
from writer import Writer
import freesans20


'''
The GUI class takes in a device object and a graphics object. 
The device object handles the display driver, it handles text, clearing the screen, updating the screen, etc.
The graphics object handles lines, shapes, etc. See main.py for an example 
Sensors are center justified while labels are left justified.

'''


class GUI:
    def __init__(self, device=None, graphics=None, width=128, height=64):
        self.width = width
        self.height = height
        self.device = device
        self.graphics = graphics
        # the writer object handles different text sizes. It is not guaranteed to work with every display.
        # if you're having errors, better to comment this out and bigSensor() as well.
        self.writer = Writer(self.device, freesans20)
        self.writer.set_clip(True, True)
        # indices is what we call lines. on a 126x64 we can fit 2 indices with this spacing.
        # changing this might break the sensor and label functions:
        # since they wipe a space bigger than what they're drawing you might get some overlap.
        self.indexSpacing = 22

    def show(self):
        self.device.show()

    def clear(self):
        self.device.fill(0)
        self.show()

    # draws a border around the screen with a title label at the top left.
    def window(self, title):
        self.clear()
        self.graphics.rect(0, 0, self.width, self.height, 1)
        self.graphics.fill_rect(0, 0, (len(title)+1)*8, 12, 1)
        self.device.text(title, 2, 2, 0)
        self.show()

    # clears a label at an index
    def clearLabel(self, index):
        self.graphics.fill_rect(
            2, self.indexSpacing*index, 30, 15, 0
        )

    # clears an index
    def clearIndex(self, index):
        self.graphics.fill_rect(
            30, self.indexSpacing*index, self.width-31, 18, 0
        )

    # everytime this is called, a sensor value is drawn to the corresponding index.
    def sensor(self, index, val_func):
        value = "{:.2f}".format(val_func())
        self.clearIndex(index)
        self.device.text(
            value,
            int((self.width / 2) - len(value) * 4), self.indexSpacing*index, 1
        )
        self.show()

    # Not guaranteed portable, draws a bigger sensor value
    def bigSensor(self, index, val_func):
        value = "{:.2f}".format(val_func())
        self.clearIndex(index)
        self.writer.set_textpos(
            self.indexSpacing*index, int((self.width / 2) - len(value)*4)
        )
        self.writer.printstring(value)
        self.show()

    # draws a label to the right of sensors
    def label(self, index, label):
        self.device.text(label, 2, self.indexSpacing*index, 1)

    # draws a sensor and a label.
    def sensorLabel(self, index, label, val_func):
        self.label(index, label)
        self.sensor(index, val_func)

    # draws a number on the screen that changes with val_func().
    # returns the value of val_func(). usefull for handling user input with an encoder
    def encoderInput(self, index, label, val_func):
        if index < 0:
            return val_func()
        else:
            self.label(index, label)
            value = val_func()
            self.sensor(index, lambda: value)
            return value
