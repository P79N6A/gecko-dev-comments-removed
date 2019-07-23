























__version__    = "2.5"
__tabversion__ = "2.4"       

import re, sys, types, copy, os


_is_identifier = re.compile(r'^[a-zA-Z0-9_]+$')






try:
    _INSTANCETYPE = (types.InstanceType, types.ObjectType)
except AttributeError:
    _INSTANCETYPE = types.InstanceType
    class object: pass       




class LexError(Exception):
    def __init__(self,message,s):
         self.args = (message,)
         self.text = s



class LexWarning(object):
   def __init__(self):
      self.warned = 0
   def __call__(self,msg):
      if not self.warned:
         sys.stderr.write("ply.lex: Warning: " + msg+"\n")
         self.warned = 1

_SkipWarning = LexWarning()         


class LexToken(object):
    def __str__(self):
        return "LexToken(%s,%r,%d,%d)" % (self.type,self.value,self.lineno,self.lexpos)
    def __repr__(self):
        return str(self)
    def skip(self,n):
        self.lexer.skip(n)
        _SkipWarning("Calling t.skip() on a token is deprecated.  Please use t.lexer.skip()")










class Lexer:
    def __init__(self):
        self.lexre = None             
                                      
                                      
                                      
        self.lexretext = None         
        self.lexstatere = {}          
        self.lexstateretext = {}      
        self.lexstaterenames = {}     
        self.lexstate = "INITIAL"     
        self.lexstatestack = []       
        self.lexstateinfo = None      
        self.lexstateignore = {}      
        self.lexstateerrorf = {}      
        self.lexreflags = 0           
        self.lexdata = None           
        self.lexpos = 0               
        self.lexlen = 0               
        self.lexerrorf = None         
        self.lextokens = None         
        self.lexignore = ""           
        self.lexliterals = ""         
        self.lexmodule = None         
        self.lineno = 1               
        self.lexdebug = 0             
        self.lexoptimize = 0          

    def clone(self,object=None):
        c = copy.copy(self)

        
        
        

        if object:
            newtab = { }
            for key, ritem in self.lexstatere.items():
                newre = []
                for cre, findex in ritem:
                     newfindex = []
                     for f in findex:
                         if not f or not f[0]:
                             newfindex.append(f)
                             continue
                         newfindex.append((getattr(object,f[0].__name__),f[1]))
                newre.append((cre,newfindex))
                newtab[key] = newre
            c.lexstatere = newtab
            c.lexstateerrorf = { }
            for key, ef in self.lexstateerrorf.items():
                c.lexstateerrorf[key] = getattr(object,ef.__name__)
            c.lexmodule = object
        return c

    
    
    
    def writetab(self,tabfile,outputdir=""):
        if isinstance(tabfile,types.ModuleType):
            return
        basetabfilename = tabfile.split(".")[-1]
        filename = os.path.join(outputdir,basetabfilename)+".py"
        tf = open(filename,"w")
        tf.write("# %s.py. This file automatically created by PLY (version %s). Don't edit!\n" % (tabfile,__version__))
        tf.write("_lextokens    = %s\n" % repr(self.lextokens))
        tf.write("_lexreflags   = %s\n" % repr(self.lexreflags))
        tf.write("_lexliterals  = %s\n" % repr(self.lexliterals))
        tf.write("_lexstateinfo = %s\n" % repr(self.lexstateinfo))

        tabre = { }
        
        initial = self.lexstatere["INITIAL"]
        initialfuncs = []
        for part in initial:
            for f in part[1]:
                if f and f[0]:
                    initialfuncs.append(f)

        for key, lre in self.lexstatere.items():
             titem = []
             for i in range(len(lre)):
                  titem.append((self.lexstateretext[key][i],_funcs_to_names(lre[i][1],self.lexstaterenames[key][i])))
             tabre[key] = titem

        tf.write("_lexstatere   = %s\n" % repr(tabre))
        tf.write("_lexstateignore = %s\n" % repr(self.lexstateignore))

        taberr = { }
        for key, ef in self.lexstateerrorf.items():
             if ef:
                  taberr[key] = ef.__name__
             else:
                  taberr[key] = None
        tf.write("_lexstateerrorf = %s\n" % repr(taberr))
        tf.close()

    
    
    
    def readtab(self,tabfile,fdict):
        if isinstance(tabfile,types.ModuleType):
            lextab = tabfile
        else:
            exec "import %s as lextab" % tabfile
        self.lextokens      = lextab._lextokens
        self.lexreflags     = lextab._lexreflags
        self.lexliterals    = lextab._lexliterals
        self.lexstateinfo   = lextab._lexstateinfo
        self.lexstateignore = lextab._lexstateignore
        self.lexstatere     = { }
        self.lexstateretext = { }
        for key,lre in lextab._lexstatere.items():
             titem = []
             txtitem = []
             for i in range(len(lre)):
                  titem.append((re.compile(lre[i][0],lextab._lexreflags),_names_to_funcs(lre[i][1],fdict)))
                  txtitem.append(lre[i][0])
             self.lexstatere[key] = titem
             self.lexstateretext[key] = txtitem
        self.lexstateerrorf = { }
        for key,ef in lextab._lexstateerrorf.items():
             self.lexstateerrorf[key] = fdict[ef]
        self.begin('INITIAL')

    
    
    
    def input(self,s):
        
        c = s[:1]
        if not (isinstance(c,types.StringType) or isinstance(c,types.UnicodeType)):
            raise ValueError, "Expected a string"
        self.lexdata = s
        self.lexpos = 0
        self.lexlen = len(s)

    
    
    
    def begin(self,state):
        if not self.lexstatere.has_key(state):
            raise ValueError, "Undefined state"
        self.lexre = self.lexstatere[state]
        self.lexretext = self.lexstateretext[state]
        self.lexignore = self.lexstateignore.get(state,"")
        self.lexerrorf = self.lexstateerrorf.get(state,None)
        self.lexstate = state

    
    
    
    def push_state(self,state):
        self.lexstatestack.append(self.lexstate)
        self.begin(state)

    
    
    
    def pop_state(self):
        self.begin(self.lexstatestack.pop())

    
    
    
    def current_state(self):
        return self.lexstate

    
    
    
    def skip(self,n):
        self.lexpos += n

    
    
    
    
    
    
    
    def token(self):
        
        lexpos    = self.lexpos
        lexlen    = self.lexlen
        lexignore = self.lexignore
        lexdata   = self.lexdata

        while lexpos < lexlen:
            
            if lexdata[lexpos] in lexignore:
                lexpos += 1
                continue

            
            for lexre,lexindexfunc in self.lexre:
                m = lexre.match(lexdata,lexpos)
                if not m: continue

                
                tok = LexToken()
                tok.value = m.group()
                tok.lineno = self.lineno
                tok.lexpos = lexpos

                i = m.lastindex
                func,tok.type = lexindexfunc[i]

                if not func:
                   
                   if tok.type:
                      self.lexpos = m.end()
                      return tok
                   else:
                      lexpos = m.end()
                      break

                lexpos = m.end()

                
                if not callable(func):
                   break

                

                tok.lexer = self      
                self.lexmatch = m
                self.lexpos = lexpos

                newtok = func(tok)

                
                if not newtok:
                    lexpos    = self.lexpos         
                    lexignore = self.lexignore      
                    break

                
                if not self.lexoptimize:
                    if not self.lextokens.has_key(newtok.type):
                        raise LexError, ("%s:%d: Rule '%s' returned an unknown token type '%s'" % (
                            func.func_code.co_filename, func.func_code.co_firstlineno,
                            func.__name__, newtok.type),lexdata[lexpos:])

                return newtok
            else:
                
                if lexdata[lexpos] in self.lexliterals:
                    tok = LexToken()
                    tok.value = lexdata[lexpos]
                    tok.lineno = self.lineno
                    tok.type = tok.value
                    tok.lexpos = lexpos
                    self.lexpos = lexpos + 1
                    return tok

                
                if self.lexerrorf:
                    tok = LexToken()
                    tok.value = self.lexdata[lexpos:]
                    tok.lineno = self.lineno
                    tok.type = "error"
                    tok.lexer = self
                    tok.lexpos = lexpos
                    self.lexpos = lexpos
                    newtok = self.lexerrorf(tok)
                    if lexpos == self.lexpos:
                        
                        raise LexError, ("Scanning error. Illegal character '%s'" % (lexdata[lexpos]), lexdata[lexpos:])
                    lexpos = self.lexpos
                    if not newtok: continue
                    return newtok

                self.lexpos = lexpos
                raise LexError, ("Illegal character '%s' at index %d" % (lexdata[lexpos],lexpos), lexdata[lexpos:])

        self.lexpos = lexpos + 1
        if self.lexdata is None:
             raise RuntimeError, "No input string given with input()"
        return None










