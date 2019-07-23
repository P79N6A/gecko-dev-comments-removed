



















































__version__    = "2.5"
__tabversion__ = "2.4"       







yaccdebug   = 1                
                               

debug_file  = 'parser.out'     
tab_module  = 'parsetab'       
default_lr  = 'LALR'           

error_count = 3                

yaccdevel   = 0                
                               

import re, types, sys, cStringIO, md5, os.path


class YaccError(Exception):   pass


class SyntaxError(Exception): pass






try:
    _INSTANCETYPE = (types.InstanceType, types.ObjectType)
except AttributeError:
    _INSTANCETYPE = types.InstanceType
    class object: pass     


















class YaccSymbol:
    def __str__(self):    return self.type
    def __repr__(self):   return str(self)










class YaccProduction:
    def __init__(self,s,stack=None):
        self.slice = s
        self.stack = stack
        self.lexer = None
        self.parser= None
    def __getitem__(self,n):
        if n >= 0: return self.slice[n].value
        else: return self.stack[n].value

    def __setitem__(self,n,v):
        self.slice[n].value = v

    def __getslice__(self,i,j):
        return [s.value for s in self.slice[i:j]]

    def __len__(self):
        return len(self.slice)

    def lineno(self,n):
        return getattr(self.slice[n],"lineno",0)

    def linespan(self,n):
        startline = getattr(self.slice[n],"lineno",0)
        endline = getattr(self.slice[n],"endlineno",startline)
        return startline,endline

    def lexpos(self,n):
        return getattr(self.slice[n],"lexpos",0)

    def lexspan(self,n):
        startpos = getattr(self.slice[n],"lexpos",0)
        endpos = getattr(self.slice[n],"endlexpos",startpos)
        return startpos,endpos

    def error(self):
       raise SyntaxError







