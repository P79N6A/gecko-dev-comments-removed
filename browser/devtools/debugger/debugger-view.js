




"use strict";

const PROPERTY_VIEW_FLASH_DURATION = 400; 





let DebuggerView = {

  


  editor: null,

  


  initializePanes: function DV_initializePanes() {
    let stackframes = document.getElementById("stackframes");
    stackframes.setAttribute("width", Prefs.stackframesWidth);

    let variables = document.getElementById("variables");
    variables.setAttribute("width", Prefs.variablesWidth);
  },

  


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

  


  destroyPanes: function DV_destroyPanes() {
    let stackframes = document.getElementById("stackframes");
    Prefs.stackframesWidth = stackframes.getAttribute("width");

    let variables = document.getElementById("variables");
    Prefs.variablesWidth = variables.getAttribute("width");
  },

  


  destroyEditor: function DV_destroyEditor() {
    DebuggerController.Breakpoints.destroy();
    this.editor = null;
  },

  



  _onEditorLoad: function DV__onEditorLoad() {
    DebuggerController.Breakpoints.initialize();
  },

  



  showCloseButton: function DV_showCloseButton(aVisibleFlag) {
    document.getElementById("close").setAttribute("hidden", !aVisibleFlag);
  }
};




function RemoteDebuggerPrompt() {

  


  this.remote = {};
}

RemoteDebuggerPrompt.prototype = {

  





  show: function RDP_show(aIsReconnectingFlag) {
    let check = { value: Prefs.remoteAutoConnect };
    let input = { value: Prefs.remoteHost + ":" + Prefs.remotePort };
    let parts;

    while (true) {
      let result = Services.prompt.prompt(null,
        L10N.getStr("remoteDebuggerPromptTitle"),
        L10N.getStr(aIsReconnectingFlag
          ? "remoteDebuggerReconnectMessage"
          : "remoteDebuggerPromptMessage"), input,
        L10N.getStr("remoteDebuggerPromptCheck"), check);

      Prefs.remoteAutoConnect = check.value;

      if (!result) {
        return false;
      }
      if ((parts = input.value.split(":")).length === 2) {
        let [host, port] = parts;

        if (host.length && port.length) {
          this.remote = { host: host, port: port };
          return true;
        }
      }
    }
  }
};