def _validate_file(filename):
    import os.path
    base,ext = os.path.splitext(filename)
    if ext != '.py': return 1        

    try:
        f = open(filename)
        lines = f.readlines()
        f.close()
    except IOError:
        return 1                     

    fre = re.compile(r'\s*def\s+(t_[a-zA-Z_0-9]*)\(')
    sre = re.compile(r'\s*(t_[a-zA-Z_0-9]*)\s*=')

    counthash = { }
    linen = 1
    noerror = 1
    for l in lines:
        m = fre.match(l)
        if not m:
            m = sre.match(l)
        if m:
            name = m.group(1)
            prev = counthash.get(name)
            if not prev:
                counthash[name] = linen
            else:
                print >>sys.stderr, "%s:%d: Rule %s redefined. Previously defined on line %d" % (filename,linen,name,prev)
                noerror = 0
        linen += 1
    return noerror








def _funcs_to_names(funclist,namelist):
    result = []
    for f,name in zip(funclist,namelist):
         if f and f[0]:
             result.append((name, f[1]))
         else:
             result.append(f)
    return result








def _names_to_funcs(namelist,fdict):
     result = []
     for n in namelist:
          if n and n[0]:
              result.append((fdict[n[0]],n[1]))
          else:
              result.append(n)
     return result









