






































"use strict";





let DebuggerView = {

  


  editor: null,

  


  initializeEditor: function DV_initializeEditor() {
    let placeholder = document.getElementById("editor");

    let config = {
      mode: SourceEditor.MODES.JAVASCRIPT,
      showLineNumbers: true,
      readOnly: true,
      showAnnotationRuler: true,
      showOverviewRuler: true,
    };

    this.editor = new SourceEditor();
    this.editor.init(placeholder, config, this._onEditorLoad.bind(this));
  },

  


  destroyEditor: function DV_destroyEditor() {
    DebuggerController.Breakpoints.destroy();
    this.editor = null;
  },

  



  _onEditorLoad: function DV__onEditorLoad() {
    DebuggerController.Breakpoints.initialize();
  }
};




function ScriptsView() {
  this._onScriptsChange = this._onScriptsChange.bind(this);
  this._onScriptsSearch = this._onScriptsSearch.bind(this);
}

ScriptsView.prototype = {

  


  empty: function DVS_empty() {
    while (this._scripts.firstChild) {
      this._scripts.removeChild(this._scripts.firstChild);
    }
  },

  


  clearSearch: function DVS_clearSearch() {
    this._searchbox.value = "";
    this._onScriptsSearch({});
  },

  







  contains: function DVS_contains(aUrl) {
    if (this._tmpScripts.some(function(element) {
      return element.script.url == aUrl;
    })) {
      return true;
    }
    if (this._scripts.getElementsByAttribute("value", aUrl).length > 0) {
      return true;
    }
    return false;
  },

  







  containsLabel: function DVS_containsLabel(aLabel) {
    if (this._tmpScripts.some(function(element) {
      return element.label == aLabel;
    })) {
      return true;
    }
    if (this._scripts.getElementsByAttribute("label", aLabel).length > 0) {
      return true;
    }
    return false;
  },

  





  selectScript: function DVS_selectScript(aUrl) {
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      if (this._scripts.getItemAtIndex(i).value == aUrl) {
        this._scripts.selectedIndex = i;
        return;
      }
    }
  },

  





  isSelected: function DVS_isSelected(aUrl) {
    if (this._scripts.selectedItem &&
        this._scripts.selectedItem.value == aUrl) {
      return true;
    }
    return false;
  },

  



  get selected() {
    return this._scripts.selectedItem ?
           this._scripts.selectedItem.value : null;
  },

  



  get scriptLabels() {
    let labels = [];
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      labels.push(this._scripts.getItemAtIndex(i).label);
    }
    return labels;
  },

  



  get scriptLocations() {
    let locations = [];
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      locations.push(this._scripts.getItemAtIndex(i).value);
    }
    return locations;
  },

  



  get visibleItemsCount() {
    let count = 0;
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      count += this._scripts.getItemAtIndex(i).hidden ? 0 : 1;
    }
    return count;
  },

  
















  addScript: function DVS_addScript(aLabel, aScript, aForceFlag) {
    
    if (!aForceFlag) {
      this._tmpScripts.push({ label: aLabel, script: aScript });
      return;
    }

    
    for (let i = 0, l = this._scripts.itemCount; i < l; i++) {
      if (this._scripts.getItemAtIndex(i).label > aLabel) {
        this._createScriptElement(aLabel, aScript, i);
        return;
      }
    }
    
    this._createScriptElement(aLabel, aScript, -1, true);
  },

  



  commitScripts: function DVS_commitScripts() {
    let newScripts = this._tmpScripts;
    this._tmpScripts = [];

    if (!newScripts || !newScripts.length) {
      return;
    }
    newScripts.sort(function(a, b) {
      return a.label.toLowerCase() > b.label.toLowerCase();
    });

    for (let i = 0, l = newScripts.length; i < l; i++) {
      let item = newScripts[i];
      this._createScriptElement(item.label, item.script, -1, true);
    }
  },

  














  _createScriptElement: function DVS__createScriptElement(
    aLabel, aScript, aIndex, aSelectIfEmptyFlag)
  {
    
    if (aLabel == "null" || this.containsLabel(aLabel)) {
      return;
    }

    let scriptItem =
      aIndex == -1 ? this._scripts.appendItem(aLabel, aScript.url)
                   : this._scripts.insertItemAt(aIndex, aLabel, aScript.url);

    scriptItem.setAttribute("tooltiptext", aScript.url);
    scriptItem.setUserData("sourceScript", aScript, null);

    if (this._scripts.itemCount == 1 && aSelectIfEmptyFlag) {
      this._scripts.selectedItem = scriptItem;
    }
  },

  


  _onScriptsChange: function DVS__onScriptsChange() {
    let script = this._scripts.selectedItem.getUserData("sourceScript");
    this._preferredScript = script;
    DebuggerController.SourceScripts.showScript(script);
  },

  


  _onScriptsSearch: function DVS__onScriptsSearch(e) {
    let editor = DebuggerView.editor;
    let scripts = this._scripts;
    let rawValue = this._searchbox.value.toLowerCase();

    let rawLength = rawValue.length;
    let lastColon = rawValue.lastIndexOf(":");
    let lastAt = rawValue.lastIndexOf("@");

    let fileEnd = lastColon != -1 ? lastColon : lastAt != -1 ? lastAt : rawLength;
    let lineEnd = lastAt != -1 ? lastAt : rawLength;

    let file = rawValue.slice(0, fileEnd);
    let line = window.parseInt(rawValue.slice(fileEnd + 1, lineEnd)) || -1;
    let token = rawValue.slice(lineEnd + 1);

    
    scripts.selectedItem = this._preferredScript;

    
    if (!file) {
      for (let i = 0, l = scripts.itemCount; i < l; i++) {
        scripts.getItemAtIndex(i).hidden = false;
      }
    } else {
      for (let i = 0, l = scripts.itemCount, found = false; i < l; i++) {
        let item = scripts.getItemAtIndex(i);
        let target = item.value.toLowerCase();

        
        if (target.match(file)) {
          item.hidden = false;

          if (!found) {
            found = true;
            scripts.selectedItem = item;
          }
        }
        
        else {
          item.hidden = true;
        }
      }
    }
    if (line > -1) {
      editor.setCaretPosition(line - 1);
    }
    if (token) {
      let offset = editor.find(token, { ignoreCase: true });
      if (offset > -1) {
        editor.setCaretPosition(0);
        editor.setCaretOffset(offset);
      }
    }
  },

  


  _onScriptsKeyUp: function DVS__onScriptsKeyUp(e) {
    if (e.keyCode === e.DOM_VK_ESCAPE) {
      DebuggerView.editor.focus();
      return;
    }

    if (e.keyCode === e.DOM_VK_RETURN || e.keyCode === e.DOM_VK_ENTER) {
      let editor = DebuggerView.editor;
      let offset = editor.findNext(true);
      if (offset > -1) {
        editor.setCaretPosition(0);
        editor.setCaretOffset(offset);
      }
    }
  },

  


  _scripts: null,
  _searchbox: null,

  


  initialize: function DVS_initialize() {
    this._scripts = document.getElementById("scripts");
    this._searchbox = document.getElementById("scripts-search");
    this._scripts.addEventListener("select", this._onScriptsChange, false);
    this._searchbox.addEventListener("select", this._onScriptsSearch, false);
    this._searchbox.addEventListener("input", this._onScriptsSearch, false);
    this._searchbox.addEventListener("keyup", this._onScriptsKeyUp, false);
    this.commitScripts();
  },

  


  destroy: function DVS_destroy() {
    this._scripts.removeEventListener("select", this._onScriptsChange, false);
    this._searchbox.removeEventListener("select", this._onScriptsSearch, false);
    this._searchbox.removeEventListener("input", this._onScriptsSearch, false);
    this._searchbox.removeEventListener("keyup", this._onScriptsKeyUp, false);
    this._scripts = null;
    this._searchbox = null;
  }
};




