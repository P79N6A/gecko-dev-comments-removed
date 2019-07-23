



































function test() {
  
  
  let fieldList = {
    "//input[@name='testinput']": Date.now().toString(),
    "//input[@name='bad name']":  Math.random().toString(),
    "//input[@type='checkbox']":  true,
    "//input[@type='radio'][1]":  false,
    "//input[@type='radio'][2]":  true,
    "//select":                   2,
    "//select[@multiple]":        [1, 3],
    "//textarea[1]":              "",
    "//textarea[3]":              "Some more test\n" + new Date()
  };
  
  function getElementByXPath(aTab, aQuery) {
    let doc = aTab.linkedBrowser.contentDocument;
    let xptype = Ci.nsIDOMXPathResult.FIRST_ORDERED_NODE_TYPE;
    return doc.evaluate(aQuery, doc, null, xptype, null).singleNodeValue;
  }
  
  function setFormValue(aTab, aQuery, aValue) {
    let node = getElementByXPath(aTab, aQuery);
    if (typeof aValue == "string")
      node.value = aValue;
    else if (typeof aValue == "boolean")
      node.checked = aValue;
    else if (typeof aValue == "number")
      node.selectedIndex = aValue;
    else
      Array.forEach(node.options, function(aOpt, aIx)
                                    (aOpt.selected = aValue.indexOf(aIx) > -1));
  }
  
  function compareFormValue(aTab, aQuery, aValue) {
    let node = getElementByXPath(aTab, aQuery);
    if (!node)
      return false;
    if (node instanceof Ci.nsIDOMHTMLInputElement)
      return aValue == (node.type == "checkbox" || node.type == "radio" ?
                        node.checked : node.value);
    if (node instanceof Ci.nsIDOMHTMLTextAreaElement)
      return aValue == node.value;
    if (!node.multiple)
      return aValue == node.selectedIndex;
    return Array.every(node.options, function(aOpt, aIx)
                                       (aValue.indexOf(aIx) > -1) == aOpt.selected);
  }
  
  
  let tabbrowser = getBrowser();
  waitForExplicitFinish();
  
  
  let privacy_level = gPrefService.getIntPref("browser.sessionstore.privacy_level");
  gPrefService.setIntPref("browser.sessionstore.privacy_level", 2);
  
  todo(false, "test doesn't run from the harness's http server");
  let tab = tabbrowser.addTab("https://bugzilla.mozilla.org/attachment.cgi?id=328502");
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    for (let xpath in fieldList)
      setFormValue(tab, xpath, fieldList[xpath]);
    
    let tab2 = tabbrowser.duplicateTab(tab);
    tab2.linkedBrowser.addEventListener("load", function(aEvent) {
      for (let xpath in fieldList)
        ok(compareFormValue(tab2, xpath, fieldList[xpath]),
           "The value for \"" + xpath + "\" was correctly restored");
      
      
      tabbrowser.removeTab(tab2);
      tabbrowser.removeTab(tab);
      
      undoCloseTab();
      
      tab = tabbrowser.selectedTab;
      tab.linkedBrowser.addEventListener("load", function(aEvent) {
        for (let xpath in fieldList)
          if (fieldList[xpath])
            ok(!compareFormValue(tab, xpath, fieldList[xpath]),
               "The value for \"" + xpath + "\" was correctly discarded");
        
        gPrefService.setIntPref("browser.sessionstore.privacy_level", privacy_level);
        tabbrowser.removeTab(tab);
        finish();
      }, true);
    }, true);
  }, true);
}
