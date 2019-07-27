


"use strict";





function test() {
  
  requestLongerTimeout(2);
  waitForExplicitFinish();

  let testTabCount = 0;
  let formData = [
  
    { },

  
    
    { id:{ "select_id": {"selectedIndex":0,"value":"val2"} } },
    
    { id:{ "select_id": {"selectedIndex":4,"value":"val8"} } },
    
    { id:{ "select_id": {"selectedIndex":8,"value":"val5"} } },
    
    { id:{ "select_id": {"selectedIndex":0,"value":"val0"} }, xpath: {} },
    
    { id:{},"xpath":{ "/xhtml:html/xhtml:body/xhtml:select[@name='select_name']": {"selectedIndex":1,"value":"val7"} } },
    
    { xpath: { "/xhtml:html/xhtml:body/xhtml:select[@name='select_name']" : {"selectedIndex":3,"value":"val3"} } },
    
    { id:{ "select_id": {"selectedIndex":3,"value":"val4"} } },
  ];

  let expectedValues = [
    null,   
    "val2",
    null,   
    "val5", 
    "val0",
    "val7",
    null,
    "val4",
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

function testTabRestoreData(aFormData, aExpectedValue, aCallback) {
  let testURL =
    getRootDirectory(gTestPath) + "browser_662743_sample.html";
  let tab = gBrowser.addTab(testURL);
  let tabState = { entries: [{ url: testURL, formdata: aFormData}] };

  whenBrowserLoaded(tab.linkedBrowser, function() {
    ss.setTabState(tab, JSON.stringify(tabState));

    whenTabRestored(tab, function() {
      let doc = tab.linkedBrowser.contentDocument;
      let select = doc.getElementById("select_id");
      let value = select.options[select.selectedIndex].value;

      
      SyncHandlers.get(tab.linkedBrowser).flush();
      let restoredTabState = JSON.parse(ss.getTabState(tab));

      
      if (!aExpectedValue) {
        ok(!restoredTabState.hasOwnProperty("formdata"), "no formdata collected");
        gBrowser.removeTab(tab);
        aCallback();
        return;
      }

      
      is(value, aExpectedValue,
        "Select Option by selectedIndex &/or value has been restored correctly");

      let restoredFormData = restoredTabState.formdata;
      let selectIdFormData = restoredFormData.id.select_id;
      value = restoredFormData.id.select_id.value;

      
      ok("id" in restoredFormData || "xpath" in restoredFormData,
        "FormData format is valid");
      
      ok("selectedIndex" in selectIdFormData && "value" in selectIdFormData,
        "select format is valid");
       
      is(value, aExpectedValue,
        "Collection has been saved correctly");

      
      gBrowser.removeTab(tab);

      aCallback();
    });
  });
}
