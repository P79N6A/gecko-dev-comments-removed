




let tempScope = {};
Cu.import("resource:///modules/devtools/CssLogic.jsm", tempScope);
Cu.import("resource:///modules/devtools/CssHtmlTree.jsm", tempScope);
Cu.import("resource:///modules/HUDService.jsm", tempScope);
let HUDService = tempScope.HUDService;
let ConsoleUtils = tempScope.ConsoleUtils;
let CssLogic = tempScope.CssLogic;
let CssHtmlTree = tempScope.CssHtmlTree;

function log(aMsg)
{
  dump("*** WebConsoleTest: " + aMsg + "\n");
}

function pprint(aObj)
{
  for (let prop in aObj) {
    if (typeof aObj[prop] == "function") {
      log("function " + prop);
    }
    else {
      log(prop + ": " + aObj[prop]);
    }
  }
}

let tab, browser, hudId, hud, hudBox, filterBox, outputNode, cs;

function addTab(aURL)
{
  gBrowser.selectedTab = gBrowser.addTab();
  content.location = aURL;
  tab = gBrowser.selectedTab;
  browser = gBrowser.getBrowserForTab(tab);
}

function afterAllTabsLoaded(callback, win) {
  win = win || window;

  let stillToLoad = 0;

  function onLoad() {
    this.removeEventListener("load", onLoad, true);
    stillToLoad--;
    if (!stillToLoad)
      callback();
  }

  for (let a = 0; a < win.gBrowser.tabs.length; a++) {
    let browser = win.gBrowser.tabs[a].linkedBrowser;
    if (browser.contentDocument.readyState != "complete") {
      stillToLoad++;
      browser.addEventListener("load", onLoad, true);
    }
  }

  if (!stillToLoad)
    callback();
}

















function testLogEntry(aOutputNode, aMatchString, aMsg, aOnlyVisible,
                      aFailIfFound, aClass)
{
  let selector = ".hud-msg-node";
  
  if (aOnlyVisible) {
    selector += ":not(.hud-filtered-by-type)";
  }
  if (aClass) {
    selector += "." + aClass;
  }

  let msgs = aOutputNode.querySelectorAll(selector);
  let found = false;
  for (let i = 0, n = msgs.length; i < n; i++) {
    let message = msgs[i].textContent.indexOf(aMatchString);
    if (message > -1) {
      found = true;
      break;
    }

    
    let labels = msgs[i].querySelectorAll("label");
    for (let j = 0; j < labels.length; j++) {
      if (labels[j].getAttribute("value").indexOf(aMatchString) > -1) {
        found = true;
        break;
      }
    }
  }

  is(found, !aFailIfFound, aMsg);
}







function findLogEntry(aString)
{
  testLogEntry(outputNode, aString, "found " + aString);
}

function addStyle(aDocument, aString)
{
  let node = aDocument.createElement('style');
  node.setAttribute("type", "text/css");
  node.textContent = aString;
  aDocument.getElementsByTagName("head")[0].appendChild(node);
  return node;
}

function openConsole()
{
  HUDService.activateHUDForContext(tab);
}

function closeConsole()
{
  HUDService.deactivateHUDForContext(tab);
}

function finishTest()
{
  finish();
}

function tearDown()
{
  try {
    HUDService.deactivateHUDForContext(gBrowser.selectedTab);
  }
  catch (ex) {
    log(ex);
  }
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
  tab = browser = hudId = hud = filterBox = outputNode = cs = null;
}




function ComputedViewPanel(aContext)
{
  this._init(aContext);
}

