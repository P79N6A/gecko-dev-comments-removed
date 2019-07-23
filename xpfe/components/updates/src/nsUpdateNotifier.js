





































const kDebug               = false;
const kUpdateCheckDelay    = 5 * 60 * 1000; 
const kUNEnabledPref       = "update_notifications.enabled";
const kUNDatasourceURIPref = "update_notifications.provider.0.datasource";
const kUNFrequencyPref     = "update_notifications.provider.0.frequency";
const kUNLastCheckedPref   = "update_notifications.provider.0.last_checked";
const kUNBundleURI         =
  "chrome://communicator/locale/update-notifications.properties";











var nsUpdateNotifier =
{
  mInitialized: false,

  onProfileStartup: function(aProfileName)
  {
    debug("onProfileStartup");

    
    if (this.mInitialized)
      return;
    this.mInitialized = true;

    
    var observerService = Components.
      classes["@mozilla.org/observer-service;1"].
      getService(Components.interfaces.nsIObserverService);
    observerService.addObserver(this, "domwindowopened", false);
  },

  mTimer: null, 

  observe: function(aSubject, aTopic, aData)
  {
    debug("observe: " + aTopic);

    if (aTopic == "domwindowopened")
    {
      try
      {
        const kITimer = Components.interfaces.nsITimer;
        this.mTimer = Components.classes["@mozilla.org/timer;1"].
          createInstance(kITimer);
        this.mTimer.init(this, kUpdateCheckDelay, kITimer.TYPE_ONE_SHOT);

        
        var observerService = Components.
          classes["@mozilla.org/observer-service;1"].
          getService(Components.interfaces.nsIObserverService);
        observerService.removeObserver(this, "domwindowopened");

        
        
        observerService.addObserver(this, "xpcom-shutdown", false);
      }
      catch (ex)
      {
        debug("Exception init'ing timer: " + ex);
      }
    }
    else if (aTopic == "timer-callback")
    {
      this.mTimer = null; 
      this.checkForUpdate();
    }
    else if (aTopic == "xpcom-shutdown")
    {
      



      this.mTimer = null;
    }
  },

  checkForUpdate: function()
  {
    debug("checkForUpdate");

    if (this.shouldCheckForUpdate())
    {
      try
      {
        
        var prefs = Components.classes["@mozilla.org/preferences-service;1"].
          getService(Components.interfaces.nsIPrefBranch);
        var updateDatasourceURI = prefs.
          getComplexValue(kUNDatasourceURIPref,
          Components.interfaces.nsIPrefLocalizedString).data;

        var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].
          getService(Components.interfaces.nsIRDFService);
        var ds = rdf.GetDataSource(updateDatasourceURI);

        ds = ds.QueryInterface(Components.interfaces.nsIRDFXMLSink);
        ds.addXMLSinkObserver(nsUpdateDatasourceObserver);
      }
      catch (ex)
      {
        debug("Exception getting updates.rdf: " + ex);
      }
    }
  },

  shouldCheckForUpdate: function()
  {
    debug("shouldCheckForUpdate");

    var shouldCheck = false;

    try
    {
      var prefs = Components.classes["@mozilla.org/preferences-service;1"].
        getService(Components.interfaces.nsIPrefBranch);

      if (prefs.getBoolPref(kUNEnabledPref))
      {
        var freq = prefs.getIntPref(kUNFrequencyPref) * (24 * 60 * 60); 
        var now = (new Date().valueOf())/1000; 

        if (!prefs.prefHasUserValue(kUNLastCheckedPref))
        {
          
          
          

          var randomizedLastChecked = now + freq * (1 + Math.random());
          prefs.setIntPref(kUNLastCheckedPref, randomizedLastChecked);

          return false;
        }

        var lastChecked = prefs.getIntPref(kUNLastCheckedPref);
        if ((lastChecked + freq) > now)
          return false;

        prefs.setIntPref(kUNLastCheckedPref, now);
        prefs = prefs.QueryInterface(Components.interfaces.nsIPrefService);
        prefs.savePrefFile(null); 

        shouldCheck = true;
      }
    }
    catch (ex)
    {
      shouldCheck = false;
      debug("Exception in shouldCheckForUpdate: " + ex);
    }

    return shouldCheck;
  },

  QueryInterface: function(aIID)
  {
    if (aIID.equals(Components.interfaces.nsIObserver) ||
        aIID.equals(Components.interfaces.nsIProfileStartupListener) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
    return null;
  }
}










