




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
  "Reflect", "resource://gre/modules/reflect.jsm");

this.EXPORTED_SYMBOLS = ["Parser"];




this.Parser = function Parser() {
  this._cache = new Map();
};

Parser.prototype = {
  








  get: function P_get(aUrl, aSource) {
    
    if (this._cache.has(aUrl)) {
      return this._cache.get(aUrl);
    }

    
    
    
    
    let regexp = /<script[^>]*>([^]*?)<\/script\s*>/gim;
    let syntaxTrees = [];
    let scriptMatches = [];
    let scriptMatch;

    if (aSource.match(/^\s*</)) {
      
      while (scriptMatch = regexp.exec(aSource)) {
        scriptMatches.push(scriptMatch[1]); 
      }
    }

    
    
    if (!scriptMatches.length) {
      
      try {
        let nodes = Reflect.parse(aSource);
        let length = aSource.length;
        syntaxTrees.push(new SyntaxTree(nodes, aUrl, length));
      } catch (e) {
        log(aUrl, e);
      }
    }
    
    else {
      for (let script of scriptMatches) {
        
        try {
          let nodes = Reflect.parse(script);
          let offset = aSource.indexOf(script);
          let length = script.length;
          syntaxTrees.push(new SyntaxTree(nodes, aUrl, length, offset));
        } catch (e) {
          log(aUrl, e);
        }
      }
    }

    let pool = new SyntaxTreesPool(syntaxTrees);
    this._cache.set(aUrl, pool);
    return pool;
  },

  


  clearCache: function P_clearCache() {
    this._cache = new Map();
  },

  _cache: null
};







function SyntaxTreesPool(aSyntaxTrees) {
  this._trees = aSyntaxTrees;
  this._cache = new Map();
}

SyntaxTreesPool.prototype = {
  


  getNamedFunctionDefinitions: function STP_getNamedFunctionDefinitions(aSubstring) {
    return this._call("getNamedFunctionDefinitions", aSubstring);
  },

  


  getFunctionAtLocation: function STP_getFunctionAtLocation(aLine, aColumn) {
    return this._call("getFunctionAtLocation", [aLine, aColumn]);
  },

  








  getScriptInfo: function STP_getScriptInfo(aOffset) {
    for (let { offset, length } of this._trees) {
      if (offset <= aOffset &&  offset + length >= aOffset) {
        return [offset, length];
      }
    }
    return [-1, -1];
  },

  









  _call: function STP__call(aFunction, aParams) {
    let results = [];
    let requestId = aFunction + aParams; 

    if (this._cache.has(requestId)) {
      return this._cache.get(requestId);
    }
    for (let syntaxTree of this._trees) {
      try {
        results.push({
          sourceUrl: syntaxTree.url,
          scriptLength: syntaxTree.length,
          scriptOffset: syntaxTree.offset,
          parseResults: syntaxTree[aFunction](aParams)
        });
      } catch (e) {
        
        
        
        log("syntax tree", e);
      }
    }
    this._cache.set(requestId, results);
    return results;
  },

  _trees: null,
  _cache: null
};













function SyntaxTree(aNodes, aUrl, aLength, aOffset = 0) {
  this.AST = aNodes;
  this.url = aUrl;
  this.length = aLength;
  this.offset = aOffset;
};

