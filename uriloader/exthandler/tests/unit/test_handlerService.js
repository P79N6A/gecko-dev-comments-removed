




































function run_test() {
  
  

  const handlerSvc = Cc["@mozilla.org/uriloader/handler-service;1"].
                     getService(Ci.nsIHandlerService);

  const mimeSvc = Cc["@mozilla.org/mime;1"].
                  getService(Ci.nsIMIMEService);

  const protoSvc = Cc["@mozilla.org/uriloader/external-protocol-service;1"].
                   getService(Ci.nsIExternalProtocolService);
  
  const prefSvc = Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefService);
                  
  const ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);

  const env = Cc["@mozilla.org/process/environment;1"].
              getService(Components.interfaces.nsIEnvironment);

  const rootPrefBranch = prefSvc.getBranch("");
  
  
  

  
  
  
  
  
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

  
  
  


  
  

  
  
  

  var handlerInfo = mimeSvc.getFromTypeAndExtension("nonexistent/type", null);

  
  do_check_true(handlerInfo instanceof Ci.nsIHandlerInfo);

  do_check_eq(handlerInfo.type, "nonexistent/type");

  
  do_check_eq(handlerInfo.MIMEType, "nonexistent/type");

  
  do_check_eq(handlerInfo.preferredAction, Ci.nsIHandlerInfo.saveToDisk);
  do_check_eq(handlerInfo.preferredApplicationHandler, null);
  do_check_eq(handlerInfo.possibleApplicationHandlers.length, 0);
  do_check_true(handlerInfo.alwaysAskBeforeHandling);

  
  
  do_check_eq(handlerInfo.description, "");
  do_check_eq(handlerInfo.hasDefaultHandler, false);
  do_check_eq(handlerInfo.defaultDescription, "");

  
  var haveDefaultHandlersVersion = false;
  try { 
    
    
    
    
    
    rootPrefBranch.getCharPref("gecko.handlerService.defaultHandlersVersion");
    haveDefaultHandlersVersion = true;
  } catch (ex) {}

  const kExternalWarningDefault = 
    "network.protocol-handler.warn-external-default";
  prefSvc.setBoolPref(kExternalWarningDefault, true);

  
  
  
  var protoInfo = protoSvc.getProtocolHandlerInfo("x-moz-rheet");
  do_check_eq(protoInfo.preferredAction, protoInfo.alwaysAsk);
  do_check_true(protoInfo.alwaysAskBeforeHandling);
  
  
  
  const kExternalWarningPrefPrefix = "network.protocol-handler.warn-external.";
  prefSvc.setBoolPref(kExternalWarningPrefPrefix + "http", false);
  protoInfo = protoSvc.getProtocolHandlerInfo("http");
  do_check_eq(0, protoInfo.possibleApplicationHandlers.length);
  do_check_false(protoInfo.alwaysAskBeforeHandling);
  
  
  
  prefSvc.setBoolPref(kExternalWarningPrefPrefix + "http", true);
  protoInfo = protoSvc.getProtocolHandlerInfo("http");
  
  
  
  do_check_eq(0, protoInfo.possibleApplicationHandlers.length);
  do_check_true(protoInfo.alwaysAskBeforeHandling);

  
  prefSvc.setBoolPref(kExternalWarningPrefPrefix + "mailto", false);
  protoInfo = protoSvc.getProtocolHandlerInfo("mailto");
  if (haveDefaultHandlersVersion)
    do_check_eq(2, protoInfo.possibleApplicationHandlers.length);
  else
    do_check_eq(0, protoInfo.possibleApplicationHandlers.length);
  do_check_false(protoInfo.alwaysAskBeforeHandling);

  
  prefSvc.setBoolPref(kExternalWarningPrefPrefix + "mailto", true);
  protoInfo = protoSvc.getProtocolHandlerInfo("mailto");
  if (haveDefaultHandlersVersion) {
    do_check_eq(2, protoInfo.possibleApplicationHandlers.length);
    
    
    
    
    do_check_false(protoInfo.alwaysAskBeforeHandling);
  } else {
    do_check_eq(0, protoInfo.possibleApplicationHandlers.length);
    do_check_true(protoInfo.alwaysAskBeforeHandling);
  }

  if (haveDefaultHandlersVersion) {
    
    
    
    prefSvc.setBoolPref(kExternalWarningPrefPrefix + "mailto", false);
    protoInfo.alwaysAskBeforeHandling = true;
    handlerSvc.store(protoInfo);
    protoInfo = protoSvc.getProtocolHandlerInfo("mailto");
    do_check_eq(2, protoInfo.possibleApplicationHandlers.length);
    do_check_true(protoInfo.alwaysAskBeforeHandling);
  }


  
  

  
  
  
  

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
  if (haveDefaultHandlersVersion) {
    handlerTypes.push("webcal");
    handlerTypes.push("mailto");
    handlerTypes.push("irc");
    handlerTypes.push("ircs");
  }
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

  
  do_check_eq(localPossibleHandler.name, localHandler.name);
  do_check_true(localPossibleHandler.equals(localHandler));
  do_check_eq(webPossibleHandler.name, webHandler.name);
  do_check_true(webPossibleHandler.equals(webHandler));

  
  
  possibleHandlersInfo.possibleApplicationHandlers.removeElementAt(localIndex);
  handlerSvc.store(possibleHandlersInfo);
  possibleHandlersInfo =
    mimeSvc.getFromTypeAndExtension("nonexistent/possible-handlers", null);
  do_check_eq(possibleHandlersInfo.possibleApplicationHandlers.length, 1);

  
  webPossibleHandler = possibleHandlersInfo.possibleApplicationHandlers.
                       queryElementAt(0, Ci.nsIWebHandlerApp);
  do_check_eq(webPossibleHandler.name, webHandler.name);
  do_check_true(webPossibleHandler.equals(webHandler));

  
  
  var localApp = Cc["@mozilla.org/uriloader/local-handler-app;1"].
                 createInstance(Ci.nsILocalHandlerApp);
  var handlerApp = localApp.QueryInterface(Ci.nsIHandlerApp);

  do_check_true(handlerApp.equals(localApp));

  localApp.executable = executable;

  do_check_eq(0, localApp.parameterCount);
  localApp.appendParameter("-test1");
  do_check_eq(1, localApp.parameterCount);
  localApp.appendParameter("-test2");
  do_check_eq(2, localApp.parameterCount);
  do_check_true(localApp.parameterExists("-test1"));
  do_check_true(localApp.parameterExists("-test2"));
  do_check_false(localApp.parameterExists("-false"));
  localApp.clearParameters();
  do_check_eq(0, localApp.parameterCount);

  var localApp2 = Cc["@mozilla.org/uriloader/local-handler-app;1"].
                  createInstance(Ci.nsILocalHandlerApp);
  
  localApp2.executable = executable;

  localApp.clearParameters();
  do_check_true(localApp.equals(localApp2));

  
  
  

  localApp.appendParameter("-test1");
  localApp.appendParameter("-test2");
  localApp.appendParameter("-test3");
  localApp2.appendParameter("-test1");
  localApp2.appendParameter("-test2");
  localApp2.appendParameter("-test3");
  do_check_true(localApp.equals(localApp2));

  
  
  

  localApp.clearParameters();
  localApp2.clearParameters();

  localApp.appendParameter("-test1");
  localApp.appendParameter("-test2");
  localApp.appendParameter("-test3");
  localApp2.appendParameter("-test2");
  localApp2.appendParameter("-test1");
  localApp2.appendParameter("-test3");
  do_check_false(localApp2.equals(localApp));

  var str;
  str = localApp.getParameter(0)
  do_check_eq(str, "-test1");
  str = localApp.getParameter(1)
  do_check_eq(str, "-test2");
  str = localApp.getParameter(2)
  do_check_eq(str, "-test3");

  
  

  
  

  
  var lolType = handlerSvc.getTypeFromExtension("lolcat");
  do_check_eq(lolType, "");


  
  var lolHandler = mimeSvc.getFromTypeAndExtension("application/lolcat", null);

  do_check_false(lolHandler.extensionExists("lolcat"));
  lolHandler.preferredAction = Ci.nsIHandlerInfo.useHelperApp;
  lolHandler.preferredApplicationHandler = localHandler;
  lolHandler.alwaysAskBeforeHandling = false;

  
  do_check_false(handlerSvc.exists(lolHandler));
  handlerSvc.store(lolHandler);
  do_check_true(handlerSvc.exists(lolHandler));

  
  var rdfFile = HandlerServiceTest._dirSvc.get("UMimTyp", Ci.nsIFile);
  var fileHandler = ioService.getProtocolHandler("file").QueryInterface(Ci.nsIFileProtocolHandler);
  var rdfFileURI = fileHandler.getURLSpecFromFile(rdfFile);

  
  
  

  
  var gRDF = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
  var mimeSource    = gRDF.GetUnicodeResource("urn:mimetype:application/lolcat");
  var valueProperty = gRDF.GetUnicodeResource("http://home.netscape.com/NC-rdf#fileExtensions");
  var mimeLiteral   = gRDF.GetLiteral("lolcat");

  var DS = gRDF.GetDataSourceBlocking(rdfFileURI);
  DS.Assert(mimeSource, valueProperty, mimeLiteral, true);


  
  lolType = handlerSvc.getTypeFromExtension("lolcat");
  do_check_eq(lolType, "application/lolcat");

  if (env.get("PERSONAL_MAILCAP")) {
    handlerInfo = mimeSvc.getFromTypeAndExtension("text/plain", null);
    do_check_eq(handlerInfo.preferredAction, Ci.nsIHandlerInfo.useSystemDefault);
    do_check_eq(handlerInfo.defaultDescription, "sed");
  }
}
