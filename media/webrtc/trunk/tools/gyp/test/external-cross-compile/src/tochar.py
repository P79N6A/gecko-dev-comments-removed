




import sys

src = open(sys.argv[1])
dst = open(sys.argv[2], 'w')
for ch in src.read():
  dst.write('%d,\n' % ord(ch))
src.close()
dst.close()
