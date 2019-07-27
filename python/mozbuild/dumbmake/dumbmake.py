



from __future__ import absolute_import, unicode_literals

from collections import OrderedDict
from itertools import groupby
from operator import itemgetter
from os.path import dirname

WHITESPACE_CHARACTERS = ' \t'

def indentation(line):
    """Number of whitespace (tab and space) characters at start of |line|."""
    i = 0
    while i < len(line):
        if line[i] not in WHITESPACE_CHARACTERS:
            break
        i += 1
    return i

def dependency_map(lines):
    """Return a dictionary with keys that are targets and values that
    are ordered lists of targets that should also be built.

    This implementation is O(n^2), but lovely and simple!  We walk the
    targets in the list, and for each target we walk backwards
    collecting its dependencies.  To make the walking easier, we
    reverse the list so that we are always walking forwards.

    """
    pairs = [(indentation(line), line.strip()) for line in lines]
    pairs.reverse()

    deps = {}

    for i, (indent, target) in enumerate(pairs):
        if not deps.has_key(target):
            deps[target] = []

        for j in range(i+1, len(pairs)):
            ind, tar = pairs[j]
            if ind < indent:
                indent = ind
                if tar not in deps[target]:
                    deps[target].append(tar)

    return deps

def all_dependencies(*targets, **kwargs):
    """Return a list containing all the dependencies of |targets|.

    The relative order of targets is maintained if possible.

    """
    dm = kwargs.pop('dependency_map', None)
    if dm is None:
        dm = dependency_map(targets)

    all_targets = OrderedDict() 

    for target in targets:
        if target in dm:
            for dependency in dm[target]:
                
                if dependency in all_targets:
                    del all_targets[dependency]
                all_targets[dependency] = True

    return all_targets.keys()

def get_components(path):
    """Take a path and return all the components of the path."""
    paths = [path]
    while True:
        parent = dirname(paths[-1])
        if parent == "":
            break
        paths.append(parent)

    paths.reverse()
    return paths

def add_extra_dependencies(target_pairs, dependency_map):
    """Take a list [(make_dir, make_target)] and expand (make_dir, None)
    entries with extra make dependencies from |dependency_map|.

    Returns an iterator of pairs (make_dir, make_target).

    """
    all_targets = OrderedDict() 
    make_dirs = OrderedDict() 

    for make_target, group in groupby(target_pairs, itemgetter(1)):
        
        if make_target is not None:
            for pair in group:
                
                
                paths = get_components(pair[1])
                
                
                for target in paths:
                    if target not in all_targets:
                        yield pair[0], target
                        all_targets[target] = True
            continue

        
        for make_dir, _ in group:
            if make_dir not in make_dirs:
                yield make_dir, None
                make_dirs[make_dir] = True

    all_components = []
    for make_dir in make_dirs.iterkeys():
        all_components.extend(get_components(make_dir))

    for i in all_dependencies(*all_components, dependency_map=dependency_map):
        if i not in make_dirs:
            yield i, None
