




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
const { DevToolsUtils } = Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm", {});

XPCOMUtils.defineLazyModuleGetter(this,
  "Reflect", "resource://gre/modules/reflect.jsm");

this.EXPORTED_SYMBOLS = ["Parser", "ParserHelpers", "SyntaxTreeVisitor"];




this.Parser = function Parser() {
  this._cache = new Map();
  this.errors = [];
  this.logExceptions = true;
};

Parser.prototype = {
  








  get: function(aSource, aUrl = "") {
    
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
        this.errors.push(e);
        if (this.logExceptions) {
          DevToolsUtils.reportException(aUrl, e);
        }
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
          this.errors.push(e);
          if (this.logExceptions) {
            DevToolsUtils.reportException(aUrl, e);
          }
        }
      }
    }

    let pool = new SyntaxTreesPool(syntaxTrees, aUrl);

    
    
    
    if (aUrl) {
      this._cache.set(aUrl, pool);
    }

    return pool;
  },

  


  clearCache: function() {
    this._cache.clear();
  },

  





  clearSource: function(aUrl) {
    this._cache.delete(aUrl);
  },

  _cache: null,
  errors: null
};









function SyntaxTreesPool(aSyntaxTrees, aUrl = "<unknown>") {
  this._trees = aSyntaxTrees;
  this._url = aUrl;
  this._cache = new Map();
}

