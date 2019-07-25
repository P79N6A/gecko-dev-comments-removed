





































"use strict";

const Cu = Components.utils;
const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import('resource://gre/modules/Services.jsm');





let DebuggerView = {

  





  getStr: function DV_getStr(aName) {
    return this.stringBundle.GetStringFromName(aName);
  },

  






  getFormatStr: function DV_getFormatStr(aName, aArray) {
    return this.stringBundle.formatStringFromName(aName, aArray, aArray.length);
  }
};

XPCOMUtils.defineLazyGetter(DebuggerView, "stringBundle", function() {
  return Services.strings.createBundle(DBG_STRINGS_URI);
});




DebuggerView.Stackframes = {

  





  updateState: function DVF_updateState(aState) {
    let resume = document.getElementById("resume");
    let status = document.getElementById("status");

    
    if (aState === "paused") {
      status.textContent = DebuggerView.getStr("pausedState");
      resume.label = DebuggerView.getStr("resumeLabel");
    } else if (aState === "attached") {
      
      status.textContent = DebuggerView.getStr("runningState");
      resume.label = DebuggerView.getStr("pauseLabel");
    } else {
      
      status.textContent = "";
    }
  },

  





  addClickListener: function DVF_addClickListener(aHandler) {
    
    this._onFramesClick = aHandler;
    this._frames.addEventListener("click", aHandler, false);
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
    item.appendChild(document.createTextNode(DebuggerView.getStr("emptyText")));

    this._frames.appendChild(item);
  },

  













  addFrame: function DVF_addFrame(aDepth, aFrameIdText, aFrameNameText) {
    
    if (document.getElementById("stackframe-" + aDepth)) {
      return null;
    }

    let frame = document.createElement("div");
    let frameId = document.createElement("span");
    let frameName = document.createElement("span");

    
    frame.id = "stackframe-" + aDepth;
    frame.className = "dbg-stackframe list-item";

    
    frameId.className = "dbg-stackframe-id";
    frameName.className = "dbg-stackframe-name";
    frameId.appendChild(document.createTextNode(aFrameIdText));
    frameName.appendChild(document.createTextNode(aFrameNameText));

    frame.appendChild(frameId);
    frame.appendChild(frameName);

    this._frames.appendChild(frame);

    
    return frame;
  },

  







  highlightFrame: function DVF_highlightFrame(aDepth, aSelect) {
    let frame = document.getElementById("stackframe-" + aDepth);

    
    if (!frame) {
      dump("The frame list item wasn't found in the stackframes container.");
      return;
    }

    
    if (aSelect && !frame.classList.contains("selected")) {
      frame.classList.add("selected");

    
    } else if (!aSelect && frame.classList.contains("selected")) {
      frame.classList.remove("selected");
    }
  },

  





  get dirty() {
    return this._dirty;
  },

  





  set dirty(aValue) {
    this._dirty = aValue;
  },

  


  _onFramesClick: null,

  


  _onFramesScroll: function DVF__onFramesScroll(aEvent) {
    
    if (this._dirty) {
      let clientHeight = this._frames.clientHeight;
      let scrollTop = this._frames.scrollTop;
      let scrollHeight = this._frames.scrollHeight;

      
      
      if (scrollTop >= (scrollHeight - clientHeight) * 0.95) {
        this._dirty = false;

        StackFrames._addMoreFrames();
      }
    }
  },

  


  _onCloseButtonClick: function DVF__onCloseButtonClick() {
    let root = document.documentElement;
    let debuggerClose = document.createEvent("Events");

    debuggerClose.initEvent("DebuggerClose", true, false);
    root.dispatchEvent(debuggerClose);
  },

  


  _onResumeButtonClick: function DVF__onResumeButtonClick() {
    if (ThreadState.activeThread.paused) {
      ThreadState.activeThread.resume();
    } else {
      ThreadState.activeThread.interrupt();
    }
  },

  


  _dirty: false,

  


  _frames: null,

  


  initialize: function DVF_initialize() {
    let close = document.getElementById("close");
    let resume = document.getElementById("resume");
    let frames = document.getElementById("stackframes");

    close.addEventListener("click", this._onCloseButtonClick, false);
    resume.addEventListener("click", this._onResumeButtonClick, false);
    frames.addEventListener("scroll", this._onFramesScroll, false);
    window.addEventListener("resize", this._onFramesScroll, false);

    this._frames = frames;
  },

  


  destroy: function DVF_destroy() {
    let close = document.getElementById("close");
    let resume = document.getElementById("resume");
    let frames = this._frames;

    close.removeEventListener("click", this._onCloseButtonClick, false);
    resume.removeEventListener("click", this._onResumeButtonClick, false);
    frames.removeEventListener("click", this._onFramesClick, false);
    frames.removeEventListener("scroll", this._onFramesScroll, false);
    window.removeEventListener("resize", this._onFramesScroll, false);

    this._frames = null;
  }
};




