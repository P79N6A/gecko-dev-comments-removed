





import sys

fh = open(sys.argv[-1], 'wb')
for filename in sys.argv[1:-1]:
  fh.write(open(filename).read())
fh.close()
