



import os
from types import StringTypes
from collections import Iterable


class Makefile(object):
    '''Provides an interface for writing simple makefiles

    Instances of this class are created, populated with rules, then
    written.
    '''

    def __init__(self):
        self._statements = []

    def create_rule(self, targets=[]):
        '''
        Create a new rule in the makefile for the given targets.
        Returns the corresponding Rule instance.
        '''
        rule = Rule(targets)
        self._statements.append(rule)
        return rule

    def add_statement(self, statement):
        '''
        Add a raw statement in the makefile. Meant to be used for
        simple variable assignments.
        '''
        self._statements.append(statement)

    def dump(self, fh, removal_guard=True):
        '''
        Dump all the rules to the given file handle. Optionally (and by
        default), add guard rules for file removals (empty rules for other
        rules' dependencies)
        '''
        all_deps = set()
        all_targets = set()
        for statement in self._statements:
            if isinstance(statement, Rule):
                statement.dump(fh)
                all_deps.update(statement.dependencies())
                all_targets.update(statement.targets())
            else:
                fh.write('%s\n' % statement)
        if removal_guard:
            guard = Rule(sorted(all_deps - all_targets))
            guard.dump(fh)


class Rule(object):
    '''Class handling simple rules in the form:
           target1 target2 ... : dep1 dep2 ...
                   command1
                   command2
                   ...
    '''
    def __init__(self, targets=[]):
        self._targets = []
        self._dependencies = []
        self._commands = []
        self.add_targets(targets)

    def add_targets(self, targets):
        '''Add additional targets to the rule.'''
        assert isinstance(targets, Iterable) and not isinstance(targets, StringTypes)
        self._targets.extend(t.replace(os.sep, '/') for t in targets
                             if not t in self._targets)
        return self

    def add_dependencies(self, deps):
        '''Add dependencies to the rule.'''
        assert isinstance(deps, Iterable) and not isinstance(deps, StringTypes)
        self._dependencies.extend(d.replace(os.sep, '/') for d in deps
                                  if not d in self._dependencies)
        return self

    def add_commands(self, commands):
        '''Add commands to the rule.'''
        assert isinstance(commands, Iterable) and not isinstance(commands, StringTypes)
        self._commands.extend(commands)
        return self

    def targets(self):
        '''Return an iterator on the rule targets.'''
        
        
        return iter(self._targets)

    def dependencies(self):
        '''Return an iterator on the rule dependencies.'''
        return iter(self._dependencies)

    def commands(self):
        '''Return an iterator on the rule commands.'''
        return iter(self._commands)

    def dump(self, fh):
        '''
        Dump the rule to the given file handle.
        '''
        if not self._targets:
            return
        fh.write('%s:' % ' '.join(self._targets))
        if self._dependencies:
            fh.write(' %s' % ' '.join(self._dependencies))
        fh.write('\n')
        for cmd in self._commands:
            fh.write('\t%s\n' % cmd)
