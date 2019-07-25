




from __future__ import with_statement
from contextlib import closing
from StringIO import StringIO

try:
    from abc import abstractmethod
except ImportError:
    
    
    def abstractmethod(method):
        line = method.func_code.co_firstlineno
        filename = method.func_code.co_filename
        def not_implemented(*args, **kwargs):
            raise NotImplementedError('Abstract method %s at File "%s", line %s  should be implemented by a concrete class' %
                                      (repr(method), filename,line))
        return not_implemented

class Output(object):
    """ Abstract base class for outputting test results """

    @abstractmethod
    def serialize(self, results_collection, file_obj):
        """ Writes the string representation of the results collection
        to the given file object"""

    def dump_string(self, results_collection):
        """ Returns the string representation of the results collection """
        with closing(StringIO()) as s:
            self.serialize(results_collection, s)
            return s.getvalue()



def count(iterable):
    """ Return the count of an iterable. Useful for generators. """
    c = 0
    for i in iterable:
        c += 1
    return c


def long_name(test):
    if test.test_class:
        return '%s.%s' % (test.test_class, test.name)
    return test.name
