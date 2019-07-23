




















































const DEBUG = false; 
const PANELS_RDF_FILE  = "UPnls"; 

const SIDEBAR_CONTRACTID   = "@mozilla.org/sidebar;1";
const SIDEBAR_CID      = Components.ID("{22117140-9c6e-11d3-aaf1-00805f8a4905}");
const CONTAINER_CONTRACTID = "@mozilla.org/rdf/container;1";
const DIR_SERV_CONTRACTID  = "@mozilla.org/file/directory_service;1"
const NETSEARCH_CONTRACTID = "@mozilla.org/rdf/datasource;1?name=internetsearch"
const IO_SERV_CONTRACTID   = "@mozilla.org/network/io-service;1";
const nsISupports      = Components.interfaces.nsISupports;
const nsIFactory       = Components.interfaces.nsIFactory;
const nsISidebar       = Components.interfaces.nsISidebar;
const nsIRDFContainer  = Components.interfaces.nsIRDFContainer;
const nsIProperties    = Components.interfaces.nsIProperties;
const nsIFileURL       = Components.interfaces.nsIFileURL;
const nsIRDFRemoteDataSource = Components.interfaces.nsIRDFRemoteDataSource;
const nsIInternetSearchService = Components.interfaces.nsIInternetSearchService;
const nsIClassInfo = Components.interfaces.nsIClassInfo;

function nsSidebar()
{
    const RDF_CONTRACTID = "@mozilla.org/rdf/rdf-service;1";
    const nsIRDFService = Components.interfaces.nsIRDFService;

    this.rdf = Components.classes[RDF_CONTRACTID].getService(nsIRDFService);
    this.datasource_uri = getSidebarDatasourceURI(PANELS_RDF_FILE);
    debug('datasource_uri is ' + this.datasource_uri);
    this.resource = 'urn:sidebar:current-panel-list';
    this.datasource = this.rdf.GetDataSource(this.datasource_uri);

    const PROMPTSERVICE_CONTRACTID = "@mozilla.org/embedcomp/prompt-service;1";
    const nsIPromptService = Components.interfaces.nsIPromptService;
    this.promptService =
        Components.classes[PROMPTSERVICE_CONTRACTID].getService(nsIPromptService);
}

nsSidebar.prototype.nc = "http://home.netscape.com/NC-rdf#";

nsSidebar.prototype.isPanel =
function (aContentURL)
{
    var container =
        Components.classes[CONTAINER_CONTRACTID].createInstance(nsIRDFContainer);

    container.Init(this.datasource, this.rdf.GetResource(this.resource));

    
    var panel_resource =
        this.rdf.GetResource("urn:sidebar:3rdparty-panel:" + aContentURL);

    return (container.IndexOf(panel_resource) != -1);
}

function sidebarURLSecurityCheck(url)
{
    if (!/(^http:|^ftp:|^https:)/i.test(url))
        throw "Script attempted to add sidebar panel from illegal source";
}


nsSidebar.prototype.addPanel =
function (aTitle, aContentURL, aCustomizeURL)
{
    debug("addPanel(" + aTitle + ", " + aContentURL + ", " +
          aCustomizeURL + ")");

    return this.addPanelInternal(aTitle, aContentURL, aCustomizeURL, false);
}

nsSidebar.prototype.addPersistentPanel =
function(aTitle, aContentURL, aCustomizeURL)
{
    debug("addPersistentPanel(" + aTitle + ", " + aContentURL + ", " +
           aCustomizeURL + ")\n");

    return this.addPanelInternal(aTitle, aContentURL, aCustomizeURL, true);
}