function ScriptsView() {
  this._onScriptsChange = this._onScriptsChange.bind(this);
  this._onScriptsSearch = this._onScriptsSearch.bind(this);
  this._onScriptsKeyUp = this._onScriptsKeyUp.bind(this);
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

  







  containsIgnoringQuery: function DVS_containsIgnoringQuery(aUrl) {
    let sourceScripts = DebuggerController.SourceScripts;
    aUrl = sourceScripts.trimUrlQuery(aUrl);

    if (this._tmpScripts.some(function(element) {
      return sourceScripts.trimUrlQuery(element.script.url) == aUrl;
    })) {
      return true;
    }
    if (this.scriptLocations.some(function(url) {
      return sourceScripts.trimUrlQuery(url) == aUrl;
    })) {
      return true;
    }
    return false;
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
    
    if (aLabel == "null" || this.containsLabel(aLabel) || this.contains(aScript.url)) {
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

  





  _getSearchboxInfo: function DVS__getSearchboxInfo() {
    let rawValue = this._searchbox.value.toLowerCase();

    let rawLength = rawValue.length;
    let lastColon = rawValue.lastIndexOf(":");
    let lastAt = rawValue.lastIndexOf("#");

    let fileEnd = lastColon != -1 ? lastColon : lastAt != -1 ? lastAt : rawLength;
    let lineEnd = lastAt != -1 ? lastAt : rawLength;

    let file = rawValue.slice(0, fileEnd);
    let line = window.parseInt(rawValue.slice(fileEnd + 1, lineEnd)) || -1;
    let token = rawValue.slice(lineEnd + 1);

    return [file, line, token];
  },

  


  _onScriptsChange: function DVS__onScriptsChange() {
    let script = this._scripts.selectedItem.getUserData("sourceScript");
    this._preferredScript = script;
    DebuggerController.SourceScripts.showScript(script);
  },

  


  _onScriptsSearch: function DVS__onScriptsSearch(e) {
    let editor = DebuggerView.editor;
    let scripts = this._scripts;
    let [file, line, token] = this._getSearchboxInfo();

    
    scripts.selectedItem = this._preferredScript;

    
    if (!file) {
      for (let i = 0, l = scripts.itemCount; i < l; i++) {
        scripts.getItemAtIndex(i).hidden = false;
      }
    } else {
      for (let i = 0, l = scripts.itemCount, found = false; i < l; i++) {
        let item = scripts.getItemAtIndex(i);
        let target = item.label.toLowerCase();

        
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
        editor.setSelection(offset, offset + token.length)
      }
    }
  },

  


  _onScriptsKeyUp: function DVS__onScriptsKeyUp(e) {
    if (e.keyCode === e.DOM_VK_ESCAPE) {
      DebuggerView.editor.focus();
      return;
    }

    if (e.keyCode === e.DOM_VK_RETURN || e.keyCode === e.DOM_VK_ENTER) {
      let token = this._getSearchboxInfo()[2];
      let editor = DebuggerView.editor;
      let offset = editor.findNext(true);
      if (offset > -1) {
        editor.setSelection(offset, offset + token.length)
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
  this._onPauseExceptionsClick = this._onPauseExceptionsClick.bind(this);
  this._onCloseButtonClick = this._onCloseButtonClick.bind(this);
  this._onResumeButtonClick = this._onResumeButtonClick.bind(this);
  this._onStepOverClick = this._onStepOverClick.bind(this);
  this._onStepInClick = this._onStepInClick.bind(this);
  this._onStepOutClick = this._onStepOutClick.bind(this);
}

StackFramesView.prototype = {

  





   updateState: function DVF_updateState(aState) {
     let resume = document.getElementById("resume");

     
     if (aState == "paused") {
       resume.setAttribute("tooltiptext", L10N.getStr("resumeTooltip"));
       resume.setAttribute("checked", true);
     }
     
     else if (aState == "attached") {
       resume.setAttribute("tooltiptext", L10N.getStr("pauseTooltip"));
       resume.removeAttribute("checked");
     }
   },

  


  empty: function DVF_empty() {
    while (this._frames.firstChild) {
      this._frames.removeChild(this._frames.firstChild);
    }
  },

  



  emptyText: function DVF_emptyText() {
    
    this.empty();

    let item = document.createElement("label");

    
    item.className = "list-item empty";
    item.setAttribute("value", L10N.getStr("emptyStackText"));

    this._frames.appendChild(item);
  },

  













  addFrame: function DVF_addFrame(aDepth, aFrameNameText, aFrameDetailsText) {
    
    if (document.getElementById("stackframe-" + aDepth)) {
      return null;
    }

    let frame = document.createElement("box");
    let frameName = document.createElement("label");
    let frameDetails = document.createElement("label");

    
    frame.id = "stackframe-" + aDepth;
    frame.className = "dbg-stackframe list-item";

    
    frameName.className = "dbg-stackframe-name plain";
    frameDetails.className = "dbg-stackframe-details plain";
    frameName.setAttribute("value", aFrameNameText);
    frameDetails.setAttribute("value", aFrameDetailsText);

    let spacer = document.createElement("spacer");
    spacer.setAttribute("flex", "1");

    frame.appendChild(frameName);
    frame.appendChild(spacer);
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

  


  _onPauseExceptionsClick: function DVF__onPauseExceptionsClick() {
    let option = document.getElementById("pause-exceptions");
    DebuggerController.StackFrames.updatePauseOnExceptions(option.checked);
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
    let pauseOnExceptions = document.getElementById("pause-exceptions");
    let resume = document.getElementById("resume");
    let stepOver = document.getElementById("step-over");
    let stepIn = document.getElementById("step-in");
    let stepOut = document.getElementById("step-out");
    let frames = document.getElementById("stackframes");

    close.addEventListener("click", this._onCloseButtonClick, false);
    pauseOnExceptions.checked = DebuggerController.StackFrames.pauseOnExceptions;
    pauseOnExceptions.addEventListener("click",
                                        this._onPauseExceptionsClick,
                                        false);
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
    let pauseOnExceptions = document.getElementById("pause-exceptions");
    let resume = document.getElementById("resume");
    let stepOver = document.getElementById("step-over");
    let stepIn = document.getElementById("step-in");
    let stepOut = document.getElementById("step-out");
    let frames = this._frames;

    close.removeEventListener("click", this._onCloseButtonClick, false);
    pauseOnExceptions.removeEventListener("click",
                                          this._onPauseExceptionsClick,
                                          false);
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
  this.addScope = this._addScope.bind(this);
  this._addVar = this._addVar.bind(this);
  this._addProperties = this._addProperties.bind(this);
}

PropertiesView.prototype = {
  



  _idCount: 1,

  












  _addScope: function DVP__addScope(aName, aId) {
    
    if (!this._vars) {
      return null;
    }

    
    aId = aId || aName.toLowerCase().trim().replace(/\s+/g, "-") + this._idCount++;

    
    let element = this._createPropertyElement(aName, aId, "scope", this._vars);

    
    if (!element) {
      return null;
    }
    element._identifier = aName;

    


    element.addVar = this._addVar.bind(this, element);

    


    element.addToHierarchy = this.addScopeToHierarchy.bind(this, element);

    
    element.refresh(function() {
      let title = element.getElementsByClassName("title")[0];
      title.classList.add("devtools-toolbar");
    }.bind(this));

    
    return element;
  },

  


  empty: function DVP_empty() {
    while (this._vars.firstChild) {
      this._vars.removeChild(this._vars.firstChild);
    }
  },

  



  emptyText: function DVP_emptyText() {
    
    this.empty();

    let item = document.createElement("label");

    
    item.className = "list-item empty";
    item.setAttribute("value", L10N.getStr("emptyVariablesText"));

    this._vars.appendChild(item);
  },

  















  _addVar: function DVP__addVar(aScope, aName, aFlags, aId) {
    
    if (!aScope) {
      return null;
    }

    
    aId = aId || (aScope.id + "->" + aName + "-variable");

    
    let element = this._createPropertyElement(aName, aId, "variable",
                                              aScope.getElementsByClassName("details")[0]);

    
    if (!element) {
      return null;
    }
    element._identifier = aName;

    


    element.setGrip = this._setGrip.bind(this, element);

    


    element.addProperties = this._addProperties.bind(this, element);

    
    element.refresh(function() {
      let separatorLabel = document.createElement("label");
      let valueLabel = document.createElement("label");
      let title = element.getElementsByClassName("title")[0];

      
      this._setAttributes(element, aName, aFlags);

      
      separatorLabel.className = "plain";
      separatorLabel.setAttribute("value", ":");

      
      valueLabel.className = "value plain";

      
      valueLabel.addEventListener("click", this._activateElementInputMode.bind({
        scope: this,
        element: element,
        valueLabel: valueLabel
      }));

      
      Object.defineProperty(element, "token", {
        value: aName,
        writable: false,
        enumerable: true,
        configurable: true
      });

      title.appendChild(separatorLabel);
      title.appendChild(valueLabel);

      
      this._saveHierarchy({
        parent: aScope,
        element: element,
        valueLabel: valueLabel
      });
    }.bind(this));

    
    return element;
  },

  









  _setAttributes: function DVP_setAttributes(aVar, aName, aFlags) {
    if (aFlags) {
      if (!aFlags.configurable) {
        aVar.setAttribute("non-configurable", "");
      }
      if (!aFlags.enumerable) {
        aVar.setAttribute("non-enumerable", "");
      }
      if (!aFlags.writable) {
        aVar.setAttribute("non-writable", "");
      }
    }
    if (aName === "this") {
      aVar.setAttribute("self", "");
    }
    if (aName === "__proto__ ") {
      aVar.setAttribute("proto", "");
    }
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

    let valueLabel = aVar.getElementsByClassName("value")[0];

    
    if (!valueLabel) {
      return null;
    }

    this._applyGrip(valueLabel, aGrip);
    return aVar;
  },

  








  _applyGrip: function DVP__applyGrip(aValueLabel, aGrip) {
    let prevGrip = aValueLabel.currentGrip;
    if (prevGrip) {
      aValueLabel.classList.remove(this._propertyColor(prevGrip));
    }

    aValueLabel.setAttribute("value", this._propertyString(aGrip));
    aValueLabel.classList.add(this._propertyColor(aGrip));
    aValueLabel.currentGrip = aGrip;
  },

  





















  _addProperties: function DVP__addProperties(aVar, aProperties) {
    
    for (let i in aProperties) {
      
      if (Object.getOwnPropertyDescriptor(aProperties, i)) {

        
        let desc = aProperties[i];

        
        
        let value = desc["value"];

        
        
        let getter = desc["get"];
        let setter = desc["set"];

        
        if (value !== undefined) {
          this._addProperty(aVar, [i, value], desc);
        }
        if (getter !== undefined || setter !== undefined) {
          let prop = this._addProperty(aVar, [i]).expand();
          prop.getter = this._addProperty(prop, ["get", getter], desc);
          prop.setter = this._addProperty(prop, ["set", setter], desc);
        }
      }
    }
    return aVar;
  },

  

























  _addProperty: function DVP__addProperty(aVar, aProperty, aFlags, aName, aId) {
    
    if (!aVar) {
      return null;
    }

    
    aId = aId || (aVar.id + "->" + aProperty[0] + "-property");

    
    let element = this._createPropertyElement(aName, aId, "property",
                                              aVar.getElementsByClassName("details")[0]);

    
    if (!element) {
      return null;
    }
    element._identifier = aName;

    


    element.setGrip = this._setGrip.bind(this, element);

    


    element.addProperties = this._addProperties.bind(this, element);

    
    element.refresh(function(pKey, pGrip) {
      let title = element.getElementsByClassName("title")[0];
      let nameLabel = title.getElementsByClassName("name")[0];
      let separatorLabel = document.createElement("label");
      let valueLabel = document.createElement("label");

      
      this._setAttributes(element, pKey, aFlags);

      if ("undefined" !== typeof pKey) {
        
        nameLabel.className = "key plain";
        nameLabel.setAttribute("value", pKey.trim());
        title.appendChild(nameLabel);
      }
      if ("undefined" !== typeof pGrip) {
        
        separatorLabel.className = "plain";
        separatorLabel.setAttribute("value", ":");

        
        valueLabel.className = "value plain";
        this._applyGrip(valueLabel, pGrip);

        title.appendChild(separatorLabel);
        title.appendChild(valueLabel);
      }

      
      valueLabel.addEventListener("click", this._activateElementInputMode.bind({
        scope: this,
        element: element,
        valueLabel: valueLabel
      }));

      
      Object.defineProperty(element, "token", {
        value: aVar.token + "['" + pKey + "']",
        writable: false,
        enumerable: true,
        configurable: true
      });

      
      this._saveHierarchy({
        parent: aVar,
        element: element,
        valueLabel: valueLabel
      });

      
      Object.defineProperty(aVar, pKey, { value: element,
                                          writable: false,
                                          enumerable: true,
                                          configurable: true });
    }.bind(this), aProperty);

    
    return element;
  },

  











  _activateElementInputMode: function DVP__activateElementInputMode(aEvent) {
    if (aEvent) {
      aEvent.stopPropagation();
    }

    let self = this.scope;
    let element = this.element;
    let valueLabel = this.valueLabel;
    let titleNode = valueLabel.parentNode;
    let initialValue = valueLabel.getAttribute("value");

    
    
    element._previouslyExpanded = element.expanded;
    element._preventExpand = true;
    element.collapse();
    element.forceHideArrow();

    
    
    let textbox = document.createElement("textbox");
    textbox.setAttribute("value", initialValue);
    textbox.className = "element-input";
    textbox.width = valueLabel.clientWidth + 1;

    
    function DVP_element_textbox_blur(aTextboxEvent) {
      DVP_element_textbox_save();
    }

    function DVP_element_textbox_keyup(aTextboxEvent) {
      if (aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_LEFT ||
          aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_RIGHT ||
          aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_UP ||
          aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_DOWN) {
        return;
      }
      if (aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_RETURN ||
          aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_ENTER) {
        DVP_element_textbox_save();
        return;
      }
      if (aTextboxEvent.keyCode === aTextboxEvent.DOM_VK_ESCAPE) {
        valueLabel.setAttribute("value", initialValue);
        DVP_element_textbox_clear();
        return;
      }
    }

    
    function DVP_element_textbox_save() {
      if (textbox.value !== valueLabel.getAttribute("value")) {
        valueLabel.setAttribute("value", textbox.value);

        let expr = "(" + element.token + "=" + textbox.value + ")";
        DebuggerController.StackFrames.evaluate(expr);
      }
      DVP_element_textbox_clear();
    }

    
    function DVP_element_textbox_clear() {
      element._preventExpand = false;
      if (element._previouslyExpanded) {
        element._previouslyExpanded = false;
        element.expand();
      }
      element.showArrow();

      textbox.removeEventListener("blur", DVP_element_textbox_blur, false);
      textbox.removeEventListener("keyup", DVP_element_textbox_keyup, false);
      titleNode.removeChild(textbox);
      titleNode.appendChild(valueLabel);
    }

    textbox.addEventListener("blur", DVP_element_textbox_blur, false);
    textbox.addEventListener("keyup", DVP_element_textbox_keyup, false);
    titleNode.removeChild(valueLabel);
    titleNode.appendChild(textbox);

    textbox.select();

    
    
    
    if (valueLabel.getAttribute("value").match(/^"[^"]*"$/)) {
      textbox.selectionEnd--;
      textbox.selectionStart++;
    }
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

    let element = document.createElement("vbox");
    let arrow = document.createElement("box");
    let name = document.createElement("label");

    let title = document.createElement("box");
    let details = document.createElement("vbox");

    
    element.id = aId;
    element.className = aClass;

    
    arrow.className = "arrow";
    arrow.style.visibility = "hidden";

    
    name.className = "name plain";
    name.setAttribute("value", aName || "");

    
    title.className = "title";
    title.setAttribute("align", "center")

    
    details.className = "details";

    
    if (aClass === "scope") {
      title.addEventListener("click", function() { element.toggle(); }, false);
    } else {
      arrow.addEventListener("click", function() { element.toggle(); }, false);
      name.addEventListener("click", function() { element.toggle(); }, false);
      name.addEventListener("mouseover", function() { element.updateTooltip(name); }, false);
    }

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

    







    element.expand = function DVP_element_expand(aSkipAnimationFlag) {
      if (element._preventExpand) {
        return;
      }
      arrow.setAttribute("open", "");
      details.setAttribute("open", "");

      if (!aSkipAnimationFlag) {
        details.setAttribute("animated", "");
      }
      if ("function" === typeof element.onexpand) {
        element.onexpand(element);
      }
      return element;
    };

    




    element.collapse = function DVP_element_collapse() {
      if (element._preventCollapse) {
        return;
      }
      arrow.removeAttribute("open");
      details.removeAttribute("open");
      details.removeAttribute("animated");

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
      if (element._forceShowArrow || details.childNodes.length) {
        arrow.style.visibility = "visible";
      }
      return element;
    };

    




    element.hideArrow = function DVP_element_hideArrow() {
      if (!element._forceShowArrow) {
        arrow.style.visibility = "hidden";
      }
      return element;
    };

    






    element.forceShowArrow = function DVP_element_forceShowArrow() {
      element._forceShowArrow = true;
      arrow.style.visibility = "visible";
      return element;
    };

    






    element.forceHideArrow = function DVP_element_forceHideArrow() {
      arrow.style.visibility = "hidden";
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

    




    Object.defineProperty(element, "arrowVisible", {
      get: function DVP_element_getArrowVisible() {
        return arrow.style.visibility !== "hidden";
      },
      set: function DVP_element_setExpanded(value) {
        if (value) {
          element.showArrow();
        } else {
          element.hideArrow();
        }
      }
    });

    





    element.updateTooltip = function DVP_element_updateTooltip(aAnchor) {
      let tooltip = document.getElementById("element-tooltip");
      if (tooltip) {
        document.documentElement.removeChild(tooltip);
      }

      tooltip = document.createElement("tooltip");
      tooltip.id = "element-tooltip";

      let configurableLabel = document.createElement("label");
      configurableLabel.id = "configurableLabel";
      configurableLabel.setAttribute("value", "configurable");

      let enumerableLabel = document.createElement("label");
      enumerableLabel.id = "enumerableLabel";
      enumerableLabel.setAttribute("value", "enumerable");

      let writableLabel = document.createElement("label");
      writableLabel.id = "writableLabel";
      writableLabel.setAttribute("value", "writable");

      tooltip.setAttribute("orient", "horizontal")
      tooltip.appendChild(configurableLabel);
      tooltip.appendChild(enumerableLabel);
      tooltip.appendChild(writableLabel);

      if (element.hasAttribute("non-configurable")) {
        configurableLabel.setAttribute("non-configurable", "");
      }
      if (element.hasAttribute("non-enumerable")) {
        enumerableLabel.setAttribute("non-enumerable", "");
      }
      if (element.hasAttribute("non-writable")) {
        writableLabel.setAttribute("non-writable", "");
      }

      document.documentElement.appendChild(tooltip);
      aAnchor.setAttribute("tooltip", tooltip.id);
    };

    








    element.refresh = function DVP_element_refresh(aFunction, aArguments) {
      if ("function" === typeof aFunction) {
        aFunction.apply(this, aArguments);
      }

      let node = aParent.parentNode;
      let arrow = node.getElementsByClassName("arrow")[0];
      let children = node.getElementsByClassName("details")[0].childNodes.length;

      
      
      if (children) {
        arrow.style.visibility = "visible";
      } else {
        arrow.style.visibility = "hidden";
      }
    }.bind(this);

    
    return element;
  },

  





  _saveHierarchy: function DVP__saveHierarchy(aProperties) {
    let parent = aProperties.parent;
    let element = aProperties.element;
    let valueLabel = aProperties.valueLabel;
    let store = aProperties.store || parent._children;

    
    if (!element || !store) {
      return;
    }

    let relation = {
      root: parent ? (parent._root || parent) : null,
      parent: parent || null,
      element: element,
      valueLabel: valueLabel,
      children: {}
    };

    store[element._identifier] = relation;
    element._root = relation.root;
    element._children = relation.children;
  },

  



  createHierarchyStore: function DVP_createHierarchyStore() {
    this._prevHierarchy = this._currHierarchy;
    this._currHierarchy = {};
  },

  





  addScopeToHierarchy: function DVP_addScopeToHierarchy(aScope) {
    this._saveHierarchy({ element: aScope, store: this._currHierarchy });
  },

  


  commitHierarchy: function DVS_commitHierarchy() {
    for (let i in this._currHierarchy) {
      let currScope = this._currHierarchy[i];
      let prevScope = this._prevHierarchy[i];

      if (!prevScope) {
        continue;
      }

      for (let v in currScope.children) {
        let currVar = currScope.children[v];
        let prevVar = prevScope.children[v];

        let action = "";

        if (prevVar) {
          let prevValue = prevVar.valueLabel.getAttribute("value");
          let currValue = currVar.valueLabel.getAttribute("value");

          if (currValue != prevValue) {
            action = "changed";
          } else {
            action = "unchanged";
          }
        } else {
          action = "added";
        }

        if (action) {
          currVar.element.setAttribute(action, "");

          window.setTimeout(function() {
           currVar.element.removeAttribute(action);
          }, PROPERTY_VIEW_FLASH_DURATION);
        }
      }
    }
  },

  



  _currHierarchy: null,
  _prevHierarchy: null,

  


  _vars: null,

  


  initialize: function DVP_initialize() {
    this.createHierarchyStore();

    this._vars = document.getElementById("variables");
  },

  


  destroy: function DVP_destroy() {
    this._currHierarchy = null;
    this._prevHierarchy = null;
    this._vars = null;
  }
};




DebuggerView.Scripts = new ScriptsView();
DebuggerView.StackFrames = new StackFramesView();
DebuggerView.Properties = new PropertiesView();




Object.defineProperty(window, "editor", {
  get: function() { return DebuggerView.editor; }
});
