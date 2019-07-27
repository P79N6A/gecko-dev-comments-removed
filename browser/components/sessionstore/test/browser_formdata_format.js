


"use strict";

function test() {
  
  waitForExplicitFinish();

  let formData = [
    { },
    
    { "#input1" : "value0" },
    { "#input1" : "value1", "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value2" },
    { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value3" },
    
    { id: { "input1" : "value4" } },
    { id: { "input1" : "value5" }, xpath: {} },
    { id: { "input1" : "value6" }, xpath: { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value7" } },
    { id: {}, xpath: { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value8" } },
    { xpath: { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value9" } },
    
    { "#input1" : "value10", id: { "input1" : "value11" } },
    { "#input1" : "value12", id: { "input1" : "value13" }, xpath: { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value14" } },
    { "#input1" : "value15", xpath: { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value16" } },
    { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value17", id: { "input1" : "value18" } },
    { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value19", id: { "input1" : "value20" }, xpath: { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value21" } },
    { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value22", xpath: { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value23" } },
    { "#input1" : "value24", "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value25", id: { "input1" : "value26" } },
    { "#input1" : "value27", "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value28", id: { "input1" : "value29" }, xpath: { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value30" } },
    { "#input1" : "value31", "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value32", xpath: { "/xhtml:html/xhtml:body/xhtml:input[@name='input2']" : "value33" } }
  ]
  let expectedValues = [
    [ "" , "" ],
    
    [ "value0", "" ],
    [ "value1", "value2" ],
    [ "", "value3" ],
    
    [ "value4", "" ],
    [ "value5", "" ],
    [ "value6", "value7" ],
    [ "", "value8" ],
    [ "", "value9" ],
    
    [ "value11", "" ],
    [ "value13", "value14" ],
    [ "", "value16" ],
    [ "value18", "" ],
    [ "value20", "value21" ],
    [ "", "value23" ],
    [ "value26", "" ],
    [ "value29", "value30" ],
    [ "", "value33" ]
  ];
  let testTabCount = 0;
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
  let URL = ROOT + "browser_formdata_format_sample.html";
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  let tabState = { entries: [{ url: URL, formdata: aFormData}] };

  Task.spawn(function () {
    yield promiseBrowserLoaded(tab.linkedBrowser);

    ss.setTabState(tab, JSON.stringify(tabState));
    yield promiseTabRestored(tab);

    TabState.flush(tab.linkedBrowser);
    let restoredTabState = JSON.parse(ss.getTabState(tab));
    let restoredFormData = restoredTabState.formdata;

    if (restoredFormData) {
      let doc = tab.linkedBrowser.contentDocument;
      let input1 = doc.getElementById("input1");
      let input2 = doc.querySelector("input[name=input2]");

      
      ok("id" in restoredFormData || "xpath" in restoredFormData,
        "FormData format is valid: " + restoredFormData);
      
      for (let key of Object.keys(restoredFormData)) {
        if (["id", "xpath", "url"].indexOf(key) === -1) {
          ok(false, "FormData format is invalid.");
        }
      }
      
      is(input1.value, aExpectedValue[0],
        "FormData by 'id' has been restored correctly");
      
      is(input2.value, aExpectedValue[1],
        "FormData by 'xpath' has been restored correctly");
    }

    
    gBrowser.removeTab(tab);

  
  }).then(aCallback);
}