DebuggerView.Properties = {

  












  _addScope: function DVP__addScope(aName, aId) {
    
    if (!this._vars) {
      return null;
    }

    
    aId = aId || (aName + "-scope");

    
    let element = this._createPropertyElement(aName, aId, "scope", this._vars);

    
    if (!element) {
      dump("The debugger scope container wasn't created properly: " + aId);
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
      dump("The debugger variable container wasn't created properly: " + aId);
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

    let info = aVar.querySelector(".info") || aVar.target.info;

    
    if (!info) {
      dump("Could not set the grip for the corresponding variable: " + aVar.id);
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
      dump("The debugger property container wasn't created properly.");
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

      if (pKey) {
        title.appendChild(key);
      }
      if (pGrip) {
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
      switch (aGrip["type"]) {
        case "undefined":
          return "undefined";
        case "null":
          return "null";
        default:
          return "[" + aGrip["type"] + " " + aGrip["class"] + "]";
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
      switch (aGrip["type"]) {
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
      dump("Duplicating a property element id is not allowed.");
      return null;
    }
    if (!aParent) {
      dump("A property element must have a valid parent node specified.");
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

  





  set globalScope(value) {
    if (value) {
      this._globalScope.show();
    } else {
      this._globalScope.hide();
    }
  },

  


  get localScope() {
    return this._localScope;
  },

  





  set localScope(value) {
    if (value) {
      this._localScope.show();
    } else {
      this._localScope.hide();
    }
  },

  


  get withScope() {
    return this._withScope;
  },

  





  set withScope(value) {
    if (value) {
      this._withScope.show();
    } else {
      this._withScope.hide();
    }
  },

  


  get closureScope() {
    return this._closureScope;
  },

  





  set closureScope(value) {
    if (value) {
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
    this._localScope = this._addScope(DebuggerView.getStr("localScope")).expand();
    this._withScope = this._addScope(DebuggerView.getStr("withScope")).hide();
    this._closureScope = this._addScope(DebuggerView.getStr("closureScope")).hide();
    this._globalScope = this._addScope(DebuggerView.getStr("globalScope"));
  },

  


  destroy: function DVP_destroy() {
    this._vars = null;
    this._globalScope = null;
    this._localScope = null;
    this._withScope = null;
    this._closureScope = null;
  }
};




DebuggerView.Scripts = {

  





  addChangeListener: function DVS_addChangeListener(aHandler) {
    
    this._onScriptsChange = aHandler;
    this._scripts.addEventListener("select", aHandler, false);
  },

  


  empty: function DVS_empty() {
    while (this._scripts.firstChild) {
      this._scripts.removeChild(this._scripts.firstChild);
    }
  },

  






  contains: function DVS_contains(aUrl) {
    if (this._scripts.getElementsByAttribute("value", aUrl).length > 0) {
      return true;
    }
    return false;
  },

  





  isSelected: function DVS_isSelected(aUrl) {
    if (this._scripts.selectedItem &&
        this._scripts.selectedItem.value == aUrl) {
      return true;
    }
    return false;
  },

  





   selectScript: function DVS_selectScript(aUrl) {
    for (let i = 0; i < this._scripts.itemCount; i++) {
      if (this._scripts.getItemAtIndex(i).value == aUrl) {
        this._scripts.selectedIndex = i;
        break;
      }
    }
   },

  













  addScript: function DVS_addScript(aUrl, aSource, aScriptNameText) {
    
    if (this.contains(aUrl)) {
      return null;
    }

    let script = this._scripts.appendItem(aScriptNameText || aUrl, aUrl);
    script.setUserData("sourceScript", aSource, null);
    this._scripts.selectedItem = script;
    return script;
  },

  


  scriptLocations: function DVS_scriptLocations() {
    let locations = [];
    for (let i = 0; i < this._scripts.itemCount; i++) {
      locations.push(this._scripts.getItemAtIndex(i).value);
    }
    return locations;
  },

  


  _onScriptsChange: null,

  


  _scripts: null,

  


  initialize: function DVS_initialize() {
    this._scripts = document.getElementById("scripts");
  },

  


  destroy: function DVS_destroy() {
    this._scripts.removeEventListener("select", this._onScriptsChange, false);
    this._scripts = null;
  }
};


let DVF = DebuggerView.Stackframes;
DVF._onFramesScroll = DVF._onFramesScroll.bind(DVF);
DVF._onCloseButtonClick = DVF._onCloseButtonClick.bind(DVF);
DVF._onResumeButtonClick = DVF._onResumeButtonClick.bind(DVF);
