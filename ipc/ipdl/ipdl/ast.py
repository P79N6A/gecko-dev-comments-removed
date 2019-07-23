































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

    def visitTransitionStmt(self, ts):
        ts.state.accept(self)
        for trans in ts.transitions:
            trans.accept(self)

    def visitTransition(self, t):
        t.toState.accept(self)

    def visitState(self, s):
        pass

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
    pretty = 'async'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)
    @classmethod
    def __str__(cls):  return cls.pretty
class RPC:
    pretty = 'rpc'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)
    @classmethod
    def __str__(cls):  return cls.pretty
class SYNC:
    pretty = 'sync'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)
    @classmethod
    def __str__(cls):  return cls.pretty

class INOUT:
    pretty = 'inout'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)
    @classmethod
    def __str__(cls):  return cls.pretty
class IN:
    pretty = 'in'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)
    @classmethod
    def __str__(cls):  return cls.pretty
    @staticmethod
    def prettySS(cls, ss): return _prettyTable['in'][ss.pretty]
class OUT:
    pretty = 'out'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)
    @classmethod
    def __str__(cls):  return cls.pretty
    @staticmethod
    def pretty(ss): return _prettyTable['out'][ss.pretty]

_prettyTable = {
    IN  : { 'async': 'AsyncRecv',
            'sync': 'SyncRecv',
            'rpc': 'RpcAnswer' },
    OUT : { 'async': 'AsyncSend',
            'sync': 'SyncSend',
            'rpc': 'RpcCall' }
    
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

class TransitionStmt(Node):
    def __init__(self, loc, state, transitions):
        Node.__init__(self, loc)
        self.state = state
        self.transitions = transitions

class Transition(Node):
    def __init__(self, loc, trigger, msg, toState):
        Node.__init__(self, loc)
        self.trigger = trigger
        self.msg = msg
        self.toState = toState

    @staticmethod
    def nameToTrigger(name):
        return { 'send': SEND, 'recv': RECV, 'call': CALL, 'answer': ANSWER }[name]

class SEND:
    pretty = 'send'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)
class RECV:
    pretty = 'recv'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)
class CALL:
    pretty = 'call'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)
class ANSWER:
    pretty = 'answer'
    @classmethod
    def __hash__(cls): return hash(cls.pretty)

class State(Node):
    def __init__(self, loc, name):
        Node.__init__(self, loc)
        self.name = name

class Param(Node):
    def __init__(self, loc, typespec, name):
        Node.__init__(self, loc)
        self.name = name
        self.typespec = typespec

class TypeSpec(Node):
    def __init__(self, loc, spec, state=None):
        Node.__init__(self, loc)
        self.spec = spec
        self.state = state

    def basename(self):
        return self.spec.baseid

    def isActor(self):
        return self.state is not None

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