SyntaxTreesPool.prototype = {
  


  getIdentifierAt: function({ line, column, scriptIndex, ignoreLiterals }) {
    return this._call("getIdentifierAt", scriptIndex, line, column, ignoreLiterals)[0];
  },

  


  getNamedFunctionDefinitions: function(aSubstring) {
    return this._call("getNamedFunctionDefinitions", -1, aSubstring);
  },

  



  get scriptCount() {
    return this._trees.length;
  },

  








  getScriptInfo: function(aOffset) {
    let info = { start: -1, length: -1, index: -1 };

    for (let { offset, length } of this._trees) {
      info.index++;
      if (offset <= aOffset && offset + length >= aOffset) {
        info.start = offset;
        info.length = length;
        return info;
      }
    }

    info.index = -1;
    return info;
  },

  













  _call: function(aFunction, aSyntaxTreeIndex, ...aParams) {
    let results = [];
    let requestId = [aFunction, aSyntaxTreeIndex, aParams].toSource();

    if (this._cache.has(requestId)) {
      return this._cache.get(requestId);
    }

    let requestedTree = this._trees[aSyntaxTreeIndex];
    let targettedTrees = requestedTree ? [requestedTree] : this._trees;

    for (let syntaxTree of targettedTrees) {
      try {
        let parseResults = syntaxTree[aFunction].apply(syntaxTree, aParams);
        if (parseResults) {
          parseResults.sourceUrl = syntaxTree.url;
          parseResults.scriptLength = syntaxTree.length;
          parseResults.scriptOffset = syntaxTree.offset;
          results.push(parseResults);
        }
      } catch (e) {
        
        
        
        DevToolsUtils.reportException("Syntax tree visitor for " + this._url, e);
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
  












  getIdentifierAt: function(aLine, aColumn, aIgnoreLiterals) {
    let info = null;

    SyntaxTreeVisitor.walk(this.AST, {
      



      onIdentifier: function(aNode) {
        if (ParserHelpers.nodeContainsPoint(aNode, aLine, aColumn)) {
          info = {
            name: aNode.name,
            location: ParserHelpers.getNodeLocation(aNode),
            evalString: ParserHelpers.getIdentifierEvalString(aNode)
          };

          
          SyntaxTreeVisitor.break = true;
        }
      },

      



      onLiteral: function(aNode) {
        if (!aIgnoreLiterals) {
          this.onIdentifier(aNode);
        }
      },

      



      onThisExpression: function(aNode) {
        this.onIdentifier(aNode);
      }
    });

    return info;
  },

  










  getNamedFunctionDefinitions: function(aSubstring) {
    let lowerCaseToken = aSubstring.toLowerCase();
    let store = [];

    SyntaxTreeVisitor.walk(this.AST, {
      



      onFunctionDeclaration: function(aNode) {
        let functionName = aNode.id.name;
        if (functionName.toLowerCase().includes(lowerCaseToken)) {
          store.push({
            functionName: functionName,
            functionLocation: ParserHelpers.getNodeLocation(aNode)
          });
        }
      },

      



      onFunctionExpression: function(aNode) {
        
        let functionName = aNode.id ? aNode.id.name : "";
        let functionLocation = ParserHelpers.getNodeLocation(aNode);

        
        let inferredInfo = ParserHelpers.inferFunctionExpressionInfo(aNode);
        let inferredName = inferredInfo.name;
        let inferredChain = inferredInfo.chain;
        let inferredLocation = inferredInfo.loc;

        
        if (aNode._parent.type == "AssignmentExpression") {
          this.onFunctionExpression(aNode._parent);
        }

        if ((functionName && functionName.toLowerCase().includes(lowerCaseToken)) ||
            (inferredName && inferredName.toLowerCase().includes(lowerCaseToken))) {
          store.push({
            functionName: functionName,
            functionLocation: functionLocation,
            inferredName: inferredName,
            inferredChain: inferredChain,
            inferredLocation: inferredLocation
          });
        }
      },

      



      onArrowFunctionExpression: function(aNode) {
        
        let inferredInfo = ParserHelpers.inferFunctionExpressionInfo(aNode);
        let inferredName = inferredInfo.name;
        let inferredChain = inferredInfo.chain;
        let inferredLocation = inferredInfo.loc;

        
        if (aNode._parent.type == "AssignmentExpression") {
          this.onFunctionExpression(aNode._parent);
        }

        if (inferredName && inferredName.toLowerCase().includes(lowerCaseToken)) {
          store.push({
            inferredName: inferredName,
            inferredChain: inferredChain,
            inferredLocation: inferredLocation
          });
        }
      }
    });

    return store;
  },

  AST: null,
  url: "",
  length: 0,
  offset: 0
};




let ParserHelpers = {
  









  getNodeLocation: function(aNode) {
    if (aNode.type != "Identifier") {
      return aNode.loc;
    }
    
    
    let { loc: parentLocation, type: parentType } = aNode._parent;
    let { loc: nodeLocation } = aNode;
    if (!nodeLocation) {
      if (parentType == "FunctionDeclaration" ||
          parentType == "FunctionExpression") {
        
        
        let loc = Cu.cloneInto(parentLocation, {});
        loc.end.line = loc.start.line;
        loc.end.column = loc.start.column + aNode.name.length;
        return loc;
      }
      if (parentType == "MemberExpression") {
        
        
        let loc = Cu.cloneInto(parentLocation, {});
        loc.start.line = loc.end.line;
        loc.start.column = loc.end.column - aNode.name.length;
        return loc;
      }
      if (parentType == "LabeledStatement") {
        
        
        let loc = Cu.cloneInto(parentLocation, {});
        loc.end.line = loc.start.line;
        loc.end.column = loc.start.column + aNode.name.length;
        return loc;
      }
      if (parentType == "ContinueStatement" || parentType == "BreakStatement") {
        
        
        let loc = Cu.cloneInto(parentLocation, {});
        loc.start.line = loc.end.line;
        loc.start.column = loc.end.column - aNode.name.length;
        return loc;
      }
    } else {
      if (parentType == "VariableDeclarator") {
        
        
        
        let loc = Cu.cloneInto(nodeLocation, {});
        loc.end.line = loc.start.line;
        loc.end.column = loc.start.column + aNode.name.length;
        return loc;
      }
    }
    return aNode.loc;
  },

  









  nodeContainsLine: function(aNode, aLine) {
    let { start: s, end: e } = this.getNodeLocation(aNode);
    return s.line <= aLine && e.line >= aLine;
  },

  











  nodeContainsPoint: function(aNode, aLine, aColumn) {
    let { start: s, end: e } = this.getNodeLocation(aNode);
    return s.line == aLine && e.line == aLine &&
           s.column <= aColumn && e.column >= aColumn;
  },

  










  inferFunctionExpressionInfo: function(aNode) {
    let parent = aNode._parent;

    
    
    
    if (parent.type == "VariableDeclarator") {
      return {
        name: parent.id.name,
        chain: null,
        loc: this.getNodeLocation(parent.id)
      };
    }

    
    
    
    if (parent.type == "AssignmentExpression") {
      let propertyChain = this._getMemberExpressionPropertyChain(parent.left);
      let propertyLeaf = propertyChain.pop();
      return {
        name: propertyLeaf,
        chain: propertyChain,
        loc: this.getNodeLocation(parent.left)
      };
    }

    
    
    
    if (parent.type == "ObjectExpression") {
      let propertyKey = this._getObjectExpressionPropertyKeyForValue(aNode);
      let propertyChain = this._getObjectExpressionPropertyChain(parent);
      let propertyLeaf = propertyKey.name;
      return {
        name: propertyLeaf,
        chain: propertyChain,
        loc: this.getNodeLocation(propertyKey)
      };
    }

    
    return {
      name: "",
      chain: null,
      loc: null
    };
  },

  














  _getObjectExpressionPropertyKeyForValue: function(aNode) {
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

  

















  _getObjectExpressionPropertyChain: function(aNode, aStore = []) {
    switch (aNode.type) {
      case "ObjectExpression":
        this._getObjectExpressionPropertyChain(aNode._parent, aStore);
        let propertyKey = this._getObjectExpressionPropertyKeyForValue(aNode);
        if (propertyKey) {
          aStore.push(propertyKey.name);
        }
        break;
      
      case "VariableDeclarator":
        aStore.push(aNode.id.name);
        break;
      
      
      
      case "AssignmentExpression":
        this._getMemberExpressionPropertyChain(aNode.left, aStore);
        break;
      
      
      
      case "NewExpression":
      case "CallExpression":
        this._getObjectExpressionPropertyChain(aNode._parent, aStore);
        break;
    }
    return aStore;
  },

  

















  _getMemberExpressionPropertyChain: function(aNode, aStore = []) {
    switch (aNode.type) {
      case "MemberExpression":
        this._getMemberExpressionPropertyChain(aNode.object, aStore);
        this._getMemberExpressionPropertyChain(aNode.property, aStore);
        break;
      case "ThisExpression":
        aStore.push("this");
        break;
      case "Identifier":
        aStore.push(aNode.name);
        break;
    }
    return aStore;
  },

  









  getIdentifierEvalString: function(aNode) {
    switch (aNode._parent.type) {
      case "ObjectExpression":
        
        
        
        if (!this._getObjectExpressionPropertyKeyForValue(aNode)) {
          let propertyChain = this._getObjectExpressionPropertyChain(aNode._parent);
          let propertyLeaf = aNode.name;
          return [...propertyChain, propertyLeaf].join(".");
        }
        break;
      case "MemberExpression":
        
        if (aNode._parent.property == aNode) {
          return this._getMemberExpressionPropertyChain(aNode._parent).join(".");
        }
        break;
    }
    switch (aNode.type) {
      case "ThisExpression":
        return "this";
      case "Identifier":
        return aNode.name;
      case "Literal":
        return uneval(aNode.value);
      default:
        return "";
    }
  }
};











let SyntaxTreeVisitor = {
  








  walk: function(aTree, aCallbacks) {
    this.break = false;
    this[aTree.type](aTree, aCallbacks);
  },

  









  filter: function(aTree, aPredicate) {
    let store = [];
    this.walk(aTree, { onNode: e => { if (aPredicate(e)) store.push(e); } });
    return store;
  },

  



  break: false,

  







  Program: function(aNode, aCallbacks) {
    if (aCallbacks.onProgram) {
      aCallbacks.onProgram(aNode);
    }
    for (let statement of aNode.body) {
      this[statement.type](statement, aNode, aCallbacks);
    }
  },

  




  Statement: function(aNode, aParent, aCallbacks) {
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

  






  EmptyStatement: function(aNode, aParent, aCallbacks) {
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

  







  BlockStatement: function(aNode, aParent, aCallbacks) {
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

  







  ExpressionStatement: function(aNode, aParent, aCallbacks) {
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

  









  IfStatement: function(aNode, aParent, aCallbacks) {
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

  








  LabeledStatement: function(aNode, aParent, aCallbacks) {
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

  







  BreakStatement: function(aNode, aParent, aCallbacks) {
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

  







  ContinueStatement: function(aNode, aParent, aCallbacks) {
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

  








  WithStatement: function(aNode, aParent, aCallbacks) {
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

  











  SwitchStatement: function(aNode, aParent, aCallbacks) {
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

  







  ReturnStatement: function(aNode, aParent, aCallbacks) {
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

  







  ThrowStatement: function(aNode, aParent, aCallbacks) {
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

  










  TryStatement: function(aNode, aParent, aCallbacks) {
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

  








  WhileStatement: function(aNode, aParent, aCallbacks) {
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

  








  DoWhileStatement: function(aNode, aParent, aCallbacks) {
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

  










  ForStatement: function(aNode, aParent, aCallbacks) {
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

  










  ForInStatement: function(aNode, aParent, aCallbacks) {
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

  









  ForOfStatement: function(aNode, aParent, aCallbacks) {
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

  








  LetStatement: function(aNode, aParent, aCallbacks) {
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

  






  DebuggerStatement: function(aNode, aParent, aCallbacks) {
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

  






  Declaration: function(aNode, aParent, aCallbacks) {
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

  













  FunctionDeclaration: function(aNode, aParent, aCallbacks) {
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

  








  VariableDeclaration: function(aNode, aParent, aCallbacks) {
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

  








  VariableDeclarator: function(aNode, aParent, aCallbacks) {
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

  





  Expression: function(aNode, aParent, aCallbacks) {
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

  






  ThisExpression: function(aNode, aParent, aCallbacks) {
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

  







  ArrayExpression: function(aNode, aParent, aCallbacks) {
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
      
      
      if (element && typeof this[element.type] == "function") {
        this[element.type](element, aNode, aCallbacks);
      }
    }
  },

  












  ObjectExpression: function(aNode, aParent, aCallbacks) {
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

  













  FunctionExpression: function(aNode, aParent, aCallbacks) {
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

  












  ArrowFunctionExpression: function(aNode, aParent, aCallbacks) {
    aNode._parent = aParent;

    if (this.break) {
      return;
    }
    if (aCallbacks.onNode) {
      if (aCallbacks.onNode(aNode, aParent) === false) {
        return;
      }
    }
    if (aCallbacks.onArrowFunctionExpression) {
      aCallbacks.onArrowFunctionExpression(aNode);
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

  







  SequenceExpression: function(aNode, aParent, aCallbacks) {
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

  









  UnaryExpression: function(aNode, aParent, aCallbacks) {
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

  









  BinaryExpression: function(aNode, aParent, aCallbacks) {
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

  









  AssignmentExpression: function(aNode, aParent, aCallbacks) {
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

  









  UpdateExpression: function(aNode, aParent, aCallbacks) {
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

  









  LogicalExpression: function(aNode, aParent, aCallbacks) {
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

  









  ConditionalExpression: function(aNode, aParent, aCallbacks) {
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

  








  NewExpression: function(aNode, aParent, aCallbacks) {
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

  








  CallExpression: function(aNode, aParent, aCallbacks) {
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

  












  MemberExpression: function(aNode, aParent, aCallbacks) {
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

  






  YieldExpression: function(aNode, aParent, aCallbacks) {
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

  










  ComprehensionExpression: function(aNode, aParent, aCallbacks) {
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

  










  GeneratorExpression: function(aNode, aParent, aCallbacks) {
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

  







  GraphExpression: function(aNode, aParent, aCallbacks) {
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

  






  GraphIndexExpression: function(aNode, aParent, aCallbacks) {
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

  








  LetExpression: function(aNode, aParent, aCallbacks) {
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

  




  Pattern: function(aNode, aParent, aCallbacks) {
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

  








  ObjectPattern: function(aNode, aParent, aCallbacks) {
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

  







  ArrayPattern: function(aNode, aParent, aCallbacks) {
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

  









  SwitchCase: function(aNode, aParent, aCallbacks) {
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

  










  CatchClause: function(aNode, aParent, aCallbacks) {
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

  








  ComprehensionBlock: function(aNode, aParent, aCallbacks) {
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

  








  Identifier: function(aNode, aParent, aCallbacks) {
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

  







  Literal: function(aNode, aParent, aCallbacks) {
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

XPCOMUtils.defineLazyGetter(Parser, "reflectionAPI", () => Reflect);