var nsUpdateDatasourceObserver =
{
  onBeginLoad: function(aSink)
  {
  },

  onInterrupt: function(aSink)
  {
  },

  onResume: function(aSink)
  {
  },

  onEndLoad: function(aSink)
  {
    debug("onEndLoad");

    aSink.removeXMLSinkObserver(this);

    var ds = aSink.QueryInterface(Components.interfaces.nsIRDFDataSource);
    var updateInfo = this.getUpdateInfo(ds);
    if (updateInfo && this.newerVersionAvailable(updateInfo))
    {
      var promptService = Components.
        classes["@mozilla.org/embedcomp/prompt-service;1"].
        getService(Components.interfaces.nsIPromptService);
      var winWatcher = Components.
        classes["@mozilla.org/embedcomp/window-watcher;1"].
        getService(Components.interfaces.nsIWindowWatcher);

      var unBundle = this.getBundle(kUNBundleURI);
      if (!unBundle)
        return;

      var title = unBundle.formatStringFromName("title",
        [updateInfo.productName], 1);
      var desc = unBundle.formatStringFromName("desc",
        [updateInfo.productName], 1);
      var button0Text = unBundle.GetStringFromName("getItNow");
      var button1Text = unBundle.GetStringFromName("noThanks");
      var checkMsg = unBundle.GetStringFromName("dontAskAgain");
      var checkVal = {value:0};

      var result = promptService.confirmEx(winWatcher.activeWindow, title, desc,
        (promptService.BUTTON_POS_0 * promptService.BUTTON_TITLE_IS_STRING) +
        (promptService.BUTTON_POS_1 * promptService.BUTTON_TITLE_IS_STRING),
        button0Text, button1Text, null, checkMsg, checkVal);

      
      
      if (result == 0)
      {
        var browserURL = "chrome://navigator/content/navigator.xul";
        try {
          browserURL = Components.classes["@mozilla.org/preferences-service;1"]
                                 .getService(Components.interfaces.nsIPrefBranch)
                                 .getCharPref("browser.chromeURL");
        } catch (e) {
        }

        var argstring = Components.classes["@mozilla.org/supports-string;1"]
                                  .createInstance(Components.interfaces.nsISupportsString);
        argstring.data = updateInfo.URL;
        winWatcher.openWindow(winWatcher.activeWindow, browserURL,
                              "_blank", "chrome,all,dialog=no", argstring);
      }

      
      if (checkVal.value)
      {
        var prefs = Components.classes["@mozilla.org/preferences-service;1"].
          getService(Components.interfaces.nsIPrefBranch);
        prefs.setBoolPref(kUNEnabledPref, false);
      }
    }
  },

  onError: function(aSink, aStatus, aErrorMsg)
  {
    debug("Error " + aStatus + ": " + aErrorMsg);
    aSink.removeXMLSinkObserver(this);
  },

  getUpdateInfo: function(aDS)
  {
    var info = null;

    try
    {
      var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].
        getService(Components.interfaces.nsIRDFService);
      var src = "urn:updates:latest";

      info = new Object;
      info.registryName = this.getTarget(rdf, aDS, src, "registryName");
      info.version = this.getTarget(rdf, aDS, src, "version");
      info.URL = this.getTarget(rdf, aDS, src, "URL");
      info.productName = this.getTarget(rdf, aDS, src, "productName");
    }
    catch (ex)
    {
      info = null;
      debug("Exception getting update info: " + ex);

      
      
      
      
      
      
      
      
      
      
      
      
    }

    return info;
  },

  getTarget: function(aRDF, aDS, aSrc, aProp)
  {
    var src = aRDF.GetResource(aSrc);
    var arc = aRDF.GetResource("http://home.netscape.com/NC-rdf#" + aProp);
    var target = aDS.GetTarget(src, arc, true);
    return target.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
  },

  newerVersionAvailable: function(aUpdateInfo)
  {
    
    if (!aUpdateInfo.registryName || !aUpdateInfo.version)
    {
      debug("Sanity check failed: aUpdateInfo is invalid!");
      return false;
    }

    
    

    if (aUpdateInfo.registryName == "Browser")
      return this.neckoHaveNewer(aUpdateInfo);

    return this.xpinstallHaveNewer(aUpdateInfo);
  },

  neckoHaveNewer: function(aUpdateInfo)
  {
    try
    {
      var httpHandler = Components.
        classes["@mozilla.org/network/protocol;1?name=http"].
        getService(Components.interfaces.nsIHttpProtocolHandler);
      var synthesized = this.synthesizeVersion(httpHandler.misc,
        httpHandler.productSub);
      var local = new nsVersion(synthesized);
      var server = new nsVersion(aUpdateInfo.version);

      return (server.isNewerThan(local));
    }
    catch (ex)
    {
      
      debug("Exception getting httpHandler: " + ex);
      return false;
    }

    return false; 
  },

  xpinstallHaveNewer: function(aUpdateInfo)
  {
    
    
    
    
    
    var ass = Components.classes["@mozilla.org/appshell/appShellService;1"].
      getService(Components.interfaces.nsIAppShellService);
    var trigger = ass.hiddenDOMWindow.InstallTrigger;
    var diffLevel = trigger.compareVersion(aUpdateInfo.registryName,
      aUpdateInfo.version);
    if (diffLevel < trigger.EQUAL && diffLevel != trigger.NOT_FOUND)
      return true;
    return false; 
                  
  },

  synthesizeVersion: function(aMisc, aProductSub)
  {
    
    
    
    
    

    var synthesized = "0.0.0.";

    
    var onlyVer = /rv:([0-9.]+)/.exec(aMisc);

    
    if (onlyVer && onlyVer.length >= 2)
    {
      var parts = onlyVer[1].split('.');
      var len = parts.length;
      if (len > 0)
      {
        synthesized = "";

        
        for (var i = 0; i < 3; ++i)
        {
          synthesized += ((len >= i+1) ? parts[i] : "0") + ".";
        }
      }
    }

    
    synthesized += aProductSub ? aProductSub : "0";

    return synthesized;
  },

  getBundle: function(aURI)
  {
    if (!aURI)
      return null;

    var bundle = null;
    try
    {
      var strBundleService = Components.
        classes["@mozilla.org/intl/stringbundle;1"].
        getService(Components.interfaces.nsIStringBundleService);
      bundle = strBundleService.createBundle(aURI);
    }
    catch (ex)
    {
      bundle = null;
      debug("Exception getting bundle " + aURI + ": " + ex);
    }

    return bundle;
  }
}












