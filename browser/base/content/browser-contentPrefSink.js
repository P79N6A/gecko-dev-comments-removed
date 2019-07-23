












































var ContentPrefSink = {
  
  

  
  __cps: null,
  get _cps() {
    if (!this.__cps)
      this.__cps = Cc["@mozilla.org/content-pref/service;1"].
                   getService(Ci.nsIContentPrefService);
    return this.__cps;
  },


  
  
  
  
  interfaces: [Components.interfaces.nsIDOMEventListener,
               Components.interfaces.nsISupports],

  QueryInterface: function ContentPrefSink_QueryInterface(iid) {
    if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },


  
  

  init: function ContentPrefSink_init() {
    gBrowser.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);
    gBrowser.addEventListener("DOMContentLoaded", this, false);
    
  },

  destroy: function ContentPrefSink_destroy() {
    gBrowser.removeEventListener("DOMContentLoaded", this, false);
    gBrowser.removeProgressListener(this);

    
    
    this.__cps = null;

    
    
    this._observers = {};
    this._genericObservers = [];
  },


  
  

  

  onLocationChange: function ContentPrefSink_onLocationChange(progress, request, uri) {
    
    
    try {
      this._handleLocationChanged(uri);
    }
    catch(ex) {
      Components.utils.reportError(ex);
    }
  },
  onStateChange: function ContentPrefSink_onStateChange(progress, request, flags, status) {},
  onProgressChange: function ContentPrefSink_onProgressChange(progress, request, curSelfProgress,
                                                              maxSelfProgress, curTotalProgress,
                                                              maxTotalProgress) {},
  onStatusChange: function ContentPrefSink_onStatusChange(progress, request, status, message) {},
  onSecurityChange: function ContentPrefSink_onSecurityChange(progress, request, state) {},

  

  handleEvent: function ContentPrefSink_handleEvent(event) {
    
    this._handleDOMContentLoaded(event);
  },

  _handleLocationChanged: function ContentPrefSink__handleLocationChanged(uri) {
    if (!uri)
      return;

    var prefs = this._cps.getPrefs(uri);

    for (var name in this._observers) {
      var value = prefs.hasKey(name) ? prefs.get(name) : undefined;
      for each (var observer in this._observers[name])
        if (observer.onLocationChanged) {
          try {
            observer.onLocationChanged(uri, name, value);
          }
          catch(ex) {
            Components.utils.reportError(ex);
          }
        }
    }

    for each (var observer in this._genericObservers)
      if (observer.onLocationChanged) {
        try {
          observer.onLocationChanged(uri, prefs);
        }
        catch(ex) {
          Components.utils.reportError(ex);
        }
      }
  },

  _handleDOMContentLoaded: function ContentPrefSink__handleDOMContentLoaded(event) {
    var browser = gBrowser.getBrowserForDocument(event.target);
    
    
    
    if (!browser)
      return;
    var uri = browser.currentURI;

    var prefs = this._cps.getPrefs(uri);

    for (var name in this._observers) {
      var value = prefs.hasKey(name) ? prefs.get(name) : undefined;
      for each (var observer in this._observers[name]) {
        if (observer.onDOMContentLoaded) {
          try {
            observer.onDOMContentLoaded(event, name, value);
          }
          catch(ex) {
            Components.utils.reportError(ex);
          }
        }
      }
    }

    for each (var observer in this._genericObservers) {
      if (observer.onDOMContentLoaded) {
        try {
          observer.onDOMContentLoaded(event, prefs);
        }
        catch(ex) {
          Components.utils.reportError(ex);
        }
      }
    }
  },


  
  

  _observers: {},
  _genericObservers: [],

  
  
  

  














  addObserver: function ContentPrefSink_addObserver(name, observer) {
    var observers;
    if (name) {
      if (!this._observers[name])
        this._observers[name] = [];
      observers = this._observers[name];
    }
    else
      observers = this._genericObservers;

    if (observers.indexOf(observer) == -1)
      observers.push(observer);

    return name ? this._cps.getPref(null, name) : null;
  },

  






  removeObserver: function ContentPrefSink_removeObserver(name, observer) {
    var observers = name ? this._observers[name] : this._genericObservers;
    if (observers.indexOf(observer) != -1)
      observers.splice(observers.indexOf(observer), 1);
  },

  _getObservers: function ContentPrefSink__getObservers(name) {
    var observers = [];

    
    
    
    
    
    if (name && this._observers[name])
      observers = observers.concat(this._observers[name]);
    observers = observers.concat(this._genericObservers);

    return observers;
  }
};
