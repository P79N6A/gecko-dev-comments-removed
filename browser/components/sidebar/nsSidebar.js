




Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const DEBUG = false; 

const SIDEBAR_CONTRACTID        = "@mozilla.org/sidebar;1";
const SIDEBAR_CID               = Components.ID("{22117140-9c6e-11d3-aaf1-00805f8a4905}");


const SHERLOCK_FILE_EXT_REGEXP = /\.src$/i;

function nsSidebar()
{
}

nsSidebar.prototype.classID = SIDEBAR_CID;

nsSidebar.prototype.validateSearchEngine =
function (engineURL, iconURL)
{
  try
  {
    
    var isWeb = /^(https?|ftp):\/\//i;

    if (!isWeb.test(engineURL))
      throw "Unsupported search engine URL";

    if (iconURL && !isWeb.test(iconURL))
      throw "Unsupported search icon URL.";
  }
  catch(ex)
  {
    debug(ex);
    Components.utils.reportError("Invalid argument passed to window.sidebar.addSearchEngine: " + ex);

    var searchBundle = Services.strings.createBundle("chrome://global/locale/search/search.properties");
    var brandBundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");
    var brandName = brandBundle.GetStringFromName("brandShortName");
    var title = searchBundle.GetStringFromName("error_invalid_engine_title");
    var msg = searchBundle.formatStringFromName("error_invalid_engine_msg",
                                                [brandName], 1);
    Services.ww.getNewPrompter(null).alert(title, msg);
    return false;
  }

  return true;
}



nsSidebar.prototype.addSearchEngine =
function (engineURL, iconURL, suggestedTitle, suggestedCategory)
{
  debug("addSearchEngine(" + engineURL + ", " + iconURL + ", " +
        suggestedCategory + ", " + suggestedTitle + ")");

  if (!this.validateSearchEngine(engineURL, iconURL))
    return;

  
  
  
  var dataType;
  if (SHERLOCK_FILE_EXT_REGEXP.test(engineURL))
    dataType = Components.interfaces.nsISearchEngine.DATA_TEXT;
  else
    dataType = Components.interfaces.nsISearchEngine.DATA_XML;

  Services.search.addEngine(engineURL, dataType, iconURL, true);
}




nsSidebar.prototype.AddSearchProvider =
function (aDescriptionURL)
{
  
  
  
  
  var win = Services.wm.getMostRecentWindow("navigator:browser");
  var browser = win.gBrowser;
  var iconURL = "";
  
  
  if (browser.shouldLoadFavIcon(browser.selectedBrowser
                                       .contentDocument
                                       .documentURIObject))
    iconURL = browser.getIcon();

  if (!this.validateSearchEngine(aDescriptionURL, iconURL))
    return;

  const typeXML = Components.interfaces.nsISearchEngine.DATA_XML;
  Services.search.addEngine(aDescriptionURL, typeXML, iconURL, true);
}









nsSidebar.prototype.IsSearchProviderInstalled =
function (aSearchURL)
{
  return 0;
}

nsSidebar.prototype.QueryInterface = XPCOMUtils.generateQI([Components.interfaces.nsISupports]);

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([nsSidebar]);


if (DEBUG)
    debug = function (s) { dump("-*- sidebar component: " + s + "\n"); }
else
    debug = function (s) {}