class Parser:
    def __init__(self,magic=None):

        
        

        if magic != "xyzzy":
            raise YaccError, "Can't directly instantiate Parser. Use yacc() instead."

        
        self.productions = None          
        self.errorfunc   = None          
        self.action      = { }           
        self.goto        = { }           
        self.require     = { }           
        self.method      = "Unknown LR"  

    def errok(self):
        self.errorok     = 1

    def restart(self):
        del self.statestack[:]
        del self.symstack[:]
        sym = YaccSymbol()
        sym.type = '$end'
        self.symstack.append(sym)
        self.statestack.append(0)

    def parse(self,input=None,lexer=None,debug=0,tracking=0,tokenfunc=None):
        if debug or yaccdevel:
            return self.parsedebug(input,lexer,debug,tracking,tokenfunc)
        elif tracking:
            return self.parseopt(input,lexer,debug,tracking,tokenfunc)
        else:
            return self.parseopt_notrack(input,lexer,debug,tracking,tokenfunc)
        

    
    
    
    
    
    
    
    
    
    
    
    
    

    def parsedebug(self,input=None,lexer=None,debug=0,tracking=0,tokenfunc=None):
        lookahead = None                 
        lookaheadstack = [ ]             
        actions = self.action            
        goto    = self.goto              
        prod    = self.productions       
        pslice  = YaccProduction(None)   
        errorcount = 0                   
        endsym  = "$end"                 
        
        if not lexer:
            import lex
            lexer = lex.lexer
        
        
        pslice.lexer = lexer
        pslice.parser = self

        
        if input is not None:
            lexer.input(input)

        if tokenfunc is None:
           
           get_token = lexer.token
        else:
           get_token = tokenfunc

        

        statestack = [ ]                
        self.statestack = statestack
        symstack   = [ ]                
        self.symstack = symstack

        pslice.stack = symstack         
        errtoken   = None               

        

        statestack.append(0)
        sym = YaccSymbol()
        sym.type = endsym
        symstack.append(sym)
        state = 0
        while 1:
            
            
            

            
            if debug > 1:
                print 'state', state
            

            if not lookahead:
                if not lookaheadstack:
                    lookahead = get_token()     
                else:
                    lookahead = lookaheadstack.pop()
                if not lookahead:
                    lookahead = YaccSymbol()
                    lookahead.type = endsym

            
            if debug:
                errorlead = ("%s . %s" % (" ".join([xx.type for xx in symstack][1:]), str(lookahead))).lstrip()
            

            
            ltype = lookahead.type
            t = actions[state].get(ltype)

            
            if debug > 1:
                print 'action', t
            

            if t is not None:
                if t > 0:
                    
                    if ltype is endsym:
                        
                        sys.stderr.write("yacc: Parse error. EOF\n")
                        return
                    statestack.append(t)
                    state = t
                    
                    
                    if debug > 1:
                        sys.stderr.write("%-60s shift state %s\n" % (errorlead, t))
                    

                    symstack.append(lookahead)
                    lookahead = None

                    
                    if errorcount: errorcount -=1
                    continue

                if t < 0:
                    
                    p = prod[-t]
                    pname = p.name
                    plen  = p.len

                    
                    sym = YaccSymbol()
                    sym.type = pname       
                    sym.value = None

                    
                    if debug > 1:
                        sys.stderr.write("%-60s reduce %d\n" % (errorlead, -t))
                    

                    if plen:
                        targ = symstack[-plen-1:]
                        targ[0] = sym

                        
                        if tracking:
                           t1 = targ[1]
                           sym.lineno = t1.lineno
                           sym.lexpos = t1.lexpos
                           t1 = targ[-1]
                           sym.endlineno = getattr(t1,"endlineno",t1.lineno)
                           sym.endlexpos = getattr(t1,"endlexpos",t1.lexpos)

                        

                        
                        
                        
                        

                        pslice.slice = targ
                        
                        try:
                            
                            p.func(pslice)
                            del symstack[-plen:]
                            del statestack[-plen:]
                            symstack.append(sym)
                            state = goto[statestack[-1]][pname]
                            statestack.append(state)
                        except SyntaxError:
                            
                            lookaheadstack.append(lookahead)
                            symstack.pop()
                            statestack.pop()
                            state = statestack[-1]
                            sym.type = 'error'
                            lookahead = sym
                            errorcount = error_count
                            self.errorok = 0
                        continue
                        
    
                    else:

                        
                        if tracking:
                           sym.lineno = lexer.lineno
                           sym.lexpos = lexer.lexpos
                        

                        targ = [ sym ]

                        
                        
                        
                        

                        pslice.slice = targ

                        try:
                            
                            p.func(pslice)
                            symstack.append(sym)
                            state = goto[statestack[-1]][pname]
                            statestack.append(state)
                        except SyntaxError:
                            
                            lookaheadstack.append(lookahead)
                            symstack.pop()
                            statestack.pop()
                            state = statestack[-1]
                            sym.type = 'error'
                            lookahead = sym
                            errorcount = error_count
                            self.errorok = 0
                        continue
                        

                if t == 0:
                    n = symstack[-1]
                    return getattr(n,"value",None)

            if t == None:

                
                if debug:
                    sys.stderr.write(errorlead + "\n")
                

                
                
                
                
                
                
                
                
                
                
                if errorcount == 0 or self.errorok:
                    errorcount = error_count
                    self.errorok = 0
                    errtoken = lookahead
                    if errtoken.type is endsym:
                        errtoken = None               
                    if self.errorfunc:
                        global errok,token,restart
                        errok = self.errok        
                        token = get_token
                        restart = self.restart
                        tok = self.errorfunc(errtoken)
                        del errok, token, restart   

                        if self.errorok:
                            
                            
                            
                            lookahead = tok
                            errtoken = None
                            continue
                    else:
                        if errtoken:
                            if hasattr(errtoken,"lineno"): lineno = lookahead.lineno
                            else: lineno = 0
                            if lineno:
                                sys.stderr.write("yacc: Syntax error at line %d, token=%s\n" % (lineno, errtoken.type))
                            else:
                                sys.stderr.write("yacc: Syntax error, token=%s" % errtoken.type)
                        else:
                            sys.stderr.write("yacc: Parse error in input. EOF\n")
                            return

                else:
                    errorcount = error_count

                
                
                

                if len(statestack) <= 1 and lookahead.type is not endsym:
                    lookahead = None
                    errtoken = None
                    state = 0
                    
                    del lookaheadstack[:]
                    continue

                
                

                
                if lookahead.type is endsym:
                    
                    return

                if lookahead.type != 'error':
                    sym = symstack[-1]
                    if sym.type == 'error':
                        
                        
                        lookahead = None
                        continue
                    t = YaccSymbol()
                    t.type = 'error'
                    if hasattr(lookahead,"lineno"):
                        t.lineno = lookahead.lineno
                    t.value = lookahead
                    lookaheadstack.append(lookahead)
                    lookahead = t
                else:
                    symstack.pop()
                    statestack.pop()
                    state = statestack[-1]       

                continue

            
            raise RuntimeError, "yacc: internal parser error!!!\n"

    
    
    
    
    
    
    


    def parseopt(self,input=None,lexer=None,debug=0,tracking=0,tokenfunc=None):
        lookahead = None                 
        lookaheadstack = [ ]             
        actions = self.action            
        goto    = self.goto              
        prod    = self.productions       
        pslice  = YaccProduction(None)   
        errorcount = 0                   

        
        if not lexer:
            import lex
            lexer = lex.lexer
        
        
        pslice.lexer = lexer
        pslice.parser = self

        
        if input is not None:
            lexer.input(input)

        if tokenfunc is None:
           
           get_token = lexer.token
        else:
           get_token = tokenfunc

        

        statestack = [ ]                
        self.statestack = statestack
        symstack   = [ ]                
        self.symstack = symstack

        pslice.stack = symstack         
        errtoken   = None               

        

        statestack.append(0)
        sym = YaccSymbol()
        sym.type = '$end'
        symstack.append(sym)
        state = 0
        while 1:
            
            
            

            if not lookahead:
                if not lookaheadstack:
                    lookahead = get_token()     
                else:
                    lookahead = lookaheadstack.pop()
                if not lookahead:
                    lookahead = YaccSymbol()
                    lookahead.type = '$end'

            
            ltype = lookahead.type
            t = actions[state].get(ltype)

            if t is not None:
                if t > 0:
                    
                    if ltype == '$end':
                        
                        sys.stderr.write("yacc: Parse error. EOF\n")
                        return
                    statestack.append(t)
                    state = t

                    symstack.append(lookahead)
                    lookahead = None

                    
                    if errorcount: errorcount -=1
                    continue

                if t < 0:
                    
                    p = prod[-t]
                    pname = p.name
                    plen  = p.len

                    
                    sym = YaccSymbol()
                    sym.type = pname       
                    sym.value = None

                    if plen:
                        targ = symstack[-plen-1:]
                        targ[0] = sym

                        
                        if tracking:
                           t1 = targ[1]
                           sym.lineno = t1.lineno
                           sym.lexpos = t1.lexpos
                           t1 = targ[-1]
                           sym.endlineno = getattr(t1,"endlineno",t1.lineno)
                           sym.endlexpos = getattr(t1,"endlexpos",t1.lexpos)

                        

                        
                        
                        
                        

                        pslice.slice = targ
                        
                        try:
                            
                            p.func(pslice)
                            del symstack[-plen:]
                            del statestack[-plen:]
                            symstack.append(sym)
                            state = goto[statestack[-1]][pname]
                            statestack.append(state)
                        except SyntaxError:
                            
                            lookaheadstack.append(lookahead)
                            symstack.pop()
                            statestack.pop()
                            state = statestack[-1]
                            sym.type = 'error'
                            lookahead = sym
                            errorcount = error_count
                            self.errorok = 0
                        continue
                        
    
                    else:

                        
                        if tracking:
                           sym.lineno = lexer.lineno
                           sym.lexpos = lexer.lexpos
                        

                        targ = [ sym ]

                        
                        
                        
                        

                        pslice.slice = targ

                        try:
                            
                            p.func(pslice)
                            symstack.append(sym)
                            state = goto[statestack[-1]][pname]
                            statestack.append(state)
                        except SyntaxError:
                            
                            lookaheadstack.append(lookahead)
                            symstack.pop()
                            statestack.pop()
                            state = statestack[-1]
                            sym.type = 'error'
                            lookahead = sym
                            errorcount = error_count
                            self.errorok = 0
                        continue
                        

                if t == 0:
                    n = symstack[-1]
                    return getattr(n,"value",None)

            if t == None:

                
                
                
                
                
                
                
                
                
                
                if errorcount == 0 or self.errorok:
                    errorcount = error_count
                    self.errorok = 0
                    errtoken = lookahead
                    if errtoken.type == '$end':
                        errtoken = None               
                    if self.errorfunc:
                        global errok,token,restart
                        errok = self.errok        
                        token = get_token
                        restart = self.restart
                        tok = self.errorfunc(errtoken)
                        del errok, token, restart   

                        if self.errorok:
                            
                            
                            
                            lookahead = tok
                            errtoken = None
                            continue
                    else:
                        if errtoken:
                            if hasattr(errtoken,"lineno"): lineno = lookahead.lineno
                            else: lineno = 0
                            if lineno:
                                sys.stderr.write("yacc: Syntax error at line %d, token=%s\n" % (lineno, errtoken.type))
                            else:
                                sys.stderr.write("yacc: Syntax error, token=%s" % errtoken.type)
                        else:
                            sys.stderr.write("yacc: Parse error in input. EOF\n")
                            return

                else:
                    errorcount = error_count

                
                
                

                if len(statestack) <= 1 and lookahead.type != '$end':
                    lookahead = None
                    errtoken = None
                    state = 0
                    
                    del lookaheadstack[:]
                    continue

                
                

                
                if lookahead.type == '$end':
                    
                    return

                if lookahead.type != 'error':
                    sym = symstack[-1]
                    if sym.type == 'error':
                        
                        
                        lookahead = None
                        continue
                    t = YaccSymbol()
                    t.type = 'error'
                    if hasattr(lookahead,"lineno"):
                        t.lineno = lookahead.lineno
                    t.value = lookahead
                    lookaheadstack.append(lookahead)
                    lookahead = t
                else:
                    symstack.pop()
                    statestack.pop()
                    state = statestack[-1]       

                continue

            
            raise RuntimeError, "yacc: internal parser error!!!\n"

    
    
    
    
    
    
    

    def parseopt_notrack(self,input=None,lexer=None,debug=0,tracking=0,tokenfunc=None):
        lookahead = None                 
        lookaheadstack = [ ]             
        actions = self.action            
        goto    = self.goto              
        prod    = self.productions       
        pslice  = YaccProduction(None)   
        errorcount = 0                   

        
        if not lexer:
            import lex
            lexer = lex.lexer
        
        
        pslice.lexer = lexer
        pslice.parser = self

        
        if input is not None:
            lexer.input(input)

        if tokenfunc is None:
           
           get_token = lexer.token
        else:
           get_token = tokenfunc

        

        statestack = [ ]                
        self.statestack = statestack
        symstack   = [ ]                
        self.symstack = symstack

        pslice.stack = symstack         
        errtoken   = None               

        

        statestack.append(0)
        sym = YaccSymbol()
        sym.type = '$end'
        symstack.append(sym)
        state = 0
        while 1:
            
            
            

            if not lookahead:
                if not lookaheadstack:
                    lookahead = get_token()     
                else:
                    lookahead = lookaheadstack.pop()
                if not lookahead:
                    lookahead = YaccSymbol()
                    lookahead.type = '$end'

            
            ltype = lookahead.type
            t = actions[state].get(ltype)

            if t is not None:
                if t > 0:
                    
                    if ltype == '$end':
                        
                        sys.stderr.write("yacc: Parse error. EOF\n")
                        return
                    statestack.append(t)
                    state = t

                    symstack.append(lookahead)
                    lookahead = None

                    
                    if errorcount: errorcount -=1
                    continue

                if t < 0:
                    
                    p = prod[-t]
                    pname = p.name
                    plen  = p.len

                    
                    sym = YaccSymbol()
                    sym.type = pname       
                    sym.value = None

                    if plen:
                        targ = symstack[-plen-1:]
                        targ[0] = sym

                        
                        
                        
                        

                        pslice.slice = targ
                        
                        try:
                            
                            p.func(pslice)
                            del symstack[-plen:]
                            del statestack[-plen:]
                            symstack.append(sym)
                            state = goto[statestack[-1]][pname]
                            statestack.append(state)
                        except SyntaxError:
                            
                            lookaheadstack.append(lookahead)
                            symstack.pop()
                            statestack.pop()
                            state = statestack[-1]
                            sym.type = 'error'
                            lookahead = sym
                            errorcount = error_count
                            self.errorok = 0
                        continue
                        
    
                    else:

                        targ = [ sym ]

                        
                        
                        
                        

                        pslice.slice = targ

                        try:
                            
                            p.func(pslice)
                            symstack.append(sym)
                            state = goto[statestack[-1]][pname]
                            statestack.append(state)
                        except SyntaxError:
                            
                            lookaheadstack.append(lookahead)
                            symstack.pop()
                            statestack.pop()
                            state = statestack[-1]
                            sym.type = 'error'
                            lookahead = sym
                            errorcount = error_count
                            self.errorok = 0
                        continue
                        

                if t == 0:
                    n = symstack[-1]
                    return getattr(n,"value",None)

            if t == None:

                
                
                
                
                
                
                
                
                
                
                if errorcount == 0 or self.errorok:
                    errorcount = error_count
                    self.errorok = 0
                    errtoken = lookahead
                    if errtoken.type == '$end':
                        errtoken = None               
                    if self.errorfunc:
                        global errok,token,restart
                        errok = self.errok        
                        token = get_token
                        restart = self.restart
                        tok = self.errorfunc(errtoken)
                        del errok, token, restart   

                        if self.errorok:
                            
                            
                            
                            lookahead = tok
                            errtoken = None
                            continue
                    else:
                        if errtoken:
                            if hasattr(errtoken,"lineno"): lineno = lookahead.lineno
                            else: lineno = 0
                            if lineno:
                                sys.stderr.write("yacc: Syntax error at line %d, token=%s\n" % (lineno, errtoken.type))
                            else:
                                sys.stderr.write("yacc: Syntax error, token=%s" % errtoken.type)
                        else:
                            sys.stderr.write("yacc: Parse error in input. EOF\n")
                            return

                else:
                    errorcount = error_count

                
                
                

                if len(statestack) <= 1 and lookahead.type != '$end':
                    lookahead = None
                    errtoken = None
                    state = 0
                    
                    del lookaheadstack[:]
                    continue

                
                

                
                if lookahead.type == '$end':
                    
                    return

                if lookahead.type != 'error':
                    sym = symstack[-1]
                    if sym.type == 'error':
                        
                        
                        lookahead = None
                        continue
                    t = YaccSymbol()
                    t.type = 'error'
                    if hasattr(lookahead,"lineno"):
                        t.lineno = lookahead.lineno
                    t.value = lookahead
                    lookaheadstack.append(lookahead)
                    lookahead = t
                else:
                    symstack.pop()
                    statestack.pop()
                    state = statestack[-1]       

                continue

            
            raise RuntimeError, "yacc: internal parser error!!!\n"
























