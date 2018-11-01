import utime
import machine


class PID:
    """
    Discrete PID control
    """

    def __init__(self, input_fun, output_fun, P=3., I=0.01, D=0.0, update_time=200):

        self.Kp = P
        self.Ki = I
        self.Kd = D

        self.current_value = 0

        self.I_value = 0
        self.P_value = 0
        self.D_value = 0

        self.I_max = 100.0
        self.I_min = 0

        self.set_point = 0.0

        self.prev_value = 0

        self.output = 0

        self.output_fun = output_fun
        self.input_fun = input_fun

        self.update_time = update_time
        self.last_update_time = utime.ticks_ms()

    def change(self, newSetPoint):
        self.set_point = newSetPoint

    def update(self, alarm):
        '''
        The alarm parameter is passed when the funciton is called
        from a Timer.Alarm object. It is unsused.
        '''

        if utime.ticks_ms()-self.last_update_time > self.update_time:
            """
            Calculate PID output value for given reference input and feedback
            """
            self.current_value = self.input_fun()
            self.error = self.set_point - self.current_value
            # print('Current: '+str(self.current_value))
            # print('SP '+str(self.set_point))

            self.P_value = self.Kp * self.error
            self.D_value = self.Kd * (self.current_value-self.prev_value)

            lapsed_time = utime.ticks_ms()-self.last_update_time
            lapsed_time /= 1000.  # convert to seconds
            self.last_update_time = utime.ticks_ms()

            self.I_value += self.error * self.Ki

            if self.I_value > self.I_max:
                self.I_value = self.I_max
            elif self.I_value < self.I_min:
                self.I_value = self.I_min

            self.output = self.P_value + self.I_value - self.D_value

            if self.output < 0:
                self.output = 0.0
            if self.output > 100:
                self.output = 100.0

            # print("Setpoint: "+str(self.set_point))
            # print("P: "+str(self.P_value))
            # print("I: "+str(self.I_value))
            # print("Output: "+str(self.output))
            # print()

            self.output_fun(self.output/100.0)

            self.last_update_time = utime.ticks_ms()