def _form_master_re(relist,reflags,ldict,toknames):
    if not relist: return []
    regex = "|".join(relist)
    try:
        lexre = re.compile(regex,re.VERBOSE | reflags)

        
        lexindexfunc = [ None ] * (max(lexre.groupindex.values())+1)
        lexindexnames = lexindexfunc[:]

        for f,i in lexre.groupindex.items():
            handle = ldict.get(f,None)
            if type(handle) in (types.FunctionType, types.MethodType):
                lexindexfunc[i] = (handle,toknames[f])
                lexindexnames[i] = f
            elif handle is not None:
                lexindexnames[i] = f
                if f.find("ignore_") > 0:
                    lexindexfunc[i] = (None,None)
                else:
                    lexindexfunc[i] = (None, toknames[f])
        
        return [(lexre,lexindexfunc)],[regex],[lexindexnames]
    except Exception,e:
        m = int(len(relist)/2)
        if m == 0: m = 1
        llist, lre, lnames = _form_master_re(relist[:m],reflags,ldict,toknames)
        rlist, rre, rnames = _form_master_re(relist[m:],reflags,ldict,toknames)
        return llist+rlist, lre+rre, lnames+rnames










def _statetoken(s,names):
    nonstate = 1
    parts = s.split("_")
    for i in range(1,len(parts)):
         if not names.has_key(parts[i]) and parts[i] != 'ANY': break
    if i > 1:
       states = tuple(parts[1:i])
    else:
       states = ('INITIAL',)

    if 'ANY' in states:
       states = tuple(names.keys())

    tokenname = "_".join(parts[i:])
    return (states,tokenname)






