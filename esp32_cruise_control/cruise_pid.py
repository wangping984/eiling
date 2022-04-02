import numpy as np
from simple_pid import PID
import matplotlib.pyplot as plt


# Vehicle model: vehicle()
#
# To develop a mathematical model we start with a force balance for
# the car body. Let v be the speed of the car, m the total mass
# (including passengers), F the force generated by the contact of the
# wheels with the road, and Fd the disturbance force due to gravity,
# friction, and aerodynamic drag.


class car:
    A = 2.4
    m = 1600
    v = 0
    dt = 0.2

    def __init__(self, x,  params={}) -> None:
        m = params.get('m', 1600.)  # mass of car
        A = params.get('A', 2.4)  # frontal area of car
        dt = params.get('dt', 0.2)  # update rate, default 0.2s
        v = x[0]

    def force_vehicle(self, x, u, params={}):
        """Vehicle force by electric motor.

        Parameters
        ----------
        x : array
            System state: car velocity in km/h
        u : array
            System input: [throttle, gear, road_slope], where throttle is
            a float between 0 and 1, gear is an integer between 1 and 5,
            and road_slope is in rad.

        Returns
        -------
        float
            Vehicle force in N

        """
        v = x[0]                           # vehicle velocity in km/h
        throttle = np.clip(u[0], 0, 1)     # vehicle throttle
        ratedPower = params.get('ratedPower', 40e3)  # rated power of car

        if(throttle > 0.3):     # acceleration
            Power = (throttle - 0.3) / 0.7 * ratedPower
        elif(throttle < 0.3):  # regenerative brake
            if v > 50:
                Power = - 0.5*ratedPower * (0.3 - throttle)/0.3
            elif v <= 50 and v > 0:
                Power = - 0.5*ratedPower * v/50 * (0.3 - throttle)/0.3
            else:
                Power = 0
        else:
            Power = 0

        if v == 0:
            F = 0
        else:
            F = Power / (v/3.6)

        return F

    def vehicle_update(self, x, u, params={}):
        """Vehicle dynamics for cruise control system.

        Parameters
        ----------
        x : array
            System state: car velocity in km/h
        u : array
            System input: [throttle, gear, road_slope], where throttle is
            a float between 0 and 1, gear is an integer between 1 and 5,
            and road_slope is in rad.

        Returns
        -------
        float
            Vehicle acceleration

        """
        from math import copysign, sin
        def sign(x): return copysign(1, x)         # define the sign() function

        # Set up the system parameters
        m = params.get('m', 1600.)  # mass of car
        g = params.get('g', 9.8)  # gravity constant
        Cr = params.get('Cr', 0.01)  # rolling friction coef
        Cd = params.get('Cd', 0.32)  # air drag coef
        rho = params.get('rho', 1.3)  # density of air
        A = params.get('A', 2.4)  # frontal area of car
        dt = params.get('dt', 0.2)  # update rate, default 0.2s

        # Define variables for vehicle state and inputs
        v = x[0]/3.6                           # vehicle velocity km/h to m/ss

        theta = u[2]                       # road slope

        # Force generated by the engine
        F = self.force_vehicle(x, u, params)

        # Disturbance forces
        #
        # The disturbance force Fd has three major components: Fg, the forces due
        # to gravity; Fr, the forces due to rolling friction; and Fa, the
        # aerodynamic drag.

        # Letting the slope of the road be \theta (theta), gravity gives the
        # force Fg = m g sin \theta.

        Fg = m * g * sin(theta)

        # A simple model of rolling friction is Fr = m g Cr sgn(v), where Cr is
        # the coefficient of rolling friction and sgn(v) is the sign of v (+/- 1) or
        # zero if v = 0.

        Fr = m * g * Cr * sign(v)

        # The aerodynamic drag is proportional to the square of the speed: Fa =
        # 1/\rho Cd A |v| v, where \rho is the density of air, Cd is the
        # shape-dependent aerodynamic drag coefficient, and A is the frontal area
        # of the car.

        Fa = 1/2 * rho * Cd * A * abs(v) * v

        # Final acceleration on the car
        Fd = Fg + Fr + Fa
        dv = (F - Fd) / m
        v = dv*dt + v

        return v*3.6

if __name__ == '__main__':
    speed_set = 50
    speed0 = 5
    throttle0 = 0.3
    throttle = np.array([throttle0])
    speed = np.array([speed0])
    pid = PID(0.1, 0.05, 0, setpoint=speed_set)
    pid.sample_time = 0  # Update everytime when called 

    mut = car(x=[speed0], params={})
    # Assume we have a system we want to control in controlled_system
    v = mut.vehicle_update(x=[speed0], u=[throttle0, 0, 0])

    while True:
        speed = np.append(speed, v)
        # Compute new output from the PID according to the systems current value
        pedal = pid(speed[-2], dt=0.2)
        throttle = np.append(throttle, pedal)

        # Feed the PID output to the system and get its current value
        v = mut.vehicle_update(x=[v], u=[pedal, 0, 0])
        if(len(speed) >= 100):
            break

    T = np.arange(0, 100)*0.2
    plt.figure()
    plt.suptitle('Response to speed set response')
    vel_axes = plt.subplot(2, 1, 1)
    throttle_axes = plt.subplot(2, 1, 2)
    # Plot the velocity
    plt.sca(vel_axes)
    plt.ylabel('km/h')
    plt.plot(T, speed,'ro')

    # Plot the throttle
    plt.sca(throttle_axes)
    plt.plot(T, throttle - 0.3, 'b*')
    plt.xlabel('time sec')
    plt.ylabel('pedal position')
    plt.show()