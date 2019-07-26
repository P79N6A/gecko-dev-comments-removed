

"""This script takes the file produced by DMD's test mode and checks its
correctness.

It produces the following output files: $TMP/test-{fixed,filtered,diff}.dmd.

It runs the appropriate fix* script to get nice stack traces.  It also
filters out platform-specific details from the test output file.

Note: you must run this from the same directory that you invoked DMD's test
mode, otherwise the fix* script will not work properly, because some of the
paths in the test output are relative.

"""

from __future__ import print_function

import os
import platform
import re
import subprocess
import sys
import tempfile


def main():

    

    if (len(sys.argv) != 3):
        print("usage:", sys.argv[0], "<srcdir> <test-output>")
        sys.exit(1)

    srcdir = sys.argv[1]

    

    tempdir = tempfile.gettempdir()
    in_name       = sys.argv[2]
    fixed_name    = tempdir + os.sep + "test-fixed.dmd"
    filtered_name = tempdir + os.sep + "test-filtered.dmd"
    diff_name     = tempdir + os.sep + "test-diff.dmd"
    expected_name = srcdir + os.sep + \
                    "memory/replace/dmd/test-expected.dmd"

    

    print("fixing output to", fixed_name)

    sysname = platform.system()
    if sysname == "Linux":
        fix = srcdir + os.sep + "tools/rb/fix-linux-stack.pl"
    elif sysname == "Darwin":
        fix = srcdir + os.sep + "tools/rb/fix_macosx_stack.py"
    else:
        print("unhandled platform: " + sysname, file=sys.stderr)
        sys.exit(1)

    subprocess.call(fix, stdin=open(in_name, "r"),
                         stdout=open(fixed_name, "w"))

    

    
    
    
    
    
    
    
    
    
    

    print("filtering output to", filtered_name)

    with open(fixed_name, "r") as fin, \
         open(filtered_name, "w") as fout:

        test_frame_re = re.compile(r".*(RunTestMode\w*).*(DMD.cpp)")

        for line in fin:
            if re.match(r" (Allocated at|Reported( again)? at)", line):
                
                print(line, end='', file=fout)

                
                for frame in fin:
                    if re.match(r"   ", frame):
                        m = test_frame_re.match(frame)
                        if m:
                            print("   ...", m.group(1), "...", m.group(2),
                                  file=fout)
                    else:
                        
                        print(frame, end='', file=fout)
                        break

            elif re.search("in stack frame record", line):
                
                
                line2 = fin.next()
                line3 = fin.next()
                line4 = fin.next()
                frame = fin.next()
                line6 = fin.next()
                m = test_frame_re.match(frame)
                if m:
                    
                    
                    print(re.sub(r"record \d+ of \d+", "record M of N", line),
                          end='', file=fout)
                    print(line2, end='', file=fout)
                    print(line3, end='', file=fout)
                    print(line4, end='', file=fout)
                    print("   ...", m.group(1), "...", m.group(2), file=fout)
                    print(line6, end='', file=fout)

            else:
                
                print(line, end='', file=fout)

    

    print("diffing output to", diff_name)

    ret = subprocess.call(["diff", "-u", filtered_name, expected_name],
                          stdout=open(diff_name, "w"))

    if ret == 0:
        print("test PASSED")
    else:
        print("test FAILED (did you remember to run this script and Firefox "
              "in the same directory?)")


if __name__ == "__main__":
    main()
