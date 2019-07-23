



































function run_test() {
  
  

  const handlerSvc = Cc["@mozilla.org/uriloader/handler-service;1"].
                     getService(Ci.nsIHandlerService);

  const mimeSvc = Cc["@mozilla.org/uriloader/external-helper-app-service;1"].
                  getService(Ci.nsIMIMEService);


  
  

  
  
  
  
  
  var executable = HandlerServiceTest._dirSvc.get("TmpD", Ci.nsIFile);
  
  
  
  

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
  
  var webHandler = Cc["@mozilla.org/uriloader/web-handler-app;1"].
                   createInstance(Ci.nsIWebHandlerApp);
  webHandler.name = "Web Handler";
  webHandler.uriTemplate = "http://www.example.com/?%s";


  
  

  function checkLocalHandlersAreEquivalent(handler1, handler2) {
    do_check_eq(handler1.name, handler2.name);
    do_check_eq(handler1.executable.path, handler2.executable.path);
  }

  function checkWebHandlersAreEquivalent(handler1, handler2) {
    do_check_eq(handler1.name, handler2.name);
    do_check_eq(handler1.uriTemplate, handler2.uriTemplate);
  }

  
  
  


  
  

  
  
  

  var handlerInfo = mimeSvc.getFromTypeAndExtension("nonexistent/type", null);

  
  do_check_true(handlerInfo instanceof Ci.nsIHandlerInfo);

  do_check_eq(handlerInfo.type, "nonexistent/type");

  
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
  do_check_true(preferredHandler instanceof Ci.nsILocalHandlerApp);
  preferredHandler.QueryInterface(Ci.nsILocalHandlerApp);
  do_check_eq(preferredHandler.executable.path, localHandler.executable.path);

  do_check_false(handlerInfo.alwaysAskBeforeHandling);

  
  
  
  var handlerInfo2 = mimeSvc.getFromTypeAndExtension("nonexistent/type2", null);
  handlerSvc.store(handlerInfo2);
  var handlerTypes = ["nonexistent/type", "nonexistent/type2"];
  var handlers = handlerSvc.enumerate();
  while (handlers.hasMoreElements()) {
    var handler = handlers.getNext().QueryInterface(Ci.nsIHandlerInfo);
    do_check_neq(handlerTypes.indexOf(handler.type), -1);
    handlerTypes.splice(handlerTypes.indexOf(handler.type), 1);
  }
  do_check_eq(handlerTypes.length, 0);

  
  handlerSvc.remove(handlerInfo2);
  handlers = handlerSvc.enumerate();
  while (handlers.hasMoreElements())
    do_check_neq(handlers.getNext().QueryInterface(Ci.nsIHandlerInfo).type,
                 handlerInfo2.type);

  
  
  var noPreferredHandlerInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/no-preferred-handler", null);
  handlerSvc.store(noPreferredHandlerInfo);
  noPreferredHandlerInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/no-preferred-handler", null);
  do_check_eq(noPreferredHandlerInfo.preferredApplicationHandler, null);

  
  
  var removePreferredHandlerInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/rem-preferred-handler", null);
  removePreferredHandlerInfo.preferredApplicationHandler = localHandler;
  handlerSvc.store(removePreferredHandlerInfo);
  removePreferredHandlerInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/rem-preferred-handler", null);
  removePreferredHandlerInfo.preferredApplicationHandler = null;
  handlerSvc.store(removePreferredHandlerInfo);
  removePreferredHandlerInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/rem-preferred-handler", null);
  do_check_eq(removePreferredHandlerInfo.preferredApplicationHandler, null);

  
  

  
  var possibleHandlersInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/possible-handlers", null);
  do_check_eq(possibleHandlersInfo.possibleApplicationHandlers.length, 0);

  
  
  handlerSvc.store(possibleHandlersInfo);
  possibleHandlersInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/possible-handlers", null);
  do_check_eq(possibleHandlersInfo.possibleApplicationHandlers.length, 0);

  
  
  possibleHandlersInfo.possibleApplicationHandlers.appendElement(localHandler,
                                                                 false);
  possibleHandlersInfo.possibleApplicationHandlers.appendElement(webHandler,
                                                                 false);
  handlerSvc.store(possibleHandlersInfo);
  possibleHandlersInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/possible-handlers", null);
  do_check_eq(possibleHandlersInfo.possibleApplicationHandlers.length, 2);

  
  
  
  var handler1 = possibleHandlersInfo.possibleApplicationHandlers.
                 queryElementAt(0, Ci.nsIHandlerApp);
  var handler2 = possibleHandlersInfo.possibleApplicationHandlers.
                 queryElementAt(1, Ci.nsIHandlerApp);
  var localPossibleHandler, webPossibleHandler, localIndex;
  if (handler1 instanceof Ci.nsILocalHandlerApp)
    [localPossibleHandler, webPossibleHandler, localIndex] = [handler1,
                                                              handler2,
                                                              0];
  else
    [localPossibleHandler, webPossibleHandler, localIndex] = [handler2,
                                                              handler1,
                                                              1];
  localPossibleHandler.QueryInterface(Ci.nsILocalHandlerApp);
  webPossibleHandler.QueryInterface(Ci.nsIWebHandlerApp);

  
  checkLocalHandlersAreEquivalent(localPossibleHandler, localHandler);
  checkWebHandlersAreEquivalent(webPossibleHandler, webHandler);

  
  
  possibleHandlersInfo.possibleApplicationHandlers.removeElementAt(localIndex);
  handlerSvc.store(possibleHandlersInfo);
  possibleHandlersInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/possible-handlers", null);
  do_check_eq(possibleHandlersInfo.possibleApplicationHandlers.length, 1);

  
  checkWebHandlersAreEquivalent(possibleHandlersInfo.
                                  possibleApplicationHandlers.
                                  queryElementAt(0, Ci.nsIHandlerApp).
                                  QueryInterface(Ci.nsIWebHandlerApp),
                                webHandler);

  
  
}
