





"""Using the JSON dumped by the dump-dependency-json generator,
generate input suitable for graphviz to render a dependency graph of
targets."""

import collections
import json
import sys


def ParseTarget(target):
  target, _, suffix = target.partition('#')
  filename, _, target = target.partition(':')
  return filename, target, suffix


def LoadEdges(filename, targets):
  """Load the edges map from the dump file, and filter it to only
  show targets in |targets| and their depedendents."""

  file = open('dump.json')
  edges = json.load(file)
  file.close()

  
  target_edges = {}
  to_visit = targets[:]
  while to_visit:
    src = to_visit.pop()
    if src in target_edges:
      continue
    target_edges[src] = edges[src]
    to_visit.extend(edges[src])

  return target_edges


def WriteGraph(edges):
  """Print a graphviz graph to stdout.
  |edges| is a map of target to a list of other targets it depends on."""

  
  files = collections.defaultdict(list)
  for src, dst in edges.items():
    build_file, target_name, toolset = ParseTarget(src)
    files[build_file].append(src)

  print 'digraph D {'
  print '  fontsize=8'  
  print '  node [fontsize=8]'

  
  
  
  for filename, targets in files.items():
    if len(targets) == 1:
      
      
      target = targets[0]
      build_file, target_name, toolset = ParseTarget(target)
      print '  "%s" [shape=box, label="%s\\n%s"]' % (target, filename,
                                                     target_name)
    else:
      
      print '  subgraph "cluster_%s" {' % filename
      print '    label = "%s"' % filename
      for target in targets:
        build_file, target_name, toolset = ParseTarget(target)
        print '    "%s" [label="%s"]' % (target, target_name)
      print '  }'

  
  
  for src, dsts in edges.items():
    for dst in dsts:
      print '  "%s" -> "%s"' % (src, dst)

  print '}'


def main():
  if len(sys.argv) < 2:
    print >>sys.stderr, __doc__
    print >>sys.stderr
    print >>sys.stderr, 'usage: %s target1 target2...' % (sys.argv[0])
    return 1

  edges = LoadEdges('dump.json', sys.argv[1:])

  WriteGraph(edges)
  return 0


if __name__ == '__main__':
  sys.exit(main())
