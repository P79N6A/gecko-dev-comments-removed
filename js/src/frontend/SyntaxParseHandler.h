





#ifndef SyntaxParseHandler_h__
#define SyntaxParseHandler_h__

namespace js {
namespace frontend {











class SyntaxParseHandler
{
    
    JSAtom *lastAtom;
    TokenPos lastStringPos;
    TokenStream &tokenStream;

  public:
    enum Node {
        NodeFailure = 0,
        NodeGeneric,
        NodeName,
        NodeGetProp,
        NodeString,
        NodeStringExprStatement,
        NodeLValue
    };
    typedef Definition::Kind DefinitionNode;

    SyntaxParseHandler(JSContext *cx, TokenStream &tokenStream, bool foldConstants,
                       Parser<SyntaxParseHandler> *syntaxParser, LazyScript *lazyOuterFunction)
      : lastAtom(NULL),
        tokenStream(tokenStream)
    {}

    static Node null() { return NodeFailure; }

    void trace(JSTracer *trc) {}

    Node newName(PropertyName *name, ParseContext<SyntaxParseHandler> *pc,
                 ParseNodeKind kind = PNK_NAME) {
        lastAtom = name;
        return NodeName;
    }
    DefinitionNode newPlaceholder(JSAtom *atom, ParseContext<SyntaxParseHandler> *pc) {
        return Definition::PLACEHOLDER;
    }
    Node newAtom(ParseNodeKind kind, JSAtom *atom, JSOp op = JSOP_NOP) {
        if (kind == PNK_STRING) {
            lastAtom = atom;
            lastStringPos = tokenStream.currentToken().pos;
        }
        return NodeString;
    }
    Node newNumber(double value, DecimalPoint decimalPoint, const TokenPos &pos) { return NodeGeneric; }
    Node newBooleanLiteral(bool cond, const TokenPos &pos) { return NodeGeneric; }
    Node newThisLiteral(const TokenPos &pos) { return NodeGeneric; }
    Node newNullLiteral(const TokenPos &pos) { return NodeGeneric; }

    template <class Boxer>
    Node newRegExp(JSObject *reobj, const TokenPos &pos, Boxer &boxer) { return NodeGeneric; }

    Node newConditional(Node cond, Node thenExpr, Node elseExpr) { return NodeGeneric; }

    Node newElision() { return NodeGeneric; }

    Node newUnary(ParseNodeKind kind, Node kid, JSOp op = JSOP_NOP) {
        if (kind == PNK_SEMI && kid == NodeString)
            return NodeStringExprStatement;
        return NodeGeneric;
    }
    Node newUnary(ParseNodeKind kind, JSOp op = JSOP_NOP) { return NodeGeneric; }
    void setUnaryKid(Node pn, Node kid) {}

    Node newBinary(ParseNodeKind kind, JSOp op = JSOP_NOP) { return NodeGeneric; }
    Node newBinary(ParseNodeKind kind, Node left, JSOp op = JSOP_NOP) { return NodeGeneric; }
    Node newBinary(ParseNodeKind kind, Node left, Node right, JSOp op = JSOP_NOP) {
        return NodeGeneric;
    }
    Node newBinaryOrAppend(ParseNodeKind kind, Node left, Node right,
                           ParseContext<SyntaxParseHandler> *pc, JSOp op = JSOP_NOP) {
        return NodeGeneric;
    }

    Node newTernary(ParseNodeKind kind, Node first, Node second, Node third, JSOp op = JSOP_NOP) {
        return NodeGeneric;
    }

    Node newLabeledStatement(PropertyName *label, Node stmt, uint32_t begin) {
        return NodeGeneric;
    }
    Node newCaseOrDefault(uint32_t begin, Node expr, Node body) {
        return NodeGeneric;
    }
    Node newBreak(PropertyName *label, uint32_t begin, uint32_t end) {
        return NodeGeneric;
    }
    Node newContinue(PropertyName *label, uint32_t begin, uint32_t end) {
        return NodeGeneric;
    }
    Node newDebuggerStatement(const TokenPos &pos) { return NodeGeneric; }
    Node newPropertyAccess(Node pn, PropertyName *name, uint32_t end)
    {
        lastAtom = name;
        return NodeGetProp;
    }
    Node newPropertyByValue(Node pn, Node kid, uint32_t end) { return NodeLValue; }

    bool addCatchBlock(Node catchList, Node letBlock,
                       Node catchName, Node catchGuard, Node catchBody) { return true; }

    void setLeaveBlockResult(Node block, Node kid, bool leaveBlockExpr) {}

    void setLastFunctionArgumentDefault(Node funcpn, Node pn) {}
    Node newFunctionDefinition() { return NodeGeneric; }
    void setFunctionBody(Node pn, Node kid) {}
    void setFunctionBox(Node pn, FunctionBox *funbox) {}
    void addFunctionArgument(Node pn, Node argpn) {}
    Node newLexicalScope(ObjectBox *blockbox) { return NodeGeneric; }
    bool isOperationWithoutParens(Node pn, ParseNodeKind kind) {
        
        
        
        return false;
    }

    bool finishInitializerAssignment(Node pn, Node init, JSOp op) { return true; }

    void setBeginPosition(Node pn, Node oth) {}
    void setBeginPosition(Node pn, uint32_t begin) {}

    void setEndPosition(Node pn, Node oth) {}
    void setEndPosition(Node pn, uint32_t end) {}


    void setPosition(Node pn, const TokenPos &pos) {}
    TokenPos getPosition(Node pn) {
        return tokenStream.currentToken().pos;
    }

    Node newList(ParseNodeKind kind, Node kid = NodeGeneric, JSOp op = JSOP_NOP) {
        return NodeGeneric;
    }
    void addList(Node pn, Node kid) {}

    void setOp(Node pn, JSOp op) {}
    void setBlockId(Node pn, unsigned blockid) {}
    void setFlag(Node pn, unsigned flag) {}
    void setListFlag(Node pn, unsigned flag) {}
    Node setInParens(Node pn) {
        
        
        return NodeGeneric;
    }
    void setPrologue(Node pn) {}

    bool isConstant(Node pn) { return false; }
    PropertyName *isName(Node pn) {
        return (pn == NodeName) ? lastAtom->asPropertyName() : NULL;
    }
    PropertyName *isGetProp(Node pn) {
        return (pn == NodeGetProp) ? lastAtom->asPropertyName() : NULL;
    }
    JSAtom *isStringExprStatement(Node pn, TokenPos *pos) {
        if (pn == NodeStringExprStatement) {
            *pos = lastStringPos;
            return lastAtom;
        }
        return NULL;
    }

    Node makeAssignment(Node pn, Node rhs) { return NodeGeneric; }

    static Node getDefinitionNode(DefinitionNode dn) { return NodeGeneric; }
    static Definition::Kind getDefinitionKind(DefinitionNode dn) { return dn; }
    void linkUseToDef(Node pn, DefinitionNode dn) {}
    DefinitionNode resolve(DefinitionNode dn) { return dn; }
    void deoptimizeUsesWithin(DefinitionNode dn, const TokenPos &pos) {}
    bool dependencyCovered(Node pn, unsigned blockid, bool functionScope) {
        
        
        
        return functionScope;
    }

    static uintptr_t definitionToBits(DefinitionNode dn) {
        
        return uintptr_t(dn << 1);
    }
    static DefinitionNode definitionFromBits(uintptr_t bits) {
        return (DefinitionNode) (bits >> 1);
    }
    static DefinitionNode nullDefinition() {
        return Definition::MISSING;
    }
    void disableSyntaxParser() {
    }
};

} 
} 

#endif 
