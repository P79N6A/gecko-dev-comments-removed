





#ifndef frontend_SyntaxParseHandler_h
#define frontend_SyntaxParseHandler_h

#include "mozilla/Attributes.h"

#include "frontend/ParseNode.h"
#include "frontend/TokenStream.h"

namespace js {
namespace frontend {

template <typename ParseHandler>
class Parser;











class SyntaxParseHandler
{
    
    JSAtom* lastAtom;
    TokenPos lastStringPos;
    TokenStream& tokenStream;

  public:
    enum Node {
        NodeFailure = 0,
        NodeGeneric,
        NodeGetProp,
        NodeStringExprStatement,
        NodeReturn,
        NodeHoistableDeclaration,
        NodeBreak,
        NodeThrow,

        
        
        
        
        
        NodeFunctionCall,

        
        NodeArgumentsName,
        NodeEvalName,
        NodeName,

        NodeDottedProperty,
        NodeElement,
        NodeSuperProperty,
        NodeSuperElement,

        
        NodeArray,
        NodeObject,

        
        
        
        
        

        
        
        
        
        
        NodeUnparenthesizedString,

        
        
        
        
        
        
        NodeUnparenthesizedCommaExpr,

        
        
        
        
        
        
        NodeUnparenthesizedYieldExpr,

        
        
        
        
        
        
        
        
        
        NodeUnparenthesizedAssignment
    };
    typedef Definition::Kind DefinitionNode;

    bool isPropertyAccess(Node node) {
        return node == NodeDottedProperty || node == NodeElement ||
               node == NodeSuperProperty || node == NodeSuperElement;
    }

    bool isFunctionCall(Node node) {
        
        return node == NodeFunctionCall;
    }

    bool isDestructuringTarget(Node node) {
        return node == NodeArray || node == NodeObject;
    }

  private:
    static bool meaningMightChangeIfParenthesized(Node node) {
        return node == NodeUnparenthesizedString ||
               node == NodeUnparenthesizedCommaExpr ||
               node == NodeUnparenthesizedYieldExpr ||
               node == NodeUnparenthesizedAssignment;
    }


  public:
    SyntaxParseHandler(ExclusiveContext* cx, LifoAlloc& alloc,
                       TokenStream& tokenStream, Parser<SyntaxParseHandler>* syntaxParser,
                       LazyScript* lazyOuterFunction)
      : lastAtom(nullptr),
        tokenStream(tokenStream)
    {}

    static Node null() { return NodeFailure; }

    void trace(JSTracer* trc) {}

    Node newName(PropertyName* name, uint32_t blockid, const TokenPos& pos, ExclusiveContext* cx) {
        lastAtom = name;
        if (name == cx->names().arguments)
            return NodeArgumentsName;
        if (name == cx->names().eval)
            return NodeEvalName;
        return NodeName;
    }

    Node newComputedName(Node expr, uint32_t start, uint32_t end) {
        return NodeName;
    }

    DefinitionNode newPlaceholder(JSAtom* atom, uint32_t blockid, const TokenPos& pos) {
        return Definition::PLACEHOLDER;
    }

    Node newObjectLiteralPropertyName(JSAtom* atom, const TokenPos& pos) {
        return NodeName;
    }

    Node newNumber(double value, DecimalPoint decimalPoint, const TokenPos& pos) { return NodeGeneric; }
    Node newBooleanLiteral(bool cond, const TokenPos& pos) { return NodeGeneric; }

    Node newStringLiteral(JSAtom* atom, const TokenPos& pos) {
        lastAtom = atom;
        lastStringPos = pos;
        return NodeUnparenthesizedString;
    }

    Node newTemplateStringLiteral(JSAtom* atom, const TokenPos& pos) {
        return NodeGeneric;
    }

    Node newCallSiteObject(uint32_t begin, unsigned blockidGen) {
        return NodeGeneric;
    }

    bool addToCallSiteObject(Node callSiteObj, Node rawNode, Node cookedNode) {
        return true;
    }

