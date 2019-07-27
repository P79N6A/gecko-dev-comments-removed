


"use strict";

const URL_LABEL_TOOLTIP = L10N.getStr("table.url.tooltiptext");
const OPTIMIZATION_FAILURE = L10N.getStr("jit.optimizationFailure");
const JIT_SAMPLES = L10N.getStr("jit.samples");
const JIT_EMPTY_TEXT = L10N.getStr("jit.empty");







let JITOptimizationsView = {

  _currentFrame: null,

  


  initialize: function () {
    this.reset = this.reset.bind(this);
    this._onFocusFrame = this._onFocusFrame.bind(this);
    this._toggleVisibility = this._toggleVisibility.bind(this);

    this.el = $("#jit-optimizations-view");

    this.tree = new TreeWidget($("#jit-optimizations-raw-view"), {
      sorted: false,
      emptyText: JIT_EMPTY_TEXT
    });

    
    this.reset();

    this._toggleVisibility();

    PerformanceController.on(EVENTS.RECORDING_SELECTED, this.reset);
    PerformanceController.on(EVENTS.PREF_CHANGED, this._toggleVisibility);
    JsCallTreeView.on("focus", this._onFocusFrame);
  },

  


  destroy: function () {
    this.tree = null;
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this.reset);
    PerformanceController.off(EVENTS.PREF_CHANGED, this._toggleVisibility);
    JsCallTreeView.off("focus", this._onFocusFrame);
  },

  





  setCurrentFrame: function (frameNode) {
    if (frameNode !== this.getCurrentFrame()) {
      this._currentFrame = frameNode;
    }
  },

  




  getCurrentFrame: function (frameNode) {
    return this._currentFrame;
  },

  



  reset: function () {
    this.setCurrentFrame(null);
    this.clear();
    this.el.classList.add("empty");
    this.emit(EVENTS.OPTIMIZATIONS_RESET);
    this.emit(EVENTS.OPTIMIZATIONS_RENDERED, this.getCurrentFrame());
  },

  


  clear: function () {
    this.tree.clear();
  },

  


  isEnabled: function () {
    return PerformanceController.getOption("show-jit-optimizations");
  },

  




  render: function () {
    if (!this.isEnabled()) {
      return;
    }

    let frameNode = this.getCurrentFrame();

    if (!frameNode) {
      this.reset();
      return;
    }

    let view = this.tree;

    
    
    let frameData = frameNode.getInfo();
    this._setHeaders(frameData);
    this.clear();

    if (!frameNode.hasOptimizations()) {
      this.reset();
      return;
    }
    this.el.classList.remove("empty");

    
    let sites = frameNode.getOptimizations().getOptimizationSites();

    for (let site of sites) {
      this._renderSite(view, site, frameData);
    }

    this.emit(EVENTS.OPTIMIZATIONS_RENDERED, this.getCurrentFrame());
  },

  


  _renderSite: function (view, site, frameData) {
    let { id, samples, data } = site;
    let { types, attempts } = data;
    let siteNode = this._createSiteNode(frameData, site);

    
    id = id + "";

    view.add([{ id: id, node: siteNode }]);

    
    
    view.add([id, { id: `${id}-types`, label: `Types (${types.length})` }]);
    this._renderIonType(view, site);

    
    view.add([id, { id: `${id}-attempts`, label: `Attempts (${attempts.length})` }]);
    for (let i = attempts.length - 1; i >= 0; i--) {
      let node = this._createAttemptNode(attempts[i]);
      view.add([id, `${id}-attempts`, { node }]);
    }
  },

  



  _renderIonType: function (view, site) {
    let { id, data: { types }} = site;
    
    id = id + "";
    for (let i = 0; i < types.length; i++) {
      let ionType = types[i];

      let ionNode = this._createIonNode(ionType);
      view.add([id, `${id}-types`, { id: `${id}-types-${i}`, node: ionNode }]);
      for (let observedType of (ionType.types || [])) {
        let node = this._createObservedTypeNode(observedType);
        view.add([id, `${id}-types`, `${id}-types-${i}`, { node }]);
      }
    }
  },

  



  _createSiteNode: function (frameData, site) {
    let node = document.createElement("span");
    let desc = document.createElement("span");
    let line = document.createElement("span");
    let column = document.createElement("span");
    let urlNode = this._createDebuggerLinkNode(frameData.url, site.data.line);

    let attempts = site.getAttempts();
    let lastStrategy = attempts[attempts.length - 1].strategy;

    if (!site.hasSuccessfulOutcome()) {
      let icon = document.createElement("span");
      icon.setAttribute("tooltiptext", OPTIMIZATION_FAILURE);
      icon.setAttribute("severity", "warning");
      icon.className = "opt-icon";
      node.appendChild(icon);
    }

    desc.textContent = `${lastStrategy} - (${site.samples} ${JIT_SAMPLES})`;
    line.textContent = site.data.line;
    line.className = "opt-line";
    column.textContent = site.data.column;
    column.className = "opt-line";
    node.appendChild(desc);
    node.appendChild(urlNode);
    node.appendChild(line);
    node.appendChild(column);

    return node;
  },

  







  _createIonNode: function (ionType) {
    let node = document.createElement("span");
    let icon = document.createElement("span");
    let typeNode = document.createElement("span");
    let siteNode = document.createElement("span");

    typeNode.textContent = ionType.mirType;
    typeNode.className = "opt-ion-type";
    siteNode.textContent = `(${ionType.site})`;
    siteNode.className = "opt-ion-type-site";
    node.appendChild(typeNode);
    node.appendChild(siteNode);

    return node;
  },

  







  _createObservedTypeNode: function (type) {
    let node = document.createElement("span");
    let typeNode = document.createElement("span");

    typeNode.textContent = `${type.keyedBy}` + (type.name ? ` → ${type.name}` : "");
    typeNode.className = "opt-type";
    node.appendChild(typeNode);

    
    
    if (type.location && type.line) {
      let urlNode = this._createDebuggerLinkNode(type.location, type.line);
      node.appendChild(urlNode);
    }
    
    
    else if (type.location) {
      let locNode = document.createElement("span");
      locNode.textContent = `@${type.location}`;
      locNode.className = "opt-url";
      node.appendChild(locNode);
    }

    if (type.line) {
      let line = document.createElement("span");
      line.textContent = type.line;
      line.className = "opt-line";
      node.appendChild(line);
    }
    return node;
  },

  







  _createAttemptNode: function (attempt) {
    let node = document.createElement("span");
    let strategyNode = document.createElement("span");
    let outcomeNode = document.createElement("span");

    strategyNode.textContent = attempt.strategy;
    strategyNode.className = "opt-strategy";
    outcomeNode.textContent = attempt.outcome;
    outcomeNode.className = "opt-outcome";
    outcomeNode.setAttribute("outcome",
      JITOptimizations.isSuccessfulOutcome(attempt.outcome) ? "success" : "failure");

    node.appendChild(strategyNode);
    node.appendChild(outcomeNode);
    node.className = "opt-attempt";
    return node;
  },

  










  _createDebuggerLinkNode: function (url, line, el) {
    let node = el || document.createElement("span");
    node.className = "opt-url";
    let fileName;

    if (this._isLinkableURL(url)) {
      fileName = url.slice(url.lastIndexOf("/") + 1);
      node.classList.add("debugger-link");
      node.setAttribute("tooltiptext", URL_LABEL_TOOLTIP + " → " + url);
      node.addEventListener("click", () => viewSourceInDebugger(url, line));
    }
    node.textContent = `@${fileName || url}`;
    return node;
  },

  



  _setHeaders: function (frameData) {
    $("#jit-optimizations-header .header-function-name").textContent = frameData.functionName;
    this._createDebuggerLinkNode(frameData.url, frameData.line, $("#jit-optimizations-header .header-file"));
    $("#jit-optimizations-header .header-line").textContent = frameData.line;
  },

  







  _isLinkableURL: function (url) {
    return url && url.indexOf &&
       (url.indexOf("http") === 0 ||
        url.indexOf("resource://") === 0 ||
        url.indexOf("file://") === 0);
  },

  




  _toggleVisibility: function () {
    let enabled = this.isEnabled();
    this.el.hidden = !enabled;

    
    
    if (enabled) {
      this.render();
    }
  },

  



  _onFocusFrame: function (_, view) {
    if (!view.frame) {
      return;
    }

    
    
    
    let shouldRender = this.getCurrentFrame() !== view.frame;

    
    
    this.setCurrentFrame(view.frame);

    if (shouldRender) {
      this.render();
    }
  },

  toString: () => "[object JITOptimizationsView]"

};

EventEmitter.decorate(JITOptimizationsView);