SyntaxTree.prototype = {
  










  getNamedFunctionDefinitions: function ST_getNamedFunctionDefinitions(aSubstring) {
    let lowerCaseToken = aSubstring.toLowerCase();
    let store = [];

    SyntaxTreeVisitor.walk(this.AST, {
      



      onFunctionDeclaration: function STW_onFunctionDeclaration(aNode) {
        let functionName = aNode.id.name;
        if (functionName.toLowerCase().contains(lowerCaseToken)) {
          store.push({
            functionName: functionName,
            functionLocation: aNode.loc
          });
        }
      },

      



      onFunctionExpression: function STW_onFunctionExpression(aNode) {
        let parent = aNode._parent;
        let functionName, inferredName, inferredChain, inferredLocation;

        
        if (aNode.id) {
          functionName = aNode.id.name;
        }
        
        if (parent) {
          let inferredInfo = ParserHelpers.inferFunctionExpressionInfo(aNode);
          inferredName = inferredInfo.name;
          inferredChain = inferredInfo.chain;
          inferredLocation = inferredInfo.loc;
        }
        
        if (parent.type == "AssignmentExpression") {
          this.onFunctionExpression(parent);
        }

        if ((functionName && functionName.toLowerCase().contains(lowerCaseToken)) ||
            (inferredName && inferredName.toLowerCase().contains(lowerCaseToken))) {
          store.push({
            functionName: functionName,
            functionLocation: aNode.loc,
            inferredName: inferredName,
            inferredChain: inferredChain,
            inferredLocation: inferredLocation
          });
        }
      }
    });

    return store;
  },

  










  getFunctionAtLocation: function STW_getFunctionAtLocation([aLine, aColumn]) {
    let self = this;
    let func = null;

    SyntaxTreeVisitor.walk(this.AST, {
      



      onNode: function STW_onNode(aNode) {
        
        
        
        return ParserHelpers.isWithinLines(aNode, aLine);
      },

      



      onIdentifier: function STW_onIdentifier(aNode) {
        
        let hovered = ParserHelpers.isWithinBounds(aNode, aLine, aColumn);
        if (!hovered) {
          return;
        }

        
        
        let expression = ParserHelpers.getEnclosingFunctionExpression(aNode);
        if (!expression) {
          return;
        }

        
        
        if (ParserHelpers.isFunctionCalleeArgument(aNode)) {
          
          if (self.functionIdentifiersCache.has(aNode.name)) {
            
            func = {
              functionName: aNode.name,
              functionLocation: aNode.loc || aNode._parent.loc
            };
          }
          return;
        }

        
        func = {
          functionName: aNode.name,
          functionLocation: ParserHelpers.getFunctionCalleeInfo(expression).loc
        };

        
        this.break = true;
      }
    });

    return func;
  },

  






  get functionIdentifiersCache() {
    if (this._functionIdentifiersCache) {
      return this._functionIdentifiersCache;
    }
    let functionDefinitions = this.getNamedFunctionDefinitions("");
    let functionIdentifiers = new Set();

    for (let { functionName, inferredName } of functionDefinitions) {
      functionIdentifiers.add(functionName);
      functionIdentifiers.add(inferredName);
    }
    return this._functionIdentifiersCache = functionIdentifiers;
  },

  AST: null,
  url: "",
  length: 0,
  offset: 0
};




