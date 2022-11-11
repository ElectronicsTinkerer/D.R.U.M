from machine import Pin, Timer

bpm = 240
led = Pin(17, Pin.OUT)
tim = Timer()
def tick(timer):
    global led
    led.toggle()
    
tim.init(freq=bpm/60, mode=Timer.PERIODIC, callback=tick)