    Node newThisLiteral(const TokenPos& pos) { return NodeGeneric; }
    Node newNullLiteral(const TokenPos& pos) { return NodeGeneric; }

    template <class Boxer>
    Node newRegExp(RegExpObject* reobj, const TokenPos& pos, Boxer& boxer) { return NodeGeneric; }

    Node newConditional(Node cond, Node thenExpr, Node elseExpr) { return NodeGeneric; }

    Node newElision() { return NodeGeneric; }

    void markAsSetCall(Node node) {
        MOZ_ASSERT(node == NodeFunctionCall);
    }

    Node newDelete(uint32_t begin, Node expr) {
        return NodeGeneric;
    }

    Node newTypeof(uint32_t begin, Node kid) {
        return NodeGeneric;
    }

    Node newUnary(ParseNodeKind kind, JSOp op, uint32_t begin, Node kid) {
        return NodeGeneric;
    }

    Node newBinary(ParseNodeKind kind, JSOp op = JSOP_NOP) { return NodeGeneric; }
    Node newBinary(ParseNodeKind kind, Node left, JSOp op = JSOP_NOP) { return NodeGeneric; }
    Node newBinary(ParseNodeKind kind, Node left, Node right, JSOp op = JSOP_NOP) {
        return NodeGeneric;
    }
    Node appendOrCreateList(ParseNodeKind kind, Node left, Node right,
                            ParseContext<SyntaxParseHandler>* pc, JSOp op = JSOP_NOP) {
        return NodeGeneric;
    }

    Node newTernary(ParseNodeKind kind, Node first, Node second, Node third, JSOp op = JSOP_NOP) {
        return NodeGeneric;
    }

    

    Node newArrayComprehension(Node body, unsigned blockid, const TokenPos& pos) {
        return NodeGeneric;
    }
    Node newArrayLiteral(uint32_t begin, unsigned blockid) { return NodeArray; }
    bool addElision(Node literal, const TokenPos& pos) { return true; }
    bool addSpreadElement(Node literal, uint32_t begin, Node inner) { return true; }
    void addArrayElement(Node literal, Node element) { }

    Node newCall() { return NodeFunctionCall; }
    Node newTaggedTemplate() { return NodeGeneric; }

    Node newObjectLiteral(uint32_t begin) { return NodeObject; }
    Node newClassMethodList(uint32_t begin) { return NodeGeneric; }

    Node newSuperProperty(PropertyName* prop, const TokenPos& pos) {
        return NodeSuperProperty;
    }

    Node newSuperElement(Node expr, const TokenPos& pos) {
        return NodeSuperElement;
    }
    Node newNewTarget(const TokenPos& pos) { return NodeGeneric; }

    bool addPrototypeMutation(Node literal, uint32_t begin, Node expr) { return true; }
    bool addPropertyDefinition(Node literal, Node name, Node expr) { return true; }
    bool addShorthand(Node literal, Node name, Node expr) { return true; }
    bool addObjectMethodDefinition(Node literal, Node name, Node fn, JSOp op) { return true; }
    bool addClassMethodDefinition(Node literal, Node name, Node fn, JSOp op, bool isStatic) { return true; }
    Node newYieldExpression(uint32_t begin, Node value, Node gen) { return NodeUnparenthesizedYieldExpr; }
    Node newYieldStarExpression(uint32_t begin, Node value, Node gen) { return NodeGeneric; }

    

    Node newStatementList(unsigned blockid, const TokenPos& pos) { return NodeGeneric; }
    void addStatementToList(Node list, Node stmt, ParseContext<SyntaxParseHandler>* pc) {}
    bool prependInitialYield(Node stmtList, Node gen) { return true; }
    Node newEmptyStatement(const TokenPos& pos) { return NodeGeneric; }

    Node newExprStatement(Node expr, uint32_t end) {
        return expr == NodeUnparenthesizedString ? NodeStringExprStatement : NodeGeneric;
    }

