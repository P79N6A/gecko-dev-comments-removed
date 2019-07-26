









"""This script generates a C header file of offsets from an ARM assembler file.

It parses an ARM assembler generated .S file, finds declarations of variables
whose names start with the string specified as the third argument in the
command-line, translates the variable names and values into constant defines and
writes them into a header file.
"""

import sys

def usage():
  print("Usage: generate_asm_header.py " +
     "<input filename> <output filename> <variable name pattern>")
  sys.exit(1)

def main(argv):
  if len(argv) != 3:
    usage()

  infile = open(argv[0])
  outfile = open(argv[1], 'w')

  for line in infile:  
    if line.startswith(argv[2]):
      outfile.write('#define ')
      outfile.write(line.split(':')[0])  
      outfile.write(' ')

    if line.find('.word') >= 0:
      outfile.write(line.split('.word')[1])  

  infile.close()
  outfile.close()

if __name__ == "__main__":
  main(sys.argv[1:])
