





#ifndef frontend_SyntaxParseHandler_h
#define frontend_SyntaxParseHandler_h

#include "frontend/ParseNode.h"
#include "frontend/TokenStream.h"

namespace js {
namespace frontend {

template <typename ParseHandler>
class Parser;











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

    SyntaxParseHandler(ExclusiveContext *cx, LifoAlloc &alloc,
                       TokenStream &tokenStream, bool foldConstants,
                       Parser<SyntaxParseHandler> *syntaxParser, LazyScript *lazyOuterFunction)
      : lastAtom(nullptr),
        tokenStream(tokenStream)
    {}

    static Node null() { return NodeFailure; }

    void trace(JSTracer *trc) {}

    Node newName(PropertyName *name, uint32_t blockid, const TokenPos &pos) {
        lastAtom = name;
        return NodeName;
    }

    Node newComputedName(Node expr, uint32_t start, uint32_t end) {
        return NodeName;
    }

    DefinitionNode newPlaceholder(JSAtom *atom, uint32_t blockid, const TokenPos &pos) {
        return Definition::PLACEHOLDER;
    }

    Node newIdentifier(JSAtom *atom, const TokenPos &pos) { return NodeString; }
    Node newNumber(double value, DecimalPoint decimalPoint, const TokenPos &pos) { return NodeGeneric; }
    Node newBooleanLiteral(bool cond, const TokenPos &pos) { return NodeGeneric; }

    Node newStringLiteral(JSAtom *atom, const TokenPos &pos) {
        lastAtom = atom;
        lastStringPos = pos;
        return NodeString;
    }

    Node newTemplateStringLiteral(JSAtom *atom, const TokenPos &pos) {
        return NodeGeneric;
    }

    Node newCallSiteObject(uint32_t begin, unsigned blockidGen) {
        return NodeGeneric;
    }

    bool addToCallSiteObject(Node callSiteObj, Node rawNode, Node cookedNode) {
        return true;
    }

    Node newThisLiteral(const TokenPos &pos) { return NodeGeneric; }
    Node newNullLiteral(const TokenPos &pos) { return NodeGeneric; }

    template <class Boxer>
    Node newRegExp(JSObject *reobj, const TokenPos &pos, Boxer &boxer) { return NodeGeneric; }

    Node newConditional(Node cond, Node thenExpr, Node elseExpr) { return NodeGeneric; }

    Node newElision() { return NodeGeneric; }

    Node newDelete(uint32_t begin, Node expr) { return NodeGeneric; }

    Node newUnary(ParseNodeKind kind, JSOp op, uint32_t begin, Node kid) {
        return NodeGeneric;
    }

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

    

    Node newArrayComprehension(Node body, unsigned blockid, const TokenPos &pos) {
        return NodeGeneric;
    }
    Node newArrayLiteral(uint32_t begin, unsigned blockid) { return NodeGeneric; }
    bool addElision(Node literal, const TokenPos &pos) { return true; }
    bool addSpreadElement(Node literal, uint32_t begin, Node inner) { return true; }
    bool addArrayElement(Node literal, Node element) { return true; }

    Node newObjectLiteral(uint32_t begin) { return NodeGeneric; }
    bool addPrototypeMutation(Node literal, uint32_t begin, Node expr) { return true; }
    bool addPropertyDefinition(Node literal, Node name, Node expr, bool isShorthand = false) { return true; }
    bool addMethodDefinition(Node literal, Node name, Node fn, JSOp op) { return true; }

    

    Node newStatementList(unsigned blockid, const TokenPos &pos) { return NodeGeneric; }
    void addStatementToList(Node list, Node stmt, ParseContext<SyntaxParseHandler> *pc) {}
    Node newEmptyStatement(const TokenPos &pos) { return NodeGeneric; }

    Node newExprStatement(Node expr, uint32_t end) {
        return expr == NodeString ? NodeStringExprStatement : NodeGeneric;
    }

