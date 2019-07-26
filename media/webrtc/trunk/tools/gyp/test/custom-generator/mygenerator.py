



"""Custom gyp generator that doesn't do much."""

import gyp.common

generator_default_variables = {}

def GenerateOutput(target_list, target_dicts, data, params):
  f = open("MyBuildFile", "wb")
  f.write("Testing...\n")
  f.close()