let ParserHelpers = {
  









  isWithinLines: function PH_isWithinLines(aNode, aLine) {
    
    if (!aNode.loc) {
      return this.isWithinLines(aNode._parent, aLine);
    }
    return aNode.loc.start.line <= aLine && aNode.loc.end.line >= aLine;
  },

  











  isWithinBounds: function PH_isWithinBounds(aNode, aLine, aColumn) {
    
    if (!aNode.loc) {
      return this.isWithinBounds(aNode._parent, aLine, aColumn);
    }
    return aNode.loc.start.line == aLine && aNode.loc.end.line == aLine &&
           aNode.loc.start.column <= aColumn && aNode.loc.end.column >= aColumn;
  },

  










  inferFunctionExpressionInfo: function PH_inferFunctionExpressionInfo(aNode) {
    let parent = aNode._parent;

    
    
    
    if (parent.type == "VariableDeclarator") {
      return {
        name: parent.id.name,
        chain: null,
        loc: parent.loc
      };
    }

    
    
    
    if (parent.type == "AssignmentExpression") {
      let assigneeChain = this.getAssignmentExpressionAssigneeChain(parent);
      let assigneeLeaf = assigneeChain.pop();
      return {
        name: assigneeLeaf,
        chain: assigneeChain,
        loc: parent.left.loc
      };
    }

    
    
    
    if (parent.type == "ObjectExpression") {
      let propertyDetails = this.getObjectExpressionPropertyKeyForValue(aNode);
      let propertyChain = this.getObjectExpressionPropertyChain(parent);
      let propertyLeaf = propertyDetails.name;
      return {
        name: propertyLeaf,
        chain: propertyChain,
        loc: propertyDetails.loc
      };
    }

    
    return {
      name: "",
      chain: null,
      loc: null
    };
  },

  









  getObjectExpressionPropertyKeyForValue:
  function PH_getObjectExpressionPropertyKeyForValue(aNode) {
    let parent = aNode._parent;
    if (parent.type != "ObjectExpression") {
      return null;
    }
    for (let property of parent.properties) {
      if (property.value == aNode) {
        return property.key;
      }
    }
  },

  











  getObjectExpressionPropertyChain:
  function PH_getObjectExpressionPropertyChain(aNode, aStore = []) {
    switch (aNode.type) {
      case "ObjectExpression":
        this.getObjectExpressionPropertyChain(aNode._parent, aStore);

        let propertyDetails = this.getObjectExpressionPropertyKeyForValue(aNode);
        if (propertyDetails) {
          aStore.push(this.getObjectExpressionPropertyKeyForValue(aNode).name);
        }
        break;
      
      
      case "AssignmentExpression":
        this.getAssignmentExpressionAssigneeChain(aNode, aStore);
        break;
      
      
      
      case "NewExpression":
      case "CallExpression":
        this.getObjectExpressionPropertyChain(aNode._parent, aStore);
        break;
      
      case "VariableDeclarator":
        aStore.push(aNode.id.name);
        break;
    }
    return aStore;
  },

  












  getAssignmentExpressionAssigneeChain:
  function PH_getAssignmentExpressionAssigneeChain(aNode, aStore = []) {
    switch (aNode.type) {
      case "AssignmentExpression":
        this.getAssignmentExpressionAssigneeChain(aNode.left, aStore);
        break;
      case "MemberExpression":
        this.getAssignmentExpressionAssigneeChain(aNode.object, aStore);
        this.getAssignmentExpressionAssigneeChain(aNode.property, aStore);
        break;
      case "ThisExpression":
        
        
        
        break;
      case "Identifier":
        aStore.push(aNode.name);
        break;
    }
    return aStore;
  },

  










  getEnclosingFunctionExpression:
  function PH_getEnclosingFunctionExpression(aNode) {
    switch (aNode.type) {
      case "NewExpression":
      case "CallExpression":
        return aNode;
      case "MemberExpression":
      case "Identifier":
        return this.getEnclosingFunctionExpression(aNode._parent);
      default:
        return null;
    }
  },

  









  getFunctionCalleeInfo: function PH_getFunctionCalleeInfo(aNode) {
    switch (aNode.type) {
      case "NewExpression":
      case "CallExpression":
        return this.getFunctionCalleeInfo(aNode.callee);
      case "MemberExpression":
        return this.getFunctionCalleeInfo(aNode.property);
      case "Identifier":
        return {
          name: aNode.name,
          loc: aNode.loc || (aNode._parent || {}).loc
        };
      default:
        return null;
    }
  },

  








  isFunctionCalleeArgument: function PH_isFunctionCalleeArgument(aNode) {
    if (!aNode._parent) {
      return false;
    }
    switch (aNode._parent.type) {
      case "NewExpression":
      case "CallExpression":
        return aNode._parent.arguments.indexOf(aNode) != -1;
      default:
        return this.isFunctionCalleeArgument(aNode._parent);
    }
  }
};