def validate_file(filename):
    base,ext = os.path.splitext(filename)
    if ext != '.py': return 1          

    try:
        f = open(filename)
        lines = f.readlines()
        f.close()
    except IOError:
        return 1                       

    
    fre = re.compile(r'\s*def\s+(p_[a-zA-Z_0-9]*)\(')
    counthash = { }
    linen = 1
    noerror = 1
    for l in lines:
        m = fre.match(l)
        if m:
            name = m.group(1)
            prev = counthash.get(name)
            if not prev:
                counthash[name] = linen
            else:
                sys.stderr.write("%s:%d: Function %s redefined. Previously defined on line %d\n" % (filename,linen,name,prev))
                noerror = 0
        linen += 1
    return noerror


def validate_dict(d):
    for n,v in d.items():
        if n[0:2] == 'p_' and type(v) in (types.FunctionType, types.MethodType): continue
        if n[0:2] == 't_': continue

        if n[0:2] == 'p_':
            sys.stderr.write("yacc: Warning. '%s' not defined as a function\n" % n)
        if 1 and isinstance(v,types.FunctionType) and v.func_code.co_argcount == 1:
            try:
                doc = v.__doc__.split(" ")
                if doc[1] == ':':
                    sys.stderr.write("%s:%d: Warning. Possible grammar rule '%s' defined without p_ prefix.\n" % (v.func_code.co_filename, v.func_code.co_firstlineno,n))
            except StandardError:
                pass









def initialize_vars():
    global Productions, Prodnames, Prodmap, Terminals
    global Nonterminals, First, Follow, Precedence, UsedPrecedence, LRitems
    global Errorfunc, Signature, Requires

    Productions  = [None]  
                           
                           

    Prodnames    = { }     
                           

    Prodmap      = { }     
                           

    Terminals    = { }     
                           

    Nonterminals = { }     
                           

    First        = { }     

    Follow       = { }     

    Precedence   = { }     
                           

    UsedPrecedence = { }   
                           
                           

    LRitems      = [ ]     
                           

    Errorfunc    = None    

    Signature    = md5.new()   
                               
                               
    
    Signature.update(__tabversion__)

    Requires     = { }     

    
    global _vf, _vfc
    _vf           = cStringIO.StringIO()
    _vfc          = cStringIO.StringIO()

























class Production:
    def __init__(self,**kw):
        for k,v in kw.items():
            setattr(self,k,v)
        self.lr_index = -1
        self.lr0_added = 0    
        self.lr1_added = 0    
        self.usyms = [ ]
        self.lookaheads = { }
        self.lk_added = { }
        self.setnumbers = [ ]

    def __str__(self):
        if self.prod:
            s = "%s -> %s" % (self.name," ".join(self.prod))
        else:
            s = "%s -> <empty>" % self.name
        return s

    def __repr__(self):
        return str(self)

    
    def lr_item(self,n):
        if n > len(self.prod): return None
        p = Production()
        p.name = self.name
        p.prod = list(self.prod)
        p.number = self.number
        p.lr_index = n
        p.lookaheads = { }
        p.setnumbers = self.setnumbers
        p.prod.insert(n,".")
        p.prod = tuple(p.prod)
        p.len = len(p.prod)
        p.usyms = self.usyms

        
        try:
            p.lrafter = Prodnames[p.prod[n+1]]
        except (IndexError,KeyError),e:
            p.lrafter = []
        try:
            p.lrbefore = p.prod[n-1]
        except IndexError:
            p.lrbefore = None

        return p

