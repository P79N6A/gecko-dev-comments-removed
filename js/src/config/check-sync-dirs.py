















import sys
import os
from os.path import join
import filecmp
import textwrap
import fnmatch

if len(sys.argv) != 3:
    print >> sys.stderr, 'TEST-UNEXPECTED-FAIL | check-sync-dirs.py | Usage: %s COPY ORIGINAL' % sys.argv[0]
    sys.exit(1)

copy = os.path.abspath(sys.argv[1])
original = os.path.abspath(sys.argv[2])


ignored_patterns = ['*~', '.#*', '#*#', '*.orig', '*.rej']





def read_exceptions(filename):
    if (os.path.exists(filename)):
        f = file(filename)
        exceptions = {}
        for line in f:
            line = line.strip()
            if line != '' and line[0] != '#':
                exceptions[line] = None
        exceptions[os.path.basename (filename)] = None
        f.close()
        return exceptions
    else:
        return {}



def fnmatch_any(filename, patterns):
    for pattern in patterns:
        if fnmatch.fnmatch(filename, pattern):
            return True
    return False





def check(copy, original, ignore):
    os.chdir(copy)
    for (dirpath, dirnames, filenames) in os.walk('.'):
        exceptions = read_exceptions(join(dirpath, 'check-sync-exceptions'))
        for filename in filenames:
            if (filename in exceptions) or fnmatch_any(filename, ignore):
                continue
            relative_name = join(dirpath, filename)
            original_name = join(original, relative_name)
            if (os.path.exists(original_name)
                and filecmp.cmp(relative_name, original_name)):
                continue
            report(copy, original, relative_name)

differences_found = False



def report(copy, original, differing):
    global differences_found
    if not differences_found:
        print >> sys.stderr, 'TEST-UNEXPECTED-FAIL | check-sync-dirs.py | build file copies are not in sync\n' \
                             'TEST-INFO | check-sync-dirs.py | file(s) found in:               %s\n' \
                             'TEST-INFO | check-sync-dirs.py | differ from their originals in: %s' \
                             % (copy, original)
    print >> sys.stderr, 'TEST-INFO | check-sync-dirs.py | differing file:                 %s' % differing
    differences_found = True

check(copy, original, ignored_patterns)

if differences_found:
    msg = '''In general, the files in '%s' should always be exact copies of
originals in '%s'.  A change made to one should also be made to the
other.  See 'check-sync-dirs.py' for more details.''' \
         % (copy, original)
    print >> sys.stderr, textwrap.fill(msg, 75)
    sys.exit(1)

print >> sys.stderr, 'TEST-PASS | check-sync-dirs.py | %s <= %s' % (copy, original)
sys.exit(0)
