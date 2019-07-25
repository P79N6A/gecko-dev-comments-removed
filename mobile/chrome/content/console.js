




































let ConsoleView = {
  _list: null,
  _inited: false,
  _evalTextbox: null,
  _evalFrame: null,
  _evalCode: "",
  _bundle: null,
  _showChromeErrors: -1,

  init: function cv_init() {
    if (this._list)
      return;

    this._list = document.getElementById("console-box");
    this._evalTextbox = document.getElementById("console-eval-textbox");
    this._bundle = Elements.browserBundle;

    this._count = 0;
    this.limit = 250;

    let self = this;
    let panels = document.getElementById("panel-items");
    panels.addEventListener("select",
                            function(aEvent) {
                              if (panels.selectedPanel.id == "console-container")
                                self._delayedInit();
                            },
                            false);
  },

  _delayedInit: function cv__delayedInit() {
    if (this._inited)
      return;
    this._inited = true;

    Services.console.registerListener(this);

    this.appendInitialItems();

    
    this._evalFrame = document.createElement("iframe");
    this._evalFrame.id = "console-evaluator";
    this._evalFrame.collapsed = true;
    document.getElementById("console-container").appendChild(this._evalFrame);

    let self = this;
    this._evalFrame.addEventListener("load", function() { self.loadOrDisplayResult(); }, true);
  },

  uninit: function cv_uninit() {
    if (this._inited)
      Services.console.unregisterListener(this);
  },

  observe: function(aObject) {
    this.appendItem(aObject);
  },

  showChromeErrors: function() {
    if (this._showChromeErrors != -1)
      return this._showChromeErrors;

    try {
      let pref = Services.prefs;
      return this._showChromeErrors = pref.getBoolPref("javascript.options.showInConsole");
    }
    catch(ex) {
      return this._showChromeErrors = false;
    }
  },

  appendItem: function cv_appendItem(aObject) {
    try {
      
      let scriptError = aObject.QueryInterface(Ci.nsIScriptError);

      
      if (!this.showChromeErrors && scriptError.sourceName.substr(0, 9) == "chrome://")
        return;
      this.appendError(scriptError);
    }
    catch (ex) {
      try {
        
        let msg = aObject.QueryInterface(Ci.nsIConsoleMessage);

        if (msg.message)
          this.appendMessage(msg.message);
        else 
          this.clearConsole();
      }
      catch (ex2) {
        
        this.appendMessage(aObject);
      }
    }
  },

  appendError: function cv_appendError(aObject) {
    let row = this.createConsoleRow();
    let nsIScriptError = Ci.nsIScriptError;

    
    let warning = aObject.flags & nsIScriptError.warningFlag != 0;

    let typetext = warning ? "typeWarning" : "typeError";
    row.setAttribute("typetext", this._bundle.getString(typetext));
    row.setAttribute("type", warning ? "warning" : "error");
    row.setAttribute("msg", aObject.errorMessage);
    row.setAttribute("category", aObject.category);
    if (aObject.lineNumber || aObject.sourceName) {
      row.setAttribute("href", aObject.sourceName);
      row.setAttribute("line", aObject.lineNumber);
    }
    else {
      row.setAttribute("hideSource", "true");
    }
    if (aObject.sourceLine) {
      row.setAttribute("code", aObject.sourceLine.replace(/\s/g, " "));
      if (aObject.columnNumber) {
        row.setAttribute("col", aObject.columnNumber);
        row.setAttribute("errorDots", this.repeatChar(" ", aObject.columnNumber));
        row.setAttribute("errorCaret", " ");
      }
      else {
        row.setAttribute("hideCaret", "true");
      }
    }
    else {
      row.setAttribute("hideCode", "true");
    }

    let mode = document.getElementById("console-filter").value;
    if (mode != "all" && mode != row.getAttribute("type"))
      row.collapsed = true;

    this.appendConsoleRow(row);
  },

  appendMessage: function cv_appendMessage (aMessage) {
    let row = this.createConsoleRow();
    row.setAttribute("type", "message");
    row.setAttribute("msg", aMessage);

    let mode = document.getElementById("console-filter").value;
    if (mode != "all" && mode != "message")
      row.collapsed = true;

    this.appendConsoleRow(row);
  },

  createConsoleRow: function cv_createConsoleRow() {
    let row = document.createElement("richlistitem");
    row.setAttribute("class", "console-row");
    return row;
  },

  appendConsoleRow: function cv_appendConsoleRow(aRow) {
    this._list.appendChild(aRow);
    if (++this._count > this.limit)
      this.deleteFirst();
  },

  deleteFirst: function cv_deleteFirst() {
    let node = this._list.firstChild;
    this._list.removeChild(node);
    --this._count;
  },

  appendInitialItems: function cv_appendInitialItems() {
    let out = {}; 
    Services.console.getMessageArray(out, {});
    let messages = out.value;

    
    if (!messages)
      messages = [];

    let limit = messages.length - this.limit;
    if (limit < 0)
      limit = 0;

    
    for (var i = messages.length - 1; i >= limit; --i)
      if (!messages[i].message)
        break;

    
    while (++i < messages.length)
      this.appendItem(messages[i]);
  },

  clearConsole: function cv_clearConsole() {
    if (this._count == 0) 
      return;
    this._count = 0;

    let newRows = this._list.cloneNode(false);
    this._list.parentNode.replaceChild(newRows, this._list);
    this._list = newRows;
    this.selectedItem = null;
  },

  changeMode: function cv_changeMode() {
    let mode = document.getElementById("console-filter").value;
    if (this._list.getAttribute("mode") != mode) {
      let rows = this._list.childNodes;
      for (let i=0; i < rows.length; i++) {
        let row = rows[i];
        if (mode == "all" || row.getAttribute ("type") == mode)
          row.collapsed = false;
        else
          row.collapsed = true;
      }
      this._list.mode = mode;
      this._list.scrollToIndex(0);
    }
  },

  onEvalKeyPress: function cv_onEvalKeyPress(aEvent) {
    if (aEvent.keyCode == 13)
      this.evaluateTypein();
  },

  onConsoleBoxKeyPress: function cv_onConsoleBoxKeyPress(aEvent) {
    if ((aEvent.charCode == 99 || aEvent.charCode == 67) && aEvent.ctrlKey && this._list && this._list.selectedItem) {
      let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper);
      clipboard.copyString(this._list.selectedItem.getAttribute("msg"));
    }
  },

  evaluateTypein: function cv_evaluateTypein() {
    this._evalCode = this._evalTextbox.value;
    this.loadOrDisplayResult();
  },

  loadOrDisplayResult: function cv_loadOrDisplayResult() {
    if (this._evalCode) {
      this._evalFrame.contentWindow.location = "javascript: " + this._evalCode.replace(/%/g, "%25");
      this._evalCode = "";
      return;
    }

    let resultRange = this._evalFrame.contentDocument.createRange();
    resultRange.selectNode(this._evalFrame.contentDocument.documentElement);
    let result = resultRange.toString();
    if (result)
      Services.console.logStringMessage(result);
      
  },

  repeatChar: function cv_repeatChar(aChar, aCol) {
    if (--aCol <= 0)
      return "";

    for (let i = 2; i < aCol; i += i)
      aChar += aChar;

    return aChar + aChar.slice(0, aCol - aChar.length);
  }
};