let SyntaxTreeVisitor = {
  








  walk: function STV_walk(aTree, aCallbacks) {
    this[aTree.type](aTree, aCallbacks);
  },

  



  break: false,

  







  Program: function STV_Program(aNode, aCallbacks) {
    if (aCallbacks.onProgram) {
      aCallbacks.onProgram(aNode);
    }
    for (let statement of aNode.body) {
      this[statement.type](statement, aNode, aCallbacks);
    }
  },

  




  Statement: function STV_Statement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onStatement) {
      aCallbacks.onStatement(aNode);
    }
  },

  






  EmptyStatement: function STV_EmptyStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onEmptyStatement) {
      aCallbacks.onEmptyStatement(aNode);
    }
  },

  







  BlockStatement: function STV_BlockStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onBlockStatement) {
      aCallbacks.onBlockStatement(aNode);
    }
    for (let statement of aNode.body) {
      this[statement.type](statement, aNode, aCallbacks);
    }
  },

  







  ExpressionStatement: function STV_ExpressionStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onExpressionStatement) {
      aCallbacks.onExpressionStatement(aNode);
    }
    this[aNode.expression.type](aNode.expression, aNode, aCallbacks);
  },

  









  IfStatement: function STV_IfStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onIfStatement) {
      aCallbacks.onIfStatement(aNode);
    }
    this[aNode.test.type](aNode.test, aNode, aCallbacks);
    this[aNode.consequent.type](aNode.consequent, aNode, aCallbacks);
    if (aNode.alternate) {
      this[aNode.alternate.type](aNode.alternate, aNode, aCallbacks);
    }
  },

  








  LabeledStatement: function STV_LabeledStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onLabeledStatement) {
      aCallbacks.onLabeledStatement(aNode);
    }
    this[aNode.label.type](aNode.label, aNode, aCallbacks);
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  







  BreakStatement: function STV_BreakStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onBreakStatement) {
      aCallbacks.onBreakStatement(aNode);
    }
    if (aNode.label) {
      this[aNode.label.type](aNode.label, aNode, aCallbacks);
    }
  },

  







  ContinueStatement: function STV_ContinueStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onContinueStatement) {
      aCallbacks.onContinueStatement(aNode);
    }
    if (aNode.label) {
      this[aNode.label.type](aNode.label, aNode, aCallbacks);
    }
  },

  








  WithStatement: function STV_WithStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onWithStatement) {
      aCallbacks.onWithStatement(aNode);
    }
    this[aNode.object.type](aNode.object, aNode, aCallbacks);
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  











  SwitchStatement: function STV_SwitchStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onSwitchStatement) {
      aCallbacks.onSwitchStatement(aNode);
    }
    this[aNode.discriminant.type](aNode.discriminant, aNode, aCallbacks);
    for (let _case of aNode.cases) {
      this[_case.type](_case, aNode, aCallbacks);
    }
  },

  







  ReturnStatement: function STV_ReturnStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onReturnStatement) {
      aCallbacks.onReturnStatement(aNode);
    }
    if (aNode.argument) {
      this[aNode.argument.type](aNode.argument, aNode, aCallbacks);
    }
  },

  







  ThrowStatement: function STV_ThrowStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onThrowStatement) {
      aCallbacks.onThrowStatement(aNode);
    }
    this[aNode.argument.type](aNode.argument, aNode, aCallbacks);
  },

  










  TryStatement: function STV_TryStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onTryStatement) {
      aCallbacks.onTryStatement(aNode);
    }
    this[aNode.block.type](aNode.block, aNode, aCallbacks);
    if (aNode.handler) {
      this[aNode.handler.type](aNode.handler, aNode, aCallbacks);
    }
    for (let guardedHandler of aNode.guardedHandlers) {
      this[guardedHandler.type](guardedHandler, aNode, aCallbacks);
    }
    if (aNode.finalizer) {
      this[aNode.finalizer.type](aNode.finalizer, aNode, aCallbacks);
    }
  },

  








  WhileStatement: function STV_WhileStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onWhileStatement) {
      aCallbacks.onWhileStatement(aNode);
    }
    this[aNode.test.type](aNode.test, aNode, aCallbacks);
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  








  DoWhileStatement: function STV_DoWhileStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onDoWhileStatement) {
      aCallbacks.onDoWhileStatement(aNode);
    }
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
    this[aNode.test.type](aNode.test, aNode, aCallbacks);
  },

  










  ForStatement: function STV_ForStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onForStatement) {
      aCallbacks.onForStatement(aNode);
    }
    if (aNode.init) {
      this[aNode.init.type](aNode.init, aNode, aCallbacks);
    }
    if (aNode.test) {
      this[aNode.test.type](aNode.test, aNode, aCallbacks);
    }
    if (aNode.update) {
      this[aNode.update.type](aNode.update, aNode, aCallbacks);
    }
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  










  ForInStatement: function STV_ForInStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onForInStatement) {
      aCallbacks.onForInStatement(aNode);
    }
    this[aNode.left.type](aNode.left, aNode, aCallbacks);
    this[aNode.right.type](aNode.right, aNode, aCallbacks);
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  









  ForOfStatement: function STV_ForOfStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onForOfStatement) {
      aCallbacks.onForOfStatement(aNode);
    }
    this[aNode.left.type](aNode.left, aNode, aCallbacks);
    this[aNode.right.type](aNode.right, aNode, aCallbacks);
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  








  LetStatement: function STV_LetStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onLetStatement) {
      aCallbacks.onLetStatement(aNode);
    }
    for (let { id, init } of aNode.head) {
      this[id.type](id, aNode, aCallbacks);
      if (init) {
        this[init.type](init, aNode, aCallbacks);
      }
    }
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  






  DebuggerStatement: function STV_DebuggerStatement(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onDebuggerStatement) {
      aCallbacks.onDebuggerStatement(aNode);
    }
  },

  






  Declaration: function STV_Declaration(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onDeclaration) {
      aCallbacks.onDeclaration(aNode);
    }
  },

  













  FunctionDeclaration: function STV_FunctionDeclaration(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onFunctionDeclaration) {
      aCallbacks.onFunctionDeclaration(aNode);
    }
    this[aNode.id.type](aNode.id, aNode, aCallbacks);
    for (let param of aNode.params) {
      this[param.type](param, aNode, aCallbacks);
    }
    for (let _default of aNode.defaults) {
      this[_default.type](_default, aNode, aCallbacks);
    }
    if (aNode.rest) {
      this[aNode.rest.type](aNode.rest, aNode, aCallbacks);
    }
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  








  VariableDeclaration: function STV_VariableDeclaration(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onVariableDeclaration) {
      aCallbacks.onVariableDeclaration(aNode);
    }
    for (let declaration of aNode.declarations) {
      this[declaration.type](declaration, aNode, aCallbacks);
    }
  },

  








  VariableDeclarator: function STV_VariableDeclarator(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onVariableDeclarator) {
      aCallbacks.onVariableDeclarator(aNode);
    }
    this[aNode.id.type](aNode.id, aNode, aCallbacks);
    if (aNode.init) {
      this[aNode.init.type](aNode.init, aNode, aCallbacks);
    }
  },

  





  Expression: function STV_Expression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onExpression) {
      aCallbacks.onExpression(aNode);
    }
  },

  






  ThisExpression: function STV_ThisExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onThisExpression) {
      aCallbacks.onThisExpression(aNode);
    }
  },

  







  ArrayExpression: function STV_ArrayExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onArrayExpression) {
      aCallbacks.onArrayExpression(aNode);
    }
    for (let element of aNode.elements) {
      if (element) {
        this[element.type](element, aNode, aCallbacks);
      }
    }
  },

  












  ObjectExpression: function STV_ObjectExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onObjectExpression) {
      aCallbacks.onObjectExpression(aNode);
    }
    for (let { key, value } of aNode.properties) {
      this[key.type](key, aNode, aCallbacks);
      this[value.type](value, aNode, aCallbacks);
    }
  },

  













  FunctionExpression: function STV_FunctionExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onFunctionExpression) {
      aCallbacks.onFunctionExpression(aNode);
    }
    if (aNode.id) {
      this[aNode.id.type](aNode.id, aNode, aCallbacks);
    }
    for (let param of aNode.params) {
      this[param.type](param, aNode, aCallbacks);
    }
    for (let _default of aNode.defaults) {
      this[_default.type](_default, aNode, aCallbacks);
    }
    if (aNode.rest) {
      this[aNode.rest.type](aNode.rest, aNode, aCallbacks);
    }
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  







  SequenceExpression: function STV_SequenceExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onSequenceExpression) {
      aCallbacks.onSequenceExpression(aNode);
    }
    for (let expression of aNode.expressions) {
      this[expression.type](expression, aNode, aCallbacks);
    }
  },

  









  UnaryExpression: function STV_UnaryExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onUnaryExpression) {
      aCallbacks.onUnaryExpression(aNode);
    }
    this[aNode.argument.type](aNode.argument, aNode, aCallbacks);
  },

  









  BinaryExpression: function STV_BinaryExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onBinaryExpression) {
      aCallbacks.onBinaryExpression(aNode);
    }
    this[aNode.left.type](aNode.left, aNode, aCallbacks);
    this[aNode.right.type](aNode.right, aNode, aCallbacks);
  },

  









  AssignmentExpression: function STV_AssignmentExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onAssignmentExpression) {
      aCallbacks.onAssignmentExpression(aNode);
    }
    this[aNode.left.type](aNode.left, aNode, aCallbacks);
    this[aNode.right.type](aNode.right, aNode, aCallbacks);
  },

  









  UpdateExpression: function STV_UpdateExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onUpdateExpression) {
      aCallbacks.onUpdateExpression(aNode);
    }
    this[aNode.argument.type](aNode.argument, aNode, aCallbacks);
  },

  









  LogicalExpression: function STV_LogicalExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onLogicalExpression) {
      aCallbacks.onLogicalExpression(aNode);
    }
    this[aNode.left.type](aNode.left, aNode, aCallbacks);
    this[aNode.right.type](aNode.right, aNode, aCallbacks);
  },

  









  ConditionalExpression: function STV_ConditionalExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onConditionalExpression) {
      aCallbacks.onConditionalExpression(aNode);
    }
    this[aNode.test.type](aNode.test, aNode, aCallbacks);
    this[aNode.alternate.type](aNode.alternate, aNode, aCallbacks);
    this[aNode.consequent.type](aNode.consequent, aNode, aCallbacks);
  },

  








  NewExpression: function STV_NewExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onNewExpression) {
      aCallbacks.onNewExpression(aNode);
    }
    this[aNode.callee.type](aNode.callee, aNode, aCallbacks);
    for (let argument of aNode.arguments) {
      if (argument) {
        this[argument.type](argument, aNode, aCallbacks);
      }
    }
  },

  








  CallExpression: function STV_CallExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onCallExpression) {
      aCallbacks.onCallExpression(aNode);
    }
    this[aNode.callee.type](aNode.callee, aNode, aCallbacks);
    for (let argument of aNode.arguments) {
      if (argument) {
        this[argument.type](argument, aNode, aCallbacks);
      }
    }
  },

  












  MemberExpression: function STV_MemberExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onMemberExpression) {
      aCallbacks.onMemberExpression(aNode);
    }
    this[aNode.object.type](aNode.object, aNode, aCallbacks);
    this[aNode.property.type](aNode.property, aNode, aCallbacks);
  },

  






  YieldExpression: function STV_YieldExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onYieldExpression) {
      aCallbacks.onYieldExpression(aNode);
    }
    if (aNode.argument) {
      this[aNode.argument.type](aNode.argument, aNode, aCallbacks);
    }
  },

  










  ComprehensionExpression: function STV_ComprehensionExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onComprehensionExpression) {
      aCallbacks.onComprehensionExpression(aNode);
    }
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
    for (let block of aNode.blocks) {
      this[block.type](block, aNode, aCallbacks);
    }
    if (aNode.filter) {
      this[aNode.filter.type](aNode.filter, aNode, aCallbacks);
    }
  },

  










  GeneratorExpression: function STV_GeneratorExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onGeneratorExpression) {
      aCallbacks.onGeneratorExpression(aNode);
    }
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
    for (let block of aNode.blocks) {
      this[block.type](block, aNode, aCallbacks);
    }
    if (aNode.filter) {
      this[aNode.filter.type](aNode.filter, aNode, aCallbacks);
    }
  },

  







  GraphExpression: function STV_GraphExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onGraphExpression) {
      aCallbacks.onGraphExpression(aNode);
    }
    this[aNode.expression.type](aNode.expression, aNode, aCallbacks);
  },

  






  GraphIndexExpression: function STV_GraphIndexExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onGraphIndexExpression) {
      aCallbacks.onGraphIndexExpression(aNode);
    }
  },

  








  LetExpression: function STV_LetExpression(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onLetExpression) {
      aCallbacks.onLetExpression(aNode);
    }
    for (let { id, init } of aNode.head) {
      this[id.type](id, aNode, aCallbacks);
      if (init) {
        this[init.type](init, aNode, aCallbacks);
      }
    }
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  




  Pattern: function STV_Pattern(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onPattern) {
      aCallbacks.onPattern(aNode);
    }
  },

  








  ObjectPattern: function STV_ObjectPattern(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onObjectPattern) {
      aCallbacks.onObjectPattern(aNode);
    }
    for (let { key, value } of aNode.properties) {
      this[key.type](key, aNode, aCallbacks);
      this[value.type](value, aNode, aCallbacks);
    }
  },

  







  ArrayPattern: function STV_ArrayPattern(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onArrayPattern) {
      aCallbacks.onArrayPattern(aNode);
    }
    for (let element of aNode.elements) {
      if (element) {
        this[element.type](element, aNode, aCallbacks);
      }
    }
  },

  









  SwitchCase: function STV_SwitchCase(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onSwitchCase) {
      aCallbacks.onSwitchCase(aNode);
    }
    if (aNode.test) {
      this[aNode.test.type](aNode.test, aNode, aCallbacks);
    }
    for (let consequent of aNode.consequent) {
      this[consequent.type](consequent, aNode, aCallbacks);
    }
  },

  










  CatchClause: function STV_CatchClause(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onCatchClause) {
      aCallbacks.onCatchClause(aNode);
    }
    this[aNode.param.type](aNode.param, aNode, aCallbacks);
    if (aNode.guard) {
      this[aNode.guard.type](aNode.guard, aNode, aCallbacks);
    }
    this[aNode.body.type](aNode.body, aNode, aCallbacks);
  },

  








  ComprehensionBlock: function STV_ComprehensionBlock(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onComprehensionBlock) {
      aCallbacks.onComprehensionBlock(aNode);
    }
    this[aNode.left.type](aNode.left, aNode, aCallbacks);
    this[aNode.right.type](aNode.right, aNode, aCallbacks);
  },

  








  Identifier: function STV_Identifier(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onIdentifier) {
      aCallbacks.onIdentifier(aNode);
    }
  },

  







  Literal: function STV_Literal(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onLiteral) {
      aCallbacks.onLiteral(aNode);
    }
  }
};









function log(aStr, aEx) {
  let msg = "Warning: " + aStr + ", " + aEx + "\n" + aEx.stack;
  Cu.reportError(msg);
  dump(msg + "\n");
};

XPCOMUtils.defineLazyGetter(Parser, "reflectionAPI", function() Reflect);
