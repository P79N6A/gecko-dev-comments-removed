









"""This script is a tool to generate special header files from input
C source files.

It first assembles the input source files to generate intermediate assembly
files (*.s). Then it parses the .s files and finds declarations of variables
whose names start with the string specified as the third argument in the
command-line, translates the variable names and values into constant defines
and writes them into header files.
"""

import os
import sys
import subprocess
from optparse import OptionParser

def main(argv):
  parser = OptionParser()
  usage = 'Usage: %prog [options] input_filename'
  parser.set_usage(usage)
  parser.add_option('--compiler', default = 'gcc', help = 'compiler name')
  parser.add_option('--options', default = '-S', help = 'compiler options')
  parser.add_option('--pattern', default = 'offset_', help = 'A match pattern'
                    ' used for searching the relevant constants.')
  parser.add_option('--dir', default = '.', help = 'output directory')
  (options, args) = parser.parse_args()

  
  input_filename = args[0]
  output_root = (options.dir + '/' +
      os.path.splitext(os.path.basename(input_filename))[0])
  interim_filename = output_root + '.s'
  out_filename = output_root + '.h'

  
  compiler_command = (options.compiler + " " + options.options + " " +
      input_filename + " -o " + interim_filename)
  
  subprocess.check_call(compiler_command, shell=True)

  interim_file = open(interim_filename)  
  out_file = open(out_filename, 'w')  

  
  for line in interim_file:  
    if line.startswith(options.pattern):
      out_file.write('#define ')
      out_file.write(line.split(':')[0])  
      out_file.write(' ')
    if line.find('.word') >= 0:
      out_file.write(line.split('.word')[1])  

  interim_file.close()
  out_file.close()

if __name__ == "__main__":
  main(sys.argv[1:])
