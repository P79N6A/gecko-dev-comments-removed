



import collections
import gyp
import gyp.common
import json
import sys

generator_supports_multiple_toolsets = True

generator_wants_static_library_dependencies_adjusted = False

generator_default_variables = {
}
for dirname in ['INTERMEDIATE_DIR', 'SHARED_INTERMEDIATE_DIR', 'PRODUCT_DIR',
                'LIB_DIR', 'SHARED_LIB_DIR']:
  
  generator_default_variables[dirname] = 'dir'
for unused in ['RULE_INPUT_PATH', 'RULE_INPUT_ROOT', 'RULE_INPUT_NAME',
               'RULE_INPUT_DIRNAME', 'RULE_INPUT_EXT',
               'EXECUTABLE_PREFIX', 'EXECUTABLE_SUFFIX',
               'STATIC_LIB_PREFIX', 'STATIC_LIB_SUFFIX',
               'SHARED_LIB_PREFIX', 'SHARED_LIB_SUFFIX']:
  generator_default_variables[unused] = ''


def CalculateVariables(default_variables, params):
  generator_flags = params.get('generator_flags', {})
  for key, val in generator_flags.items():
    default_variables.setdefault(key, val)
  default_variables.setdefault('OS', gyp.common.GetFlavor(params))


def CalculateGeneratorInputInfo(params):
  """Calculate the generator specific info that gets fed to input (called by
  gyp)."""
  generator_flags = params.get('generator_flags', {})
  if generator_flags.get('adjust_static_libraries', False):
    global generator_wants_static_library_dependencies_adjusted
    generator_wants_static_library_dependencies_adjusted = True


def GenerateOutput(target_list, target_dicts, data, params):
  
  edges = {}

  
  targets_to_visit = target_list[:]

  while len(targets_to_visit) > 0:
    target = targets_to_visit.pop()
    if target in edges:
      continue
    edges[target] = []

    for dep in target_dicts[target].get('dependencies', []):
      edges[target].append(dep)
      targets_to_visit.append(dep)

  filename = 'dump.json'
  f = open(filename, 'w')
  json.dump(edges, f)
  f.close()
  print 'Wrote json to %s.' % filename
