import os
import serial
import matplotlib.pyplot as plt
#import io
import time
import numpy as np
from scipy.optimize import curve_fit
import re
#import threading


def fit_func(x, a, k, c, d):
    return a*np.exp(-(x-d)*k) + c


def fit_func_ambient(x, a, k):
    return a*np.exp(-(x)*k) + T_ambient


def lin_fit(time, temperatures):
    """Use with fit_func_ambient"""
    x = time
    y = np.log(temperatures - T_ambient)
#    n = len(time)
    x_average = np.average(x)
    y_average = np.average(y)
#    b = (np.sum(x*y) - n*x_average*y_average)/(np.sum(x**2) - n*x_average**2)
    b = np.sum((x - x_average)*y)/np.sum((x - x_average)**2)
    a = y_average - b*x_average
    
    # convert
    A = np.exp(a)
    k = -b
    
    return A, k


def expofit(time, temperatures):
    xinit = 7.85, 1e-2, 20, 40
    bounds = ([0,0,0,0], [np.inf, np.inf, np.inf, np.inf])
    time = np.array(time)
    temperatures = np.array(temperatures)
    popt = curve_fit(fit_func, time, temperatures, p0=xinit, bounds=bounds)
    
    return popt


def expofit_ambient(time, temperatures):
    xinit = 7.85, 1e-2
    bounds = ([0,0], [np.inf, np.inf])
    time = np.array(time)
    temperatures = np.array(temperatures)
    popt = curve_fit(fit_func_ambient, time, temperatures, p0=xinit, bounds=bounds)
    
    return popt


def linfit(time, temperatures):
    time = np.array(time)
    temperatures = np.array(temperatures)
    popt = lin_fit(time, temperatures)
    
    return popt


def plot_data(t, T, times, data, T_trink):
    """Create the plot"""
    try:
#        print('fit')
        # popt, pcov = expofit(times, data)
#        popt2, pcov2 = expofit_ambient(times, data)
        popt3 = linfit(times, data)
        # print(popt)       
#        a, k, c, d = popt
        a, k = popt3
#        print(popt3)
#        if c < T_trink:
#            time_trink = d - np.log((T_trink - c)/a)/k
        time_trink = np.log(a/(T_trink - T_ambient))/k
        wait_time = (time_trink - times[-1])/60 # in minutes
        if wait_time < 60: # less than an hour
            if wait_time < 1:
                mesg = 'Trinkbar in {:} Sekunden'.format(int(wait_time*60))
            else:
                mesg = 'Trinkbar in {:} Minuten'.format(int(wait_time))
#            print("plot")
#            x = np.linspace(times[0], times[-1]+60, 100) # + one minute
            x = np.linspace(t[0], t[-1], 100)
            plt.clf()
            plt.ylim(30, 95)
            plt.plot(t, T, '--', color='grey')
            plt.plot(times, data, '.')
            # plt.plot(x, fit_func(x, *list(popt)), label='Normal')
#            plt.plot(x, fit_func_ambient(x, *list(popt2)), label='Ambient')
            plt.plot(x, fit_func_ambient(x, *list(popt3)), label='Linear')
            plt.plot([t[0], t[-1]], [T_trink, T_trink], '--', color='grey')
            plt.legend(loc='upper right')
            plt.title(mesg, fontsize=14)
            plt.annotate("{}".format(data[-1]), xy=(times[-1], data[-1]), xycoords='data', xytext=(5, 2), textcoords='offset points')
        plt.xlabel("t [s]")
        plt.ylabel("T [$^{\circ}$C]")
    except Exception as e:
        print(e)
    finally:
        plt.pause(0.0001)
#        plt.ion()


def temperature_logger():
    try:
        arduino = serial.Serial("Com3", timeout=15)
        # sio = io.TextIOWrapper(io.BufferedRWPair(arduino, arduino))
    except Exception as e:
        print("Please check port")
        print(e)
    print('Begin plotting')
    data = []    
    times = []
    t_zero = time.time() # Starting time
