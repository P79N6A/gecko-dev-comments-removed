






































var gDialog;
var gBundleBrand;
var gPKIBundle;
var gSSLStatus;
var gCert;
var gChecking;
var gBroken;
var gNeedReset;

function badCertListener() {}
badCertListener.prototype = {
  getInterface: function (aIID) {
    return this.QueryInterface(aIID);
  },
  QueryInterface: function(aIID) {
    if (aIID.equals(Components.interfaces.nsIBadCertListener2) ||
        aIID.equals(Components.interfaces.nsIInterfaceRequestor) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },  
  handle_test_result: function () {
    if (gSSLStatus)
      gCert = gSSLStatus.QueryInterface(Components.interfaces.nsISSLStatus).serverCert;
  },
  notifyCertProblem: function MSR_notifyCertProblem(socketInfo, sslStatus, targetHost) {
    gBroken = true;
    gSSLStatus = sslStatus;
    this.handle_test_result();
    return true; 
  }
}

function initExceptionDialog() {
  gNeedReset = false;
  gDialog = document.documentElement;
  gBundleBrand = srGetStrBundle("chrome://branding/locale/brand.properties");
  gPKIBundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");

  var brandName = gBundleBrand.GetStringFromName("brandShortName");
  
  setText("warningText", gPKIBundle.formatStringFromName("addExceptionBrandedWarning2",
                                                         [brandName], 1));
  gDialog.getButton("extra1").disabled = true;
  
  var args = window.arguments;
  if (args && args[0]) {
    if (args[0].location) {
      
      document.getElementById("locationTextBox").value = args[0].location;
      document.getElementById('checkCertButton').disabled = false;
      
      
      
      
      
      
      
      
      
      
      
      
      if (args[0].prefetchCert) {
        
        gChecking = true;
        updateCertStatus();
        
        window.setTimeout(checkCert, 2000);
      }
    }
    
    
    args[0].exceptionAdded = false; 
  }
}


function findRecentBadCert(uri) {
  try {
    var recentCertsSvc = Components.classes["@mozilla.org/security/recentbadcerts;1"]
                         .getService(Components.interfaces.nsIRecentBadCertsService);
    if (!recentCertsSvc)
      return false;

    var hostWithPort = uri.host + ":" + uri.port;
    gSSLStatus = recentCertsSvc.getRecentBadCert(hostWithPort);
    if (!gSSLStatus)
      return false;

    gCert = gSSLStatus.QueryInterface(Components.interfaces.nsISSLStatus).serverCert;
    if (!gCert)
      return false;

    gBroken = true;
  }
  catch (e) {
    return false;
  }
  updateCertStatus();  
  return true;
}





function checkCert() {
  
  gCert = null;
  gSSLStatus = null;
  gChecking = true;
  gBroken = false;
  updateCertStatus();

  var uri = getURI();

  
  if (findRecentBadCert(uri) == true)
    return;

  var req = new XMLHttpRequest();
  try {
    if(uri) {
      req.open('GET', uri.prePath, false);
      req.channel.notificationCallbacks = new badCertListener();
      req.send(null);
    }
  } catch (e) {
    
    
    
    Components.utils.reportError("Attempted to connect to a site with a bad certificate in the add exception dialog. " +
                                 "This results in a (mostly harmless) exception being thrown. " +
                                 "Logged for information purposes only: " + e);
  } finally {
    gChecking = false;
  }
      
  if(req.channel && req.channel.securityInfo) {
    const Ci = Components.interfaces;
    gSSLStatus = req.channel.securityInfo
                    .QueryInterface(Ci.nsISSLStatusProvider).SSLStatus;
    gCert = gSSLStatus.QueryInterface(Ci.nsISSLStatus).serverCert;
  }
  updateCertStatus();  
}





function getURI() {
  
  
  
  var fus = Components.classes["@mozilla.org/docshell/urifixup;1"]
                      .getService(Components.interfaces.nsIURIFixup);
  var uri = fus.createFixupURI(document.getElementById("locationTextBox").value, 0);
  
  if(!uri)
    return null;

  if(uri.scheme == "http")
    uri.scheme = "https";

  if (uri.port == -1)
    uri.port = 443;

  return uri;
}

function resetDialog() {
  document.getElementById("viewCertButton").disabled = true;
  document.getElementById("permanent").disabled = true;
  gDialog.getButton("extra1").disabled = true;
  setText("headerDescription", "");
  setText("statusDescription", "");
  setText("statusLongDescription", "");
  setText("status2Description", "");
  setText("status2LongDescription", "");
  setText("status3Description", "");
  setText("status3LongDescription", "");
}




function handleTextChange() {
  var checkCertButton = document.getElementById('checkCertButton');
  checkCertButton.disabled = !(document.getElementById("locationTextBox").value);
  if (gNeedReset) {
    gNeedReset = false;
    resetDialog();
  }
}

function updateCertStatus() {
  var shortDesc, longDesc;
  var shortDesc2, longDesc2;
  var shortDesc3, longDesc3;
  var use2 = false;
  var use3 = false;
  if(gCert) {
    if(gBroken) { 
      var mms = "addExceptionDomainMismatchShort";
      var mml = "addExceptionDomainMismatchLong";
      var exs = "addExceptionExpiredShort";
      var exl = "addExceptionExpiredLong";
      var uts = "addExceptionUnverifiedShort";
      var utl = "addExceptionUnverifiedLong";
      var use1 = false;
      if (gSSLStatus.isDomainMismatch) {
        use1 = true;
        shortDesc = mms;
        longDesc  = mml;
      }
      if (gSSLStatus.isNotValidAtThisTime) {
        if (!use1) {
          use1 = true;
          shortDesc = exs;
          longDesc  = exl;
        }
        else {
          use2 = true;
          shortDesc2 = exs;
          longDesc2  = exl;
        }
      }
      if (gSSLStatus.isUntrusted) {
        if (!use1) {
          use1 = true;
          shortDesc = uts;
          longDesc  = utl;
        }
        else if (!use2) {
          use2 = true;
          shortDesc2 = uts;
          longDesc2  = utl;
        } 
        else {
          use3 = true;
          shortDesc3 = uts;
          longDesc3  = utl;
        }
      }
      
      
      gDialog.getButton("extra1").disabled = false;

      
      
      
      var inPrivateBrowsing = inPrivateBrowsingMode();
      var pe = document.getElementById("permanent");
      pe.disabled = inPrivateBrowsing;
      pe.checked = !inPrivateBrowsing;

      setText("headerDescription", gPKIBundle.GetStringFromName("addExceptionInvalidHeader"));
    }
    else {
      shortDesc = "addExceptionValidShort";
      longDesc  = "addExceptionValidLong";
      gDialog.getButton("extra1").disabled = true;
      document.getElementById("permanent").disabled = true;
    }
    
    document.getElementById("viewCertButton").disabled = false;

    
    Components.classes["@mozilla.org/observer-service;1"]
              .getService(Components.interfaces.nsIObserverService)
              .notifyObservers(null, "cert-exception-ui-ready", null);
  }
  else if (gChecking) {
    shortDesc = "addExceptionCheckingShort";
    longDesc  = "addExceptionCheckingLong";
    document.getElementById("viewCertButton").disabled = true;
    gDialog.getButton("extra1").disabled = true;
    document.getElementById("permanent").disabled = true;
  }
  else {
    shortDesc = "addExceptionNoCertShort";
    longDesc  = "addExceptionNoCertLong";
    document.getElementById("viewCertButton").disabled = true;
    gDialog.getButton("extra1").disabled = true;
    document.getElementById("permanent").disabled = true;
  }
  
  setText("statusDescription", gPKIBundle.GetStringFromName(shortDesc));
  setText("statusLongDescription", gPKIBundle.GetStringFromName(longDesc));

  if (use2) {
    setText("status2Description", gPKIBundle.GetStringFromName(shortDesc2));
    setText("status2LongDescription", gPKIBundle.GetStringFromName(longDesc2));
  }

  if (use3) {
    setText("status3Description", gPKIBundle.GetStringFromName(shortDesc3));
    setText("status3LongDescription", gPKIBundle.GetStringFromName(longDesc3));
  }

  gNeedReset = true;
}




function viewCertButtonClick() {
  if (gCert)
    viewCertHelper(this, gCert);
    
}




function addException() {
  if(!gCert || !gSSLStatus)
    return;

  var overrideService = Components.classes["@mozilla.org/security/certoverride;1"]
                                  .getService(Components.interfaces.nsICertOverrideService);
  var flags = 0;
  if(gSSLStatus.isUntrusted)
    flags |= overrideService.ERROR_UNTRUSTED;
  if(gSSLStatus.isDomainMismatch)
    flags |= overrideService.ERROR_MISMATCH;
  if(gSSLStatus.isNotValidAtThisTime)
    flags |= overrideService.ERROR_TIME;
  
  var permanentCheckbox = document.getElementById("permanent");
  var shouldStorePermanently = permanentCheckbox.checked && !inPrivateBrowsingMode();

  var uri = getURI();
  overrideService.rememberValidityOverride(
    uri.asciiHost, uri.port,
    gCert,
    flags,
    !shouldStorePermanently);
  
  var args = window.arguments;
  if (args && args[0])
    args[0].exceptionAdded = true;
  
  gDialog.acceptDialog();
}





function inPrivateBrowsingMode() {
  
  var args = window.arguments;
  if (args && args[0] && args[0].handlePrivateBrowsing) {
    
    try {
      var pb = Components.classes["@mozilla.org/privatebrowsing;1"].
               getService(Components.interfaces.nsIPrivateBrowsingService);
      return pb.privateBrowsingEnabled;
    } catch (ex) {
      Components.utils.reportError("Could not get the Private Browsing service");
    }
  }

  return false;
}
