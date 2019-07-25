






































"""A wrapper around pywebsocket's standalone.py which causes it to ignore
SIGINT.

"""

import signal
import sys

if __name__ == '__main__':
    sys.path = ['pywebsocket'] + sys.path
    import standalone

    signal.signal(signal.SIGINT, signal.SIG_IGN)
    standalone._main()
