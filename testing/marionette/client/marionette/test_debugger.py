



































from marionette import Marionette, HTMLElement

if __name__ == '__main__':

    
    m = Marionette(host='localhost', port=2828)
    assert(m.start_session())
    assert(10 == m.execute_script('return 10;'))