class MiniProduction:
    pass


_is_identifier = re.compile(r'^[a-zA-Z0-9_-]+$')


















def add_production(f,file,line,prodname,syms):

    if Terminals.has_key(prodname):
        sys.stderr.write("%s:%d: Illegal rule name '%s'. Already defined as a token.\n" % (file,line,prodname))
        return -1
    if prodname == 'error':
        sys.stderr.write("%s:%d: Illegal rule name '%s'. error is a reserved word.\n" % (file,line,prodname))
        return -1

    if not _is_identifier.match(prodname):
        sys.stderr.write("%s:%d: Illegal rule name '%s'\n" % (file,line,prodname))
        return -1

    for x in range(len(syms)):
        s = syms[x]
        if s[0] in "'\"":
             try:
                 c = eval(s)
                 if (len(c) > 1):
                      sys.stderr.write("%s:%d: Literal token %s in rule '%s' may only be a single character\n" % (file,line,s, prodname))
                      return -1
                 if not Terminals.has_key(c):
                      Terminals[c] = []
                 syms[x] = c
                 continue
             except SyntaxError:
                 pass
        if not _is_identifier.match(s) and s != '%prec':
            sys.stderr.write("%s:%d: Illegal name '%s' in rule '%s'\n" % (file,line,s, prodname))
            return -1

    
    map = "%s -> %s" % (prodname,syms)
    if Prodmap.has_key(map):
        m = Prodmap[map]
        sys.stderr.write("%s:%d: Duplicate rule %s.\n" % (file,line, m))
        sys.stderr.write("%s:%d: Previous definition at %s:%d\n" % (file,line, m.file, m.line))
        return -1

    p = Production()
    p.name = prodname
    p.prod = syms
    p.file = file
    p.line = line
    p.func = f
    p.number = len(Productions)


    Productions.append(p)
    Prodmap[map] = p
    if not Nonterminals.has_key(prodname):
        Nonterminals[prodname] = [ ]

    
    i = 0
    while i < len(p.prod):
        t = p.prod[i]
        if t == '%prec':
            try:
                precname = p.prod[i+1]
            except IndexError:
                sys.stderr.write("%s:%d: Syntax error. Nothing follows %%prec.\n" % (p.file,p.line))
                return -1

            prec = Precedence.get(precname,None)
            if not prec:
                sys.stderr.write("%s:%d: Nothing known about the precedence of '%s'\n" % (p.file,p.line,precname))
                return -1
            else:
                p.prec = prec
                UsedPrecedence[precname] = 1
            del p.prod[i]
            del p.prod[i]
            continue

        if Terminals.has_key(t):
            Terminals[t].append(p.number)
            
            if not hasattr(p,"prec"):
                p.prec = Precedence.get(t,('right',0))
        else:
            if not Nonterminals.has_key(t):
                Nonterminals[t] = [ ]
            Nonterminals[t].append(p.number)
        i += 1

    if not hasattr(p,"prec"):
        p.prec = ('right',0)

    
    p.len  = len(p.prod)
    p.prod = tuple(p.prod)

    
    p.usyms = [ ]
    for s in p.prod:
        if s not in p.usyms:
            p.usyms.append(s)

    
    try:
        Prodnames[p.name].append(p)
    except KeyError:
        Prodnames[p.name] = [ p ]
    return 0




def add_function(f):
    line = f.func_code.co_firstlineno
    file = f.func_code.co_filename
    error = 0

    if isinstance(f,types.MethodType):
        reqdargs = 2
    else:
        reqdargs = 1

    if f.func_code.co_argcount > reqdargs:
        sys.stderr.write("%s:%d: Rule '%s' has too many arguments.\n" % (file,line,f.__name__))
        return -1

    if f.func_code.co_argcount < reqdargs:
        sys.stderr.write("%s:%d: Rule '%s' requires an argument.\n" % (file,line,f.__name__))
        return -1

    if f.__doc__:
        
        pstrings = f.__doc__.splitlines()
        lastp = None
        dline = line
        for ps in pstrings:
            dline += 1
            p = ps.split()
            if not p: continue
            try:
                if p[0] == '|':
                    
                    if not lastp:
                        sys.stderr.write("%s:%d: Misplaced '|'.\n" % (file,dline))
                        return -1
                    prodname = lastp
                    if len(p) > 1:
                        syms = p[1:]
                    else:
                        syms = [ ]
                else:
                    prodname = p[0]
                    lastp = prodname
                    assign = p[1]
                    if len(p) > 2:
                        syms = p[2:]
                    else:
                        syms = [ ]
                    if assign != ':' and assign != '::=':
                        sys.stderr.write("%s:%d: Syntax error. Expected ':'\n" % (file,dline))
                        return -1


                e = add_production(f,file,dline,prodname,syms)
                error += e


            except StandardError:
                sys.stderr.write("%s:%d: Syntax error in rule '%s'\n" % (file,dline,ps))
                error -= 1
    else:
        sys.stderr.write("%s:%d: No documentation string specified in function '%s'\n" % (file,line,f.__name__))
    return error




def compute_reachable():
    '''
    Find each symbol that can be reached from the start symbol.
    Print a warning for any nonterminals that can't be reached.
    (Unused terminals have already had their warning.)
    '''
    Reachable = { }
    for s in Terminals.keys() + Nonterminals.keys():
        Reachable[s] = 0

    mark_reachable_from( Productions[0].prod[0], Reachable )

    for s in Nonterminals.keys():
        if not Reachable[s]:
            sys.stderr.write("yacc: Symbol '%s' is unreachable.\n" % s)

def mark_reachable_from(s, Reachable):
    '''
    Mark all symbols that are reachable from symbol s.
    '''
    if Reachable[s]:
        
        return
    Reachable[s] = 1
    for p in Prodnames.get(s,[]):
        for r in p.prod:
            mark_reachable_from(r, Reachable)








def compute_terminates():
    '''
    Raise an error for any symbols that don't terminate.
    '''
    Terminates = {}

    
    for t in Terminals.keys():
        Terminates[t] = 1

    Terminates['$end'] = 1

    

    
    for n in Nonterminals.keys():
        Terminates[n] = 0

    
    while 1:
        some_change = 0
        for (n,pl) in Prodnames.items():
            
            for p in pl:
                
                for s in p.prod:
                    if not Terminates[s]:
                        
                        
                        p_terminates = 0
                        break
                else:
                    
                    
                    
                    p_terminates = 1

                if p_terminates:
                    
                    if not Terminates[n]:
                        Terminates[n] = 1
                        some_change = 1
                    
                    break

        if not some_change:
            break

    some_error = 0
    for (s,terminates) in Terminates.items():
        if not terminates:
            if not Prodnames.has_key(s) and not Terminals.has_key(s) and s != 'error':
                
                
                pass
            else:
                sys.stderr.write("yacc: Infinite recursion detected for symbol '%s'.\n" % s)
                some_error = 1

    return some_error






