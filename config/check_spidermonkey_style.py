




































from __future__ import print_function

import difflib
import os
import re
import subprocess
import sys
import traceback



ignored_js_src_dirs = [
   'js/src/config/',            
   'js/src/ctypes/libffi/',     
   'js/src/devtools/',          
   'js/src/editline/',          
   'js/src/gdb/',               
   'js/src/vtune/'              
]


included_inclnames_to_ignore = set([
    'ffi.h',                    
    'devtools/sharkctl.h',      
    'devtools/Instruments.h',   
    'double-conversion.h',      
    'javascript-trace.h',       
    'jsautokw.h',               
    'jscustomallocator.h',      
    'js-config.h',              
    'pratom.h',                 
    'prcvar.h',                 
    'prerror.h',                
    'prinit.h',                 
    'prlink.h',                 
    'prlock.h',                 
    'prprf.h',                  
    'prthread.h',               
    'prtypes.h',                
    'selfhosted.out.h',         
    'unicode/locid.h',          
    'unicode/numsys.h',         
    'unicode/ucal.h',           
    'unicode/uclean.h',         
    'unicode/ucol.h',           
    'unicode/udat.h',           
    'unicode/udatpg.h',         
    'unicode/uenum.h',          
    'unicode/unorm.h',          
    'unicode/unum.h',           
    'unicode/ustring.h',        
    'unicode/utypes.h',         
    'vtune/VTuneWrapper.h'      
])



oddly_ordered_inclnames = set([
    'ctypes/typedefs.h',        
    'jsautokw.h',               
    'jswin.h',                  
    'machine/endian.h',         
    'winbase.h',                
    'windef.h'                  
])









expected_output = '''\
js/src/tests/style/BadIncludes2.h:1: error:
    vanilla header includes an inline-header file "tests/style/BadIncludes2-inl.h"

js/src/tests/style/BadIncludes.h:3: error:
    the file includes itself

js/src/tests/style/BadIncludes.h:6: error:
    "BadIncludes2.h" is included using the wrong path;
    did you forget a prefix, or is the file not yet committed?

js/src/tests/style/BadIncludes.h:8: error:
    <tests/style/BadIncludes2.h> should be included using
    the #include "..." form

js/src/tests/style/BadIncludes.h:10: error:
    "stdio.h" is included using the wrong path;
    did you forget a prefix, or is the file not yet committed?

js/src/tests/style/BadIncludesOrder-inl.h:5:6: error:
    "vm/Interpreter-inl.h" should be included after "jsscriptinlines.h"

js/src/tests/style/BadIncludesOrder-inl.h:6:7: error:
    "jsscriptinlines.h" should be included after "js/Value.h"

js/src/tests/style/BadIncludesOrder-inl.h:7:8: error:
    "js/Value.h" should be included after "ds/LifoAlloc.h"

js/src/tests/style/BadIncludesOrder-inl.h:8:9: error:
    "ds/LifoAlloc.h" should be included after "jsapi.h"

js/src/tests/style/BadIncludesOrder-inl.h:9:10: error:
    "jsapi.h" should be included after <stdio.h>

js/src/tests/style/BadIncludesOrder-inl.h:10:11: error:
    <stdio.h> should be included after "mozilla/HashFunctions.h"

js/src/tests/style/BadIncludesOrder-inl.h:27:28: error:
    "jsobj.h" should be included after "jsfun.h"

(multiple files): error:
    header files form one or more cycles

   tests/style/HeaderCycleA1.h
   -> tests/style/HeaderCycleA2.h
      -> tests/style/HeaderCycleA3.h
         -> tests/style/HeaderCycleA1.h

   tests/style/HeaderCycleB1-inl.h
   -> tests/style/HeaderCycleB2-inl.h
      -> tests/style/HeaderCycleB3-inl.h
         -> tests/style/HeaderCycleB4-inl.h
            -> tests/style/HeaderCycleB1-inl.h
            -> tests/style/jsheadercycleB5inlines.h
               -> tests/style/HeaderCycleB1-inl.h
      -> tests/style/HeaderCycleB4-inl.h

'''.splitlines(True)

actual_output = []


def out(*lines):
    for line in lines:
        actual_output.append(line + '\n')


def error(filename, linenum, *lines):
    location = filename
    if linenum is not None:
        location += ':' + str(linenum)
    out(location + ': error:')
    for line in (lines):
        out('    ' + line)
    out('')


class FileKind(object):
    C = 1
    CPP = 2
    INL_H = 3
    H = 4
    TBL = 5
    MSG = 6

    @staticmethod
    def get(filename):
        if filename.endswith('.c'):
            return FileKind.C

        if filename.endswith('.cpp'):
            return FileKind.CPP

        if filename.endswith(('inlines.h', '-inl.h')):
            return FileKind.INL_H

        if filename.endswith('.h'):
            return FileKind.H

        if filename.endswith('.tbl'):
            return FileKind.TBL

        if filename.endswith('.msg'):
            return FileKind.MSG

        error(filename, None, 'unknown file kind')


