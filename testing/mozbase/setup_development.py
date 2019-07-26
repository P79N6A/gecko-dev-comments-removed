





"""
Setup mozbase packages for development.

Packages may be specified as command line arguments.
If no arguments are given, install all packages.

See https://wiki.mozilla.org/Auto-tools/Projects/MozBase
"""

import pkg_resources
import os
import subprocess
import sys
from optparse import OptionParser
from subprocess import PIPE
try:
    from subprocess import check_call as call
except ImportError:
    from subprocess import call



here = os.path.dirname(os.path.abspath(__file__))


mozbase_packages = [i for i in os.listdir(here)
                    if os.path.exists(os.path.join(here, i, 'setup.py'))]
extra_packages = ["sphinx"]

def cycle_check(order, dependencies):
    """ensure no cyclic dependencies"""
    order_dict = dict([(j, i) for i, j in enumerate(order)])
    for package, deps in dependencies.items():
        index = order_dict[package]
        for d in deps:
            assert index > order_dict[d], "Cyclic dependencies detected"

def info(directory):
    "get the package setup.py information"

    assert os.path.exists(os.path.join(directory, 'setup.py'))

    
    try:
        call([sys.executable, 'setup.py', 'egg_info'], cwd=directory, stdout=PIPE)
    except subprocess.CalledProcessError:
        print "Error running setup.py in %s" % directory
        raise

    
    egg_info = [entry for entry in os.listdir(directory)
                if entry.endswith('.egg-info')]
    assert len(egg_info) == 1, 'Expected one .egg-info directory in %s, got: %s' % (directory, egg_info)
    egg_info = os.path.join(directory, egg_info[0])
    assert os.path.isdir(egg_info), "%s is not a directory" % egg_info

    
    pkg_info = os.path.join(egg_info, 'PKG-INFO')
    info_dict = {}
    for line in file(pkg_info).readlines():
        if not line or line[0].isspace():
            continue 
        assert ':' in line
        key, value = [i.strip() for i in line.split(':', 1)]
        info_dict[key] = value

    return info_dict

def get_dependencies(directory):
    "returns the package name and dependencies given a package directory"

    
    info_dict = info(directory)

    
    egg_info = [entry for entry in os.listdir(directory)
                if entry.endswith('.egg-info')][0]

    
    requires = os.path.join(directory, egg_info, 'requires.txt')
    if os.path.exists(requires):
        dependencies = [line.strip()
                        for line in file(requires).readlines()
                        if line.strip()]
    else:
        dependencies = []

    
    return info_dict['Name'], dependencies

def dependency_info(dep):
    "return dictionary of dependency information from a dependency string"
    retval = dict(Name=None, Type=None, Version=None)
    for joiner in ('==', '<=', '>='):
        if joiner in dep:
            retval['Type'] = joiner
            name, version = [i.strip() for i in dep.split(joiner, 1)]
            retval['Name'] = name
            retval['Version'] = version
            break
    else:
        retval['Name'] = dep.strip()
    return retval

def unroll_dependencies(dependencies):
    """
    unroll a set of dependencies to a flat list

    dependencies = {'packageA': set(['packageB', 'packageC', 'packageF']),
                    'packageB': set(['packageC', 'packageD', 'packageE', 'packageG']),
                    'packageC': set(['packageE']),
                    'packageE': set(['packageF', 'packageG']),
                    'packageF': set(['packageG']),
                    'packageX': set(['packageA', 'packageG'])}
    """

    order = []

    
    packages = set(dependencies.keys())
    for deps in dependencies.values():
        packages.update(deps)

    while len(order) != len(packages):

        for package in packages.difference(order):
            if set(dependencies.get(package, set())).issubset(order):
                order.append(package)
                break
        else:
            raise AssertionError("Cyclic dependencies detected")

    cycle_check(order, dependencies) 

    return order


def main(args=sys.argv[1:]):

    
    usage = '%prog [options] [package] [package] [...]'
    parser = OptionParser(usage=usage, description=__doc__)
    parser.add_option('-d', '--dependencies', dest='list_dependencies',
                      action='store_true', default=False,
                      help="list dependencies for the packages")
    parser.add_option('--list', action='store_true', default=False,
                      help="list what will be installed")
    parser.add_option('--extra', '--install-extra-packages', action='store_true', default=False,
                      help="installs extra supporting packages as well as core mozbase ones")
    options, packages = parser.parse_args(args)

    if not packages:
        
        packages = sorted(mozbase_packages)

    
    assert set(packages).issubset(mozbase_packages), "Packages should be in %s (You gave: %s)" % (mozbase_packages, packages)

    if options.list_dependencies:
        
        for package in packages:
            print '%s: %s' % get_dependencies(os.path.join(here, package))
        parser.exit()

    
    
    deps = {}
    alldeps = {}
    mapping = {} 
    
    for package in packages:
        key, value = get_dependencies(os.path.join(here, package))
        deps[key] = [dependency_info(dep)['Name'] for dep in value]
        mapping[package] = key

        
        for dep in value:
            alldeps[dependency_info(dep)['Name']] = ''.join(dep.split())

    
    flag = True
    while flag:
        flag = False
        for value in deps.values():
            for dep in value:
                if dep in mozbase_packages and dep not in deps:
                    key, value = get_dependencies(os.path.join(here, dep))
                    deps[key] = [sanitize_dependency(dep) for dep in value]

                    for dep in value:
                        alldeps[sanitize_dependency(dep)] = ''.join(dep.split())
                    mapping[package] = key
                    flag = True
                    break
            if flag:
                break

    
    for package in mozbase_packages:
        if package in mapping:
            continue
        key, value = get_dependencies(os.path.join(here, package))
        mapping[package] = key

    
    unrolled = unroll_dependencies(deps)

    
    reverse_mapping = dict([(j,i) for i, j in mapping.items()])

    
    unrolled = [package for package in unrolled if package in reverse_mapping]

    if options.list:
        
        for package in unrolled:
            print package
        parser.exit()

    
    for package in unrolled:
        call([sys.executable, 'setup.py', 'develop', '--no-deps'],
             cwd=os.path.join(here, reverse_mapping[package]))

    
    
    
    
    pypi_deps = dict([(i, j) for i,j in alldeps.items()
                      if i not in unrolled])
    for package, version in pypi_deps.items():
        
        call(['easy_install', version])

    
    if options.extra:
        for package in extra_packages:
            call(['easy_install', package])

if __name__ == '__main__':
    main()