def verify_productions(cycle_check=1):
    error = 0
    for p in Productions:
        if not p: continue

        for s in p.prod:
            if not Prodnames.has_key(s) and not Terminals.has_key(s) and s != 'error':
                sys.stderr.write("%s:%d: Symbol '%s' used, but not defined as a token or a rule.\n" % (p.file,p.line,s))
                error = 1
                continue

    unused_tok = 0
    
    if yaccdebug:
        _vf.write("Unused terminals:\n\n")
    for s,v in Terminals.items():
        if s != 'error' and not v:
            sys.stderr.write("yacc: Warning. Token '%s' defined, but not used.\n" % s)
            if yaccdebug: _vf.write("   %s\n"% s)
            unused_tok += 1

    
    if yaccdebug:
        _vf.write("\nGrammar\n\n")
        for i in range(1,len(Productions)):
            _vf.write("Rule %-5d %s\n" % (i, Productions[i]))

    unused_prod = 0
    
    for s,v in Nonterminals.items():
        if not v:
            p = Prodnames[s][0]
            sys.stderr.write("%s:%d: Warning. Rule '%s' defined, but not used.\n" % (p.file,p.line, s))
            unused_prod += 1


    if unused_tok == 1:
        sys.stderr.write("yacc: Warning. There is 1 unused token.\n")
    if unused_tok > 1:
        sys.stderr.write("yacc: Warning. There are %d unused tokens.\n" % unused_tok)

    if unused_prod == 1:
        sys.stderr.write("yacc: Warning. There is 1 unused rule.\n")
    if unused_prod > 1:
        sys.stderr.write("yacc: Warning. There are %d unused rules.\n" % unused_prod)

    if yaccdebug:
        _vf.write("\nTerminals, with rules where they appear\n\n")
        ks = Terminals.keys()
        ks.sort()
        for k in ks:
            _vf.write("%-20s : %s\n" % (k, " ".join([str(s) for s in Terminals[k]])))
        _vf.write("\nNonterminals, with rules where they appear\n\n")
        ks = Nonterminals.keys()
        ks.sort()
        for k in ks:
            _vf.write("%-20s : %s\n" % (k, " ".join([str(s) for s in Nonterminals[k]])))

    if (cycle_check):
        compute_reachable()
        error += compute_terminates()

    return error
















def build_lritems():
    for p in Productions:
        lastlri = p
        lri = p.lr_item(0)
        i = 0
        while 1:
            lri = p.lr_item(i)
            lastlri.lr_next = lri
            if not lri: break
            lri.lr_num = len(LRitems)
            LRitems.append(lri)
            lastlri = lri
            i += 1

    
    
    
    







def add_precedence(plist):
    plevel = 0
    error = 0
    for p in plist:
        plevel += 1
        try:
            prec = p[0]
            terms = p[1:]
            if prec != 'left' and prec != 'right' and prec != 'nonassoc':
                sys.stderr.write("yacc: Invalid precedence '%s'\n" % prec)
                return -1
            for t in terms:
                if Precedence.has_key(t):
                    sys.stderr.write("yacc: Precedence already specified for terminal '%s'\n" % t)
                    error += 1
                    continue
                Precedence[t] = (prec,plevel)
        except:
            sys.stderr.write("yacc: Invalid precedence table.\n")
            error += 1

    return error








def check_precedence():
    error = 0
    for precname in Precedence.keys():
        if not (Terminals.has_key(precname) or UsedPrecedence.has_key(precname)):
            sys.stderr.write("yacc: Precedence rule '%s' defined for unknown symbol '%s'\n" % (Precedence[precname][0],precname))
            error += 1
    return error








def augment_grammar(start=None):
    if not start:
        start = Productions[1].name
    Productions[0] = Production(name="S'",prod=[start],number=0,len=1,prec=('right',0),func=None)
    Productions[0].usyms = [ start ]
    Nonterminals[start].append(0)










def first(beta):

    
    result = [ ]
    for x in beta:
        x_produces_empty = 0

        
        for f in First[x]:
            if f == '<empty>':
                x_produces_empty = 1
            else:
                if f not in result: result.append(f)

        if x_produces_empty:
            
            
            pass
        else:
            
            break
    else:
        
        
        
        result.append('<empty>')

    return result






def compute_follow(start=None):
    
    for k in Nonterminals.keys():
        Follow[k] = [ ]

    if not start:
        start = Productions[1].name

    Follow[start] = [ '$end' ]

    while 1:
        didadd = 0
        for p in Productions[1:]:
            
            for i in range(len(p.prod)):
                B = p.prod[i]
                if Nonterminals.has_key(B):
                    
                    fst = first(p.prod[i+1:])
                    hasempty = 0
                    for f in fst:
                        if f != '<empty>' and f not in Follow[B]:
                            Follow[B].append(f)
                            didadd = 1
                        if f == '<empty>':
                            hasempty = 1
                    if hasempty or i == (len(p.prod)-1):
                        
                        for f in Follow[p.name]:
                            if f not in Follow[B]:
                                Follow[B].append(f)
                                didadd = 1
        if not didadd: break

    if 0 and yaccdebug:
        _vf.write('\nFollow:\n')
        for k in Nonterminals.keys():
            _vf.write("%-20s : %s\n" % (k, " ".join([str(s) for s in Follow[k]])))






def compute_first1():

    
    for t in Terminals.keys():
        First[t] = [t]

    First['$end'] = ['$end']
    First['#'] = ['#'] 

    

    
    for n in Nonterminals.keys():
        First[n] = []

    
    while 1:
        some_change = 0
        for n in Nonterminals.keys():
            for p in Prodnames[n]:
                for f in first(p.prod):
                    if f not in First[n]:
                        First[n].append( f )
                        some_change = 1
        if not some_change:
            break

    if 0 and yaccdebug:
        _vf.write('\nFirst:\n')
        for k in Nonterminals.keys():
            _vf.write("%-20s : %s\n" %
                (k, " ".join([str(s) for s in First[k]])))









def lr_init_vars():
    global _lr_action, _lr_goto, _lr_method
    global _lr_goto_cache, _lr0_cidhash

    _lr_action       = { }        
    _lr_goto         = { }        
    _lr_method       = "Unknown"  
    _lr_goto_cache   = { }
    _lr0_cidhash     = { }





_add_count = 0       

def lr0_closure(I):
    global _add_count

    _add_count += 1
    prodlist = Productions

    
    J = I[:]
    didadd = 1
    while didadd:
        didadd = 0
        for j in J:
            for x in j.lrafter:
                if x.lr0_added == _add_count: continue
                
                J.append(x.lr_next)
                x.lr0_added = _add_count
                didadd = 1

    return J








def lr0_goto(I,x):
    
    g = _lr_goto_cache.get((id(I),x),None)
    if g: return g

    
    

    s = _lr_goto_cache.get(x,None)
    if not s:
        s = { }
        _lr_goto_cache[x] = s

    gs = [ ]
    for p in I:
        n = p.lr_next
        if n and n.lrbefore == x:
            s1 = s.get(id(n),None)
            if not s1:
                s1 = { }
                s[id(n)] = s1
            gs.append(n)
            s = s1
    g = s.get('$end',None)
    if not g:
        if gs:
            g = lr0_closure(gs)
            s['$end'] = g
        else:
            s['$end'] = gs
    _lr_goto_cache[(id(I),x)] = g
    return g

_lr0_cidhash = { }


