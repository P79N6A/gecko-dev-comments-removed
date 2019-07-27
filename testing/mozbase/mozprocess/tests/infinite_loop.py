import threading
import time
import sys
import signal

if 'deadlock' in sys.argv:
    lock = threading.Lock()

    def trap(sig, frame):
        lock.acquire()

    
    lock.acquire()
    
    signal.signal(signal.SIGTERM, trap)

while 1:
    time.sleep(1)
