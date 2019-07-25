import time
from marionette import Marionette, HTMLElement











if __name__ == '__main__':
    
    driver1 = Marionette(emulator=True, port=2828)
    assert(driver1.emulator.is_running)
    assert(driver1.emulator.port)
    print 'emulator1 is running on port', driver1.emulator.port
    assert(driver1.port != 2828)
    print 'emulator1 port forwarding configured from port', driver1.port
    print 'on localhost to port 2828 on the device'
    assert(driver1.start_session())

    driver2 = Marionette(emulator=True, port=2828)
    assert(driver2.emulator.is_running)
    assert(driver2.emulator.port)
    print 'emulator2 is running on port', driver2.emulator.port
    assert(driver2.port != 2828)
    print 'emulator1 port forwarding configured from port', driver2.port
    print 'on localhost to port 2828 on the device'
    assert(driver2.start_session())

    
    assert(driver2.emulator.close() == 0)
    assert(not driver2.emulator.is_running)
    assert(driver1.emulator.close() == 0)
    assert(not driver1.emulator.is_running)