def lr0_items():

    C = [ lr0_closure([Productions[0].lr_next]) ]
    i = 0
    for I in C:
        _lr0_cidhash[id(I)] = i
        i += 1

    
    i = 0
    while i < len(C):
        I = C[i]
        i += 1

        
        asyms = { }
        for ii in I:
            for s in ii.usyms:
                asyms[s] = None

        for x in asyms.keys():
            g = lr0_goto(I,x)
            if not g:  continue
            if _lr0_cidhash.has_key(id(g)): continue
            _lr0_cidhash[id(g)] = len(C)
            C.append(g)

    return C
































def compute_nullable_nonterminals():
    nullable = {}
    num_nullable = 0
    while 1:
       for p in Productions[1:]:
           if p.len == 0:
                nullable[p.name] = 1
                continue
           for t in p.prod:
                if not nullable.has_key(t): break
           else:
                nullable[p.name] = 1
       if len(nullable) == num_nullable: break
       num_nullable = len(nullable)
    return nullable












def find_nonterminal_transitions(C):
     trans = []
     for state in range(len(C)):
         for p in C[state]:
             if p.lr_index < p.len - 1:
                  t = (state,p.prod[p.lr_index+1])
                  if Nonterminals.has_key(t[1]):
                        if t not in trans: trans.append(t)
         state = state + 1
     return trans










def dr_relation(C,trans,nullable):
    dr_set = { }
    state,N = trans
    terms = []

    g = lr0_goto(C[state],N)
    for p in g:
       if p.lr_index < p.len - 1:
           a = p.prod[p.lr_index+1]
           if Terminals.has_key(a):
               if a not in terms: terms.append(a)

    
    if state == 0 and N == Productions[0].prod[0]:
       terms.append('$end')

    return terms







def reads_relation(C, trans, empty):
    
    rel = []
    state, N = trans

    g = lr0_goto(C[state],N)
    j = _lr0_cidhash.get(id(g),-1)
    for p in g:
        if p.lr_index < p.len - 1:
             a = p.prod[p.lr_index + 1]
             if empty.has_key(a):
                  rel.append((j,a))

    return rel





























def compute_lookback_includes(C,trans,nullable):

    lookdict = {}          
    includedict = {}       

    
    dtrans = {}
    for t in trans:
        dtrans[t] = 1

    
    for state,N in trans:
        lookb = []
        includes = []
        for p in C[state]:
            if p.name != N: continue

            
            

            lr_index = p.lr_index
            j = state
            while lr_index < p.len - 1:
                 lr_index = lr_index + 1
                 t = p.prod[lr_index]

                 
                 if dtrans.has_key((j,t)):
                       
                       
                       

                       li = lr_index + 1
                       while li < p.len:
                            if Terminals.has_key(p.prod[li]): break      
                            if not nullable.has_key(p.prod[li]): break
                            li = li + 1
                       else:
                            
                            includes.append((j,t))

                 g = lr0_goto(C[j],t)               
                 j = _lr0_cidhash.get(id(g),-1)     

            
            for r in C[j]:
                 if r.name != p.name: continue
                 if r.len != p.len:   continue
                 i = 0
                 
                 while i < r.lr_index:
                      if r.prod[i] != p.prod[i+1]: break
                      i = i + 1
                 else:
                      lookb.append((j,r))
        for i in includes:
             if not includedict.has_key(i): includedict[i] = []
             includedict[i].append((state,N))
        lookdict[(state,N)] = lookb

    return lookdict,includedict


















def digraph(X,R,FP):
    N = { }
    for x in X:
       N[x] = 0
    stack = []
    F = { }
    for x in X:
        if N[x] == 0: traverse(x,N,stack,F,X,R,FP)
    return F

def traverse(x,N,stack,F,X,R,FP):
    stack.append(x)
    d = len(stack)
    N[x] = d
    F[x] = FP(x)             

    rel = R(x)               
    for y in rel:
        if N[y] == 0:
             traverse(y,N,stack,F,X,R,FP)
        N[x] = min(N[x],N[y])
        for a in F.get(y,[]):
            if a not in F[x]: F[x].append(a)
    if N[x] == d:
       N[stack[-1]] = sys.maxint
       F[stack[-1]] = F[x]
       element = stack.pop()
       while element != x:
           N[stack[-1]] = sys.maxint
           F[stack[-1]] = F[x]
           element = stack.pop()













def compute_read_sets(C, ntrans, nullable):
    FP = lambda x: dr_relation(C,x,nullable)
    R =  lambda x: reads_relation(C,x,nullable)
    F = digraph(ntrans,R,FP)
    return F

















def compute_follow_sets(ntrans,readsets,inclsets):
     FP = lambda x: readsets[x]
     R  = lambda x: inclsets.get(x,[])
     F = digraph(ntrans,R,FP)
     return F













def add_lookaheads(lookbacks,followset):
    for trans,lb in lookbacks.items():
        
        for state,p in lb:
             if not p.lookaheads.has_key(state):
                  p.lookaheads[state] = []
             f = followset.get(trans,[])
             for a in f:
                  if a not in p.lookaheads[state]: p.lookaheads[state].append(a)








def add_lalr_lookaheads(C):
    
    nullable = compute_nullable_nonterminals()

    
    trans = find_nonterminal_transitions(C)

    
    readsets = compute_read_sets(C,trans,nullable)

    
    lookd, included = compute_lookback_includes(C,trans,nullable)

    
    followsets = compute_follow_sets(trans,readsets,included)

    
    add_lookaheads(lookd,followsets)






