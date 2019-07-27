




'''Prints the given arguments in sorted order with duplicates removed.'''

import sys

print(' '.join(sorted(set(sys.argv[1:]))))
