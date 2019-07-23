





































gDirSvc.unregisterProvider(dirProvider);

if (gXHR) {
  gXHRCallback     = null;

  gXHR.responseXML = null;
  
  gXHR.onerror     = null;
  gXHR.onload      = null;
  gXHR.onprogress  = null;

  gXHR             = null;
}

gUpdateManager = null;
gUpdateChecker = null;
gAUS           = null;
gPrefs         = null;
gTestserver    = null;
