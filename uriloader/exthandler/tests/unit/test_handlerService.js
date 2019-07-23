



































function run_test() {
  
  
  
  var executable = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
  executable.initWithPath("/usr/bin/test");
  
  var localHandler = {
    name: "Local Handler",
    executable: executable,
    interfaces: [Ci.nsIHandlerApp, Ci.nsILocalHandlerApp, Ci.nsISupports],
    QueryInterface: function(iid) {
      if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
        throw Cr.NS_ERROR_NO_INTERFACE;
      return this;
    }
  };
  
  var webHandler = {
    name: "Web Handler",
    uriTemplate: "http://www.example.com/?%s",
    interfaces: [Ci.nsIHandlerApp, Ci.nsIWebHandlerApp, Ci.nsISupports],
    QueryInterface: function(iid) {
      if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
        throw Cr.NS_ERROR_NO_INTERFACE;
      return this;
    }
  };
  
  var handlerSvc = Cc["@mozilla.org/uriloader/handler-service;1"].
                   getService(Ci.nsIHandlerService);

  var mimeSvc = Cc["@mozilla.org/uriloader/external-helper-app-service;1"].
                getService(Ci.nsIMIMEService);


  
  

  
  
  

  var handlerInfo = mimeSvc.getFromTypeAndExtension("nonexistent/type", null);

  do_check_eq(handlerInfo.MIMEType, "nonexistent/type");

  
  do_check_eq(handlerInfo.preferredAction, Ci.nsIHandlerInfo.saveToDisk);
  do_check_eq(handlerInfo.preferredApplicationHandler, null);
  do_check_true(handlerInfo.alwaysAskBeforeHandling);

  do_check_eq(handlerInfo.description, "");
  do_check_eq(handlerInfo.hasDefaultHandler, false);
  do_check_eq(handlerInfo.defaultDescription, "");


  
  

  
  
  
  

  handlerInfo.preferredAction = Ci.nsIHandlerInfo.useHelperApp;
  handlerInfo.preferredApplicationHandler = localHandler;
  handlerInfo.alwaysAskBeforeHandling = false;

  handlerSvc.store(handlerInfo);

  handlerInfo = mimeSvc.getFromTypeAndExtension("nonexistent/type", null);

  do_check_eq(handlerInfo.preferredAction, Ci.nsIHandlerInfo.useHelperApp);

  do_check_neq(handlerInfo.preferredApplicationHandler, null);
  var preferredHandler = handlerInfo.preferredApplicationHandler;
  do_check_eq(typeof preferredHandler, "object");
  do_check_eq(preferredHandler.name, "Local Handler");
  var localHandler = preferredHandler.QueryInterface(Ci.nsILocalHandlerApp);
  do_check_eq(localHandler.executable.path, "/usr/bin/test");

  do_check_false(handlerInfo.alwaysAskBeforeHandling);

  
  
}
