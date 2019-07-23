






































var gPromptService;
var gFindBundle;

function nsFindInstData() {}
nsFindInstData.prototype =
{
  
  browser : null,

  get rootSearchWindow() { return this._root || this.window.content; },
  set rootSearchWindow(val) { this._root = val; },

  get currentSearchWindow() {
    if (this._current)
      return this._current;

    var focusedWindow = this.window.document.commandDispatcher.focusedWindow;
    if (!focusedWindow || focusedWindow == this.window)
      focusedWindow = this.window.content;

    return focusedWindow;
  },
  set currentSearchWindow(val) { this._current = val; },

  get webBrowserFind() { return this.browser.webBrowserFind; },

  init : function() {
    var findInst = this.webBrowserFind;
    
    var findInFrames = findInst.QueryInterface(Components.interfaces.nsIWebBrowserFindInFrames);
    findInFrames.rootSearchFrame = this.rootSearchWindow;
    findInFrames.currentSearchFrame = this.currentSearchWindow;
  
    
    findInst.searchFrames = true;
  },

  window : window,
  _root : null,
  _current : null
}




function findInPage(findInstData)
{
  
  if ("findDialog" in window && window.findDialog)
    window.findDialog.focus();
  else
  {
    findInstData.init();
    window.findDialog = window.openDialog("chrome://global/content/finddialog.xul", "_blank", "chrome,resizable=no,dependent=yes", findInstData);
  }
}

function findAgainInPage(findInstData, reverse)
{
  
  
  
  var findService = Components.classes["@mozilla.org/find/find_service;1"]
                         .getService(Components.interfaces.nsIFindService);

  var searchString = findService.searchString;
  if (searchString.length == 0) {
    
    findInPage(findInstData);
    return;
  }

  findInstData.init();
  var findInst = findInstData.webBrowserFind;
  findInst.searchString  = searchString;
  findInst.matchCase     = findService.matchCase;
  findInst.wrapFind      = findService.wrapFind;
  findInst.entireWord    = findService.entireWord;
  findInst.findBackwards = findService.findBackwards ^ reverse;

  var found = findInst.findNext();
  if (!found) {
    if (!gPromptService)
      gPromptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService()
                                 .QueryInterface(Components.interfaces.nsIPromptService);                                     
    if (!gFindBundle)
      gFindBundle = document.getElementById("findBundle");
        
    gPromptService.alert(window, gFindBundle.getString("notFoundTitle"), gFindBundle.getString("notFoundWarning"));
  }      

  
  findInst.findBackwards = findService.findBackwards; 
}

function canFindAgainInPage()
{
    var findService = Components.classes["@mozilla.org/find/find_service;1"]
                           .getService(Components.interfaces.nsIFindService);
    return (findService.searchString.length > 0);
}

