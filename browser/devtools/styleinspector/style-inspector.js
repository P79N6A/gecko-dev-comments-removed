





const {Cc, Cu, Ci} = require("chrome");

let ToolDefinitions = require("main").Tools;

Cu.import("resource://gre/modules/Services.jsm");

loader.lazyGetter(this, "gDevTools", () => Cu.import("resource:///modules/devtools/gDevTools.jsm", {}).gDevTools);
loader.lazyGetter(this, "RuleView", () => require("devtools/styleinspector/rule-view"));
loader.lazyGetter(this, "ComputedView", () => require("devtools/styleinspector/computed-view"));
loader.lazyGetter(this, "_strings", () => Services.strings
  .createBundle("chrome://global/locale/devtools/styleinspector.properties"));




function RuleViewTool(aInspector, aWindow, aIFrame)
{
  this.inspector = aInspector;
  this.doc = aWindow.document;
  this.outerIFrame = aIFrame;

  this.view = new RuleView.CssRuleView(aInspector, this.doc);
  this.doc.documentElement.appendChild(this.view.element);

  this._changeHandler = () => {
    this.inspector.markDirty();
  };

  this.view.element.addEventListener("CssRuleViewChanged", this._changeHandler);

  this._refreshHandler = () => {
    this.inspector.emit("rule-view-refreshed");
  };

  this.view.element.addEventListener("CssRuleViewRefreshed", this._refreshHandler);

  this._cssLinkHandler = (aEvent) => {
    let rule = aEvent.detail.rule;
    let line = rule.line || 0;

    
    
    
    
    
    
    let href = rule.href;
    let sheet = rule.parentStyleSheet;
    if (sheet && href && !sheet.isSystem) {
      let target = this.inspector.target;
      if (ToolDefinitions.styleEditor.isTargetSupported(target)) {
        gDevTools.showToolbox(target, "styleeditor").then(function(toolbox) {
          toolbox.getCurrentPanel().selectStyleSheet(href, line);
        });
      }
      return;
    }

    let contentDoc = this.inspector.selection.document;
    let viewSourceUtils = this.inspector.viewSourceUtils;
    viewSourceUtils.viewSource(href, null, contentDoc, line);
  }

  this.view.element.addEventListener("CssRuleViewCSSLinkClicked",
                                     this._cssLinkHandler);

  this._onSelect = this.onSelect.bind(this);
  this.inspector.selection.on("detached", this._onSelect);
  this.inspector.selection.on("new-node-front", this._onSelect);
  this.refresh = this.refresh.bind(this);
  this.inspector.on("layout-change", this.refresh);

  this.inspector.selection.on("pseudoclass", this.refresh);
  if (this.inspector.highlighter) {
    this.inspector.highlighter.on("locked", this._onSelect);
  }

  this.onSelect();
}

exports.RuleViewTool = RuleViewTool;

RuleViewTool.prototype = {
  onSelect: function RVT_onSelect(aEvent) {
    this.view.setPageStyle(this.inspector.pageStyle);

    if (!this.inspector.selection.isConnected() ||
        !this.inspector.selection.isElementNode()) {
      this.view.highlight(null);
      return;
    }

    if (!aEvent || aEvent == "new-node-front") {
      if (this.inspector.selection.reason == "highlighter") {
        this.view.highlight(null);
      } else {
        let done = this.inspector.updating("rule-view");
        this.view.highlight(this.inspector.selection.nodeFront).then(done, done);
      }
    }

    if (aEvent == "locked") {
      let done = this.inspector.updating("rule-view");
      this.view.highlight(this.inspector.selection.nodeFront).then(done, done);
    }
  },

  refresh: function RVT_refresh() {
    this.view.nodeChanged();
  },

  destroy: function RVT_destroy() {
    this.inspector.off("layout-change", this.refresh);
    this.inspector.selection.off("pseudoclass", this.refresh);
    this.inspector.selection.off("new-node-front", this._onSelect);
    if (this.inspector.highlighter) {
      this.inspector.highlighter.off("locked", this._onSelect);
    }

    this.view.element.removeEventListener("CssRuleViewCSSLinkClicked",
      this._cssLinkHandler);

    this.view.element.removeEventListener("CssRuleViewChanged",
      this._changeHandler);

    this.view.element.removeEventListener("CssRuleViewRefreshed",
      this._refreshHandler);

    this.doc.documentElement.removeChild(this.view.element);

    this.view.destroy();

    delete this.outerIFrame;
    delete this.view;
    delete this.doc;
    delete this.inspector;
  }
}

function ComputedViewTool(aInspector, aWindow, aIFrame)
{
  this.inspector = aInspector;
  this.window = aWindow;
  this.document = aWindow.document;
  this.outerIFrame = aIFrame;
  this.view = new ComputedView.CssHtmlTree(this, aInspector.pageStyle);

  this._onSelect = this.onSelect.bind(this);
  this.inspector.selection.on("detached", this._onSelect);
  this.inspector.selection.on("new-node-front", this._onSelect);
  if (this.inspector.highlighter) {
    this.inspector.highlighter.on("locked", this._onSelect);
  }
  this.refresh = this.refresh.bind(this);
  this.inspector.on("layout-change", this.refresh);
  this.inspector.selection.on("pseudoclass", this.refresh);

  this.view.highlight(null);

  this.onSelect();
}

exports.ComputedViewTool = ComputedViewTool;

ComputedViewTool.prototype = {
  onSelect: function CVT_onSelect(aEvent)
  {
    this.view.setPageStyle(this.inspector.pageStyle);

    if (!this.inspector.selection.isConnected() ||
        !this.inspector.selection.isElementNode()) {
      this.view.highlight(null);
      return;
    }

    if (!aEvent || aEvent == "new-node-front") {
      if (this.inspector.selection.reason == "highlighter") {
        
      } else {
        let done = this.inspector.updating("computed-view");
        this.view.highlight(this.inspector.selection.nodeFront).then(() => {
          done();
        });
      }
    }

    if (aEvent == "locked" && this.inspector.selection.nodeFront != this.view.viewedElement) {
      let done = this.inspector.updating("computed-view");
      this.view.highlight(this.inspector.selection.nodeFront).then(() => {
        done();
      });
    }
  },

  refresh: function CVT_refresh() {
    this.view.refreshPanel();
  },

  destroy: function CVT_destroy(aContext)
  {
    this.inspector.off("layout-change", this.refresh);
    this.inspector.sidebar.off("computedview-selected", this.refresh);
    this.inspector.selection.off("pseudoclass", this.refresh);
    this.inspector.selection.off("new-node-front", this._onSelect);
    if (this.inspector.highlighter) {
      this.inspector.highlighter.off("locked", this._onSelect);
    }

    this.view.destroy();
    delete this.view;

    delete this.outerIFrame;
    delete this.cssLogic;
    delete this.cssHtmlTree;
    delete this.window;
    delete this.document;
    delete this.inspector;
  }
};
