





from optparse import OptionParser

parser = OptionParser()
parser.add_option('-a', dest='platform')
parser.add_option('-o', dest='output')
parser.add_option('-p', dest='path')
(options, args) = parser.parse_args()

f = open(options.output, 'w')
print >>f, 'options', options
print >>f, 'args', args
f.close()
