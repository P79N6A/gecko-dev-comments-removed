





import sys
import time

output = sys.argv[1]
persistoutput = "%s.persist" % sys.argv[1]

count = 0
try:
  count = open(persistoutput, 'r').read()
except:
  pass
count = int(count) + 1

if len(sys.argv) > 2:
  max_count = int(sys.argv[2])
  if count > max_count:
    count = max_count

oldcount = 0
try:
  oldcount = open(output, 'r').read()
except:
  pass







open(persistoutput, 'w').write('%d' % (count))


if int(oldcount) != count:
  open(output, 'w').write('%d' % (count))
  
  
  time.sleep(1)

sys.exit(0)