ComputedViewPanel.prototype = {
  _init: function CVP_init(aContext)
  {
    this.window = aContext;
    this.document = this.window.document;
    this.cssLogic = new CssLogic();
    this.panelReady = false;
    this.iframeReady = false;
  },

  



  createPanel: function SI_createPanel(aSelection, aCallback)
  {
    let popupSet = this.document.getElementById("mainPopupSet");
    let panel = this.document.createElement("panel");

    panel.setAttribute("class", "styleInspector");
    panel.setAttribute("orient", "vertical");
    panel.setAttribute("ignorekeys", "true");
    panel.setAttribute("noautofocus", "true");
    panel.setAttribute("noautohide", "true");
    panel.setAttribute("titlebar", "normal");
    panel.setAttribute("close", "true");
    panel.setAttribute("label", "Computed View");
    panel.setAttribute("width", 350);
    panel.setAttribute("height", this.window.screen.height / 2);

    this._openCallback = aCallback;
    this.selectedNode = aSelection;

    let iframe = this.document.createElement("iframe");
    let boundIframeOnLoad = function loadedInitializeIframe()
    {
      this.iframeReady = true;
      this.iframe.removeEventListener("load", boundIframeOnLoad, true);
      this.panel.openPopup(this.window.gBrowser.selectedBrowser, "end_before", 0, 0, false, false);
    }.bind(this);

    iframe.flex = 1;
    iframe.setAttribute("tooltip", "aHTMLTooltip");
    iframe.addEventListener("load", boundIframeOnLoad, true);
    iframe.setAttribute("src", "chrome://browser/content/devtools/csshtmltree.xul");

    panel.appendChild(iframe);
    popupSet.appendChild(panel);

    this._boundPopupShown = this.popupShown.bind(this);
    panel.addEventListener("popupshown", this._boundPopupShown, false);

    this.panel = panel;
    this.iframe = iframe;

    return panel;
  },

  


  popupShown: function SI_popupShown()
  {
    this.panelReady = true;
    this.cssHtmlTree = new CssHtmlTree(this);
    let selectedNode = this.selectedNode || null;
    this.cssLogic.highlight(selectedNode);
    this.cssHtmlTree.highlight(selectedNode);
    if (this._openCallback) {
      this._openCallback();
      delete this._openCallback;
    }
  },

  isLoaded: function SI_isLoaded()
  {
    return this.iframeReady && this.panelReady;
  },

  



  selectFromPath: function SI_selectFromPath(aNode)
  {
    this.selectNode(aNode);
  },

  



  selectNode: function SI_selectNode(aNode)
  {
    this.selectedNode = aNode;

    if (this.isLoaded()) {
      this.cssLogic.highlight(aNode);
      this.cssHtmlTree.highlight(aNode);
    }
  },

  


  destroy: function SI_destroy()
  {
    this.panel.hidePopup();

    if (this.cssHtmlTree) {
      this.cssHtmlTree.destroy();
      delete this.cssHtmlTree;
    }

    if (this.iframe) {
      this.iframe.parentNode.removeChild(this.iframe);
      delete this.iframe;
    }

    delete this.cssLogic;
    this.panel.removeEventListener("popupshown", this._boundPopupShown, false);
    delete this._boundPopupShown;
    this.panel.parentNode.removeChild(this.panel);
    delete this.panel;
    delete this.doc;
    delete this.win;
    delete CssHtmlTree.win;
  },
};

function ruleView()
{
  return InspectorUI.sidebar._toolContext("ruleview").view;
}

function waitForEditorFocus(aParent, aCallback)
{
  aParent.addEventListener("focus", function onFocus(evt) {
    if (inplaceEditor(evt.target) && evt.target.tagName == "input") {
      aParent.removeEventListener("focus", onFocus, true);
      let editor = inplaceEditor(evt.target);
      executeSoon(function() {
        aCallback(editor);
      });
    }
  }, true);
}

function waitForEditorBlur(aEditor, aCallback)
{
  let input = aEditor.input;
  input.addEventListener("blur", function onBlur() {
    input.removeEventListener("blur", onBlur, false);
    executeSoon(function() {
      aCallback();
    });
  }, false);
}

registerCleanupFunction(tearDown);

waitForExplicitFinish();

