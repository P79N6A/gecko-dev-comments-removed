




































function test() {
  

  function test(aLambda) {
    try {
      return aLambda() || true;
    } catch(ex) { }
    return false;
  }

  let fieldList = {
    "//input[@name='input']":     Date.now().toString(),
    "//input[@name='spaced 1']":  Math.random().toString(),
    "//input[3]":                 "three",
    "//input[@type='checkbox']":  true,
    "//input[@name='uncheck']":   false,
    "//input[@type='radio'][1]":  false,
    "//input[@type='radio'][2]":  true,
    "//input[@type='radio'][3]":  false,
    "//select":                   2,
    "//select[@multiple]":        [1, 3],
    "//textarea[1]":              "",
    "//textarea[2]":              "Some text... " + Math.random(),
    "//textarea[3]":              "Some more text\n" + new Date(),
    "//input[@type='file']":      "/dev/null"
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

  
  waitForExplicitFinish();

  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);

  
  let ss = test(function() Cc["@mozilla.org/browser/sessionstore;1"].
                           getService(Ci.nsISessionStore));

  
  
  

  const testURL = "chrome://mochikit/content/browser/" +
  "browser/components/sessionstore/test/browser/browser_248970_b_sample.html";
  const testURL2 = "http://localhost:8888/browser/" +
  "browser/components/sessionstore/test/browser/browser_248970_b_sample.html";

  
  let count = ss.getClosedTabCount(window);
  let max_tabs_undo = gPrefService.getIntPref("browser.sessionstore.max_tabs_undo");
  ok(0 <= count && count <= max_tabs_undo,
    "getClosedTabCount should return zero or at most max_tabs_undo");

  
  let key = "key";
  let value = "Value " + Math.random();
  let state = { entries: [{ url: testURL }], extData: { key: value } };

  
  let tab_A = gBrowser.addTab(testURL);
  ss.setTabState(tab_A, state.toSource());
  tab_A.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);

    
    gPrefService.setIntPref("browser.sessionstore.max_tabs_undo", max_tabs_undo + 1)

    
    for (let i in fieldList)
      setFormValue(tab_A, i, fieldList[i]);

    
    gBrowser.removeTab(tab_A);

    
    ok(ss.getClosedTabCount(window) > count, "getClosedTabCount has increased after closing a tab");

    
    let tab_A_restored = test(function() ss.undoCloseTab(window, 0));
    ok(tab_A_restored, "a tab is in undo list");
    tab_A_restored.linkedBrowser.addEventListener("load", function(aEvent) {
      this.removeEventListener("load", arguments.callee, true);

      is(testURL, this.currentURI.spec, "it's the same tab that we expect");
      gBrowser.removeTab(tab_A_restored);

      
      pb.privateBrowsingEnabled = true;
      ok(pb.privateBrowsingEnabled, "private browsing enabled");

      
      let key1 = "key1";
      let value1 = "Value " + Math.random();
      let state1 = { entries: [{ url: testURL2 }], extData: { key1: value1 } };

      
      let tab_B = gBrowser.addTab(testURL2);
      ss.setTabState(tab_B, state1.toSource());
      tab_B.linkedBrowser.addEventListener("load", function(aEvent) {
        this.removeEventListener("load", arguments.callee, true);

        
        for (let item in fieldList)
          setFormValue(tab_B, item, fieldList[item]);

        
        let tab_C = gBrowser.duplicateTab(tab_B);
        tab_C.linkedBrowser.addEventListener("load", function(aEvent) {
          this.removeEventListener("load", arguments.callee, true);

          
          is(ss.getTabValue(tab_C, key1), value1,
            "tab successfully duplicated - correct state");

          for (let item in fieldList)
            ok(compareFormValue(tab_C, item, fieldList[item]),
              "The value for \"" + item + "\" was correctly duplicated");

          
          gBrowser.removeTab(tab_C);
          gBrowser.removeTab(tab_B);

          
          pb.privateBrowsingEnabled = false;
          ok(!pb.privateBrowsingEnabled, "private browsing disabled");

          
          gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
          finish();
        }, true);
      }, true);
    }, true);
  }, true);
}