function StackFramesView() {
  this._onFramesScroll = this._onFramesScroll.bind(this);
  this._onCloseButtonClick = this._onCloseButtonClick.bind(this);
  this._onResumeButtonClick = this._onResumeButtonClick.bind(this);
  this._onStepOverClick = this._onStepOverClick.bind(this);
  this._onStepInClick = this._onStepInClick.bind(this);
  this._onStepOutClick = this._onStepOutClick.bind(this);
}

StackFramesView.prototype = {

  





  updateState: function DVF_updateState(aState) {
    let resume = document.getElementById("resume");
    let status = document.getElementById("status");

    
    if (aState == "paused") {
      status.textContent = L10N.getStr("pausedState");
      resume.label = L10N.getStr("resumeLabel");
    }
    
    else if (aState == "attached") {
      status.textContent = L10N.getStr("runningState");
      resume.label = L10N.getStr("pauseLabel");
    }
    
    else {
      status.textContent = "";
    }

    DebuggerView.Scripts.clearSearch();
  },

  


  empty: function DVF_empty() {
    while (this._frames.firstChild) {
      this._frames.removeChild(this._frames.firstChild);
    }
  },

  



  emptyText: function DVF_emptyText() {
    
    this.empty();

    let item = document.createElement("div");

    
    item.className = "empty list-item";
    item.appendChild(document.createTextNode(L10N.getStr("emptyText")));

    this._frames.appendChild(item);
  },

  













  addFrame: function DVF_addFrame(aDepth, aFrameNameText, aFrameDetailsText) {
    
    if (document.getElementById("stackframe-" + aDepth)) {
      return null;
    }

    let frame = document.createElement("div");
    let frameName = document.createElement("span");
    let frameDetails = document.createElement("span");

    
    frame.id = "stackframe-" + aDepth;
    frame.className = "dbg-stackframe list-item";

    
    frameName.className = "dbg-stackframe-name";
    frameDetails.className = "dbg-stackframe-details";
    frameName.appendChild(document.createTextNode(aFrameNameText));
    frameDetails.appendChild(document.createTextNode(aFrameDetailsText));

    frame.appendChild(frameName);
    frame.appendChild(frameDetails);

    this._frames.appendChild(frame);

    
    return frame;
  },

  







  highlightFrame: function DVF_highlightFrame(aDepth, aFlag) {
    let frame = document.getElementById("stackframe-" + aDepth);

    
    if (!frame) {
      return;
    }

    
    if (!aFlag && !frame.classList.contains("selected")) {
      frame.classList.add("selected");
    }
    
    else if (aFlag && frame.classList.contains("selected")) {
      frame.classList.remove("selected");
    }
  },

  





  unhighlightFrame: function DVF_unhighlightFrame(aDepth) {
    this.highlightFrame(aDepth, true);
  },

  





  get dirty() {
    return this._dirty;
  },

  





  set dirty(aValue) {
    this._dirty = aValue;
  },

  


  _onFramesClick: function DVF__onFramesClick(aEvent) {
    let target = aEvent.target;

    while (target) {
      if (target.debuggerFrame) {
        DebuggerController.StackFrames.selectFrame(target.debuggerFrame.depth);
        return;
      }
      target = target.parentNode;
    }
  },

  


  _onFramesScroll: function DVF__onFramesScroll(aEvent) {
    
    if (this._dirty) {
      let clientHeight = this._frames.clientHeight;
      let scrollTop = this._frames.scrollTop;
      let scrollHeight = this._frames.scrollHeight;

      
      
      if (scrollTop >= (scrollHeight - clientHeight) * 0.95) {
        this._dirty = false;

        DebuggerController.StackFrames.addMoreFrames();
      }
    }
  },

  


  _onCloseButtonClick: function DVF__onCloseButtonClick() {
    DebuggerController.dispatchEvent("Debugger:Close");
  },

  


  _onResumeButtonClick: function DVF__onResumeButtonClick() {
    if (DebuggerController.activeThread.paused) {
      DebuggerController.activeThread.resume();
    } else {
      DebuggerController.activeThread.interrupt();
    }
  },

  


  _onStepOverClick: function DVF__onStepOverClick() {
    DebuggerController.activeThread.stepOver();
  },

  


  _onStepInClick: function DVF__onStepInClick() {
    DebuggerController.activeThread.stepIn();
  },

  


  _onStepOutClick: function DVF__onStepOutClick() {
    DebuggerController.activeThread.stepOut();
  },

  


  _dirty: false,

  


  _frames: null,

  


  initialize: function DVF_initialize() {
    let close = document.getElementById("close");
    let resume = document.getElementById("resume");
    let stepOver = document.getElementById("step-over");
    let stepIn = document.getElementById("step-in");
    let stepOut = document.getElementById("step-out");
    let frames = document.getElementById("stackframes");

    close.addEventListener("click", this._onCloseButtonClick, false);
    resume.addEventListener("click", this._onResumeButtonClick, false);
    stepOver.addEventListener("click", this._onStepOverClick, false);
    stepIn.addEventListener("click", this._onStepInClick, false);
    stepOut.addEventListener("click", this._onStepOutClick, false);
    frames.addEventListener("click", this._onFramesClick, false);
    frames.addEventListener("scroll", this._onFramesScroll, false);
    window.addEventListener("resize", this._onFramesScroll, false);

    this._frames = frames;
  },

  


  destroy: function DVF_destroy() {
    let close = document.getElementById("close");
    let resume = document.getElementById("resume");
    let stepOver = document.getElementById("step-over");
    let stepIn = document.getElementById("step-in");
    let stepOut = document.getElementById("step-out");
    let frames = this._frames;

    close.removeEventListener("click", this._onCloseButtonClick, false);
    resume.removeEventListener("click", this._onResumeButtonClick, false);
    stepOver.removeEventListener("click", this._onStepOverClick, false);
    stepIn.removeEventListener("click", this._onStepInClick, false);
    stepOut.removeEventListener("click", this._onStepOutClick, false);
    frames.removeEventListener("click", this._onFramesClick, false);
    frames.removeEventListener("scroll", this._onFramesScroll, false);
    window.removeEventListener("resize", this._onFramesScroll, false);

    this._frames = null;
  }
};