nsSidebar.prototype.addPanelInternal =
function (aTitle, aContentURL, aCustomizeURL, aPersist)
{
    sidebarURLSecurityCheck(aContentURL);

    
    
    var panel_list = this.datasource.GetTarget(this.rdf.GetResource(this.resource), this.rdf.GetResource(nsSidebar.prototype.nc+"panel-list"), true);
    if (panel_list) {
        panel_list.QueryInterface(Components.interfaces.nsIRDFResource);
    } else {
        
        debug("Sidebar datasource is busted\n");
    }

    var container = Components.classes[CONTAINER_CONTRACTID].createInstance(nsIRDFContainer);
    container.Init(this.datasource, panel_list);

    
    var panel_resource =
        this.rdf.GetResource("urn:sidebar:3rdparty-panel:" + aContentURL);
    var panel_index = container.IndexOf(panel_resource);
    var stringBundle, brandStringBundle, titleMessage, dialogMessage;
    if (panel_index != -1)
    {
        try {
            stringBundle = srGetStrBundle("chrome://communicator/locale/sidebar/sidebar.properties");
            brandStringBundle = srGetStrBundle("chrome://branding/locale/brand.properties");
            if (stringBundle) {
                sidebarName = brandStringBundle.GetStringFromName("sidebarName");
                titleMessage = stringBundle.GetStringFromName("dupePanelAlertTitle");
                dialogMessage = stringBundle.GetStringFromName("dupePanelAlertMessage");
                dialogMessage = dialogMessage.replace(/%url%/, aContentURL);
                dialogMessage = dialogMessage.replace(/%name%/, sidebarName);
            }
        }
        catch (e) {
            titleMessage = "Sidebar";
            dialogMessage = aContentURL + " already exists in Sidebar.  No string bundle";
        }

        this.promptService.alert(null, titleMessage, dialogMessage);

        return;
    }

    try {
        stringBundle = srGetStrBundle("chrome://communicator/locale/sidebar/sidebar.properties");
        brandStringBundle = srGetStrBundle("chrome://branding/locale/brand.properties");
        if (stringBundle) {
            sidebarName = brandStringBundle.GetStringFromName("sidebarName");
            titleMessage = stringBundle.GetStringFromName("addPanelConfirmTitle");
            dialogMessage = stringBundle.GetStringFromName("addPanelConfirmMessage");
            if (aPersist)
            {
                var warning = stringBundle.GetStringFromName("persistentPanelWarning");
                dialogMessage += "\n" + warning;
            }
            dialogMessage = dialogMessage.replace(/%title%/, aTitle);
            dialogMessage = dialogMessage.replace(/%url%/, aContentURL);
            dialogMessage = dialogMessage.replace(/#/g, "\n");
            dialogMessage = dialogMessage.replace(/%name%/g, sidebarName);
        }
    }
    catch (e) {
        titleMessage = "Add Tab to Sidebar";
        dialogMessage = "No string bundle.  Add the Tab '" + aTitle + "' to Sidebar?\n\n" + "Source: " + aContentURL;
    }

    var rv = this.promptService.confirm(null, titleMessage, dialogMessage);

    if (!rv)
        return;

    
    this.datasource.Assert(panel_resource,
                           this.rdf.GetResource(this.nc + "title"),
                           this.rdf.GetLiteral(aTitle),
                           true);
    this.datasource.Assert(panel_resource,
                           this.rdf.GetResource(this.nc + "content"),
                           this.rdf.GetLiteral(aContentURL),
                           true);
    if (aCustomizeURL)
        this.datasource.Assert(panel_resource,
                               this.rdf.GetResource(this.nc + "customize"),
                               this.rdf.GetLiteral(aCustomizeURL),
                               true);
    var persistValue = aPersist ? "true" : "false";
    this.datasource.Assert(panel_resource,
                           this.rdf.GetResource(this.nc + "persist"),
                           this.rdf.GetLiteral(persistValue),
                           true);

    container.AppendElement(panel_resource);

    
    
    this.datasource.Assert(this.rdf.GetResource(this.resource),
                           this.rdf.GetResource(this.nc + "refresh"),
                           this.rdf.GetLiteral("true"),
                           true);
    this.datasource.Unassert(this.rdf.GetResource(this.resource),
                             this.rdf.GetResource(this.nc + "refresh"),
                             this.rdf.GetLiteral("true"));

    
    this.datasource.QueryInterface(nsIRDFRemoteDataSource).Flush();

}


nsSidebar.prototype.addSearchEngine =
function (engineURL, iconURL, suggestedTitle, suggestedCategory)
{
    debug("addSearchEngine(" + engineURL + ", " + iconURL + ", " +
          suggestedCategory + ", " + suggestedTitle + ")");

    try
    {
        
        
        if (! /^https?:\/\/.+\.src$/i.test(engineURL))
            throw "Unsupported search engine URL";

        
        
        if (! /^https?:\/\/.+\.(gif|jpg|jpeg|png)$/i.test(iconURL))
            throw "Unsupported search icon URL";
    }
    catch(ex)
    {
        debug(ex);
        this.promptService.alert(null, "Error", "Failed to add the search engine.");
        throw Components.results.NS_ERROR_INVALID_ARG;
    }

    var titleMessage, dialogMessage;
    try {
        var stringBundle = srGetStrBundle("chrome://communicator/locale/sidebar/sidebar.properties");
        var brandStringBundle = srGetStrBundle("chrome://branding/locale/brand.properties");
        if (stringBundle) {
            sidebarName = brandStringBundle.GetStringFromName("sidebarName");
            titleMessage = stringBundle.GetStringFromName("addEngineConfirmTitle");
            dialogMessage = stringBundle.GetStringFromName("addEngineConfirmMessage");
            dialogMessage = dialogMessage.replace(/%title%/, suggestedTitle);
            dialogMessage = dialogMessage.replace(/%category%/, suggestedCategory);
            dialogMessage = dialogMessage.replace(/%url%/, engineURL);
            dialogMessage = dialogMessage.replace(/#/g, "\n");
            dialogMessage = dialogMessage.replace(/%name%/, sidebarName);
        }
    }
    catch (e) {
        titleMessage = "Add Search Engine";
        dialogMessage = "Add the following search engine?\n\nName: " + suggestedTitle;
        dialogMessage += "\nSearch Category: " + suggestedCategory;
        dialogMessage += "\nSource: " + engineURL;
    }

    var rv = this.promptService.confirm(null, titleMessage, dialogMessage);

    if (!rv)
        return;

    var internetSearch = Components.classes[NETSEARCH_CONTRACTID].getService();
    if (internetSearch)
        internetSearch = internetSearch.QueryInterface(nsIInternetSearchService);
    if (internetSearch)
    {
        internetSearch.AddSearchEngine(engineURL, iconURL, suggestedTitle,
                                       suggestedCategory);
    }
}


nsSidebar.prototype.flags = nsIClassInfo.DOM_OBJECT;


nsSidebar.prototype.classDescription = "Sidebar";


nsSidebar.prototype.getInterfaces = function(count) {
    var interfaceList = [nsISidebar, nsIClassInfo];
    count.value = interfaceList.length;
    return interfaceList;
}


nsSidebar.prototype.getHelperForLanguage = function(count) {return null;}

nsSidebar.prototype.QueryInterface =
function (iid) {
    if (iid.equals(nsISidebar) ||
        iid.equals(nsIClassInfo) ||
        iid.equals(nsISupports))
        return this;

    Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
    return null;
}

var sidebarModule = new Object();

sidebarModule.registerSelf =
function (compMgr, fileSpec, location, type)
{
    debug("registering (all right -- a JavaScript module!)");
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);

    compMgr.registerFactoryLocation(SIDEBAR_CID,
                                    "Sidebar JS Component",
                                    SIDEBAR_CONTRACTID,
                                    fileSpec,
                                    location,
                                    type);

    const CATMAN_CONTRACTID = "@mozilla.org/categorymanager;1";
    const nsICategoryManager = Components.interfaces.nsICategoryManager;
    var catman = Components.classes[CATMAN_CONTRACTID].
                            getService(nsICategoryManager);

    const JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY = "JavaScript global property";
    catman.addCategoryEntry(JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                            "sidebar",
                            SIDEBAR_CONTRACTID,
                            true,
                            true);
}

sidebarModule.getClassObject =
function (compMgr, cid, iid) {
    if (!cid.equals(SIDEBAR_CID))
        throw Components.results.NS_ERROR_NO_INTERFACE;

    if (!iid.equals(Components.interfaces.nsIFactory))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return sidebarFactory;
}

sidebarModule.canUnload =
function(compMgr)
{
    debug("Unloading component.");
    return true;
}


var sidebarFactory = new Object();

sidebarFactory.createInstance =
function (outer, iid) {
    debug("CI: " + iid);
    if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;

    return (new nsSidebar()).QueryInterface(iid);
}


function NSGetModule(compMgr, fileSpec) {
    return sidebarModule;
}


if (DEBUG)
    debug = function (s) { dump("-*- sidebar component: " + s + "\n"); }
else
    debug = function (s) {}

function getSidebarDatasourceURI(panels_file_id)
{
    try
    {
        


        var directory_service = Components.classes[DIR_SERV_CONTRACTID].getService(Components.interfaces.nsIProperties);

        

        var sidebar_file = directory_service.get(panels_file_id, Components.interfaces.nsIFile);

        if (!sidebar_file.exists())
        {
            

            debug("sidebar file does not exist");
            return null;
        }

        var io_service = Components.classes[IO_SERV_CONTRACTID].getService(Components.interfaces.nsIIOService);
        var file_handler = io_service.getProtocolHandler("file").QueryInterface(Components.interfaces.nsIFileProtocolHandler);
        var sidebar_uri = file_handler.getURLSpecFromFile(sidebar_file);
        debug("sidebar uri is " + sidebar_uri);
        return sidebar_uri;
    }
    catch (ex)
    {
        
        debug("caught " + ex + " getting sidebar datasource uri");
        return null;
    }
}


var strBundleService = null;
function srGetStrBundle(path)
{
   var strBundle = null;
   if (!strBundleService) {
       try {
          strBundleService =
          Components.classes["@mozilla.org/intl/stringbundle;1"].getService();
          strBundleService =
          strBundleService.QueryInterface(Components.interfaces.nsIStringBundleService);
       } catch (ex) {
          dump("\n--** strBundleService failed: " + ex + "\n");
          return null;
      }
   }
   strBundle = strBundleService.createBundle(path);
   if (!strBundle) {
       dump("\n--** strBundle createInstance failed **--\n");
   }
   return strBundle;
}


