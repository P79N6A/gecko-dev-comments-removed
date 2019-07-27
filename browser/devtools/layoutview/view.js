





"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const {InplaceEditor, editableItem} = devtools.require("devtools/shared/inplace-editor");
const {parseDeclarations} = devtools.require("devtools/styleinspector/css-parsing-utils");
const {ReflowFront} = devtools.require("devtools/server/actors/layout");

const SHARED_L10N = new ViewHelpers.L10N("chrome://browser/locale/devtools/shared.properties");
const NUMERIC = /^-?[\d\.]+$/;
const LONG_TEXT_ROTATE_LIMIT = 3;










function EditingSession(doc, rules) {
  this._doc = doc;
  this._rules = rules;
  this._modifications = new Map();
}

EditingSession.prototype = {
  





  getPropertyFromRule: function(rule, property) {
    let dummyStyle = this._element.style;

    dummyStyle.cssText = rule.cssText;
    return dummyStyle.getPropertyValue(property);
  },

  





  getProperty: function(property) {
    
    let div = this._doc.createElement("div");
    div.setAttribute("style", "display: none");
    this._doc.body.appendChild(div);
    this._element = this._doc.createElement("p");
    div.appendChild(this._element);

    
    
    for (let rule of this._rules) {
      let value = this.getPropertyFromRule(rule, property);
      if (value !== "") {
        div.remove();
        return value;
      }
    }
    div.remove();
    return "";
  },

  







  setProperties: function(properties) {
    let modifications = this._rules[0].startModifyingProperties();

    for (let property of properties) {
      if (!this._modifications.has(property.name)) {
        this._modifications.set(property.name,
          this.getPropertyFromRule(this._rules[0], property.name));
      }

      if (property.value == "") {
        modifications.removeProperty(property.name);
      } else {
        modifications.setProperty(property.name, property.value, "");
      }
    }

    return modifications.apply().then(null, console.error);
  },

  



  revert: function() {
    let modifications = this._rules[0].startModifyingProperties();

    for (let [property, value] of this._modifications) {
      if (value != "") {
        modifications.setProperty(property, value, "");
      } else {
        modifications.removeProperty(property);
      }
    }

    return modifications.apply().then(null, console.error);
  },

  destroy: function() {
    this._doc = null;
    this._rules = null;
    this._modifications.clear();
  }
};







function LayoutView(inspector, win) {
  this.inspector = inspector;

  this.doc = win.document;
  this.sizeLabel = this.doc.querySelector(".size > span");
  this.sizeHeadingLabel = this.doc.getElementById("element-size");

  this.init();
}

