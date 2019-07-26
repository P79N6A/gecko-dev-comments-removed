



import sys

input = open(sys.argv[1], "r").read()
open(sys.argv[2], "w").write(input + "Modified.")