def lr_parse_table(method):
    global _lr_method
    goto = _lr_goto           
    action = _lr_action       
    actionp = { }             

    _lr_method = method

    n_srconflict = 0
    n_rrconflict = 0

    if yaccdebug:
        sys.stderr.write("yacc: Generating %s parsing table...\n" % method)
        _vf.write("\n\nParsing method: %s\n\n" % method)

    
    

    C = lr0_items()

    if method == 'LALR':
        add_lalr_lookaheads(C)


    
    st = 0
    for I in C:
        
        actlist = [ ]              
        st_action  = { }
        st_actionp = { }
        st_goto    = { }
        if yaccdebug:
            _vf.write("\nstate %d\n\n" % st)
            for p in I:
                _vf.write("    (%d) %s\n" % (p.number, str(p)))
            _vf.write("\n")

        for p in I:
            try:
                if p.len == p.lr_index + 1:
                    if p.name == "S'":
                        
                        st_action["$end"] = 0
                        st_actionp["$end"] = p
                    else:
                        
                        if method == 'LALR':
                            laheads = p.lookaheads[st]
                        else:
                            laheads = Follow[p.name]
                        for a in laheads:
                            actlist.append((a,p,"reduce using rule %d (%s)" % (p.number,p)))
                            r = st_action.get(a,None)
                            if r is not None:
                                
                                if r > 0:
                                    
                                    
                                    
                                    sprec,slevel = Productions[st_actionp[a].number].prec
                                    rprec,rlevel = Precedence.get(a,('right',0))
                                    if (slevel < rlevel) or ((slevel == rlevel) and (rprec == 'left')):
                                        
                                        st_action[a] = -p.number
                                        st_actionp[a] = p
                                        if not slevel and not rlevel:
                                            _vfc.write("shift/reduce conflict in state %d resolved as reduce.\n" % st)
                                            _vf.write("  ! shift/reduce conflict for %s resolved as reduce.\n" % a)
                                            n_srconflict += 1
                                    elif (slevel == rlevel) and (rprec == 'nonassoc'):
                                        st_action[a] = None
                                    else:
                                        
                                        if not rlevel:
                                            _vfc.write("shift/reduce conflict in state %d resolved as shift.\n" % st)
                                            _vf.write("  ! shift/reduce conflict for %s resolved as shift.\n" % a)
                                            n_srconflict +=1
                                elif r < 0:
                                    
                                    
                                    oldp = Productions[-r]
                                    pp = Productions[p.number]
                                    if oldp.line > pp.line:
                                        st_action[a] = -p.number
                                        st_actionp[a] = p
                                    
                                    n_rrconflict += 1
                                    _vfc.write("reduce/reduce conflict in state %d resolved using rule %d (%s).\n" % (st, st_actionp[a].number, st_actionp[a]))
                                    _vf.write("  ! reduce/reduce conflict for %s resolved using rule %d (%s).\n" % (a,st_actionp[a].number, st_actionp[a]))
                                else:
                                    sys.stderr.write("Unknown conflict in state %d\n" % st)
                            else:
                                st_action[a] = -p.number
                                st_actionp[a] = p
                else:
                    i = p.lr_index
                    a = p.prod[i+1]       
                    if Terminals.has_key(a):
                        g = lr0_goto(I,a)
                        j = _lr0_cidhash.get(id(g),-1)
                        if j >= 0:
                            
                            actlist.append((a,p,"shift and go to state %d" % j))
                            r = st_action.get(a,None)
                            if r is not None:
                                
                                if r > 0:
                                    if r != j:
                                        sys.stderr.write("Shift/shift conflict in state %d\n" % st)
                                elif r < 0:
                                    
                                    
                                    
                                    
                                    rprec,rlevel = Productions[st_actionp[a].number].prec
                                    sprec,slevel = Precedence.get(a,('right',0))
                                    if (slevel > rlevel) or ((slevel == rlevel) and (rprec == 'right')):
                                        
                                        st_action[a] = j
                                        st_actionp[a] = p
                                        if not rlevel:
                                            n_srconflict += 1
                                            _vfc.write("shift/reduce conflict in state %d resolved as shift.\n" % st)
                                            _vf.write("  ! shift/reduce conflict for %s resolved as shift.\n" % a)
                                    elif (slevel == rlevel) and (rprec == 'nonassoc'):
                                        st_action[a] = None
                                    else:
                                        
                                        if not slevel and not rlevel:
                                            n_srconflict +=1
                                            _vfc.write("shift/reduce conflict in state %d resolved as reduce.\n" % st)
                                            _vf.write("  ! shift/reduce conflict for %s resolved as reduce.\n" % a)

                                else:
                                    sys.stderr.write("Unknown conflict in state %d\n" % st)
                            else:
                                st_action[a] = j
                                st_actionp[a] = p

            except StandardError,e:
               print sys.exc_info()
               raise YaccError, "Hosed in lr_parse_table"

        
        if yaccdebug:
          _actprint = { }
          for a,p,m in actlist:
            if st_action.has_key(a):
                if p is st_actionp[a]:
                    _vf.write("    %-15s %s\n" % (a,m))
                    _actprint[(a,m)] = 1
          _vf.write("\n")
          for a,p,m in actlist:
            if st_action.has_key(a):
                if p is not st_actionp[a]:
                    if not _actprint.has_key((a,m)):
                        _vf.write("  ! %-15s [ %s ]\n" % (a,m))
                        _actprint[(a,m)] = 1

        
        if yaccdebug:
            _vf.write("\n")
        nkeys = { }
        for ii in I:
            for s in ii.usyms:
                if Nonterminals.has_key(s):
                    nkeys[s] = None
        for n in nkeys.keys():
            g = lr0_goto(I,n)
            j = _lr0_cidhash.get(id(g),-1)
            if j >= 0:
                st_goto[n] = j
                if yaccdebug:
                    _vf.write("    %-30s shift and go to state %d\n" % (n,j))

        action[st] = st_action
        actionp[st] = st_actionp
        goto[st] = st_goto

        st += 1

    if yaccdebug:
        if n_srconflict == 1:
            sys.stderr.write("yacc: %d shift/reduce conflict\n" % n_srconflict)
        if n_srconflict > 1:
            sys.stderr.write("yacc: %d shift/reduce conflicts\n" % n_srconflict)
        if n_rrconflict == 1:
            sys.stderr.write("yacc: %d reduce/reduce conflict\n" % n_rrconflict)
        if n_rrconflict > 1:
            sys.stderr.write("yacc: %d reduce/reduce conflicts\n" % n_rrconflict)











def lr_write_tables(modulename=tab_module,outputdir=''):
    if isinstance(modulename, types.ModuleType):
        print >>sys.stderr, "Warning module %s is inconsistent with the grammar (ignored)" % modulename
        return

    basemodulename = modulename.split(".")[-1]
    filename = os.path.join(outputdir,basemodulename) + ".py"
    try:
        f = open(filename,"w")

        f.write("""
# %s
# This file is automatically generated. Do not edit.

_lr_method = %s

_lr_signature = %s
""" % (filename, repr(_lr_method), repr(Signature.digest())))

        
        smaller = 1

        
        if smaller:
            items = { }

            for s,nd in _lr_action.items():
               for name,v in nd.items():
                  i = items.get(name)
                  if not i:
                     i = ([],[])
                     items[name] = i
                  i[0].append(s)
                  i[1].append(v)

            f.write("\n_lr_action_items = {")
            for k,v in items.items():
                f.write("%r:([" % k)
                for i in v[0]:
                    f.write("%r," % i)
                f.write("],[")
                for i in v[1]:
                    f.write("%r," % i)

                f.write("]),")
            f.write("}\n")

            f.write("""
_lr_action = { }
for _k, _v in _lr_action_items.items():
   for _x,_y in zip(_v[0],_v[1]):
      if not _lr_action.has_key(_x):  _lr_action[_x] = { }
      _lr_action[_x][_k] = _y
del _lr_action_items
""")

        else:
            f.write("\n_lr_action = { ");
            for k,v in _lr_action.items():
                f.write("(%r,%r):%r," % (k[0],k[1],v))
            f.write("}\n");

        if smaller:
            
            items = { }

            for s,nd in _lr_goto.items():
               for name,v in nd.items():
                  i = items.get(name)
                  if not i:
                     i = ([],[])
                     items[name] = i
                  i[0].append(s)
                  i[1].append(v)

            f.write("\n_lr_goto_items = {")
            for k,v in items.items():
                f.write("%r:([" % k)
                for i in v[0]:
                    f.write("%r," % i)
                f.write("],[")
                for i in v[1]:
                    f.write("%r," % i)

                f.write("]),")
            f.write("}\n")

            f.write("""
_lr_goto = { }
for _k, _v in _lr_goto_items.items():
   for _x,_y in zip(_v[0],_v[1]):
       if not _lr_goto.has_key(_x): _lr_goto[_x] = { }
       _lr_goto[_x][_k] = _y
del _lr_goto_items
""")
        else:
            f.write("\n_lr_goto = { ");
            for k,v in _lr_goto.items():
                f.write("(%r,%r):%r," % (k[0],k[1],v))
            f.write("}\n");

        
        f.write("_lr_productions = [\n")
        for p in Productions:
            if p:
                if (p.func):
                    f.write("  (%r,%d,%r,%r,%d),\n" % (p.name, p.len, p.func.__name__,p.file,p.line))
                else:
                    f.write("  (%r,%d,None,None,None),\n" % (p.name, p.len))
            else:
                f.write("  None,\n")
        f.write("]\n")

        f.close()

    except IOError,e:
        print >>sys.stderr, "Unable to create '%s'" % filename
        print >>sys.stderr, e
        return

