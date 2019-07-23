


















































from compiler import parse, syntax, compile
from compiler.pycodegen import ModuleCodeGenerator
import compiler.ast
import new

def _fix_src(src):
    
    
    return src.replace("\r\n", "\n").replace("\r", "\n") + "\n"


def set_filename_and_offset(filename, offset, tree):
    """Set the filename attribute to filename on every node in tree"""
    worklist = [tree]
    while worklist:
        node = worklist.pop(0)
        node.filename = filename
        if node.lineno is not None:
            node.lineno += offset
        worklist.extend(node.getChildNodes())

def parse_function(src, func_name, arg_names, defaults=[]):
    tree = parse(src, "exec")
    defaults = [compiler.ast.Const(d) for d in defaults]
    
    try:
        decs = compiler.ast.Decorators([])
    except AttributeError:
        
        func = compiler.ast.Function(func_name, arg_names, defaults, 0, None,
                                     tree.node)
    else:
        
        func = compiler.ast.Function(decs, func_name, arg_names, defaults, 0, None,
                                     tree.node)
    stmt = compiler.ast.Stmt((func,))
    tree.node = stmt
    syntax.check(tree)
    return tree

def compile_function(src, filename, func_name, arg_names, defaults=[],
                     
                     lineno=0): 
    assert filename, "filename is required"
    try:
        tree = parse_function(_fix_src(src), func_name, arg_names, defaults)
    except SyntaxError, err:
        err.lineno += lineno
        err.filename = filename
        raise SyntaxError, err

    set_filename_and_offset(filename, lineno, tree)

    gen = ModuleCodeGenerator(tree)
    return gen.getCode()


def compile(src, filename, mode='exec', flags=None, dont_inherit=None, lineno=0):
    if flags is not None or dont_inherit is not None or mode != 'exec':
        raise RuntimeError, "not implemented yet"
    try:
        tree = parse(_fix_src(src), mode)
    except SyntaxError, err:
        err.lineno += lineno
        err.filename = filename
        raise SyntaxError, err

    set_filename_and_offset(filename, lineno, tree)

    gen = ModuleCodeGenerator(tree)
    return gen.getCode()