#    delta_t = 0.5 # interval in seconds
    duration = 3*60*60 # 60 minutes
    T_trink = 60 # Target temperature
    try:
        while time.time() - t_zero < duration:
#            received = arduino.readline()
#            if re.fullmatch(r"\d{2}\.\d{2}", received) is None:
#                continue
            data.append(float(arduino.readline()))
            new_time = time.time()-t_zero
            times.append(new_time)
            plt.figure(0)
            plt.ion()
            plt.clf()
            plt.plot(times, data, '.')
            if len(times) < 4:         
                continue
            plot_data(times, data, T_trink)
    except Exception as e:        
        print(e)       
    finally:
        arduino.close()
        outfile = os.path.join(os.getcwd(), 'test_data4.npz')
        np.savez(outfile, time=times, temp=data)
       
        
def receive_state():
    try:
        arduino = serial.Serial("Com3", timeout=15)
        # sio = io.TextIOWrapper(io.BufferedRWPair(arduino, arduino))
    except Exception as e:
        print("Please check port")
        print(e)
    try:
        plt.ion()
        arraysize = 50
        T_trink = 60
        T_cold = 40
        while(True):
            received = arduino.readline()
            data = [float(rec) for rec in received.split(b',')]
            if len(data) < arraysize+4:
                continue # data not complete
            temperatures = np.array(data[:arraysize])
            times = np.array(data[arraysize:2*arraysize])
            A = data[-4]
            k = data[-3]
            time_trink = data[-2]
            time_cold = data[-1]
            plt.figure(1)
            plt.clf()
            plt.plot(times, temperatures)
            A1, k1 = lin_fit(times, temperatures)
            time_trink1 = np.log(A1/(T_trink - T_ambient))/k1
            time_cold1 = np.log(A1/(T_cold - T_ambient))/k1
            t_plot = np.linspace(0, 3000)
            wait_time = (time_trink1 - times[-1])/60 # in minutes
            if wait_time < 60: # less than an hour
                if wait_time < 1:
                    mesg = 'Trinkbar in {:} Sekunden'.format(int(wait_time*60))
                else:
                    mesg = 'Trinkbar in {:} Minuten'.format(int(wait_time))
                plt.title(mesg, fontsize=14)
            plt.plot(t_plot, fit_func_ambient(t_plot, A, k), label='Arduino')
            plt.plot(t_plot, fit_func_ambient(t_plot, A1, k1), label= 'Python')
            plt.plot([time_trink, time_trink], [30, 70], label = 'Arduino')
            plt.plot([time_trink1, time_trink1], [30, 70], label= 'Python')
            plt.plot([time_cold, time_cold], [30, 70], label = 'Arduino')
            plt.plot([time_cold1, time_cold1], [30, 70], label= 'Python')
            plt.legend(loc='best')
            plt.pause(0.001)
    finally:
        arduino.close() 
    
        
def simulation(t, T):
    t = t[::5] # lower sampling rate
    T = T[::5]
    plt.figure(0)
    plt.ion()
    time_deltas = np.diff(t)
    T_trink = 60 # Target temperature
    counter = 0
    while counter < len(t)-1:
        datalength = 40
        low = counter-datalength if counter > datalength else 0
        times = t[low:counter] # acquire data
        data = T[low:counter]
        dt = time_deltas[counter]
        counter += 1
        if len(times) < 4:         # skip if not enough data
            continue
        plot_data(t, T, times, data, T_trink)
        time.sleep(dt/40)
#        plt.pause(0.0001)
        
        
        
if __name__ == "__main__":
    T_ambient = 28.4 # Ambient temperature
    infile = os.path.join(os.getcwd(), 'test_data2.npz')
    file = np.load(infile)
    t = file['time']
    T = file['temp']
    simulation(t, T)  
