

from optparse import OptionParser
import sys
import re

o = OptionParser()
o.add_option("--buildid", dest="buildid")
o.add_option("--version", dest="version")

(options, args) = o.parse_args()

if not options.buildid:
    print >>sys.stderr, "--buildid is required"
    sys.exit(1)

if not options.version:
    print >>sys.stderr, "--version is required"
    sys.exit(1)







buildid = open(options.buildid, 'r').read()


majorVersion = re.match(r'^(\d+)[^\d].*', options.version).group(1)

twodigityear = buildid[2:4]
month = buildid[4:6]
if month[0] == '0':
  month = month[1]
day = buildid[6:8]
if day[0] == '0':
  day = day[1]

print '%s.%s.%s' % (majorVersion + twodigityear, month, day)
