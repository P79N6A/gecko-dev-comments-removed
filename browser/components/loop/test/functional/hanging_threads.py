import sys
import threading
import time











def monitor():
    while 1:
        time.sleep(1.)
        _ = sys._current_frames()


def start_monitoring():
    thread = threading.Thread(target=monitor)
    thread.daemon = True
    thread.start()
    return thread

start_monitoring()