LayoutView.prototype = {
  init: function() {
    this.update = this.update.bind(this);

    this.onNewSelection = this.onNewSelection.bind(this);
    this.inspector.selection.on("new-node-front", this.onNewSelection);

    this.onNewNode = this.onNewNode.bind(this);
    this.inspector.sidebar.on("layoutview-selected", this.onNewNode);

    this.onSidebarSelect = this.onSidebarSelect.bind(this);
    this.inspector.sidebar.on("select", this.onSidebarSelect);

    
    
    
    
    this.map = {
      position: {selector: "#element-position",
                 property: "position",
                 value: undefined},
      marginTop: {selector: ".margin.top > span",
                  property: "margin-top",
                  value: undefined},
      marginBottom: {selector: ".margin.bottom > span",
                  property: "margin-bottom",
                  value: undefined},
      marginLeft: {selector: ".margin.left > span",
                  property: "margin-left",
                  value: undefined},
      marginRight: {selector: ".margin.right > span",
                  property: "margin-right",
                  value: undefined},
      paddingTop: {selector: ".padding.top > span",
                  property: "padding-top",
                  value: undefined},
      paddingBottom: {selector: ".padding.bottom > span",
                  property: "padding-bottom",
                  value: undefined},
      paddingLeft: {selector: ".padding.left > span",
                  property: "padding-left",
                  value: undefined},
      paddingRight: {selector: ".padding.right > span",
                  property: "padding-right",
                  value: undefined},
      borderTop: {selector: ".border.top > span",
                  property: "border-top-width",
                  value: undefined},
      borderBottom: {selector: ".border.bottom > span",
                  property: "border-bottom-width",
                  value: undefined},
      borderLeft: {selector: ".border.left > span",
                  property: "border-left-width",
                  value: undefined},
      borderRight: {selector: ".border.right > span",
                  property: "border-right-width",
                  value: undefined},
    };

    
    for (let i in this.map) {
      if (i == "position")
        continue;

      let dimension = this.map[i];
      editableItem({
        element: this.doc.querySelector(dimension.selector)
      }, (element, event) => {
        this.initEditor(element, event, dimension);
      });
    }

    this.onNewNode();
  },

  


  trackReflows: function() {
    if (!this.reflowFront) {
      let toolbox = this.inspector.toolbox;
      if (toolbox.target.form.reflowActor) {
        this.reflowFront = ReflowFront(toolbox.target.client, toolbox.target.form);
      } else {
        return;
      }
    }

    this.reflowFront.on("reflows", this.update);
    this.reflowFront.start();
  },

  


  untrackReflows: function() {
    if (!this.reflowFront) {
      return;
    }

    this.reflowFront.off("reflows", this.update);
    this.reflowFront.stop();
  },

  


  initEditor: function(element, event, dimension) {
    let { property } = dimension;
    let session = new EditingSession(document, this.elementRules);
    let initialValue = session.getProperty(property);

    let editor = new InplaceEditor({
      element: element,
      initial: initialValue,

      start: (editor) => {
        editor.elt.parentNode.classList.add("editing");
      },

      change: (value) => {
        if (NUMERIC.test(value)) {
          value += "px";
        }

        let properties = [
          { name: property, value: value }
        ];

        if (property.substring(0, 7) == "border-") {
          let bprop = property.substring(0, property.length - 5) + "style";
          let style = session.getProperty(bprop);
          if (!style || style == "none" || style == "hidden") {
            properties.push({ name: bprop, value: "solid" });
          }
        }

        session.setProperties(properties);
      },

      done: (value, commit) => {
        editor.elt.parentNode.classList.remove("editing");
        if (!commit) {
          session.revert();
          session.destroy();
        }
      }
    }, event);
  },

  


  isActive: function() {
    return this.inspector &&
           this.inspector.sidebar.getCurrentTabID() == "layoutview";
  },

  


  destroy: function() {
    this.inspector.sidebar.off("layoutview-selected", this.onNewNode);
    this.inspector.selection.off("new-node-front", this.onNewSelection);
    this.inspector.sidebar.off("select", this.onSidebarSelect);

    this.sizeHeadingLabel = null;
    this.sizeLabel = null;
    this.inspector = null;
    this.doc = null;

    if (this.reflowFront) {
      this.untrackReflows();
      this.reflowFront.destroy();
      this.reflowFront = null;
    }
  },

  onSidebarSelect: function(e, sidebar) {
    if (sidebar !== "layoutview") {
      this.dim();
    }
  },

  


  onNewSelection: function() {
    let done = this.inspector.updating("layoutview");
    this.onNewNode().then(done, (err) => { console.error(err); done() });
  },

  


  onNewNode: function() {
    if (this.isActive() &&
        this.inspector.selection.isConnected() &&
        this.inspector.selection.isElementNode()) {
      this.undim();
    } else {
      this.dim();
    }

    return this.update();
  },

  



  dim: function() {
    this.untrackReflows();
    this.doc.body.classList.add("dim");
    this.dimmed = true;
  },

  



  undim: function() {
    this.trackReflows();
    this.doc.body.classList.remove("dim");
    this.dimmed = false;
  },

  




  update: function() {
    let lastRequest = Task.spawn((function*() {
      if (!this.isActive() ||
          !this.inspector.selection.isConnected() ||
          !this.inspector.selection.isElementNode()) {
        return;
      }

      let node = this.inspector.selection.nodeFront;
      let layout = yield this.inspector.pageStyle.getLayout(node, {
        autoMargins: !this.dimmed
      });
      let styleEntries = yield this.inspector.pageStyle.getApplied(node, {});

      
      if (this._lastRequest != lastRequest) {
        return this._lastRequest;
      }

      this._lastRequest = null;
      let width = layout.width;
      let height = layout.height;
      let newLabel = SHARED_L10N.getFormatStr("dimensions", width, height);

      if (this.sizeHeadingLabel.textContent != newLabel) {
        this.sizeHeadingLabel.textContent = newLabel;
      }

      
      if (this.dimmed) {
        this.inspector.emit("layoutview-updated");
        return null;
      }

      for (let i in this.map) {
        let property = this.map[i].property;
        if (!(property in layout)) {
          
          
          continue;
        }
        let parsedValue = parseFloat(layout[property]);
        if (Number.isNaN(parsedValue)) {
          
          
          this.map[i].value = layout[property];
        } else {
          this.map[i].value = parsedValue;
        }
      }

      let margins = layout.autoMargins;
      if ("top" in margins) this.map.marginTop.value = "auto";
      if ("right" in margins) this.map.marginRight.value = "auto";
      if ("bottom" in margins) this.map.marginBottom.value = "auto";
      if ("left" in margins) this.map.marginLeft.value = "auto";

      for (let i in this.map) {
        let selector = this.map[i].selector;
        let span = this.doc.querySelector(selector);
        if (span.textContent.length > 0 &&
            span.textContent == this.map[i].value) {
          continue;
        }
        span.textContent = this.map[i].value;
        this.manageOverflowingText(span);
      }

      width -= this.map.borderLeft.value + this.map.borderRight.value +
               this.map.paddingLeft.value + this.map.paddingRight.value;
      width = parseFloat(width.toPrecision(6));
      height -= this.map.borderTop.value + this.map.borderBottom.value +
                this.map.paddingTop.value + this.map.paddingBottom.value;
      height = parseFloat(height.toPrecision(6));

      let newValue = width + "\u00D7" + height;
      if (this.sizeLabel.textContent != newValue) {
        this.sizeLabel.textContent = newValue;
      }

      this.elementRules = [e.rule for (e of styleEntries)];

      this.inspector.emit("layoutview-updated");
    }).bind(this)).then(null, console.error);

    return this._lastRequest = lastRequest;
  },

  



  showBoxModel: function(options={}) {
    let toolbox = this.inspector.toolbox;
    let nodeFront = this.inspector.selection.nodeFront;

    toolbox.highlighterUtils.highlightNodeFront(nodeFront, options);
  },

  


  hideBoxModel: function() {
    let toolbox = this.inspector.toolbox;

    toolbox.highlighterUtils.unhighlight();
  },

  manageOverflowingText: function(span) {
    let classList = span.parentNode.classList;

    if (classList.contains("left") || classList.contains("right")) {
      let force = span.textContent.length > LONG_TEXT_ROTATE_LIMIT;
      classList.toggle("rotate", force);
    }
  }
};

let elts;

let onmouseover = function(e) {
  let region = e.target.getAttribute("data-box");
  this.layoutview.showBoxModel({region});

  return false;
}.bind(window);

let onmouseout = function(e) {
  this.layoutview.hideBoxModel();

  return false;
}.bind(window);

window.setPanel = function(panel) {
  this.layoutview = new LayoutView(panel, window);

  
  elts = document.querySelectorAll("*[title]");
  for (let i = 0; i < elts.length; i++) {
    let elt = elts[i];
    elt.addEventListener("mouseover", onmouseover, true);
    elt.addEventListener("mouseout", onmouseout, true);
  }

  
  let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
    getService(Ci.nsIXULChromeRegistry);
  let dir = chromeReg.isLocaleRTL("global");
  document.body.setAttribute("dir", dir ? "rtl" : "ltr");

  window.parent.postMessage("layoutview-ready", "*");
};

window.onunload = function() {
  if (this.layoutview) {
    this.layoutview.destroy();
  }
  if (elts) {
    for (let i = 0; i < elts.length; i++) {
      let elt = elts[i];
      elt.removeEventListener("mouseover", onmouseover, true);
      elt.removeEventListener("mouseout", onmouseout, true);
    }
  }
};
