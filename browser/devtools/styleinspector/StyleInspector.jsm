







































const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var EXPORTED_SYMBOLS = ["StyleInspector"];







function StyleInspector(aContext, aIUI)
{
  this._init(aContext, aIUI);
};

StyleInspector.prototype = {

  





  _init: function SI__init(aContext, aIUI)
  {
    this.window = aContext;
    this.IUI = aIUI;
    this.document = this.window.document;
    this.cssLogic = new CssLogic();
    this.panelReady = false;
    this.iframeReady = false;

    
    if (this.IUI) {
      this.openDocked = true;
      let isOpen = this.isOpen.bind(this);

      this.registrationObject = {
        id: "styleinspector",
        label: this.l10n("style.highlighter.button.label1"),
        tooltiptext: this.l10n("style.highlighter.button.tooltip"),
        accesskey: this.l10n("style.highlighter.accesskey1"),
        context: this,
        get isOpen() isOpen(),
        onSelect: this.selectNode,
        onChanged: this.updateNode,
        show: this.open,
        hide: this.close,
        dim: this.dimTool,
        panel: null,
        unregister: this.destroy,
        sidebar: true,
      };

      
      this.IUI.registerTool(this.registrationObject);
      this.createSidebarContent(true);
    }
  },

  



  createSidebarContent: function SI_createSidebarContent(aPreserveOnHide)
  {
    this.preserveOnHide = !!aPreserveOnHide;

    let boundIframeOnLoad = function loadedInitializeIframe() {
      if (this.iframe &&
          this.iframe.getAttribute("src") ==
          "chrome://browser/content/devtools/csshtmltree.xul") {
        let selectedNode = this.selectedNode || null;
        this.cssHtmlTree = new CssHtmlTree(this);
        this.cssLogic.highlight(selectedNode);
        this.cssHtmlTree.highlight(selectedNode);
        this.iframe.removeEventListener("load", boundIframeOnLoad, true);
        this.iframeReady = true;
        Services.obs.notifyObservers(null, "StyleInspector-opened", null);
      }
    }.bind(this);

    this.iframe = this.IUI.getToolIframe(this.registrationObject);

    this.iframe.addEventListener("load", boundIframeOnLoad, true);
  },

  






  createPanel: function SI_createPanel(aPreserveOnHide, aCallback)
  {
    let popupSet = this.document.getElementById("mainPopupSet");
    let panel = this.document.createElement("panel");
    this.preserveOnHide = !!aPreserveOnHide;

    panel.setAttribute("class", "styleInspector");
    panel.setAttribute("orient", "vertical");
    panel.setAttribute("ignorekeys", "true");
    panel.setAttribute("noautofocus", "true");
    panel.setAttribute("noautohide", "true");
    panel.setAttribute("titlebar", "normal");
    panel.setAttribute("close", "true");
    panel.setAttribute("label", this.l10n("panelTitle"));
    panel.setAttribute("width", 350);
    panel.setAttribute("height", this.window.screen.height / 2);

    let iframe = this.document.createElement("iframe");
    let boundIframeOnLoad = function loadedInitializeIframe()
    {
      this.iframe.removeEventListener("load", boundIframeOnLoad, true);
      this.iframeReady = true;
      if (aCallback)
        aCallback(this);
    }.bind(this);

    iframe.flex = 1;
    iframe.setAttribute("tooltip", "aHTMLTooltip");
    iframe.addEventListener("load", boundIframeOnLoad, true);
    iframe.setAttribute("src", "chrome://browser/content/devtools/csshtmltree.xul");

    panel.appendChild(iframe);
    popupSet.appendChild(panel);

    this._boundPopupShown = this.popupShown.bind(this);
    this._boundPopupHidden = this.popupHidden.bind(this);
    panel.addEventListener("popupshown", this._boundPopupShown, false);
    panel.addEventListener("popuphidden", this._boundPopupHidden, false);

    this.panel = panel;
    this.iframe = iframe;

    return panel;
  },

  


  popupShown: function SI_popupShown()
  {
    this.panelReady = true;
    if (this.iframeReady) {
      this.cssHtmlTree = new CssHtmlTree(this);
      let selectedNode = this.selectedNode || null;
      this.cssLogic.highlight(selectedNode);
      this.cssHtmlTree.highlight(selectedNode);
      Services.obs.notifyObservers(null, "StyleInspector-opened", null);
    }
  },

  



  popupHidden: function SI_popupHidden()
  {
    if (this.preserveOnHide) {
      Services.obs.notifyObservers(null, "StyleInspector-closed", null);
    } else {
      this.destroy();
    }
  },

  



  isOpen: function SI_isOpen()
  {
    return this.openDocked ? this.iframeReady && this.IUI.isSidebarOpen &&
            (this.IUI.sidebarDeck.selectedPanel == this.iframe) :
           this.panel && this.panel.state && this.panel.state == "open";
  },

  



  selectFromPath: function SI_selectFromPath(aNode)
  {
    if (this.IUI && this.IUI.selection) {
      if (aNode != this.IUI.selection) {
        this.IUI.inspectNode(aNode);
      }
    } else {
      this.selectNode(aNode);
    }
  },

  



  selectNode: function SI_selectNode(aNode)
  {
    this.selectedNode = aNode;
    if (this.isOpen() && !this.dimmed) {
      this.cssLogic.highlight(aNode);
      this.cssHtmlTree.highlight(aNode);
    }
  },

  


  updateNode: function SI_updateNode()
  {
    if (this.isOpen() && !this.dimmed) {
      this.cssLogic.highlight(this.selectedNode);
      this.cssHtmlTree.refreshPanel();
    }
  },

  




  dimTool: function SI_dimTool(aState)
  {
    this.dimmed = aState;
  },

  



  open: function SI_open(aSelection)
  {
    this.selectNode(aSelection);
    if (this.openDocked) {
      if (!this.iframeReady) {
        this.iframe.setAttribute("src", "chrome://browser/content/devtools/csshtmltree.xul");
      }
    } else {
      this.panel.openPopup(this.window.gBrowser.selectedBrowser, "end_before", 0, 0,
        false, false);
    }
  },

  


  close: function SI_close()
  {
    if (this.openDocked) {
      Services.obs.notifyObservers(null, "StyleInspector-closed", null);
    } else {
      this.panel.hidePopup();
    }
  },

  




  l10n: function SI_l10n(aName)
  {
    try {
      return _strings.GetStringFromName(aName);
    } catch (ex) {
      Services.console.logStringMessage("Error reading '" + aName + "'");
      throw new Error("l10n error with " + aName);
    }
  },

  


  destroy: function SI_destroy()
  {
    if (this.isOpen())
      this.close();
    if (this.cssHtmlTree)
      this.cssHtmlTree.destroy();
    if (this.iframe) {
      this.iframe.parentNode.removeChild(this.iframe);
      delete this.iframe;
    }

    delete this.cssLogic;
    delete this.cssHtmlTree;
    if (this.panel) {
      this.panel.removeEventListener("popupshown", this._boundPopupShown, false);
      this.panel.removeEventListener("popuphidden", this._boundPopupHidden, false);
      delete this._boundPopupShown;
      delete this._boundPopupHidden;
      this.panel.parentNode.removeChild(this.panel);
      delete this.panel;
    }
    delete this.doc;
    delete this.win;
    delete CssHtmlTree.win;
    Services.obs.notifyObservers(null, "StyleInspector-closed", null);
  },
};

XPCOMUtils.defineLazyGetter(this, "_strings", function() Services.strings
  .createBundle("chrome:

XPCOMUtils.defineLazyGetter(this, "CssLogic", function() {
  let tmp = {};
  Cu.import("resource:///modules/devtools/CssLogic.jsm", tmp);
  return tmp.CssLogic;
});

XPCOMUtils.defineLazyGetter(this, "CssHtmlTree", function() {
  let tmp = {};
  Cu.import("resource:///modules/devtools/CssHtmlTree.jsm", tmp);
  return tmp.CssHtmlTree;
});
