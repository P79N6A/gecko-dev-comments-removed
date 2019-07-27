


"use strict";




function WatchExpressionsView(DebuggerController, DebuggerView) {
  dumpn("WatchExpressionsView was instantiated");

  this.StackFrames = DebuggerController.StackFrames;
  this.DebuggerView = DebuggerView;

  this.switchExpression = this.switchExpression.bind(this);
  this.deleteExpression = this.deleteExpression.bind(this);
  this._createItemView = this._createItemView.bind(this);
  this._onClick = this._onClick.bind(this);
  this._onClose = this._onClose.bind(this);
  this._onBlur = this._onBlur.bind(this);
  this._onKeyPress = this._onKeyPress.bind(this);
}

WatchExpressionsView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the WatchExpressionsView");

    this.widget = new SimpleListWidget(document.getElementById("expressions"));
    this.widget.setAttribute("context", "debuggerWatchExpressionsContextMenu");
    this.widget.addEventListener("click", this._onClick, false);

    this.headerText = L10N.getStr("addWatchExpressionText");
    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the WatchExpressionsView");

    this.widget.removeEventListener("click", this._onClick, false);
  },

  


  _addCommands: function() {
    XULUtils.addCommands(document.getElementById('debuggerCommands'), {
      addWatchExpressionCommand: () => this._onCmdAddExpression(),
      removeAllWatchExpressionsCommand: () => this._onCmdRemoveAllExpressions()
    });
  },

  








  addExpression: function(aExpression = "", aSkipUserInput = false) {
    
    this.DebuggerView.showInstrumentsPane();

    
    let itemView = this._createItemView(aExpression);

    
    let expressionItem = this.push([itemView.container], {
      index: 0, 
      attachment: {
        view: itemView,
        initialExpression: aExpression,
        currentExpression: "",
      }
    });

    
    
    if (!aSkipUserInput) {
      expressionItem.attachment.view.inputNode.select();
      expressionItem.attachment.view.inputNode.focus();
      this.DebuggerView.Variables.parentNode.scrollTop = 0;
    }
    
    else {
      this.toggleContents(false);
      this._onBlur({ target: expressionItem.attachment.view.inputNode });
    }
  },

  









  switchExpression: function(aVar, aExpression) {
    let expressionItem =
      [i for (i of this) if (i.attachment.currentExpression == aVar.name)][0];

    
    if (!aExpression || this.getAllStrings().indexOf(aExpression) != -1) {
      this.deleteExpression(aVar);
      return;
    }

    
    expressionItem.attachment.currentExpression = aExpression;
    expressionItem.attachment.view.inputNode.value = aExpression;

    
    this.StackFrames.syncWatchExpressions();
  },

  







  deleteExpression: function(aVar) {
    let expressionItem =
      [i for (i of this) if (i.attachment.currentExpression == aVar.name)][0];

    
    this.remove(expressionItem);

    
    this.StackFrames.syncWatchExpressions();
  },

  







  getString: function(aIndex) {
    return this.getItemAtIndex(aIndex).attachment.currentExpression;
  },

  





  getAllStrings: function() {
    return this.items.map(e => e.attachment.currentExpression);
  },

  





  _createItemView: function(aExpression) {
    let container = document.createElement("hbox");
    container.className = "list-widget-item dbg-expression";
    container.setAttribute("align", "center");

    let arrowNode = document.createElement("hbox");
    arrowNode.className = "dbg-expression-arrow";

    let inputNode = document.createElement("textbox");
    inputNode.className = "plain dbg-expression-input devtools-monospace";
    inputNode.setAttribute("value", aExpression);
    inputNode.setAttribute("flex", "1");

    let closeNode = document.createElement("toolbarbutton");
    closeNode.className = "plain variables-view-delete";

    closeNode.addEventListener("click", this._onClose, false);
    inputNode.addEventListener("blur", this._onBlur, false);
    inputNode.addEventListener("keypress", this._onKeyPress, false);

    container.appendChild(arrowNode);
    container.appendChild(inputNode);
    container.appendChild(closeNode);

    return {
      container: container,
      arrowNode: arrowNode,
      inputNode: inputNode,
      closeNode: closeNode
    };
  },

  


  _onCmdAddExpression: function(aText) {
    
    if (this.getAllStrings().indexOf("") == -1) {
      this.addExpression(aText || this.DebuggerView.editor.getSelection());
    }
  },

  


  _onCmdRemoveAllExpressions: function() {
    
    this.empty();

    
    this.StackFrames.syncWatchExpressions();
  },

  


  _onClick: function(e) {
    if (e.button != 0) {
      
      return;
    }
    let expressionItem = this.getItemForElement(e.target);
    if (!expressionItem) {
      
      this.addExpression();
    }
  },

  


  _onClose: function(e) {
    
    this.remove(this.getItemForElement(e.target));

    
    this.StackFrames.syncWatchExpressions();

    
    e.preventDefault();
    e.stopPropagation();
  },

  


  _onBlur: function({ target: textbox }) {
    let expressionItem = this.getItemForElement(textbox);
    let oldExpression = expressionItem.attachment.currentExpression;
    let newExpression = textbox.value.trim();

    
    if (!newExpression) {
      this.remove(expressionItem);
    }
    
    else if (!oldExpression && this.getAllStrings().indexOf(newExpression) != -1) {
      this.remove(expressionItem);
    }
    
    else {
      expressionItem.attachment.currentExpression = newExpression;
    }

    
    this.StackFrames.syncWatchExpressions();
  },

  


  _onKeyPress: function(e) {
    switch (e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ESCAPE:
        e.stopPropagation();
        this.DebuggerView.editor.focus();
    }
  }
});

DebuggerView.WatchExpressions = new WatchExpressionsView(DebuggerController,
                                                         DebuggerView);