function PropertiesView() {
  this._addScope = this._addScope.bind(this);
  this._addVar = this._addVar.bind(this);
  this._addProperties = this._addProperties.bind(this);
}

PropertiesView.prototype = {

  












  _addScope: function DVP__addScope(aName, aId) {
    
    if (!this._vars) {
      return null;
    }

    
    aId = aId || (aName.toLowerCase().trim().replace(" ", "-") + "-scope");

    
    let element = this._createPropertyElement(aName, aId, "scope", this._vars);

    
    if (!element) {
      return null;
    }

    


    element.addVar = this._addVar.bind(this, element);

    
    return element;
  },

  













  _addVar: function DVP__addVar(aScope, aName, aId) {
    
    if (!aScope) {
      return null;
    }

    
    aId = aId || (aScope.id + "->" + aName + "-variable");

    
    let element = this._createPropertyElement(aName, aId, "variable",
                                              aScope.querySelector(".details"));

    
    if (!element) {
      return null;
    }

    


    element.setGrip = this._setGrip.bind(this, element);

    


    element.addProperties = this._addProperties.bind(this, element);

    
    element.refresh(function() {
      let separator = document.createElement("span");
      let info = document.createElement("span");
      let title = element.querySelector(".title");
      let arrow = element.querySelector(".arrow");

      
      separator.className = "unselectable";
      separator.appendChild(document.createTextNode(": "));

      
      info.className = "info";

      title.appendChild(separator);
      title.appendChild(info);

    }.bind(this));

    
    return element;
  },

  




















  _setGrip: function DVP__setGrip(aVar, aGrip) {
    
    if (!aVar) {
      return null;
    }
    if (aGrip === undefined) {
      aGrip = { type: "undefined" };
    }
    if (aGrip === null) {
      aGrip = { type: "null" };
    }

    let info = aVar.querySelector(".info") || aVar.target.info;

    
    if (!info) {
      return null;
    }

    info.textContent = this._propertyString(aGrip);
    info.classList.add(this._propertyColor(aGrip));

    return aVar;
  },

  





















  _addProperties: function DVP__addProperties(aVar, aProperties) {
    
    for (let i in aProperties) {
      
      if (Object.getOwnPropertyDescriptor(aProperties, i)) {

        
        let desc = aProperties[i];

        
        
        let value = desc["value"];

        
        
        let getter = desc["get"];
        let setter = desc["set"];

        
        if (value !== undefined) {
          this._addProperty(aVar, [i, value]);
        }
        if (getter !== undefined || setter !== undefined) {
          let prop = this._addProperty(aVar, [i]).expand();
          prop.getter = this._addProperty(prop, ["get", getter]);
          prop.setter = this._addProperty(prop, ["set", setter]);
        }
      }
    }
    return aVar;
  },

  























  _addProperty: function DVP__addProperty(aVar, aProperty, aName, aId) {
    
    if (!aVar) {
      return null;
    }

    
    aId = aId || (aVar.id + "->" + aProperty[0] + "-property");

    
    let element = this._createPropertyElement(aName, aId, "property",
                                              aVar.querySelector(".details"));

    
    if (!element) {
      return null;
    }

    


    element.setGrip = this._setGrip.bind(this, element);

    


    element.addProperties = this._addProperties.bind(this, element);

    
    element.refresh(function(pKey, pGrip) {
      let propertyString = this._propertyString(pGrip);
      let propertyColor = this._propertyColor(pGrip);
      let key = document.createElement("div");
      let value = document.createElement("div");
      let separator = document.createElement("span");
      let title = element.querySelector(".title");
      let arrow = element.querySelector(".arrow");

      
      key.className = "key";
      key.appendChild(document.createTextNode(pKey));

      
      value.className = "value";
      value.appendChild(document.createTextNode(propertyString));
      value.classList.add(propertyColor);

      
      separator.className = "unselectable";
      separator.appendChild(document.createTextNode(": "));

      if ("undefined" !== typeof pKey) {
        title.appendChild(key);
      }
      if ("undefined" !== typeof pGrip) {
        title.appendChild(separator);
        title.appendChild(value);
      }

      
      
      element.target = {
        info: value
      };

      
      Object.defineProperty(aVar, pKey, { value: element,
                                          writable: false,
                                          enumerable: true,
                                          configurable: true });
    }.bind(this), aProperty);

    
    return element;
  },

  







  _propertyString: function DVP__propertyString(aGrip) {
    if (aGrip && "object" === typeof aGrip) {
      switch (aGrip.type) {
        case "undefined":
          return "undefined";
        case "null":
          return "null";
        default:
          return "[" + aGrip.type + " " + aGrip.class + "]";
      }
    } else {
      switch (typeof aGrip) {
        case "string":
          return "\"" + aGrip + "\"";
        case "boolean":
          return aGrip ? "true" : "false";
        default:
          return aGrip + "";
      }
    }
    return aGrip + "";
  },

  








  _propertyColor: function DVP__propertyColor(aGrip) {
    if (aGrip && "object" === typeof aGrip) {
      switch (aGrip.type) {
        case "undefined":
          return "token-undefined";
        case "null":
          return "token-null";
      }
    } else {
      switch (typeof aGrip) {
        case "string":
          return "token-string";
        case "boolean":
          return "token-boolean";
        case "number":
          return "token-number";
      }
    }
    return "token-other";
  },

  














  _createPropertyElement: function DVP__createPropertyElement(aName, aId, aClass, aParent) {
    
    if (document.getElementById(aId)) {
      return null;
    }
    if (!aParent) {
      return null;
    }

    let element = document.createElement("div");
    let arrow = document.createElement("span");
    let name = document.createElement("span");
    let title = document.createElement("div");
    let details = document.createElement("div");

    
    element.id = aId;
    element.className = aClass;

    
    arrow.className = "arrow";
    arrow.style.visibility = "hidden";

    
    name.className = "name unselectable";
    name.appendChild(document.createTextNode(aName || ""));

    
    title.className = "title";
    title.addEventListener("click", function() { element.toggle(); }, true);

    
    details.className = "details";

    title.appendChild(arrow);
    title.appendChild(name);

    element.appendChild(title);
    element.appendChild(details);

    aParent.appendChild(element);

    




    element.show = function DVP_element_show() {
      element.style.display = "-moz-box";

      if ("function" === typeof element.onshow) {
        element.onshow(element);
      }
      return element;
    };

    




    element.hide = function DVP_element_hide() {
      element.style.display = "none";

      if ("function" === typeof element.onhide) {
        element.onhide(element);
      }
      return element;
    };

    




    element.expand = function DVP_element_expand() {
      arrow.setAttribute("open", "");
      details.setAttribute("open", "");

      if ("function" === typeof element.onexpand) {
        element.onexpand(element);
      }
      return element;
    };

    




    element.collapse = function DVP_element_collapse() {
      arrow.removeAttribute("open");
      details.removeAttribute("open");

      if ("function" === typeof element.oncollapse) {
        element.oncollapse(element);
      }
      return element;
    };

    




    element.toggle = function DVP_element_toggle() {
      element.expanded = !element.expanded;

      if ("function" === typeof element.ontoggle) {
        element.ontoggle(element);
      }
      return element;
    };

    




    element.showArrow = function DVP_element_showArrow() {
      if (details.childNodes.length) {
        arrow.style.visibility = "visible";
      }
      return element;
    };

    








    element.forceShowArrow = function DVP_element_forceShowArrow(aPreventHideFlag) {
      element._preventHide = aPreventHideFlag;
      arrow.style.visibility = "visible";
      return element;
    };

    




    element.hideArrow = function DVP_element_hideArrow() {
      if (!element._preventHide) {
        arrow.style.visibility = "hidden";
      }
      return element;
    };

    




    Object.defineProperty(element, "visible", {
      get: function DVP_element_getVisible() {
        return element.style.display !== "none";
      },
      set: function DVP_element_setVisible(value) {
        if (value) {
          element.show();
        } else {
          element.hide();
        }
      }
    });

    




    Object.defineProperty(element, "expanded", {
      get: function DVP_element_getExpanded() {
        return arrow.hasAttribute("open");
      },
      set: function DVP_element_setExpanded(value) {
        if (value) {
          element.expand();
        } else {
          element.collapse();
        }
      }
    });

    




    element.empty = function DVP_element_empty() {
      
      arrow.style.visibility = "hidden";
      while (details.firstChild) {
        details.removeChild(details.firstChild);
      }

      if ("function" === typeof element.onempty) {
        element.onempty(element);
      }
      return element;
    };

    




    element.remove = function DVP_element_remove() {
      element.parentNode.removeChild(element);

      if ("function" === typeof element.onremove) {
        element.onremove(element);
      }
      return element;
    };

    








    element.refresh = function DVP_element_refresh(aFunction, aArguments) {
      if ("function" === typeof aFunction) {
        aFunction.apply(this, aArguments);
      }

      let node = aParent.parentNode;
      let arrow = node.querySelector(".arrow");
      let children = node.querySelector(".details").childNodes.length;

      
      
      if (children) {
        arrow.style.visibility = "visible";
      } else {
        arrow.style.visibility = "hidden";
      }
    }.bind(this);

    
    return element;
  },

  


  get globalScope() {
    return this._globalScope;
  },

  





  set globalScope(aFlag) {
    if (aFlag) {
      this._globalScope.show();
    } else {
      this._globalScope.hide();
    }
  },

  


  get localScope() {
    return this._localScope;
  },

  





  set localScope(aFlag) {
    if (aFlag) {
      this._localScope.show();
    } else {
      this._localScope.hide();
    }
  },

  


  get withScope() {
    return this._withScope;
  },

  





  set withScope(aFlag) {
    if (aFlag) {
      this._withScope.show();
    } else {
      this._withScope.hide();
    }
  },

  


  get closureScope() {
    return this._closureScope;
  },

  





  set closureScope(aFlag) {
    if (aFlag) {
      this._closureScope.show();
    } else {
      this._closureScope.hide();
    }
  },

  


  _vars: null,

  


  _globalScope: null,
  _localScope: null,
  _withScope: null,
  _closureScope: null,

  


  initialize: function DVP_initialize() {
    this._vars = document.getElementById("variables");
    this._localScope = this._addScope(L10N.getStr("localScope")).expand();
    this._withScope = this._addScope(L10N.getStr("withScope")).hide();
    this._closureScope = this._addScope(L10N.getStr("closureScope")).hide();
    this._globalScope = this._addScope(L10N.getStr("globalScope"));
  },

  


  destroy: function DVP_destroy() {
    this._vars = null;
    this._globalScope = null;
    this._localScope = null;
    this._withScope = null;
    this._closureScope = null;
  }
};




DebuggerView.Scripts = new ScriptsView();
DebuggerView.StackFrames = new StackFramesView();
DebuggerView.Properties = new PropertiesView();




Object.defineProperty(window, "editor", {
  get: function() { return DebuggerView.editor; }
});
