






































"""A wrapper around pywebsocket's standalone.py which causes it to ignore
SIGINT.

"""

import signal
import sys

if __name__ == '__main__':
    sys.path = ['pywebsocket'] + sys.path
    import standalone

    
    
    
    if len(sys.argv) >= 2 and sys.argv[1] == '--interactive':
        del sys.argv[1]
        signal.signal(signal.SIGINT, signal.SIG_IGN)
    else:
        signal.signal(signal.SIGINT, lambda signum, frame: sys.exit(1))

    standalone._main()
