










import os
import sys
import traceback
try:
    
    
    execfile(os.path.join(testlibdir, 'prologue.py'))
except Exception as err:
    sys.stderr.write('Error running GDB prologue:\n')
    traceback.print_exc()
    sys.exit(1)