    Node newIfStatement(uint32_t begin, Node cond, Node then, Node else_) { return NodeGeneric; }
    Node newDoWhileStatement(Node body, Node cond, const TokenPos &pos) { return NodeGeneric; }
    Node newWhileStatement(uint32_t begin, Node cond, Node body) { return NodeGeneric; }
    Node newSwitchStatement(uint32_t begin, Node discriminant, Node caseList) { return NodeGeneric; }
    Node newCaseOrDefault(uint32_t begin, Node expr, Node body) { return NodeGeneric; }
    Node newContinueStatement(PropertyName *label, const TokenPos &pos) { return NodeGeneric; }
    Node newBreakStatement(PropertyName *label, const TokenPos &pos) { return NodeGeneric; }
    Node newReturnStatement(Node expr, const TokenPos &pos) { return NodeGeneric; }

    Node newLabeledStatement(PropertyName *label, Node stmt, uint32_t begin) {
        return NodeGeneric;
    }

    Node newThrowStatement(Node expr, const TokenPos &pos) { return NodeGeneric; }
    Node newTryStatement(uint32_t begin, Node body, Node catchList, Node finallyBlock) {
        return NodeGeneric;
    }
    Node newDebuggerStatement(const TokenPos &pos) { return NodeGeneric; }

    Node newPropertyAccess(Node pn, PropertyName *name, uint32_t end) {
        lastAtom = name;
        return NodeGetProp;
    }

    Node newPropertyByValue(Node pn, Node kid, uint32_t end) { return NodeLValue; }

    bool addCatchBlock(Node catchList, Node letBlock,
                       Node catchName, Node catchGuard, Node catchBody) { return true; }

    void setLastFunctionArgumentDefault(Node funcpn, Node pn) {}
    Node newFunctionDefinition() { return NodeGeneric; }
    void setFunctionBody(Node pn, Node kid) {}
    void setFunctionBox(Node pn, FunctionBox *funbox) {}
    void addFunctionArgument(Node pn, Node argpn) {}

    Node newForStatement(uint32_t begin, Node forHead, Node body, unsigned iflags) {
        return NodeGeneric;
    }

    Node newForHead(ParseNodeKind kind, Node decls, Node lhs, Node rhs, const TokenPos &pos) {
        return NodeGeneric;
    }

    Node newLexicalScope(ObjectBox *blockbox) { return NodeGeneric; }
    void setLexicalScopeBody(Node block, Node body) {}

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
    bool isUnparenthesizedYield(Node pn) { return false; }

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
        return (pn == NodeName) ? lastAtom->asPropertyName() : nullptr;
    }
    PropertyName *isGetProp(Node pn) {
        return (pn == NodeGetProp) ? lastAtom->asPropertyName() : nullptr;
    }
    JSAtom *isStringExprStatement(Node pn, TokenPos *pos) {
        if (pn == NodeStringExprStatement) {
            *pos = lastStringPos;
            return lastAtom;
        }
        return nullptr;
    }

    Node makeAssignment(Node pn, Node rhs) { return NodeGeneric; }

    static Node getDefinitionNode(DefinitionNode dn) { return NodeGeneric; }
    static Definition::Kind getDefinitionKind(DefinitionNode dn) { return dn; }
    static bool isPlaceholderDefinition(DefinitionNode dn) { return dn == Definition::PLACEHOLDER; }
    void linkUseToDef(Node pn, DefinitionNode dn) {}
    DefinitionNode resolve(DefinitionNode dn) { return dn; }
    void deoptimizeUsesWithin(DefinitionNode dn, const TokenPos &pos) {}
    bool dependencyCovered(Node pn, unsigned blockid, bool functionScope) {
        
        
        
        return functionScope;
    }
    void markMaybeUninitializedLexicalUseInSwitch(Node pn, DefinitionNode dn,
                                                  uint16_t firstDominatingLexicalSlot) {}

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
