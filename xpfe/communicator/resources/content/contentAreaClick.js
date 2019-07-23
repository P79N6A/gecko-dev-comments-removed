















































  var pref = null;
  pref = Components.classes["@mozilla.org/preferences-service;1"]
                   .getService(Components.interfaces.nsIPrefBranch);

  
  function prefillTextBox(target) {
    
    var walletService = Components.classes["@mozilla.org/wallet/wallet-service;1"].getService(Components.interfaces.nsIWalletService);
    var value = walletService.WALLET_PrefillOneElement(window.content, target);
    if (value) {

      
      
      var separator = value[0];
      var valueList = value.substring(1, value.length).split(separator);

      target.value = valueList[0];













































    }
  }
  
  function hrefAndLinkNodeForClickEvent(event)
  {
    var target = event.target;
    var href = "";
    var linkNode = null;

    var isKeyPress = (event.type == "keypress");

    if ( target instanceof HTMLAnchorElement ||
         target instanceof HTMLAreaElement   ||
         target instanceof HTMLLinkElement ) {
      if (target.hasAttribute("href")) 
        linkNode = target;
    }
    else if ( target instanceof HTMLInputElement
            && (event.target.type == "text") 
            && !isKeyPress       
            && event.detail == 2 
            && event.button == 0 
            && event.target.value.length == 0 
            && "@mozilla.org/wallet/wallet-service;1" in Components.classes 
    ) {
      prefillTextBox(target); 
    }
    else {
      linkNode = event.originalTarget;
      while (linkNode && !(linkNode instanceof HTMLAnchorElement))
        linkNode = linkNode.parentNode;
      
      
      if (linkNode && !linkNode.hasAttribute("href"))
        linkNode = null;
    }

    if (linkNode) {
      href = linkNode.href;
    } else {
      
      linkNode = target;
      while (linkNode) {
        if (linkNode.nodeType == Node.ELEMENT_NODE) {
          href = linkNode.getAttributeNS("http://www.w3.org/1999/xlink", "href");
          break;
        }
        linkNode = linkNode.parentNode;
      }
      if (href) {
        href = makeURLAbsolute(linkNode.baseURI, href);
      }
    }

    return href ? {href: href, linkNode: linkNode} : null;
  }

  
  
  
  function contentAreaClick(event) 
  {
    if (!event.isTrusted || event.getPreventDefault()) {
      return true;
    }

    var isKeyPress = (event.type == "keypress");
    var ceParams = hrefAndLinkNodeForClickEvent(event);
    if (ceParams) {
      var href = ceParams.href;
      if (isKeyPress) {
        openNewTabWith(href, true, event.shiftKey);
        event.stopPropagation();
      }
      else {
        handleLinkClick(event, href, ceParams.linkNode);
        
        
        if ("gMessengerBundle" in this && !event.button)
          return !isPhishingURL(ceParams.linkNode, false, href);
      }
      return true;
    }

    if (pref && !isKeyPress && event.button == 1 &&
        pref.getBoolPref("middlemouse.contentLoadURL")) {
      if (middleMousePaste(event)) {
        event.stopPropagation();
      }
    }
    return true;
  }

  function contentAreaMouseDown(event)
  {
    if (event.button == 1 && (event.target != event.currentTarget)
        && !hrefAndLinkNodeForClickEvent(event)
        && !isAutoscrollBlocker(event.originalTarget)) {
      startScrolling(event);
      return false;
    }
    return true;
  }

  function isAutoscrollBlocker(node)
  {
    if (!pref)
      return false;
    
    if (pref.getBoolPref("middlemouse.contentLoadURL"))
      return true;
    
    if (!pref.getBoolPref("middlemouse.paste"))
      return false;
    
    if (node.ownerDocument.designMode == "on")
      return true;
    
    while (node) {
      if (node instanceof HTMLTextAreaElement ||
          (node instanceof HTMLInputElement &&
           (node.type == "text" || node.type == "password")))
        return true;
      
      node = node.parentNode;
    }
    return false;
  }

  function openNewTabOrWindow(event, href, sendReferrer)
  {
    
    if (pref && pref.getBoolPref("browser.tabs.opentabfor.middleclick")) {
      openNewTabWith(href, sendReferrer, event.shiftKey);
      event.stopPropagation();
      return true;
    }

    
    if (pref && pref.getBoolPref("middlemouse.openNewWindow")) {
      openNewWindowWith(href, sendReferrer);
      event.stopPropagation();
      return true;
    }

    
    return false;
  }

  function handleLinkClick(event, href, linkNode)
  {
    
    urlSecurityCheck(href, document);

    switch (event.button) {                                   
      case 0:                                                         
        if (event.metaKey || event.ctrlKey) {                         
          if (openNewTabOrWindow(event, href, true))
            return true;
        } 
        var saveModifier = true;
        if (pref) {
          try {
            saveModifier = pref.getBoolPref("ui.key.saveLink.shift");
          }
          catch(ex) {            
          }
        }
        saveModifier = saveModifier ? event.shiftKey : event.altKey;
          
        if (saveModifier) {                                           
          saveURL(href, linkNode ? gatherTextUnder(linkNode) : "",
                  "SaveLinkTitle", false, getReferrer(document));
          return true;
        }
        if (event.altKey)                                             
          return true;                                                
        return false;
      case 1:                                                         
        if (openNewTabOrWindow(event, href, true))
          return true;
        break;
    }
    return false;
  }

  function middleMousePaste( event )
  {
    var url = readFromClipboard();
    if (!url)
      return false;
    addToUrlbarHistory(url);
    url = getShortcutOrURI(url);

    
    if (event.ctrlKey) {
      
      const nsIURIFixup = Components.interfaces.nsIURIFixup;
      if (!gURIFixup)
        gURIFixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                              .getService(nsIURIFixup);

      url = gURIFixup.createFixupURI(url, nsIURIFixup.FIXUP_FLAGS_MAKE_ALTERNATE_URI).spec;

      return openNewTabOrWindow(event, url, false);
    }

    
    var browser = getBrowser();
    var tab = event.originalTarget;
    if (tab.localName == "tab" &&
        tab.parentNode == browser.mTabContainer) {
      tab.linkedBrowser.userTypedValue = url;
      if (tab == browser.mCurrentTab && url != "about:blank") {
          gURLBar.value = url;
      }
      tab.linkedBrowser.loadURI(url);
      if (event.shiftKey != (pref && pref.getBoolPref("browser.tabs.loadInBackground")))
        browser.selectedTab = tab;
    }
    else if (event.target == browser) {
      tab = browser.addTab(url);
      if (event.shiftKey != (pref && pref.getBoolPref("browser.tabs.loadInBackground")))
        browser.selectedTab = tab;
    }
    else {
      if (url != "about:blank") {
        gURLBar.value = url;
      }
      loadURI(url);
    }

    event.stopPropagation();
    return true;
  }

  function addToUrlbarHistory(aUrlToAdd)
  {
    
    aUrlToAdd = aUrlToAdd.replace(/^\s+/, '').replace(/\s+$/, '');

    if (!aUrlToAdd)
      return;
    if (aUrlToAdd.search(/[\x00-\x1F]/) != -1) 
      return;

    if (!gRDF)
      gRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                       .getService(Components.interfaces.nsIRDFService);
 
    if (!gGlobalHistory)
      gGlobalHistory = Components.classes["@mozilla.org/browser/global-history;2"]
                                 .getService(Components.interfaces.nsIBrowserHistory);

    if (!gURIFixup)
      gURIFixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                            .getService(Components.interfaces.nsIURIFixup);
    if (!gLocalStore)
      gLocalStore = gRDF.GetDataSource("rdf:local-store");

    if (!gRDFC)
      gRDFC = Components.classes["@mozilla.org/rdf/container-utils;1"]
                        .getService(Components.interfaces.nsIRDFContainerUtils);

    var entries = gRDFC.MakeSeq(gLocalStore, gRDF.GetResource("nc:urlbar-history"));
    if (!entries)
      return;
    var elements = entries.GetElements();
    if (!elements)
      return;
    var index = 0;

    var urlToCompare = aUrlToAdd.toUpperCase();
    while(elements.hasMoreElements()) {
      var entry = elements.getNext();
      if (!entry) continue;

      index ++;
      try {
        entry = entry.QueryInterface(Components.interfaces.nsIRDFLiteral);
      } catch(ex) {
        
        continue;
      }

      if (urlToCompare == entry.Value.toUpperCase()) {
        
        
        
        entries.RemoveElementAt(index, true);
        break;
      }
    }   

    

    try {
      var url = getShortcutOrURI(aUrlToAdd);
      var fixedUpURI = gURIFixup.createFixupURI(url, 0);
      if (!fixedUpURI.schemeIs("data"))
        gGlobalHistory.markPageAsTyped(fixedUpURI);
    }
    catch(ex) {
    }

    
    
    var entryToAdd = gRDF.GetLiteral(aUrlToAdd);
    entries.InsertElementAt(entryToAdd, 1, true);

    
    
    for (index = entries.GetCount(); index > MAX_URLBAR_HISTORY_ITEMS; --index) {
        entries.RemoveElementAt(index, true);
    }  
  }

  function makeURLAbsolute(base, url)
  {
    
    var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService);
    var baseURI  = ioService.newURI(base, null, null);

    return ioService.newURI(baseURI.resolve(url), null, null).spec;
  }
