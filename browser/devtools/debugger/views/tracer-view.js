


"use strict";




function TracerView(DebuggerController, DebuggerView) {
  this._selectedItem = null;
  this._matchingItems = null;
  this.widget = null;

  this.Tracer = DebuggerController.Tracer;
  this.DebuggerView = DebuggerView;

  this._highlightItem = this._highlightItem.bind(this);
  this._isNotSelectedItem = this._isNotSelectedItem.bind(this);

  this._unhighlightMatchingItems =
    DevToolsUtils.makeInfallible(this._unhighlightMatchingItems.bind(this));
  this._onToggleTracing =
    DevToolsUtils.makeInfallible(this._onToggleTracing.bind(this));
  this._onStartTracing =
    DevToolsUtils.makeInfallible(this._onStartTracing.bind(this));
  this._onClear =
    DevToolsUtils.makeInfallible(this._onClear.bind(this));
  this._onSelect =
    DevToolsUtils.makeInfallible(this._onSelect.bind(this));
  this._onMouseOver =
    DevToolsUtils.makeInfallible(this._onMouseOver.bind(this));
  this._onSearch =
    DevToolsUtils.makeInfallible(this._onSearch.bind(this));
}

TracerView.MAX_TRACES = 200;

TracerView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the TracerView");

    this._traceButton = document.getElementById("trace");
    this._tracerTab = document.getElementById("tracer-tab");

    
    
    if (!Prefs.tracerEnabled) {
      this._traceButton.remove();
      this._traceButton = null;
      this._tracerTab.remove();
      this._tracerTab = null;
      return;
    }

    this.widget = new FastListWidget(document.getElementById("tracer-traces"));
    this._traceButton.removeAttribute("hidden");
    this._tracerTab.removeAttribute("hidden");

    this._search = document.getElementById("tracer-search");
    this._template = document.getElementsByClassName("trace-item-template")[0];
    this._templateItem = this._template.getElementsByClassName("trace-item")[0];
    this._templateTypeIcon = this._template.getElementsByClassName("trace-type")[0];
    this._templateNameNode = this._template.getElementsByClassName("trace-name")[0];

    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("mouseover", this._onMouseOver, false);
    this.widget.addEventListener("mouseout", this._unhighlightMatchingItems, false);
    this._search.addEventListener("input", this._onSearch, false);

    this._startTooltip = L10N.getStr("startTracingTooltip");
    this._stopTooltip = L10N.getStr("stopTracingTooltip");
    this._tracingNotStartedString = L10N.getStr("tracingNotStartedText");
    this._noFunctionCallsString = L10N.getStr("noFunctionCallsText");

    this._traceButton.setAttribute("tooltiptext", this._startTooltip);
    this.emptyText = this._tracingNotStartedString;

    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the TracerView");

    if (!this.widget) {
      return;
    }

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("mouseover", this._onMouseOver, false);
    this.widget.removeEventListener("mouseout", this._unhighlightMatchingItems, false);
    this._search.removeEventListener("input", this._onSearch, false);
  },

  


  _addCommands: function() {
    XULUtils.addCommands(document.getElementById('debuggerCommands'), {
      toggleTracing: () => this._onToggleTracing(),
      startTracing: () => this._onStartTracing(),
      clearTraces: () => this._onClear()
    });
  },

  


  _onToggleTracing: function() {
    if (this.Tracer.tracing) {
      this._onStopTracing();
    } else {
      this._onStartTracing();
    }
  },

  






  _onStartTracing: function() {
    this._traceButton.setAttribute("checked", true);
    this._traceButton.setAttribute("tooltiptext", this._stopTooltip);

    this.empty();
    this.emptyText = this._noFunctionCallsString;

    let deferred = promise.defer();
    this.Tracer.startTracing(deferred.resolve);
    return deferred.promise;
  },

  






  _onStopTracing: function() {
    this._traceButton.removeAttribute("checked");
    this._traceButton.setAttribute("tooltiptext", this._startTooltip);

    this.emptyText = this._tracingNotStartedString;

    let deferred = promise.defer();
    this.Tracer.stopTracing(deferred.resolve);
    return deferred.promise;
  },

  


  _onClear: function() {
    this.empty();
  },

  










  _populateVariable: function(aName, aParent, aValue) {
    let item = aParent.addItem(aName, { value: aValue });

    if (aValue) {
      let wrappedValue = new this.Tracer.WrappedObject(aValue);
      this.DebuggerView.Variables.controller.populate(item, wrappedValue);
      item.expand();
      item.twisty = false;
    }
  },

  







  _onSelect: function _onSelect({ detail: traceItem }) {
    if (!traceItem) {
      return;
    }

    const data = traceItem.attachment.trace;
    const { location: { url, line } } = data;
    this.DebuggerView.setEditorLocation(
      this.DebuggerView.Sources.getActorForLocation({ url }),
      line,
      { noDebug: true }
    );

    this.DebuggerView.Variables.empty();
    const scope = this.DebuggerView.Variables.addScope();

    if (data.type == "call") {
      const params = DevToolsUtils.zip(data.parameterNames, data.arguments);
      for (let [name, val] of params) {
        if (val === undefined) {
          scope.addItem(name, { value: "<value not available>" });
        } else {
          this._populateVariable(name, scope, val);
        }
      }
    } else {
      const varName = "<" + (data.type == "throw" ? "exception" : data.type) + ">";
      this._populateVariable(varName, scope, data.returnVal);
    }

    scope.expand();
    this.DebuggerView.showInstrumentsPane();
  },

  


  _highlightItem: function(aItem) {
    if (!aItem || !aItem.target) {
      return;
    }
    const trace = aItem.target.querySelector(".trace-item");
    trace.classList.add("selected-matching");
  },

  


  _unhighlightItem: function(aItem) {
    if (!aItem || !aItem.target) {
      return;
    }
    const match = aItem.target.querySelector(".selected-matching");
    if (match) {
      match.classList.remove("selected-matching");
    }
  },

  


  _unhighlightMatchingItems: function() {
    if (this._matchingItems) {
      this._matchingItems.forEach(this._unhighlightItem);
      this._matchingItems = null;
    }
  },

  


  _isNotSelectedItem: function(aItem) {
    return aItem !== this.selectedItem;
  },

  


  _highlightMatchingItems: function(aItem) {
    const frameId = aItem.attachment.trace.frameId;
    const predicate = e => e.attachment.trace.frameId == frameId;

    this._unhighlightMatchingItems();
    this._matchingItems = this.items.filter(predicate);
    this._matchingItems
      .filter(this._isNotSelectedItem)
      .forEach(this._highlightItem);
  },

  


  _onMouseOver: function({ target }) {
    const traceItem = this.getItemForElement(target);
    if (traceItem) {
      this._highlightMatchingItems(traceItem);
    }
  },

  


  _onSearch: function() {
    const query = this._search.value.trim().toLowerCase();
    const predicate = name => name.toLowerCase().contains(query);
    this.filterContents(item => predicate(item.attachment.trace.name));
  },

  


  selectTab: function() {
    const tabs = this._tracerTab.parentElement;
    tabs.selectedIndex = Array.indexOf(tabs.children, this._tracerTab);
  },

  



  commit: function() {
    WidgetMethods.commit.call(this);
    
    
    this.widget.flush();
  },

  





  addTrace: function(aTrace) {
    
    let view = this._createView(aTrace);

    
    this.push([view], {
      staged: true,
      attachment: {
        trace: aTrace
      }
    });
  },

  





  _createView: function(aTrace) {
    let { type, name, location, blackBoxed, depth, frameId } = aTrace;
    let { parameterNames, returnVal, arguments: args } = aTrace;
    let fragment = document.createDocumentFragment();

    this._templateItem.classList.toggle("black-boxed", blackBoxed);
    this._templateItem.setAttribute("tooltiptext", SourceUtils.trimUrl(location.url));
    this._templateItem.style.MozPaddingStart = depth + "em";

    const TYPES = ["call", "yield", "return", "throw"];
    for (let t of TYPES) {
      this._templateTypeIcon.classList.toggle("trace-" + t, t == type);
    }
    this._templateTypeIcon.setAttribute("value", {
      call: "\u2192",
      yield: "Y",
      return: "\u2190",
      throw: "E",
      terminated: "TERMINATED"
    }[type]);

    this._templateNameNode.setAttribute("value", name);

    
    const addedNodes = [];

    if (parameterNames) {
      const syntax = (p) => {
        const el = document.createElement("label");
        el.setAttribute("value", p);
        el.classList.add("trace-syntax");
        el.classList.add("plain");
        addedNodes.push(el);
        return el;
      };

      this._templateItem.appendChild(syntax("("));

      for (let i = 0, n = parameterNames.length; i < n; i++) {
        let param = document.createElement("label");
        param.setAttribute("value", parameterNames[i]);
        param.classList.add("trace-param");
        param.classList.add("plain");
        addedNodes.push(param);
        this._templateItem.appendChild(param);

        if (i + 1 !== n) {
          this._templateItem.appendChild(syntax(", "));
        }
      }

      this._templateItem.appendChild(syntax(")"));
    }

    
    for (let node of this._template.childNodes) {
      fragment.appendChild(node.cloneNode(true));
    }

    
    for (let node of addedNodes) {
      this._templateItem.removeChild(node);
    }

    return fragment;
  }
});

DebuggerView.Tracer = new TracerView(DebuggerController, DebuggerView);