def lex(module=None,object=None,debug=0,optimize=0,lextab="lextab",reflags=0,nowarn=0,outputdir=""):
    global lexer
    ldict = None
    stateinfo  = { 'INITIAL' : 'inclusive'}
    error = 0
    files = { }
    lexobj = Lexer()
    lexobj.lexdebug = debug
    lexobj.lexoptimize = optimize
    global token,input

    if nowarn: warn = 0
    else: warn = 1

    if object: module = object

    if module:
        
        if isinstance(module, types.ModuleType):
            ldict = module.__dict__
        elif isinstance(module, _INSTANCETYPE):
            _items = [(k,getattr(module,k)) for k in dir(module)]
            ldict = { }
            for (i,v) in _items:
                ldict[i] = v
        else:
            raise ValueError,"Expected a module or instance"
        lexobj.lexmodule = module

    else:
        
        try:
            raise RuntimeError
        except RuntimeError:
            e,b,t = sys.exc_info()
            f = t.tb_frame
            f = f.f_back                    
            if f.f_globals is f.f_locals:   
               ldict = f.f_globals
            else:
               ldict = f.f_globals.copy()
               ldict.update(f.f_locals)

    if optimize and lextab:
        try:
            lexobj.readtab(lextab,ldict)
            token = lexobj.token
            input = lexobj.input
            lexer = lexobj
            return lexobj

        except ImportError:
            pass

    

    tokens = ldict.get("tokens",None)
    states = ldict.get("states",None)
    literals = ldict.get("literals","")

    if not tokens:
        raise SyntaxError,"lex: module does not define 'tokens'"

    if not (isinstance(tokens,types.ListType) or isinstance(tokens,types.TupleType)):
        raise SyntaxError,"lex: tokens must be a list or tuple."

    
    lexobj.lextokens = { }
    if not optimize:
        for n in tokens:
            if not _is_identifier.match(n):
                print >>sys.stderr, "lex: Bad token name '%s'" % n
                error = 1
            if warn and lexobj.lextokens.has_key(n):
                print >>sys.stderr, "lex: Warning. Token '%s' multiply defined." % n
            lexobj.lextokens[n] = None
    else:
        for n in tokens: lexobj.lextokens[n] = None

    if debug:
        print "lex: tokens = '%s'" % lexobj.lextokens.keys()

    try:
         for c in literals:
               if not (isinstance(c,types.StringType) or isinstance(c,types.UnicodeType)) or len(c) > 1:
                    print >>sys.stderr, "lex: Invalid literal %s. Must be a single character" % repr(c)
                    error = 1
                    continue

    except TypeError:
         print >>sys.stderr, "lex: Invalid literals specification. literals must be a sequence of characters."
         error = 1

    lexobj.lexliterals = literals

    
    if states:
         if not (isinstance(states,types.TupleType) or isinstance(states,types.ListType)):
              print >>sys.stderr, "lex: states must be defined as a tuple or list."
              error = 1
         else:
              for s in states:
                    if not isinstance(s,types.TupleType) or len(s) != 2:
                           print >>sys.stderr, "lex: invalid state specifier %s. Must be a tuple (statename,'exclusive|inclusive')" % repr(s)
                           error = 1
                           continue
                    name, statetype = s
                    if not isinstance(name,types.StringType):
                           print >>sys.stderr, "lex: state name %s must be a string" % repr(name)
                           error = 1
                           continue
                    if not (statetype == 'inclusive' or statetype == 'exclusive'):
                           print >>sys.stderr, "lex: state type for state %s must be 'inclusive' or 'exclusive'" % name
                           error = 1
                           continue
                    if stateinfo.has_key(name):
                           print >>sys.stderr, "lex: state '%s' already defined." % name
                           error = 1
                           continue
                    stateinfo[name] = statetype

    
    tsymbols = [f for f in ldict.keys() if f[:2] == 't_' ]

    

    funcsym =  { }        
    strsym =   { }        
    toknames = { }        

    for s in stateinfo.keys():
         funcsym[s] = []
         strsym[s] = []

    ignore   = { }        
    errorf   = { }        

    if len(tsymbols) == 0:
        raise SyntaxError,"lex: no rules of the form t_rulename are defined."

    for f in tsymbols:
        t = ldict[f]
        states, tokname = _statetoken(f,stateinfo)
        toknames[f] = tokname

        if callable(t):
            for s in states: funcsym[s].append((f,t))
        elif (isinstance(t, types.StringType) or isinstance(t,types.UnicodeType)):
            for s in states: strsym[s].append((f,t))
        else:
            print >>sys.stderr, "lex: %s not defined as a function or string" % f
            error = 1

    
    for f in funcsym.values():
        f.sort(lambda x,y: cmp(x[1].func_code.co_firstlineno,y[1].func_code.co_firstlineno))

    
    for s in strsym.values():
        s.sort(lambda x,y: (len(x[1]) < len(y[1])) - (len(x[1]) > len(y[1])))

    regexs = { }

    
    for state in stateinfo.keys():
        regex_list = []

        
        for fname, f in funcsym[state]:
            line = f.func_code.co_firstlineno
            file = f.func_code.co_filename
            files[file] = None
            tokname = toknames[fname]

            ismethod = isinstance(f, types.MethodType)

            if not optimize:
                nargs = f.func_code.co_argcount
                if ismethod:
                    reqargs = 2
                else:
                    reqargs = 1
                if nargs > reqargs:
                    print >>sys.stderr, "%s:%d: Rule '%s' has too many arguments." % (file,line,f.__name__)
                    error = 1
                    continue

                if nargs < reqargs:
                    print >>sys.stderr, "%s:%d: Rule '%s' requires an argument." % (file,line,f.__name__)
                    error = 1
                    continue

                if tokname == 'ignore':
                    print >>sys.stderr, "%s:%d: Rule '%s' must be defined as a string." % (file,line,f.__name__)
                    error = 1
                    continue

            if tokname == 'error':
                errorf[state] = f
                continue

            if f.__doc__:
                if not optimize:
                    try:
                        c = re.compile("(?P<%s>%s)" % (fname,f.__doc__), re.VERBOSE | reflags)
                        if c.match(""):
                             print >>sys.stderr, "%s:%d: Regular expression for rule '%s' matches empty string." % (file,line,f.__name__)
                             error = 1
                             continue
                    except re.error,e:
                        print >>sys.stderr, "%s:%d: Invalid regular expression for rule '%s'. %s" % (file,line,f.__name__,e)
                        if '#' in f.__doc__:
                             print >>sys.stderr, "%s:%d. Make sure '#' in rule '%s' is escaped with '\\#'." % (file,line, f.__name__)
                        error = 1
                        continue

                    if debug:
                        print "lex: Adding rule %s -> '%s' (state '%s')" % (f.__name__,f.__doc__, state)

                
                

                regex_list.append("(?P<%s>%s)" % (fname,f.__doc__))
            else:
                print >>sys.stderr, "%s:%d: No regular expression defined for rule '%s'" % (file,line,f.__name__)

        
        for name,r in strsym[state]:
            tokname = toknames[name]

            if tokname == 'ignore':
                 if "\\" in r:
                      print >>sys.stderr, "lex: Warning. %s contains a literal backslash '\\'" % name
                 ignore[state] = r
                 continue

            if not optimize:
                if tokname == 'error':
                    raise SyntaxError,"lex: Rule '%s' must be defined as a function" % name
                    error = 1
                    continue

                if not lexobj.lextokens.has_key(tokname) and tokname.find("ignore_") < 0:
                    print >>sys.stderr, "lex: Rule '%s' defined for an unspecified token %s." % (name,tokname)
                    error = 1
                    continue
                try:
                    c = re.compile("(?P<%s>%s)" % (name,r),re.VERBOSE | reflags)
                    if (c.match("")):
                         print >>sys.stderr, "lex: Regular expression for rule '%s' matches empty string." % name
                         error = 1
                         continue
                except re.error,e:
                    print >>sys.stderr, "lex: Invalid regular expression for rule '%s'. %s" % (name,e)
                    if '#' in r:
                         print >>sys.stderr, "lex: Make sure '#' in rule '%s' is escaped with '\\#'." % name

                    error = 1
                    continue
                if debug:
                    print "lex: Adding rule %s -> '%s' (state '%s')" % (name,r,state)

            regex_list.append("(?P<%s>%s)" % (name,r))

        if not regex_list:
             print >>sys.stderr, "lex: No rules defined for state '%s'" % state
             error = 1

        regexs[state] = regex_list


    if not optimize:
        for f in files.keys():
           if not _validate_file(f):
                error = 1

    if error:
        raise SyntaxError,"lex: Unable to build lexer."

    
    

    

    for state in regexs.keys():
        lexre, re_text, re_names = _form_master_re(regexs[state],reflags,ldict,toknames)
        lexobj.lexstatere[state] = lexre
        lexobj.lexstateretext[state] = re_text
        lexobj.lexstaterenames[state] = re_names
        if debug:
            for i in range(len(re_text)):
                 print "lex: state '%s'. regex[%d] = '%s'" % (state, i, re_text[i])

    
    for state,type in stateinfo.items():
        if state != "INITIAL" and type == 'inclusive':
             lexobj.lexstatere[state].extend(lexobj.lexstatere['INITIAL'])
             lexobj.lexstateretext[state].extend(lexobj.lexstateretext['INITIAL'])
             lexobj.lexstaterenames[state].extend(lexobj.lexstaterenames['INITIAL'])

    lexobj.lexstateinfo = stateinfo
    lexobj.lexre = lexobj.lexstatere["INITIAL"]
    lexobj.lexretext = lexobj.lexstateretext["INITIAL"]

    
    lexobj.lexstateignore = ignore
    lexobj.lexignore = lexobj.lexstateignore.get("INITIAL","")

    
    lexobj.lexstateerrorf = errorf
    lexobj.lexerrorf = errorf.get("INITIAL",None)
    if warn and not lexobj.lexerrorf:
        print >>sys.stderr, "lex: Warning. no t_error rule is defined."

    
    for s,stype in stateinfo.items():
        if stype == 'exclusive':
              if warn and not errorf.has_key(s):
                   print >>sys.stderr, "lex: Warning. no error rule is defined for exclusive state '%s'" % s
              if warn and not ignore.has_key(s) and lexobj.lexignore:
                   print >>sys.stderr, "lex: Warning. no ignore rule is defined for exclusive state '%s'" % s
        elif stype == 'inclusive':
              if not errorf.has_key(s):
                   errorf[s] = errorf.get("INITIAL",None)
              if not ignore.has_key(s):
                   ignore[s] = ignore.get("INITIAL","")


    
    token = lexobj.token
    input = lexobj.input
    lexer = lexobj

    
    if lextab and optimize:
        lexobj.writetab(lextab,outputdir)

    return lexobj







def runmain(lexer=None,data=None):
    if not data:
        try:
            filename = sys.argv[1]
            f = open(filename)
            data = f.read()
            f.close()
        except IndexError:
            print "Reading from standard input (type EOF to end):"
            data = sys.stdin.read()

    if lexer:
        _input = lexer.input
    else:
        _input = input
    _input(data)
    if lexer:
        _token = lexer.token
    else:
        _token = token

    while 1:
        tok = _token()
        if not tok: break
        print "(%s,%r,%d,%d)" % (tok.type, tok.value, tok.lineno,tok.lexpos)









def TOKEN(r):
    def set_doc(f):
        if callable(r):
            f.__doc__ = r.__doc__
        else:
            f.__doc__ = r
        return f
    return set_doc


Token = TOKEN