    Node newIfStatement(uint32_t begin, Node cond, Node then, Node else_) { return NodeGeneric; }
    Node newDoWhileStatement(Node body, Node cond, const TokenPos& pos) { return NodeGeneric; }
    Node newWhileStatement(uint32_t begin, Node cond, Node body) { return NodeGeneric; }
    Node newSwitchStatement(uint32_t begin, Node discriminant, Node caseList) { return NodeGeneric; }
    Node newCaseOrDefault(uint32_t begin, Node expr, Node body) { return NodeGeneric; }
    Node newContinueStatement(PropertyName* label, const TokenPos& pos) { return NodeGeneric; }
    Node newBreakStatement(PropertyName* label, const TokenPos& pos) { return NodeBreak; }
    Node newReturnStatement(Node expr, Node genrval, const TokenPos& pos) { return NodeReturn; }

    Node newLabeledStatement(PropertyName* label, Node stmt, uint32_t begin) {
        return NodeGeneric;
    }

    Node newThrowStatement(Node expr, const TokenPos& pos) { return NodeThrow; }
    Node newTryStatement(uint32_t begin, Node body, Node catchList, Node finallyBlock) {
        return NodeGeneric;
    }
    Node newDebuggerStatement(const TokenPos& pos) { return NodeGeneric; }

    Node newPropertyAccess(Node pn, PropertyName* name, uint32_t end) {
        lastAtom = name;
        return NodeDottedProperty;
    }

    Node newPropertyByValue(Node pn, Node kid, uint32_t end) { return NodeElement; }

    bool addCatchBlock(Node catchList, Node letBlock,
                       Node catchName, Node catchGuard, Node catchBody) { return true; }

    bool setLastFunctionArgumentDefault(Node funcpn, Node pn) { return true; }
    void setLastFunctionArgumentDestructuring(Node funcpn, Node pn) {}
    Node newFunctionDefinition() { return NodeHoistableDeclaration; }
    void setFunctionBody(Node pn, Node kid) {}
    void setFunctionBox(Node pn, FunctionBox* funbox) {}
    void addFunctionArgument(Node pn, Node argpn) {}

    Node newForStatement(uint32_t begin, Node forHead, Node body, unsigned iflags) {
        return NodeGeneric;
    }

    Node newForHead(ParseNodeKind kind, Node decls, Node lhs, Node rhs, const TokenPos& pos) {
        return NodeGeneric;
    }

    Node newLexicalScope(ObjectBox* blockbox) { return NodeGeneric; }
    void setLexicalScopeBody(Node block, Node body) {}

    Node newLetBlock(Node vars, Node block, const TokenPos& pos) {
        return NodeGeneric;
    }

    bool finishInitializerAssignment(Node pn, Node init, JSOp op) { return true; }

    void setBeginPosition(Node pn, Node oth) {}
    void setBeginPosition(Node pn, uint32_t begin) {}

    void setEndPosition(Node pn, Node oth) {}
    void setEndPosition(Node pn, uint32_t end) {}


    void setPosition(Node pn, const TokenPos& pos) {}
    TokenPos getPosition(Node pn) {
        return tokenStream.currentToken().pos;
    }

    Node newList(ParseNodeKind kind, JSOp op = JSOP_NOP) {
        MOZ_ASSERT(kind != PNK_VAR);
        return NodeGeneric;
    }
    Node newList(ParseNodeKind kind, uint32_t begin, JSOp op = JSOP_NOP) {
        return NodeGeneric;
    }
    Node newDeclarationList(ParseNodeKind kind, JSOp op = JSOP_NOP) {
        MOZ_ASSERT(kind == PNK_VAR || kind == PNK_CONST || kind == PNK_LET ||
                   kind == PNK_GLOBALCONST);
        return kind == PNK_VAR ? NodeHoistableDeclaration : NodeGeneric;
    }
    Node newList(ParseNodeKind kind, Node kid, JSOp op = JSOP_NOP) {
        MOZ_ASSERT(kind != PNK_VAR);
        return NodeGeneric;
    }
    Node newDeclarationList(ParseNodeKind kind, Node kid, JSOp op = JSOP_NOP) {
        MOZ_ASSERT(kind == PNK_VAR || kind == PNK_CONST || kind == PNK_LET ||
                   kind == PNK_GLOBALCONST);
        return kind == PNK_VAR ? NodeHoistableDeclaration : NodeGeneric;
    }