def lr_read_tables(module=tab_module,optimize=0):
    global _lr_action, _lr_goto, _lr_productions, _lr_method
    try:
        if isinstance(module,types.ModuleType):
            parsetab = module
        else:
            exec "import %s as parsetab" % module

        if (optimize) or (Signature.digest() == parsetab._lr_signature):
            _lr_action = parsetab._lr_action
            _lr_goto   = parsetab._lr_goto
            _lr_productions = parsetab._lr_productions
            _lr_method = parsetab._lr_method
            return 1
        else:
            return 0

    except (ImportError,AttributeError):
        return 0








def yacc(method=default_lr, debug=yaccdebug, module=None, tabmodule=tab_module, start=None, check_recursion=1, optimize=0,write_tables=1,debugfile=debug_file,outputdir=''):
    global yaccdebug
    yaccdebug = debug

    initialize_vars()
    files = { }
    error = 0


    
    Signature.update(method)

    
    

    if module:
        
        if isinstance(module, types.ModuleType):
            ldict = module.__dict__
        elif isinstance(module, _INSTANCETYPE):
            _items = [(k,getattr(module,k)) for k in dir(module)]
            ldict = { }
            for i in _items:
                ldict[i[0]] = i[1]
        else:
            raise ValueError,"Expected a module"

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

    
    if not start:
        start = ldict.get("start",None)
    if start:
        Signature.update(start)

    
    ef = ldict.get('p_error',None)
    if ef:
        if isinstance(ef,types.FunctionType):
            ismethod = 0
        elif isinstance(ef, types.MethodType):
            ismethod = 1
        else:
            raise YaccError,"'p_error' defined, but is not a function or method."
        eline = ef.func_code.co_firstlineno
        efile = ef.func_code.co_filename
        files[efile] = None

        if (ef.func_code.co_argcount != 1+ismethod):
            raise YaccError,"%s:%d: p_error() requires 1 argument." % (efile,eline)
        global Errorfunc
        Errorfunc = ef
    else:
        print >>sys.stderr, "yacc: Warning. no p_error() function is defined."

    

    if (optimize and lr_read_tables(tabmodule,1)):
        
        del Productions[:]
        for p in _lr_productions:
            if not p:
                Productions.append(None)
            else:
                m = MiniProduction()
                m.name = p[0]
                m.len  = p[1]
                m.file = p[3]
                m.line = p[4]
                if p[2]:
                    m.func = ldict[p[2]]
                Productions.append(m)

    else:
        
        if (module and isinstance(module,_INSTANCETYPE)):
            tokens = getattr(module,"tokens",None)
        else:
            tokens = ldict.get("tokens",None)

        if not tokens:
            raise YaccError,"module does not define a list 'tokens'"
        if not (isinstance(tokens,types.ListType) or isinstance(tokens,types.TupleType)):
            raise YaccError,"tokens must be a list or tuple."

        
        requires = ldict.get("require",None)
        if requires:
            if not (isinstance(requires,types.DictType)):
                raise YaccError,"require must be a dictionary."

            for r,v in requires.items():
                try:
                    if not (isinstance(v,types.ListType)):
                        raise TypeError
                    v1 = [x.split(".") for x in v]
                    Requires[r] = v1
                except StandardError:
                    print >>sys.stderr, "Invalid specification for rule '%s' in require. Expected a list of strings" % r


        
        
        

        if 'error' in tokens:
            print >>sys.stderr, "yacc: Illegal token 'error'.  Is a reserved word."
            raise YaccError,"Illegal token name"

        for n in tokens:
            if Terminals.has_key(n):
                print >>sys.stderr, "yacc: Warning. Token '%s' multiply defined." % n
            Terminals[n] = [ ]

        Terminals['error'] = [ ]

        
        prec = ldict.get("precedence",None)
        if prec:
            if not (isinstance(prec,types.ListType) or isinstance(prec,types.TupleType)):
                raise YaccError,"precedence must be a list or tuple."
            add_precedence(prec)
            Signature.update(repr(prec))

        for n in tokens:
            if not Precedence.has_key(n):
                Precedence[n] = ('right',0)         

        
        symbols = [ldict[f] for f in ldict.keys()
               if (type(ldict[f]) in (types.FunctionType, types.MethodType) and ldict[f].__name__[:2] == 'p_'
                   and ldict[f].__name__ != 'p_error')]

        
        if len(symbols) == 0:
            raise YaccError,"no rules of the form p_rulename are defined."

        
        symbols.sort(lambda x,y: cmp(x.func_code.co_firstlineno,y.func_code.co_firstlineno))

        
        for f in symbols:
            if (add_function(f)) < 0:
                error += 1
            else:
                files[f.func_code.co_filename] = None

        
        for f in symbols:
            if f.__doc__:
                Signature.update(f.__doc__)

        lr_init_vars()

        if error:
            raise YaccError,"Unable to construct parser."

        if not lr_read_tables(tabmodule):

            
            for filename in files.keys():
                if not validate_file(filename):
                    error = 1

            
            validate_dict(ldict)

            if start and not Prodnames.has_key(start):
                raise YaccError,"Bad starting symbol '%s'" % start

            augment_grammar(start)
            error = verify_productions(cycle_check=check_recursion)
            otherfunc = [ldict[f] for f in ldict.keys()
               if (type(f) in (types.FunctionType,types.MethodType) and ldict[f].__name__[:2] != 'p_')]

            
            if check_precedence():
                error = 1

            if error:
                raise YaccError,"Unable to construct parser."

            build_lritems()
            compute_first1()
            compute_follow(start)

            if method in ['SLR','LALR']:
                lr_parse_table(method)
            else:
                raise YaccError, "Unknown parsing method '%s'" % method

            if write_tables:
                lr_write_tables(tabmodule,outputdir)

            if yaccdebug:
                try:
                    f = open(os.path.join(outputdir,debugfile),"w")
                    f.write(_vfc.getvalue())
                    f.write("\n\n")
                    f.write(_vf.getvalue())
                    f.close()
                except IOError,e:
                    print >>sys.stderr, "yacc: can't create '%s'" % debugfile,e

    
    

    p = Parser("xyzzy")
    p.productions = Productions
    p.errorfunc = Errorfunc
    p.action = _lr_action
    p.goto   = _lr_goto
    p.method = _lr_method
    p.require = Requires

    global parse
    parse = p.parse

    global parser
    parser = p

    
    if (not optimize):
        yacc_cleanup()
    return p




def yacc_cleanup():
    global _lr_action, _lr_goto, _lr_method, _lr_goto_cache
    del _lr_action, _lr_goto, _lr_method, _lr_goto_cache

    global Productions, Prodnames, Prodmap, Terminals
    global Nonterminals, First, Follow, Precedence, UsedPrecedence, LRitems
    global Errorfunc, Signature, Requires

    del Productions, Prodnames, Prodmap, Terminals
    del Nonterminals, First, Follow, Precedence, UsedPrecedence, LRitems
    del Errorfunc, Signature, Requires

    global _vf, _vfc
    del _vf, _vfc



def parse(*args,**kwargs):
    raise YaccError, "yacc: No parser built with yacc()"
