import serial
import time
import pyaudio
import numpy as np
import struct
from scipy.fft import fft
import pyqtgraph as pg
import math

import RPi.GPIO as GPIO
PI_IS_REQUESTING_PIN = 40
ARDUINO_IS_READY_PIN = 38
GPIO.setmode(GPIO.BOARD)
GPIO.setup(PI_IS_REQUESTING_PIN, GPIO.OUT)
GPIO.setup(ARDUINO_IS_READY_PIN, GPIO.IN)

CHUNK = 2048

RATE = 44100

MAX = .4
MIN = 0.15


ser = serial.Serial(port="/dev/ttyACM0", baudrate=115200, timeout=5)
print("Connected over " + ser.name)
time.sleep(1)

p = pyaudio.PyAudio()
stream = p.open(format=pyaudio.paInt16,
                channels=1,
                rate=RATE,
                input=True,
                frames_per_buffer=CHUNK,
                input_device_index=3)

pw = pg.plot()
pw.setRange(yRange=[0, .5]);

def sendData(data):
    GPIO.output(PI_IS_REQUESTING_PIN, GPIO.HIGH)
    startTime = time.time() * 1000.0
    while (GPIO.input(ARDUINO_IS_READY_PIN) == 0) and (time.time() * 1000.0 - startTime < 50): pass
    if GPIO.input(ARDUINO_IS_READY_PIN == 1):
        GPIO.output(PI_IS_REQUESTING_PIN, GPIO.LOW)
        ser.write(data)
        return True
    else:
        print("Timeout waiting for arduino ack")
        GPIO.output(PI_IS_REQUESTING_PIN, GPIO.LOW)
        return False

while True:
    raw_sound = stream.read(CHUNK)
    data_int = struct.unpack(str(2 * CHUNK) + 'B', raw_sound)
    data_np = np.array(data_int, dtype='b')[::2] + 128

    fft_data = (np.abs(fft(data_int)[0:CHUNK]) / (128 * CHUNK))[:90]
    fft_data[0] = 0

    for data in range(len(fft_data)):
        if fft_data[data] <= MIN: fft_data[data] = 0

    pw.plot(np.arange(90), fft_data, clear=True)
    pg.QtGui.QApplication.processEvents()

   #maximum = np.where(fft_data == np.max(fft_data))[0][0]

    #print(maximum)

    #if maximum > 255:
        #maximum = 255

    if sendData(bytes([0, 17, 0, 0])): sendData(bytes(fft_data))