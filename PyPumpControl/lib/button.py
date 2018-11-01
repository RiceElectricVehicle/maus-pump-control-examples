from machine import Pin
from utime import sleep_ms
from machine import reset

'''
Simple class for a button. Is software debounced.
If button is held for a long time, will reset the pyboard
'''


class Button(object):
    def __init__(self, pin_btn='P6', pin_mode=Pin.PULL_UP):
        self.pin_btn = (
            pin_btn if isinstance(pin_btn, Pin)
            else Pin(pin_btn, mode=Pin.IN, pull=pin_mode)
        )
        self._presses = 0
        self.callback(Pin.IRQ_FALLING, self.handler)

    def callback(self, type, func):
        self.irq_btn = self.pin_btn.callback(
            trigger=type, handler=func
        )

    def handler(self, line):
        cur_val = self.pin_btn.value()
        active = 0
        reset_cnt = 0
        while active < 20:
            if self.pin_btn.value() != cur_val:
                active += 1
            else:
                active = 0
                reset_cnt += 1

        if (reset_cnt >= 400000):
            print("#### RESETTING ####")
            reset()
        print("####################### " + str(reset_cnt))

        self._presses += 1

    def presses(self):
        return self._presses

    def reset(self):
        self._presses = 0
