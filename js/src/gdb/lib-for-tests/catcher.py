










import sys
import traceback
try:
    execfile(sys.argv.pop(0))
except Exception as err:
    sys.stderr.write('Error running GDB prologue:\n')
    traceback.print_exc()
    sys.exit(1)
