







































const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var EXPORTED_SYMBOLS = ["StyleInspector"];

var StyleInspector = {
  



  get isEnabled()
  {
    return Services.prefs.getBoolPref("devtools.styleinspector.enabled");
  },

  





  createPanel: function SI_createPanel(aPreserveOnHide)
  {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    let popupSet = win.document.getElementById("mainPopupSet");
    let ns = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    let panel = win.document.createElementNS(ns, "panel");

    panel.setAttribute("class", "styleInspector");
    panel.setAttribute("orient", "vertical");
    panel.setAttribute("ignorekeys", "true");
    panel.setAttribute("noautofocus", "true");
    panel.setAttribute("noautohide", "true");
    panel.setAttribute("titlebar", "normal");
    panel.setAttribute("close", "true");
    panel.setAttribute("label", StyleInspector.l10n("panelTitle"));
    panel.setAttribute("width", 350);
    panel.setAttribute("height", win.screen.height / 2);

    let vbox = win.document.createElement("vbox");
    vbox.setAttribute("flex", "1");
    panel.appendChild(vbox);

    let iframe = win.document.createElementNS(ns, "iframe");
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("tooltip", "aHTMLTooltip");
    iframe.setAttribute("src", "chrome://browser/content/csshtmltree.xhtml");
    iframe.addEventListener("load", SI_iframeOnload, true);
    vbox.appendChild(iframe);

    let hbox = win.document.createElement("hbox");
    hbox.setAttribute("class", "resizerbox");
    vbox.appendChild(hbox);

    let spacer = win.document.createElement("spacer");
    spacer.setAttribute("flex", "1");
    hbox.appendChild(spacer);

    let resizer = win.document.createElement("resizer");
    resizer.setAttribute("dir", "bottomend");
    hbox.appendChild(resizer);
    popupSet.appendChild(panel);

    


    let iframeReady = false;
    function SI_iframeOnload() {
      iframe.removeEventListener("load", SI_iframeOnload, true);
      iframeReady = true;
      if (panelReady) {
        SI_popupShown.call(panel);
      }
    }

    


    let panelReady = false;
    function SI_popupShown() {
      panelReady = true;
      if (iframeReady) {
        if (!this.cssLogic) {
          this.cssLogic = new CssLogic();
          this.cssHtmlTree = new CssHtmlTree(iframe, this.cssLogic, this);
        }
        let selectedNode = this.selectedNode || null;
        this.cssLogic.highlight(selectedNode);
        this.cssHtmlTree.highlight(selectedNode);
        Services.obs.notifyObservers(null, "StyleInspector-opened", null);
      }
    }

    


    function SI_popupHidden() {
      if (panel.preserveOnHide) {
        Services.obs.notifyObservers(null, "StyleInspector-closed", null);
      } else {
        panel.destroy();
      }
    }

    panel.addEventListener("popupshown", SI_popupShown);
    panel.addEventListener("popuphidden", SI_popupHidden);
    panel.preserveOnHide = !!aPreserveOnHide;

    


    panel.isOpen = function SI_isOpen()
    {
      return this.state && this.state == "open";
    };

    




    panel.selectNode = function SI_selectNode(aNode)
    {
      this.selectedNode = aNode;
      if (this.isOpen() && !this.hasAttribute("dimmed")) {
        this.cssLogic.highlight(aNode);
        this.cssHtmlTree.highlight(aNode);
      }
    };

    


    panel.destroy = function SI_destroy()
    {
      if (!this.cssLogic)
        return;
      if (this.isOpen())
        this.hideTool();
      this.cssLogic = null;
      this.cssHtmlTree = null;
      this.removeEventListener("popupshown", SI_popupShown);
      this.removeEventListener("popuphidden", SI_popupHidden);
      this.parentNode.removeChild(this);
      Services.obs.notifyObservers(null, "StyleInspector-closed", null);
    };

    





    panel.dimTool = function SI_dimTool(aState)
    {
      if (!this.isOpen())
        return;

      if (aState) {
        this.setAttribute("dimmed", "true");
      } else if (this.hasAttribute("dimmed")) {
        this.removeAttribute("dimmed");
      }
    };

    panel.showTool = function SI_showTool(aSelection)
    {
      this.selectNode(aSelection);
      let win = Services.wm.getMostRecentWindow("navigator:browser");
      this.openPopup(win.gBrowser.selectedBrowser, "end_before", 0, 0,
        false, false);
    };

    panel.hideTool = function SI_hideTool()
    {
      this.hidePopup();
    };

    



    function isInitialized()
    {
      return panel.cssLogic && panel.cssHtmlTree;
    }

    return panel;
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