def get_all_filenames():
    '''Get a list of all the files in the (Mercurial or Git) repository.'''
    cmds = [['hg', 'manifest', '-q'], ['git', 'ls-files', '--full-name', '../..']]
    for cmd in cmds:
        try:
            all_filenames = subprocess.check_output(cmd, universal_newlines=True,
                                                    stderr=subprocess.PIPE).split('\n')
            return all_filenames
        except:
            continue
    else:
        raise Exception('failed to run any of the repo manifest commands', cmds)


def check_style():
    
    
    
    
    
    
    
    
    

    mfbt_inclnames = set()      
    mozalloc_inclnames = set()  
    js_names = dict()           

    
    for filename in get_all_filenames():
        if filename.startswith('mfbt/') and filename.endswith('.h'):
            inclname = 'mozilla/' + filename.split('/')[-1]
            mfbt_inclnames.add(inclname)

        if filename.startswith('memory/mozalloc/') and filename.endswith('.h'):
            inclname = 'mozilla/' + filename.split('/')[-1]
            mozalloc_inclnames.add(inclname)

        if filename.startswith('js/public/') and filename.endswith('.h'):
            inclname = 'js/' + filename[len('js/public/'):]
            js_names[filename] = inclname

        if filename.startswith('js/src/') and \
           not filename.startswith(tuple(ignored_js_src_dirs)) and \
           filename.endswith(('.c', '.cpp', '.h', '.tbl', '.msg')):
            inclname = filename[len('js/src/'):]
            js_names[filename] = inclname

    all_inclnames = mfbt_inclnames | mozalloc_inclnames | set(js_names.values())

    edges = dict()      

    
    
    for inclname in mfbt_inclnames | mozalloc_inclnames:
        edges[inclname] = set()

    
    for filename in js_names.keys():
        inclname = js_names[filename]
        file_kind = FileKind.get(filename)
        if file_kind == FileKind.C or file_kind == FileKind.CPP or \
           file_kind == FileKind.H or file_kind == FileKind.INL_H:
            included_h_inclnames = set()    

            
            
            with open(os.path.join('../..', filename)) as f:
                do_file(filename, inclname, file_kind, f, all_inclnames, included_h_inclnames)

        edges[inclname] = included_h_inclnames

    find_cycles(all_inclnames, edges)

    
    difflines = difflib.unified_diff(expected_output, actual_output,
                                     fromfile='check_spider_monkey_style.py expected output',
                                       tofile='check_spider_monkey_style.py actual output')
    ok = True
    for diffline in difflines:
        ok = False
        print(diffline, end='')

    return ok


def module_name(name):
    '''Strip the trailing .cpp, .h, inlines.h or -inl.h from a filename.'''

    return name.replace('inlines.h', '').replace('-inl.h', '').replace('.h', '').replace('.cpp', '')


def is_module_header(enclosing_inclname, header_inclname):
    '''Determine if an included name is the "module header", i.e. should be
    first in the file.'''

    module = module_name(enclosing_inclname)

    
    if module == module_name(header_inclname):
        return True

    
    m = re.match(r'js\/(.*)\.h', header_inclname)
    if m is not None and module.endswith('/' + m.group(1)):
        return True

    return False


class Include(object):
    '''Important information for a single #include statement.'''

    def __init__(self, inclname, linenum, is_system):
        self.inclname = inclname
        self.linenum = linenum
        self.is_system = is_system

    def isLeaf(self):
        return True

    def section(self, enclosing_inclname):
        '''Identify which section inclname belongs to.

        The section numbers are as follows.
          0. Module header (e.g. jsfoo.h or jsfooinlines.h within jsfoo.cpp)
          1. mozilla/Foo.h
          2. <foo.h> or <foo>
          3. jsfoo.h, prmjtime.h, etc
          4. foo/Bar.h
          5. jsfooinlines.h
          6. foo/Bar-inl.h
          7. non-.h, e.g. *.tbl, *.msg
        '''

        if self.is_system:
            return 2

        if not self.inclname.endswith('.h'):
            return 7

        
        
        if is_module_header(enclosing_inclname, self.inclname):
            return 0

        if '/' in self.inclname:
            if self.inclname.startswith('mozilla/'):
                return 1

            if self.inclname.endswith('-inl.h'):
                return 6

            return 4

        if self.inclname.endswith('inlines.h'):
            return 5

        return 3

    def quote(self):
        if self.is_system:
            return '<' + self.inclname + '>'
        else:
            return '"' + self.inclname + '"'


class HashIfBlock(object):
    '''Important information about a #if/#endif block.

    A #if/#endif block is the contents of a #if/#endif (or similar) section.
    The top-level block, which is not within a #if/#endif pair, is also
    considered a block.

    Each leaf is either an Include (representing a #include), or another
    nested HashIfBlock.'''
    def __init__(self):
        self.kids = []

    def isLeaf(self):
        return False


