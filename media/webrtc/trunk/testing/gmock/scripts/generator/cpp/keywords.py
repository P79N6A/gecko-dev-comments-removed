
















"""C++ keywords and helper utilities for determining keywords."""

__author__ = 'nnorwitz@google.com (Neal Norwitz)'


try:
    
    import builtins
except ImportError:
    
    import __builtin__ as builtins


if not hasattr(builtins, 'set'):
    
    from sets import Set as set


TYPES = set('bool char int long short double float void wchar_t unsigned signed'.split())
TYPE_MODIFIERS = set('auto register const inline extern static virtual volatile mutable'.split())
ACCESS = set('public protected private friend'.split())

CASTS = set('static_cast const_cast dynamic_cast reinterpret_cast'.split())

OTHERS = set('true false asm class namespace using explicit this operator sizeof'.split())
OTHER_TYPES = set('new delete typedef struct union enum typeid typename template'.split())

CONTROL = set('case switch default if else return goto'.split())
EXCEPTION = set('try catch throw'.split())
LOOP = set('while do for break continue'.split())

ALL = TYPES | TYPE_MODIFIERS | ACCESS | CASTS | OTHERS | OTHER_TYPES | CONTROL | EXCEPTION | LOOP


def IsKeyword(token):
    return token in ALL

def IsBuiltinType(token):
    if token in ('virtual', 'inline'):
        
        return False
    return token in TYPES or token in TYPE_MODIFIERS
