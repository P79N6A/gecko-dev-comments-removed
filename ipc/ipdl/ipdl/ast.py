































import sys

class Visitor:
    def defaultVisit(self, node):
        raise Exception, "INTERNAL ERROR: no visitor for node type `%s'"% (
            node.__class__.__name__)

    def visitTranslationUnit(self, tu):
        for cxxInc in tu.cxxIncludes:
            cxxInc.accept(self)
        for protoInc in tu.protocolIncludes:
            protoInc.accept(self)
        for using in tu.using:
            using.accept(self)
        tu.protocol.accept(self)

    def visitCxxInclude(self, inc):
        pass

    def visitProtocolInclude(self, inc):
        
        
        pass

    def visitUsingStmt(self, using):
        pass

    def visitProtocol(self, p):
        for namespace in p.namespaces:
            namespace.accept(self)
        if p.manager is not None:
            p.manager.accept(self)
        for managed in p.managesStmts:
            managed.accept(self)
        for msgDecl in p.messageDecls:
            msgDecl.accept(self)
        for transitionStmt in p.transitionStmts:
            transitionStmt.accept(self)

    def visitNamespace(self, ns):
        pass

    def visitManagerStmt(self, mgr):
        pass

    def visitManagesStmt(self, mgs):
        pass

    def visitMessageDecl(self, md):
        for inParam in md.inParams:
            inParam.accept(self)
        for outParam in md.outParams:
            outParam.accept(self)

    def visitParam(self, decl):
        pass

    def visitTypeSpec(self, ts):
        pass

    def visitDecl(self, d):
        pass

class Loc:
    def __init__(self, filename='<??>', lineno=0):
        assert filename
        self.filename = filename
        self.lineno = lineno
    def __repr__(self):
        return '%r:%r'% (self.filename, self.lineno)
    def __str__(self):
        return '%s:%s'% (self.filename, self.lineno)

Loc.NONE = Loc(filename='<??>', lineno=0)

class _struct():
    pass

class Node:
    def __init__(self, loc=Loc.NONE):
        self.loc = loc

    def accept(self, visitor):
        visit = getattr(visitor, 'visit'+ self.__class__.__name__, None)
        if visit is None:
            return getattr(visitor, 'defaultVisit')(self)
        return visit(self)

    def addAttrs(self, attrsName):
        if not hasattr(self, attrsName):
            setattr(self, attrsName, _struct())

class TranslationUnit(Node):
    def __init__(self):
        Node.__init__(self)
        self.filename = None
        self.cxxIncludes = [ ]
        self.protocolIncludes = [ ]
        self.using = [ ]
        self.protocol = None

    def addCxxInclude(self, cxxInclude): self.cxxIncludes.append(cxxInclude)
    def addProtocolInclude(self, pInc): self.protocolIncludes.append(pInc)
    def addUsingStmt(self, using): self.using.append(using)

    def setProtocol(self, protocol): self.protocol = protocol

class CxxInclude(Node):
    def __init__(self, loc, cxxFile):
        Node.__init__(self, loc)
        self.file = cxxFile

class ProtocolInclude(Node):
    def __init__(self, loc, protocolFile):
        Node.__init__(self, loc)
        self.file = protocolFile

class UsingStmt(Node):
    def __init__(self, loc, cxxTypeSpec):
        Node.__init__(self, loc)
        self.type = cxxTypeSpec
        

class ASYNC:
    pretty = 'Async'
class RPC:
    pretty = 'Rpc'
class SYNC:
    pretty = 'Sync'

class INOUT:
    pretty = 'InOut'
class IN:
    @staticmethod
    def pretty(ss): return _prettyTable['In'][ss.pretty]
class OUT:
    @staticmethod
    def pretty(ss): return _prettyTable['Out'][ss.pretty]

_prettyTable = {
    'In'  : { 'Async': 'AsyncRecv',
             'Sync': 'SyncRecv',
             'Rpc': 'RpcAnswer' },
    'Out' : { 'Async': 'AsyncSend',
              'Sync': 'SyncSend',
              'Rpc': 'RpcCall' }
    
}


class Protocol(Node):
    def __init__(self, loc):
        Node.__init__(self, loc)
        self.name = None
        self.namespaces = [ ]
        self.sendSemantics = ASYNC
        self.managesStmts = [ ]
        self.messageDecls = [ ]
        self.transitionStmts = [ ]

    def addOuterNamespace(self, namespace):
        self.namespaces.insert(0, namespace)

    def addManagesStmts(self, managesStmts):
        self.managesStmts += managesStmts

    def addMessageDecls(self, messageDecls):
        self.messageDecls += messageDecls

    def addTransitionStmts(self, transStmts):
        self.transitionStmts += transStmts

class Namespace(Node):
    def __init__(self, loc, namespace):
        Node.__init__(self, loc)
        self.namespace = namespace

class ManagerStmt(Node):
    def __init__(self, loc, managerName):
        Node.__init__(self, loc)
        self.name = managerName

class ManagesStmt(Node):
    def __init__(self, loc, managedName):
        Node.__init__(self, loc)
        self.name = managedName

class MessageDecl(Node):
    def __init__(self, loc):
        Node.__init__(self, loc)
        self.name = None
        self.sendSemantics = ASYNC
        self.direction = None
        self.inParams = [ ]
        self.outParams = [ ]

    def addInParams(self, inParamsList):
        self.inParams += inParamsList

    def addOutParams(self, outParamsList):
        self.outParams += outParamsList

    def hasReply(self):
        return self.sendSemantics is SYNC or self.sendSemantics is RPC

class Param(Node):
    def __init__(self, loc, typespec, name):
        Node.__init__(self, loc)
        self.name = name
        self.typespec = typespec

class TypeSpec(Node):
    def __init__(self, loc, spec):
        Node.__init__(self, loc)
        self.spec = spec

    def basename(self):
        return self.spec.baseid

    def __str__(self):  return str(self.spec)

class QualifiedId:              
    def __init__(self, loc, baseid, quals=[ ]):
        self.loc = loc
        self.baseid = baseid
        self.quals = quals

    def qualify(self, id):
        self.quals.append(self.baseid)
        self.baseid = id

    def __str__(self):
        if 0 == len(self.quals):
            return self.baseid
        return '::'.join(self.quals) +'::'+ self.baseid


class Decl(Node):
    def __init__(self, loc):
        Node.__init__(self, loc)
        self.progname = None    
        self.shortname = None   
        self.fullname = None    
        self.loc = loc
        self.type = None
        self.scope = None