def do_file(filename, inclname, file_kind, f, all_inclnames, included_h_inclnames):
    block_stack = [HashIfBlock()]

    
    for linenum, line in enumerate(f, start=1):
        
        if not '#' in line:
            continue

        
        m = re.match(r'\s*#\s*include\s+"([^"]*)"', line)
        if m is not None:
            block_stack[-1].kids.append(Include(m.group(1), linenum, False))

        
        m = re.match(r'\s*#\s*include\s+<([^>]*)>', line)
        if m is not None:
            block_stack[-1].kids.append(Include(m.group(1), linenum, True))

        
        m = re.match(r'\s*#\s*(if|ifdef|ifndef)\b', line)
        if m is not None:
            
            new_block = HashIfBlock()
            block_stack[-1].kids.append(new_block)
            block_stack.append(new_block)

        
        m = re.match(r'\s*#\s*(elif|else)\b', line)
        if m is not None:
            
            block_stack.pop()
            new_block = HashIfBlock()
            block_stack[-1].kids.append(new_block)
            block_stack.append(new_block)

        
        m = re.match(r'\s*#\s*endif\b', line)
        if m is not None:
            
            block_stack.pop()

    def check_include_statement(include):
        '''Check the style of a single #include statement.'''

        if include.is_system:
            
            if include.inclname in included_inclnames_to_ignore or \
               include.inclname in all_inclnames:
                error(filename, include.linenum,
                      include.quote() + ' should be included using',
                      'the #include "..." form')

        else:
            if include.inclname not in included_inclnames_to_ignore:
                included_kind = FileKind.get(include.inclname)

                
                if include.inclname not in all_inclnames:
                    error(filename, include.linenum,
                          include.quote() + ' is included using the wrong path;',
                          'did you forget a prefix, or is the file not yet committed?')

                
                
                elif included_kind == FileKind.H or included_kind == FileKind.INL_H:
                    included_h_inclnames.add(include.inclname)

                
                if file_kind == FileKind.H and included_kind == FileKind.INL_H:
                    error(filename, include.linenum,
                          'vanilla header includes an inline-header file ' + include.quote())

                
                
                if inclname == include.inclname:
                    error(filename, include.linenum, 'the file includes itself')

    def check_includes_order(include1, include2):
        '''Check the ordering of two #include statements.'''

        if include1.inclname in oddly_ordered_inclnames or \
           include2.inclname in oddly_ordered_inclnames:
            return

        section1 = include1.section(inclname)
        section2 = include2.section(inclname)
        if (section1 > section2) or \
           ((section1 == section2) and (include1.inclname.lower() > include2.inclname.lower())):
            error(filename, str(include1.linenum) + ':' + str(include2.linenum),
                  include1.quote() + ' should be included after ' + include2.quote())

    
    
    def pair_traverse(prev, this):
        if this.isLeaf():
            check_include_statement(this)
            if prev is not None and prev.isLeaf():
                check_includes_order(prev, this)
        else:
            for prev2, this2 in zip([None] + this.kids[0:-1], this.kids):
                pair_traverse(prev2, this2)

    pair_traverse(None, block_stack[-1])


def find_cycles(all_inclnames, edges):
    '''Find and draw any cycles.'''

    SCCs = tarjan(all_inclnames, edges)

    

    def draw_SCC(c):
        cset = set(c)
        drawn = set()
        def draw(v, indent):
            out('   ' * indent + ('-> ' if indent else '   ') + v)
            if v in drawn:
                return
            drawn.add(v)
            for succ in sorted(edges[v]):
                if succ in cset:
                    draw(succ, indent + 1)
        draw(sorted(c)[0], 0)
        out('')

    have_drawn_an_SCC = False
    for scc in sorted(SCCs):
        if len(scc) != 1:
            if not have_drawn_an_SCC:
                error('(multiple files)', None, 'header files form one or more cycles')
                have_drawn_an_SCC = True

            draw_SCC(scc)




def tarjan(V, E):
    vertex_index = {}
    vertex_lowlink = {}
    index = 0
    S = []
    all_SCCs = []

    def strongconnect(v, index):
        
        vertex_index[v] = index
        vertex_lowlink[v] = index
        index += 1
        S.append(v)

        
        for w in E[v]:
            if w not in vertex_index:
                
                index = strongconnect(w, index)
                vertex_lowlink[v] = min(vertex_lowlink[v], vertex_lowlink[w])
            elif w in S:
                
                vertex_lowlink[v] = min(vertex_lowlink[v], vertex_index[w])

        
        if vertex_lowlink[v] == vertex_index[v]:
            i = S.index(v)
            scc = S[i:]
            del S[i:]
            all_SCCs.append(scc)

        return index

    for v in V:
        if v not in vertex_index:
            index = strongconnect(v, index)

    return all_SCCs


def main():
    ok = check_style()

    if ok:
        print('TEST-PASS | check_spidermonkey_style.py | ok')
    else:
        print('TEST-UNEXPECTED-FAIL | check_spidermonkey_style.py | actual output does not match expected output;  diff is above')

    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
