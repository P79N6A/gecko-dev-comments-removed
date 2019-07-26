


































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
    'jsautooplen.h',            
    'jscustomallocator.h',      
    'js-config.h',              
    'pratom.h',                 
    'prcvar.h',                 
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
    'unicode/unum.h',           
    'unicode/ustring.h',        
    'unicode/utypes.h',         
    'vtune/VTuneWrapper.h'      
])









expected_output = '''\
js/src/tests/style/BadIncludes2.h:1: error:
    vanilla header includes an inline-header file "tests/style/BadIncludes2-inl.h"

js/src/tests/style/BadIncludes.h:1: error:
    the file includes itself

js/src/tests/style/BadIncludes.h:3: error:
    "BadIncludes2.h" is included using the wrong path;
    did you forget a prefix, or is the file not yet committed?

js/src/tests/style/BadIncludes.h:4: error:
    <tests/style/BadIncludes2.h> should be included using
    the #include "..." form

js/src/tests/style/BadIncludes.h:5: error:
    "stdio.h" is included using the wrong path;
    did you forget a prefix, or is the file not yet committed?

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
    if linenum != None:
        location += ":" + str(linenum)
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
    """Get a list of all the files in the (Mercurial or Git) repository."""
    cmds = [['hg', 'manifest', '-q'], ['git', 'ls-files']]
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
    js_names = dict()           

    
    for filename in get_all_filenames():
        if filename.startswith('mfbt/') and filename.endswith('.h'):
            inclname = 'mozilla/' + filename[len('mfbt/'):]
            mfbt_inclnames.add(inclname)

        if filename.startswith('js/public/') and filename.endswith('.h'):
            inclname = 'js/' + filename[len('js/public/'):]
            js_names[filename] = inclname

        if filename.startswith('js/src/') and \
           not filename.startswith(tuple(ignored_js_src_dirs)) and \
           filename.endswith(('.c', '.cpp', '.h', '.tbl', '.msg')):
            inclname = filename[len('js/src/'):]
            js_names[filename] = inclname

    all_inclnames = mfbt_inclnames | set(js_names.values())

    edges = dict()      

    
    
    for inclname in mfbt_inclnames:
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


def do_file(filename, inclname, file_kind, f, all_inclnames, included_h_inclnames):
    for linenum, line in enumerate(f, start=1):
        
        m = re.match(r'\s*#\s*include\s+"([^"]*)"', line)
        if m is not None:
            included_inclname = m.group(1)

            if included_inclname not in included_inclnames_to_ignore:
                included_kind = FileKind.get(included_inclname)

                
                if included_inclname not in all_inclnames:
                    error(filename, linenum,
                          '"' + included_inclname + '" is included ' + 'using the wrong path;',
                          'did you forget a prefix, or is the file not yet committed?')

                
                
                elif included_kind == FileKind.H or included_kind == FileKind.INL_H:
                    included_h_inclnames.add(included_inclname)

                
                if file_kind == FileKind.H and included_kind == FileKind.INL_H:
                    error(filename, linenum,
                          'vanilla header includes an inline-header file "' + included_inclname + '"')

                
                
                if inclname == included_inclname:
                    error(filename, linenum, 'the file includes itself')

        
        m = re.match(r'\s*#\s*include\s+<([^>]*)>', line)
        if m is not None:
            included_inclname = m.group(1)

            
            
            if included_inclname in included_inclnames_to_ignore or \
               included_inclname in all_inclnames:
                error(filename, linenum,
                      '<' + included_inclname + '> should be included using',
                      'the #include "..." form')


def find_cycles(all_inclnames, edges):
    """Find and draw any cycles."""
    
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


if __name__ == "__main__":
    main()
