

const Cr = Components.results;
const Cm = Components.manager;


var didFail = false;


const kPromptServiceUUID = "{6cc9c9fe-bc0b-432b-a410-253ef8bcc699}";
const kPromptServiceContractID = "@mozilla.org/embedcomp/prompt-service;1";


const kPromptServiceFactory = Cm.getClassObject(Cc[kPromptServiceContractID],
                                                Ci.nsIFactory);

let fakePromptServiceFactory = {
  createInstance: function(aOuter, aIid) {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return promptService.QueryInterface(aIid);
  }
};

let promptService = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPromptService]),
  alert: function() {
    didFail = true;
  }
};

Cm.QueryInterface(Ci.nsIComponentRegistrar)
  .registerFactory(Components.ID(kPromptServiceUUID), "Prompt Service",
                   kPromptServiceContractID, fakePromptServiceFactory);

const kCompleteState = Ci.nsIWebProgressListener.STATE_STOP +
                       Ci.nsIWebProgressListener.STATE_IS_NETWORK;

const kDummyPage = "http://example.org/browser/browser/base/content/test/dummy_page.html";
const kURIs = [
  "bad://www.mozilla.org/",
  kDummyPage,
  kDummyPage,
];

var gProgressListener = {
  _runCount: 0,
  onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
    if ((aStateFlags & kCompleteState) == kCompleteState) {
      if (++this._runCount != kURIs.length)
        return;
      
      ok(didFail, "Correctly failed on unknown protocol");
      
      ok(gBrowser.mTabs.length == kURIs.length, "Correctly opened all expected tabs");
      finishTest();
    }
  },

  onProgressChange: function () {},
  onLocationChange: function () {},
  onStatusChange: function () {},
  onSecurityChange: function () {}
}

function test() {
  waitForExplicitFinish();
  
  gBrowser.addTabsProgressListener(gProgressListener);
  loadOneOrMoreURIs(kURIs.join("|"));
}

function finishTest() {
  
  Cm.QueryInterface(Ci.nsIComponentRegistrar)
    .unregisterFactory(Components.ID(kPromptServiceUUID),
                       fakePromptServiceFactory);

  
  Cm.QueryInterface(Ci.nsIComponentRegistrar)
    .registerFactory(Components.ID(kPromptServiceUUID), "Prompt Service",
                     kPromptServiceContractID, kPromptServiceFactory);

  
  gBrowser.removeTabsProgressListener(gProgressListener);

  
  for (var i = gBrowser.mTabs.length-1; i > 0; i--)
    gBrowser.removeTab(gBrowser.mTabs[i]);

  finish();
}
