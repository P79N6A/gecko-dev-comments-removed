



import expandlibs_exec
import sys
import threading
import time

from win32 import procmem

def measure_vsize_threadfunc(proc, output_file):
    """
    Measure the virtual memory usage of |proc| at regular intervals
    until it exits, then print the maximum value and write it to
    |output_file|.  Also, print something to the console every
    half an hour to prevent the build job from getting killed when
    linking a large PGOed binary.
    """
    maxvsize = 0
    idleTime = 0
    while proc.returncode is None:
        maxvsize, vsize = procmem.get_vmsize(proc._handle)
        time.sleep(0.5)
        idleTime += 0.5
        if idleTime > 30 * 60:
          print "Still linking, 30 minutes passed..."
          sys.stdout.flush()
          idleTime = 0
    print "TinderboxPrint: linker max vsize: %d" % maxvsize
    with open(output_file, "w") as f:
        f.write("%d\n" % maxvsize)

def measure_link_vsize(output_file, args):
    """
    Execute |args|, and measure the maximum virtual memory usage of the process,
    printing it to stdout when finished.
    """

    
    
    t = [None]
    def callback(proc):
        t[0] = threading.Thread(target=measure_vsize_threadfunc,
                             args=(proc, output_file))
        t[0].start()
    exitcode = expandlibs_exec.main(args, proc_callback=callback)
    
    t[0].join()
    return exitcode

if __name__ == "__main__":
    if sys.platform != "win32":
        print >>sys.stderr, "link.py is only for use on Windows!"
        sys.exit(1)
    if len(sys.argv) < 3:
        print >>sys.stderr, "Usage: link.py <output filename> <commandline>"
        sys.exit(1)
    output_file = sys.argv.pop(1)
    sys.exit(measure_link_vsize(output_file, sys.argv[1:]))
