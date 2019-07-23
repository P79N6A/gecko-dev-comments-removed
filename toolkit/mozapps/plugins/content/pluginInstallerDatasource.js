




































const RDF_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const PFS_NS = "http://www.mozilla.org/2004/pfs-rdf#";

function nsRDFItemUpdater(aClientOS, aChromeLocale){
  this._rdfService = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                        .getService(Components.interfaces.nsIRDFService);
  this._os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);

  var app = Components.classes["@mozilla.org/xre/app-info;1"]
                      .getService(Components.interfaces.nsIXULAppInfo);
  this.appID = app.ID;
  this.buildID = app.platformBuildID;

  this.clientOS = aClientOS;
  this.chromeLocale = aChromeLocale;

  var prefBranch = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefBranch);
  this.dsURI = prefBranch.getCharPref("pfs.datasource.url");
}

nsRDFItemUpdater.prototype = {
  checkForPlugin: function (aPluginRequestItem){
    var dsURI = this.dsURI;
    
    dsURI = dsURI.replace(/%PLUGIN_MIMETYPE%/g, encodeURIComponent(aPluginRequestItem.mimetype));
    dsURI = dsURI.replace(/%APP_ID%/g, this.appID);
    dsURI = dsURI.replace(/%APP_VERSION%/g, this.buildID);
    dsURI = dsURI.replace(/%CLIENT_OS%/g, this.clientOS);
    dsURI = dsURI.replace(/%CHROME_LOCALE%/g, this.chromeLocale);

    var ds = this._rdfService.GetDataSource(dsURI);
    var rds = ds.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource)
    if (rds.loaded)
      this.onDatasourceLoaded(ds, aPluginRequestItem);
    else {
      var sink = ds.QueryInterface(Components.interfaces.nsIRDFXMLSink);
      sink.addXMLSinkObserver(new nsPluginXMLRDFDSObserver(this, aPluginRequestItem));
    }
  },

  onDatasourceLoaded: function pfs_onDatasourceLoaded (aDatasource, aPluginRequestItem){
    var container = Components.classes["@mozilla.org/rdf/container;1"].
                  createInstance(Components.interfaces.nsIRDFContainer);
    var resultRes = this._rdfService.GetResource("urn:mozilla:plugin-results:" + aPluginRequestItem.mimetype);
    var pluginList = aDatasource.GetTarget(resultRes, this._rdfService.GetResource(PFS_NS+"plugins"), true);

    var pluginInfo = null;
  
    container = Components.classes["@mozilla.org/rdf/container;1"].createInstance(Components.interfaces.nsIRDFContainer);
    try {
      container.Init(aDatasource, pluginList);

      var children = container.GetElements();
      var target;

      
      var child = children.getNext();
      if (child instanceof Components.interfaces.nsIRDFResource){
        var name = this._rdfService.GetResource("http://www.mozilla.org/2004/pfs-rdf#updates");
        target = aDatasource.GetTarget(child, name, true);
      }

      try {
        container.Init(aDatasource, target);
        target = null;
        children = container.GetElements();

        var child = children.getNext();
        if (child instanceof Components.interfaces.nsIRDFResource){
          target = child;
        }

        var rdfs = this._rdfService;

        function getPFSValueFromRDF(aValue){
          var rv = null;

          var myTarget = aDatasource.GetTarget(target, rdfs.GetResource(PFS_NS + aValue), true);
          if (myTarget)
            rv = myTarget.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;

          return rv;
        }

        pluginInfo = {
          name: getPFSValueFromRDF("name"),
          pid: getPFSValueFromRDF("guid"),
          version: getPFSValueFromRDF("version"),
          IconUrl: getPFSValueFromRDF("IconUrl"),
          InstallerLocation: getPFSValueFromRDF("InstallerLocation"),
          InstallerHash: getPFSValueFromRDF("InstallerHash"),
          XPILocation: getPFSValueFromRDF("XPILocation"),
          XPIHash: getPFSValueFromRDF("XPIHash"),
          InstallerShowsUI: getPFSValueFromRDF("InstallerShowsUI"),
          manualInstallationURL: getPFSValueFromRDF("manualInstallationURL"),
          requestedMimetype: getPFSValueFromRDF("requestedMimetype"),
          licenseURL: getPFSValueFromRDF("licenseURL"),
          needsRestart: getPFSValueFromRDF("needsRestart")
        };
      }
      catch (ex){}
    }
    catch (ex){}
    
    gPluginInstaller.pluginInfoReceived(pluginInfo);
  },

  onDatasourceError: function pfs_onDatasourceError (aPluginRequestItem, aError){
    this._os.notifyObservers(aPluginRequestItem, "error", aError);
    gPluginInstaller.pluginInfoReceived(null);
  }
};

function nsPluginXMLRDFDSObserver(aUpdater, aPluginRequestItem){
  this._updater = aUpdater;
  this._item    = aPluginRequestItem;
}

nsPluginXMLRDFDSObserver.prototype = 
{ 
  _updater  : null,
  _item     : null,

  
  onBeginLoad: function(aSink){},
  onInterrupt: function(aSink){},
  onResume: function(aSink){},
  onEndLoad: function(aSink){
    aSink.removeXMLSinkObserver(this);
    
    var ds = aSink.QueryInterface(Components.interfaces.nsIRDFDataSource);
    this._updater.onDatasourceLoaded(ds, this._item);
  },
  
  onError: function(aSink, aStatus, aErrorMsg){  
    aSink.removeXMLSinkObserver(this);   
    this._updater.onDatasourceError(this._item, aStatus.toString());
  }
};
