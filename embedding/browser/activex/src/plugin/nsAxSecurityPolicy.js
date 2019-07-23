



































const NS_IACTIVEXSECURITYPOLICY_CONTRACTID =
    "@mozilla.org/nsactivexsecuritypolicy;1";
const NS_IACTIVEXSECURITYPOLICY_CID =
    Components.ID("{B9BDB43B-109E-44b8-858C-B68C6C3DF86F}");

const nsISupports               = Components.interfaces.nsISupports;
const nsIObserver               = Components.interfaces.nsIObserver;
const nsIActiveXSecurityPolicy  = Components.interfaces.nsIActiveXSecurityPolicy;






const kTotalSecurityHostingFlags =
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_HOST_NOTHING;


const kHighSecurityHostingFlags =
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_HOST_SAFE_OBJECTS;


const kMediumSecurityGlobalHostingFlags =
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_HOST_SAFE_OBJECTS |
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_DOWNLOAD_CONTROLS |
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_SCRIPT_SAFE_OBJECTS;


const kLowSecurityHostFlags =
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_HOST_SAFE_OBJECTS |
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_DOWNLOAD_CONTROLS |
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_SCRIPT_SAFE_OBJECTS |
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_HOST_ALL_OBJECTS;


const kNoSecurityHostingFlags =
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_HOST_SAFE_OBJECTS |
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_DOWNLOAD_CONTROLS |
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_SCRIPT_SAFE_OBJECTS |
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_SCRIPT_ALL_OBJECTS |
    nsIActiveXSecurityPolicy.HOSTING_FLAGS_HOST_ALL_OBJECTS;






const kDefaultGlobalHostingFlags = kMediumSecurityGlobalHostingFlags;
const kDefaultOtherHostingFlags  = kMediumSecurityGlobalHostingFlags;


const kHostingPrefPart1 = "security.xpconnect.activex.";
const kHostingPrefPart2 = ".hosting_flags";
const kGlobalHostingFlagsPref = kHostingPrefPart1 + "global" + kHostingPrefPart2;

var gPref = null;

function addPrefListener(observer, prefStr)
{
    try {
        if (gPref == null) {
            var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                                        .getService(Components.interfaces.nsIPrefService);
            gPref = prefService.getBranch(null);
        }
        var pbi = gPref.QueryInterface(Components.interfaces.nsIPrefBranch2);
        pbi.addObserver(prefStr, observer, false);
    } catch(ex) {
        dump("Failed to observe prefs: " + ex + "\n");
    }
}

function AxSecurityPolicy()
{
    addPrefListener(this, kGlobalHostingFlagsPref);
    this.syncPrefs();
    this.globalHostingFlags = kDefaultGlobalHostingFlags;
}

AxSecurityPolicy.prototype = {
    syncPrefs: function()
    {
        var hostingFlags = this.globalHostingFlags;
        try {
            if (gPref == null) {
                var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                                            .getService(Components.interfaces.nsIPrefService);
                gPref = prefService.getBranch(null);
            }
            hostingFlags = gPref.getIntPref(kGlobalHostingFlagsPref);
        }
        catch (ex) {
            dump("Failed to read control hosting flags from \"" + kGlobalHostingFlagsPref + "\"\n");
            hostingFlags = kDefaultGlobalHostingFlags;
        }
        if (hostingFlags != this.globalHostingFlags)
        {
            dump("Global activex hosting flags have changed value to " + hostingFlags + "\n");
            this.globalHostingFlags = hostingFlags
        }
    },

    
    getHostingFlags: function(context)
    {
        var hostingFlags;
        if (context == null || context == "global") {
            hostingFlags = this.globalHostingFlags;
        }
        else {
            try {
                var prefName = kHostingPrefPart1 + context + kHostingPrefPart2;
                hostingFlags = gPref.getIntPref(prefName);
            }
            catch (ex) {
                dump("Failed to read control hosting prefs for \"" + context + "\" from \"" + prefName + "\" pref\n");
                hostingFlags = kDefaultOtherHostingFlags;
            }
            hostingFlags = kDefaultOtherHostingFlags;
        }
        return hostingFlags;
    },
    
    observe: function(subject, topic, prefName)
    {
        if (topic != "nsPref:changed")
            return;
        this.syncPrefs();

    },
    
    QueryInterface: function(iid) {
        if (iid.equals(nsISupports) ||
            iid.equals(nsIActiveXSecurityPolicy) ||
            iid.equals(nsIObserver))
            return this;

        throw Components.results.NS_ERROR_NO_INTERFACE;
    }
};


var AxSecurityPolicyFactory = {
    createInstance: function (outer, iid)
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
        if (!iid.equals(nsISupports) &&
            !iid.equals(nsIActiveXSecurityPolicy) &&
            !iid.equals(nsIObserver))
            throw Components.results.NS_ERROR_INVALID_ARG;
        return new AxSecurityPolicy();
    }
};


var AxSecurityPolicyModule = {
    registerSelf: function (compMgr, fileSpec, location, type)
    {
        debug("*** Registering axsecurity policy.\n");
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.registerFactoryLocation(
            NS_IACTIVEXSECURITYPOLICY_CID ,
            "ActiveX Security Policy Service",
            NS_IACTIVEXSECURITYPOLICY_CONTRACTID,
            fileSpec,
            location,
            type);
    },
    unregisterSelf: function(compMgr, fileSpec, location)
    {
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.unregisterFactoryLocation(NS_IACTIVEXSECURITYPOLICY_CID, fileSpec);
    },
    getClassObject: function(compMgr, cid, iid)
    {
        if (cid.equals(NS_IACTIVEXSECURITYPOLICY_CID))
            return AxSecurityPolicyFactory;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        throw Components.results.NS_ERROR_NO_INTERFACE;
    },
    canUnload: function(compMgr)
    {
        return true;
    }
};


function NSGetModule(compMgr, fileSpec) {
    return AxSecurityPolicyModule;
}


