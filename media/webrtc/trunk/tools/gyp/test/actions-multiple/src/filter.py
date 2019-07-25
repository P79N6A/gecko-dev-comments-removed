





import sys

data = open(sys.argv[3], 'r').read()
fh = open(sys.argv[4], 'w')
fh.write(data.replace(sys.argv[1], sys.argv[2]))
fh.close()
