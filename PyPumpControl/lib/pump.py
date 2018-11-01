from PID import PID
from machine import Timer
from machine import Pin
from machine import DAC
from machine import PWM

'''
The Pump class handles PID, tachometer, and control output.
Keyword arguments are self explanatory. and

ISSUE: DAC and PWM output have a small DC offset
'''


class Pump:
    def __init__(self, pin_tach='P23', pin_out='P3', kp=10.0, ki=30.5, kd=6, interval=200, ticks_stroke=527.0, cc_stroke=1):
        self.tach = Pin(pin_tach, mode=Pin.IN, pull=Pin.PULL_DOWN)
        # comment pwm lines and uncomment dac line to use dac, (pin_out must be P22 or P21) also check _outStrokeRate()
        # self.dac = DAC(pin_out)
        self.pwm = PWM(0, frequency=78000)
        self.pwm_c = self.pwm.channel(0, pin=pin_out, duty_cycle=0)
        self.interval = interval
        self.ticks_stroke = ticks_stroke
        self.stroke_rate = 0
        self.cc_stroke = cc_stroke

        # init tachometer interrupt
        self.tach_count = 0
        self._setTachCallback(self._tachCallback)

        # Internal PID Object
        self.pid = PID(
            self._updateStrokeRate, self._outStrokeRate, P=kp, I=ki, D=kd, update_time=self.interval
        )

        # This will make the PID loop update itself in the background so pump control is not lost.
        self.pidAlarm = Timer.Alarm(
            handler=self.pid.update, us=interval, arg=None, periodic=True
        )

    # tack just increments. Speed is updated when PID
    def _tachCallback(self, line):
        self.tach_count += 1

    def _setTachCallback(self, tachCallback=None):
        self.irqTach = self.tach.callback(
            trigger=Pin.IRQ_RISING, handler=tachCallback
        )

    # used by PID object to calculate strokes/sec
    def _updateStrokeRate(self):
        strokeRate = (1000.0 / self.interval) * \
            (1 / self.ticks_stroke) * self.tach_count
        self.tach_count = 0
        return strokeRate

    # used internally by PID object to control pump
    def _outStrokeRate(self, value):
        # self.dac.write(value)
        self.pwm_c.duty_cycle(value)

    # used externally to set a new gal/day
    def setStrokeRate(self, value):
        # convert gal/day to stroke/sec
        cc_sec = 0.0438126364 * value
        stroke_sec = cc_sec * (1/self.cc_stroke)
        self.pid.change(stroke_sec)

    # used externally to get gals/day
    def getStrokeRate(self):
        # convert stroke/sec to gal/day
        cc_sec = self.cc_stroke * self.pid.current_value
        gal_day = 22.8245
        return gal_day * cc_sec
