




import sys

fh = open(sys.argv[1], 'w')

filenames = sys.argv[2:]

for filename in filenames:
  subfile = open(filename)
  data = subfile.read()
  subfile.close()
  fh.write(data)

fh.close()
