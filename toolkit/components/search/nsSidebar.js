




const { interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");


const SHERLOCK_FILE_EXT_REGEXP = /\.src$/i;

function nsSidebar() {
}

nsSidebar.prototype = {
  init: function(window) {
    this.window = window;
    this.mm = window.QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIDocShell)
                    .QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIContentFrameMessageManager);
  },

  
  
  addSearchEngine: function(engineURL, iconURL, suggestedTitle, suggestedCategory) {
    let dataType = SHERLOCK_FILE_EXT_REGEXP.test(engineURL) ?
                   Ci.nsISearchEngine.DATA_TEXT :
                   Ci.nsISearchEngine.DATA_XML;

    this.mm.sendAsyncMessage("Search:AddEngine", {
      pageURL: this.window.document.documentURIObject.spec,
      engineURL,
      type: dataType,
      iconURL
    });
  },

  
  
  
  AddSearchProvider: function(engineURL) {
    this.mm.sendAsyncMessage("Search:AddEngine", {
      pageURL: this.window.document.documentURIObject.spec,
      engineURL,
      type: Ci.nsISearchEngine.DATA_XML
    });
  },

  
  
  
  IsSearchProviderInstalled: function(engineURL) {
    return 0;
  },

  classID: Components.ID("{22117140-9c6e-11d3-aaf1-00805f8a4905}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([nsSidebar]);
