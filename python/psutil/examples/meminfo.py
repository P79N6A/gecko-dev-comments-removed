





"""
Print system memory information.

$ python examples/meminfo.py
MEMORY
------
Total      :    9.7G
Available  :    4.9G
Percent    :    49.0
Used       :    8.2G
Free       :    1.4G
Active     :    5.6G
Inactive   :    2.1G
Buffers    :  341.2M
Cached     :    3.2G

SWAP
----
Total      :      0B
Used       :      0B
Free       :      0B
Percent    :     0.0
Sin        :      0B
Sout       :      0B
"""

import psutil
from psutil._compat import print_


def bytes2human(n):
    
    
    
    
    
    symbols = ('K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y')
    prefix = {}
    for i, s in enumerate(symbols):
        prefix[s] = 1 << (i + 1) * 10
    for s in reversed(symbols):
        if n >= prefix[s]:
            value = float(n) / prefix[s]
            return '%.1f%s' % (value, s)
    return "%sB" % n


def pprint_ntuple(nt):
    for name in nt._fields:
        value = getattr(nt, name)
        if name != 'percent':
            value = bytes2human(value)
        print_('%-10s : %7s' % (name.capitalize(), value))


def main():
    print_('MEMORY\n------')
    pprint_ntuple(psutil.virtual_memory())
    print_('\nSWAP\n----')
    pprint_ntuple(psutil.swap_memory())

if __name__ == '__main__':
    main()