function nsVersion(aStringVersion)
{
  var parts = aStringVersion.split('.');
  var len = parts.length;

  this.mMajor   = (len >= 1) ? this.getValidInt(parts[0]) : 0;
  this.mMinor   = (len >= 2) ? this.getValidInt(parts[1]) : 0;
  this.mRelease = (len >= 3) ? this.getValidInt(parts[2]) : 0;
  this.mBuild   = (len >= 4) ? this.getValidInt(parts[3]) : 0;
}

nsVersion.prototype =
{
  isNewerThan: function(aOther)
  {
    if (this.mMajor == aOther.mMajor)
    {
      if (this.mMinor == aOther.mMinor)
      {
        if (this.mRelease == aOther.mRelease)
        {
          if (this.mBuild <= aOther.mBuild)
            return false;
          else
            return true; 
        }
        else if (this.mRelease < aOther.mRelease)
          return false;
        else
          return true; 
      }
      else if (this.mMinor < aOther.mMinor)
        return false;
      else
        return true; 
    }
    else if (this.mMajor < aOther.mMajor)
      return false;
    else
      return true; 

    return false;
  },

  getValidInt: function(aString)
  {
    var integer = parseInt(aString);
    if (isNaN(integer))
      return 0;
    return integer;
  }
}







var nsUpdateNotifierModule =
{
  mClassName:     "Update Notifier",
  mContractID:    "@mozilla.org/update-notifier;1",
  mClassID:       Components.ID("8b6dcf5e-3b5a-4fff-bff5-65a8fa9d71b2"),

  getClassObject: function(aCompMgr, aCID, aIID)
  {
    if (!aCID.equals(this.mClassID))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return this.mFactory;
  },

  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType)
  {
    if (kDebug)
      dump("*** Registering nsUpdateNotifier (a JavaScript Module)\n");

    aCompMgr = aCompMgr.QueryInterface(
                 Components.interfaces.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(this.mClassID, this.mClassName,
      this.mContractID, aFileSpec, aLocation, aType);

    
    
    this.getCategoryManager().addCategoryEntry("profile-startup-category",
      this.mContractID, "", true, true);
  },

  unregisterSelf: function(aCompMgr, aFileSpec, aLocation)
  {
    aCompMgr = aCompMgr.QueryInterface(
                 Components.interfaces.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(this.mClassID, aFileSpec);

    this.getCategoryManager().deleteCategoryEntry("profile-startup-category",
      this.mContractID, true);
  },

  canUnload: function(aCompMgr)
  {
    return true;
  },

  getCategoryManager: function()
  {
    return Components.classes["@mozilla.org/categorymanager;1"].
      getService(Components.interfaces.nsICategoryManager);
  },

  
  
  
  
  
  mFactory:
  {
    createInstance: function(aOuter, aIID)
    {
      if (aOuter != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      if (!aIID.equals(Components.interfaces.nsIObserver) &&
          !aIID.equals(Components.interfaces.nsIProfileStartupListener) &&
          !aIID.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_INVALID_ARG;

      
      return nsUpdateNotifier.QueryInterface(aIID);
    },

    lockFactory: function(aLock)
    {
      
    }
  }
};

function NSGetModule(aCompMgr, aFileSpec)
{
  return nsUpdateNotifierModule;
}






if (!kDebug)
  debug = function(m) {};
else
  debug = function(m) {dump("\t *** nsUpdateNotifier: " + m + "\n");};
