






function test() {
  
  waitForExplicitFinish();

  let testTabCount = 0;
  let formData = [
  
    { },
  
    { "#select_id" : 0 },
    { "#select_id" : 2 },
    
    { "#select_id" : 8 },
    { "/xhtml:html/xhtml:body/xhtml:select" : 5},
    { "/xhtml:html/xhtml:body/xhtml:select[@name='select_name']" : 6},

  
    
    { id:{ "select_id": {"selectedIndex":0,"value":"val2"} } },
    
    { id:{ "select_id": {"selectedIndex":4,"value":"val8"} } },
    
    { id:{ "select_id": {"selectedIndex":8,"value":"val5"} } },
    
    { id:{ "select_id": {"selectedIndex":0,"value":"val0"} }, xpath: {} },
    
    { id:{},"xpath":{ "/xhtml:html/xhtml:body/xhtml:select[@name='select_name']": {"selectedIndex":1,"value":"val7"} } },
    
    { xpath: { "/xhtml:html/xhtml:body/xhtml:select[@name='select_name']" : {"selectedIndex":3,"value":"val3"} } },
    
    { id:{ "select_id": {"selectedIndex":3,"value":"val4"} } },

  
    { "#select_id" : 3, id:{ "select_id": {"selectedIndex":1,"value":"val1"} } },
    { "#select_id" : 5, xpath: { "/xhtml:html/xhtml:body/xhtml:select[@name='select_name']" : {"selectedIndex":4,"value":"val4"} } },
    { "/xhtml:html/xhtml:body/xhtml:select" : 5, id:{ "select_id": {"selectedIndex":1,"value":"val1"} }},
    { "/xhtml:html/xhtml:body/xhtml:select[@name='select_name']" : 2, xpath: { "/xhtml:html/xhtml:body/xhtml:select[@name='select_name']" : {"selectedIndex":7,"value":"val7"} } }
  ];

  let expectedValues = [
    [ "val3"], 
    [ "val0"],
    [ "val2"],
    [ "val3"], 
    [ "val5"],
    [ "val6"],
    [ "val2"],
    [ "val3"], 
    [ "val5"], 
    [ "val0"],
    [ "val7"],
    [ "val3"],
    [ "val4"],
    [ "val1"],
    [ "val4"],
    [ "val1"],
    [ "val7"]
  ];
  let callback = function() {
    testTabCount--;
    if (testTabCount == 0) {
      finish();
    }
  };

  for (let i = 0; i < formData.length; i++) {
    testTabCount++;
    testTabRestoreData(formData[i], expectedValues[i], callback);
  }
}

function testTabRestoreData(aFormData, aExpectedValues, aCallback) {
  let testURL =
    getRootDirectory(gTestPath) + "browser_662743_sample.html";
  let tab = gBrowser.addTab(testURL);
  let tabState = { entries: [{ url: testURL, formdata: aFormData}] };

  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    ss.setTabState(tab, JSON.stringify(tabState));

    tab.addEventListener("SSTabRestored", function(aEvent) {
      tab.removeEventListener("SSTabRestored", arguments.callee);
      let doc = tab.linkedBrowser.contentDocument;
      let select = doc.getElementById("select_id");
      let value = select.options[select.selectedIndex].value;

      
      is(value, aExpectedValues[0],
        "Select Option by selectedIndex &/or value has been restored correctly");

      
      gBrowser.removeTab(tab);
      aCallback();
    });

    tab.addEventListener("TabClose", function(aEvent) {
      tab.removeEventListener("TabClose", arguments.callee);
      let restoredTabState = JSON.parse(ss.getTabState(tab));
      let restoredFormData = restoredTabState.entries[0].formdata;
      let selectIdFormData = restoredFormData.id.select_id;
      let value = restoredFormData.id.select_id.value;

      
      ok("id" in restoredFormData && "xpath" in restoredFormData,
        "FormData format is valid");
      
      is(Object.keys(restoredFormData).length, 2,
        "FormData key length is valid");
      
      ok("selectedIndex" in selectIdFormData && "value" in selectIdFormData,
        "select format is valid");
      
      is(Object.keys(selectIdFormData).length, 2,
        "select_id length is valid");
       
      is(value, aExpectedValues[0],
        "Collection has been saved correctly");
    });
  }, true);
}
