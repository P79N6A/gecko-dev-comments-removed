





"""Prints the information in a sln file in a diffable way.

   It first outputs each projects in alphabetical order with their
   dependencies.

   Then it outputs a possible build order.
"""

__author__ = 'nsylvain (Nicolas Sylvain)'

import os
import re
import sys
import pretty_vcproj

def BuildProject(project, built, projects, deps):
  
  
  
  for dep in deps[project]:
    if dep not in built:
      BuildProject(dep, built, projects, deps)
  print project
  built.append(project)

def ParseSolution(solution_file):
  
  projects = dict()

  
  dependencies = dict()

  
  
  begin_project = re.compile(('^Project\("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942'
                              '}"\) = "(.*)", "(.*)", "(.*)"$'))
  
  end_project = re.compile('^EndProject$')
  
  begin_dep = re.compile('ProjectSection\(ProjectDependencies\) = postProject$')
  
  end_dep = re.compile('EndProjectSection$')
  
  dep_line = re.compile(' *({.*}) = ({.*})$')

  in_deps = False
  solution = open(solution_file)
  for line in solution:
    results = begin_project.search(line)
    if results:
      
      if results.group(1).find('icu') != -1:
        continue
      
      current_project = results.group(1).replace('_gyp', '')
      projects[current_project] = [results.group(2).replace('_gyp', ''),
                                   results.group(3),
                                   results.group(2)]
      dependencies[current_project] = []
      continue

    results = end_project.search(line)
    if results:
      current_project = None
      continue

    results = begin_dep.search(line)
    if results:
      in_deps = True
      continue

    results = end_dep.search(line)
    if results:
      in_deps = False
      continue

    results = dep_line.search(line)
    if results and in_deps and current_project:
      dependencies[current_project].append(results.group(1))
      continue

  
  for project in dependencies:
    
    new_dep_array = []
    for dep in dependencies[project]:
      
      for project_info in projects:
        if projects[project_info][1] == dep:
          new_dep_array.append(project_info)
    dependencies[project] = sorted(new_dep_array)

  return (projects, dependencies)

def PrintDependencies(projects, deps):
  print "---------------------------------------"
  print "Dependencies for all projects"
  print "---------------------------------------"
  print "--                                   --"

  for (project, dep_list) in sorted(deps.items()):
    print "Project : %s" % project
    print "Path : %s" % projects[project][0]
    if dep_list:
      for dep in dep_list:
        print "  - %s" % dep
    print ""

  print "--                                   --"

def PrintBuildOrder(projects, deps):
  print "---------------------------------------"
  print "Build order                            "
  print "---------------------------------------"
  print "--                                   --"

  built = []
  for (project, _) in sorted(deps.items()):
    if project not in built:
      BuildProject(project, built, projects, deps)

  print "--                                   --"

def PrintVCProj(projects):

  for project in projects:
    print "-------------------------------------"
    print "-------------------------------------"
    print project
    print project
    print project
    print "-------------------------------------"
    print "-------------------------------------"

    project_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[1]),
                                                projects[project][2]))

    pretty = pretty_vcproj
    argv = [ '',
             project_path,
             '$(SolutionDir)=%s\\' % os.path.dirname(sys.argv[1]),
           ]
    argv.extend(sys.argv[3:])
    pretty.main(argv)

def main():
  
  if len(sys.argv) < 2:
    print 'Usage: %s "c:\\path\\to\\project.sln"' % sys.argv[0]
    return 1

  (projects, deps) = ParseSolution(sys.argv[1])
  PrintDependencies(projects, deps)
  PrintBuildOrder(projects, deps)

  if '--recursive' in sys.argv:
    PrintVCProj(projects)
  return 0


if __name__ == '__main__':
  sys.exit(main())