    Node newCatchList() {
        return newList(PNK_CATCHLIST, JSOP_NOP);
    }

    Node newCommaExpressionList(Node kid) {
        return NodeUnparenthesizedCommaExpr;
    }

    void addList(Node list, Node kid) {
        MOZ_ASSERT(list == NodeGeneric ||
                   list == NodeArray ||
                   list == NodeObject ||
                   list == NodeUnparenthesizedCommaExpr ||
                   list == NodeHoistableDeclaration ||
                   list == NodeFunctionCall);
    }

    Node newAssignment(ParseNodeKind kind, Node lhs, Node rhs,
                       ParseContext<SyntaxParseHandler>* pc, JSOp op)
    {
        if (kind == PNK_ASSIGN)
            return NodeUnparenthesizedAssignment;
        return newBinary(kind, lhs, rhs, op);
    }

    bool isUnparenthesizedYieldExpression(Node node) {
        return node == NodeUnparenthesizedYieldExpr;
    }

    bool isUnparenthesizedCommaExpression(Node node) {
        return node == NodeUnparenthesizedCommaExpr;
    }

    bool isUnparenthesizedAssignment(Node node) {
        return node == NodeUnparenthesizedAssignment;
    }

    bool isReturnStatement(Node node) {
        return node == NodeReturn;
    }

    bool isStatementPermittedAfterReturnStatement(Node pn) {
        return pn == NodeHoistableDeclaration || pn == NodeBreak || pn == NodeThrow;
    }

    void setOp(Node pn, JSOp op) {}
    void setBlockId(Node pn, unsigned blockid) {}
    void setFlag(Node pn, unsigned flag) {}
    void setListFlag(Node pn, unsigned flag) {}
    MOZ_WARN_UNUSED_RESULT Node parenthesize(Node node) {
        if (meaningMightChangeIfParenthesized(node))
            return NodeGeneric;

        
        
        return node;
    }
    MOZ_WARN_UNUSED_RESULT Node setLikelyIIFE(Node pn) {
        return pn; 
    }
    void setPrologue(Node pn) {}

    bool isConstant(Node pn) { return false; }
    PropertyName* maybeName(Node pn) {
        if (pn == NodeName || pn == NodeArgumentsName || pn == NodeEvalName)
            return lastAtom->asPropertyName();
        return nullptr;
    }

    PropertyName* maybeDottedProperty(Node node) {
        
        
        
        
        
        if (node != NodeDottedProperty)
            return nullptr;
        return lastAtom->asPropertyName();
    }

    JSAtom* isStringExprStatement(Node pn, TokenPos* pos) {
        if (pn == NodeStringExprStatement) {
            *pos = lastStringPos;
            return lastAtom;
        }
        return nullptr;
    }

    void markAsAssigned(Node node) {}
    void adjustGetToSet(Node node) {}
    void maybeDespecializeSet(Node node) {}

    Node makeAssignment(Node pn, Node rhs) { return NodeGeneric; }

    static Node getDefinitionNode(DefinitionNode dn) { return NodeGeneric; }
    static Definition::Kind getDefinitionKind(DefinitionNode dn) { return dn; }
    static bool isPlaceholderDefinition(DefinitionNode dn) { return dn == Definition::PLACEHOLDER; }
    void linkUseToDef(Node pn, DefinitionNode dn) {}
    DefinitionNode resolve(DefinitionNode dn) { return dn; }
    void deoptimizeUsesWithin(DefinitionNode dn, const TokenPos& pos) {}
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
