







"""Module which provides compatibility with older Python versions."""

__all__ = ["PY3", "int", "long", "xrange", "exec_", "callable",
           "namedtuple", "property", "defaultdict"]

import sys




PY3 = sys.version_info >= (3,)

try:
    import __builtin__
except ImportError:
    import builtins as __builtin__  

if PY3:
    int = int
    long = int
    xrange = range
    exec_ = getattr(__builtin__, "exec")
    print_ = getattr(__builtin__, "print")
else:
    int = int
    long = long
    xrange = xrange

    def exec_(code, globs=None, locs=None):
        if globs is None:
            frame = _sys._getframe(1)
            globs = frame.f_globals
            if locs is None:
                locs = frame.f_locals
            del frame
        elif locs is None:
            locs = globs
        exec("""exec code in globs, locs""")

    def print_(s):
        sys.stdout.write(s + '\n')
        sys.stdout.flush()



try:
    callable = callable
except Exception:
    def callable(obj):
        for klass in type(obj).__mro__:
            if "__call__" in klass.__dict__:
                return True
        return False




try:
    from collections import namedtuple
except ImportError:
    from operator import itemgetter as _itemgetter
    from keyword import iskeyword as _iskeyword
    import sys as _sys

    def namedtuple(typename, field_names, verbose=False, rename=False):
        """A collections.namedtuple implementation written in Python
        to support Python versions < 2.6.

        Taken from: http://code.activestate.com/recipes/500261/
        """
        
        
        
        if isinstance(field_names, basestring):
             
            field_names = field_names.replace(',', ' ').split()
        field_names = tuple(map(str, field_names))
        if rename:
            names = list(field_names)
            seen = set()
            for i, name in enumerate(names):
                if (not min(c.isalnum() or c=='_' for c in name) or _iskeyword(name)
                    or not name or name[0].isdigit() or name.startswith('_')
                    or name in seen):
                        names[i] = '_%d' % i
                seen.add(name)
            field_names = tuple(names)
        for name in (typename,) + field_names:
            if not min(c.isalnum() or c=='_' for c in name):
                raise ValueError('Type names and field names can only contain ' \
                                 'alphanumeric characters and underscores: %r'
                                 % name)
            if _iskeyword(name):
                raise ValueError('Type names and field names cannot be a keyword: %r' \
                                 % name)
            if name[0].isdigit():
                raise ValueError('Type names and field names cannot start with a ' \
                                 'number: %r' % name)
        seen_names = set()
        for name in field_names:
            if name.startswith('_') and not rename:
                raise ValueError('Field names cannot start with an underscore: %r'
                                 % name)
            if name in seen_names:
                raise ValueError('Encountered duplicate field name: %r' % name)
            seen_names.add(name)

        
        numfields = len(field_names)
        
        argtxt = repr(field_names).replace("'", "")[1:-1]
        reprtxt = ', '.join('%s=%%r' % name for name in field_names)
        template = '''class %(typename)s(tuple):
        '%(typename)s(%(argtxt)s)' \n
        __slots__ = () \n
        _fields = %(field_names)r \n
        def __new__(_cls, %(argtxt)s):
            return _tuple.__new__(_cls, (%(argtxt)s)) \n
        @classmethod
        def _make(cls, iterable, new=tuple.__new__, len=len):
            'Make a new %(typename)s object from a sequence or iterable'
            result = new(cls, iterable)
            if len(result) != %(numfields)d:
                raise TypeError('Expected %(numfields)d arguments, got %%d' %% len(result))
            return result \n
        def __repr__(self):
            return '%(typename)s(%(reprtxt)s)' %% self \n
        def _asdict(self):
            'Return a new dict which maps field names to their values'
            return dict(zip(self._fields, self)) \n
        def _replace(_self, **kwds):
            'Return a new %(typename)s object replacing specified fields with new values'
            result = _self._make(map(kwds.pop, %(field_names)r, _self))
            if kwds:
                raise ValueError('Got unexpected field names: %%r' %% kwds.keys())
            return result \n
        def __getnewargs__(self):
            return tuple(self) \n\n''' % locals()
        for i, name in enumerate(field_names):
            template += '        %s = _property(_itemgetter(%d))\n' % (name, i)
        if verbose:
            sys.stdout.write(template + '\n')
            sys.stdout.flush()

        
        namespace = dict(_itemgetter=_itemgetter, __name__='namedtuple_%s' % typename,
                         _property=property, _tuple=tuple)
        try:
            exec_(template, namespace)
        except SyntaxError:
            e = sys.exc_info()[1]
            raise SyntaxError(e.message + ':\n' + template)
        result = namespace[typename]

        
        
        
        
        
        try:
            result.__module__ = _sys._getframe(1).f_globals.get('__name__', '__main__')
        except (AttributeError, ValueError):
            pass

        return result




if hasattr(property, 'setter'):
    property = property
else:
    class property(__builtin__.property):
        __metaclass__ = type

        def __init__(self, fget, *args, **kwargs):
            super(property, self).__init__(fget, *args, **kwargs)
            self.__doc__ = fget.__doc__

        def getter(self, method):
            return property(method, self.fset, self.fdel)

        def setter(self, method):
            return property(self.fget, method, self.fdel)

        def deleter(self, method):
            return property(self.fget, self.fset, method)






try:
    from collections import defaultdict
except ImportError:
    class defaultdict(dict):

        def __init__(self, default_factory=None, *a, **kw):
            if (default_factory is not None and
                not hasattr(default_factory, '__call__')):
                raise TypeError('first argument must be callable')
            dict.__init__(self, *a, **kw)
            self.default_factory = default_factory

        def __getitem__(self, key):
            try:
                return dict.__getitem__(self, key)
            except KeyError:
                return self.__missing__(key)

        def __missing__(self, key):
            if self.default_factory is None:
                raise KeyError(key)
            self[key] = value = self.default_factory()
            return value

        def __reduce__(self):
            if self.default_factory is None:
                args = tuple()
            else:
                args = self.default_factory,
            return type(self), args, None, None, self.items()

        def copy(self):
            return self.__copy__()

        def __copy__(self):
            return type(self)(self.default_factory, self)

        def __deepcopy__(self, memo):
            import copy
            return type(self)(self.default_factory,
                              copy.deepcopy(self.items()))

        def __repr__(self):
            return 'defaultdict(%s, %s)' % (self.default_factory,
                                            dict.__repr__(self))



try:
    from functools import wraps
except ImportError:
    def wraps(original):
        def inner(fn):
            
            for attribute in ['__module__',
                              '__name__',
                              '__doc__'
                              ]:
                setattr(fn, attribute, getattr(original, attribute))
            
            for attribute in ['__dict__',
                              ]:
                if hasattr(fn, attribute):
                    getattr(fn, attribute).update(getattr(original, attribute))
                else:
                    setattr(fn, attribute,
                            getattr(original, attribute).copy())
            return fn
        return inner
