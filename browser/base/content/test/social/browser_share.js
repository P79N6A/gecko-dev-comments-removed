
let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

let baseURL = "https://example.com/browser/browser/base/content/test/social/";

let manifest = { 
  name: "provider 1",
  origin: "https://example.com",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png",
  shareURL: "https://example.com/browser/browser/base/content/test/social/share.html"
};
let activationPage = "https://example.com/browser/browser/base/content/test/social/share_activate.html";

function sendActivationEvent(subframe) {
  
  Social.lastEventReceived = 0;
  let doc = subframe.contentDocument;
  
  let button = doc.getElementById("activation");
  ok(!!button, "got the activation button");
  EventUtils.synthesizeMouseAtCenter(button, {}, doc.defaultView);
}

function promiseShareFrameEvent(iframe, eventName) {
  let deferred = Promise.defer();
  iframe.addEventListener(eventName, function load() {
    info("page load is " + iframe.contentDocument.location.href);
    if (iframe.contentDocument.location.href != "data:text/plain;charset=utf8,") {
      iframe.removeEventListener(eventName, load, true);
      deferred.resolve();
    }
  }, true);
  return deferred.promise;
}

function test() {
  waitForExplicitFinish();
  Services.prefs.setCharPref("social.shareDirectory", activationPage);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("social.directories");
    Services.prefs.clearUserPref("social.shareDirectory");
    Services.prefs.clearUserPref("social.share.activationPanelEnabled");
  });
  runSocialTests(tests, undefined, function(next) {
    let shareButton = SocialShare.shareButton;
    if (shareButton) {
      CustomizableUI.removeWidgetFromArea("social-share-button", CustomizableUI.AREA_NAVBAR)
      shareButton.remove();
    }
    ok(CustomizableUI.inDefaultState, "Should start in default state.");
    next();
  });
}

let corpus = [
  {
    url: baseURL+"opengraph/opengraph.html",
    options: {
      
      title: ">This is my title<",
      
      description: "A test corpus file for open graph tags we care about",
      
      
      
      url: "https://www.mozilla.org/",
      
      
      previews:["https://www.mozilla.org/favicon.png"],
      
      siteName: ">My simple test page<"
    }
  },
  {
    
    url: baseURL+"opengraph/og_invalid_url.html",
    options: {
      description: "A test corpus file for open graph tags passing a bad url",
      url: baseURL+"opengraph/og_invalid_url.html",
      previews: [],
      siteName: "Evil chrome delivering website"
    }
  },
  {
    url: baseURL+"opengraph/shorturl_link.html",
    options: {
      previews: ["http://example.com/1234/56789.jpg"],
      url: "http://www.example.com/photos/56789/",
      shortUrl: "http://imshort/p/abcde"
    }
  },
  {
    url: baseURL+"opengraph/shorturl_linkrel.html",
    options: {
      previews: ["http://example.com/1234/56789.jpg"],
      url: "http://www.example.com/photos/56789/",
      shortUrl: "http://imshort/p/abcde"
    }
  },
  {
    url: baseURL+"opengraph/shortlink_linkrel.html",
    options: {
      previews: ["http://example.com/1234/56789.jpg"],
      url: "http://www.example.com/photos/56789/",
      shortUrl: "http://imshort/p/abcde"
    }
  }
];

function hasoptions(testOptions, options) {
  let msg;
  for (let option in testOptions) {
    let data = testOptions[option];
    info("data: "+JSON.stringify(data));
    let message_data = options[option];
    info("message_data: "+JSON.stringify(message_data));
    if (Array.isArray(data)) {
      
      
      
      ok(Array.every(data, function(item) { return message_data.indexOf(item) >= 0 }), "option "+option);
    } else {
      is(message_data, data, "option "+option);
    }
  }
}

var tests = {
  testShareDisabledOnActivation: function(next) {
    
    
    is(gBrowser.currentURI.spec, "about:blank");

    
    CustomizableUI.addWidgetToArea("social-share-button", CustomizableUI.AREA_NAVBAR);
    
    SocialUI.onCustomizeEnd(window);

    SocialService.addProvider(manifest, function(provider) {
      is(SocialUI.enabled, true, "SocialUI is enabled");
      checkSocialUI();
      
      let shareButton = SocialShare.shareButton;
      
      is(shareButton.getAttribute("disabled"), "true", "share button attribute is disabled");
      
      is(shareButton.hidden, false, "share button is visible");
      SocialService.disableProvider(manifest.origin, next);
    });
  },
  testShareEnabledOnActivation: function(next) {
    
    
    
    CustomizableUI.addWidgetToArea("social-share-button", CustomizableUI.AREA_NAVBAR);
    
    SocialUI.onCustomizeEnd(window);

    let testData = corpus[0];
    addTab(testData.url, function(tab) {
      SocialService.addProvider(manifest, function(provider) {
        is(SocialUI.enabled, true, "SocialUI is enabled");
        checkSocialUI();
        
        let shareButton = SocialShare.shareButton;
        
        ok(!shareButton.hasAttribute("disabled"), "share button is enabled");
        
        is(shareButton.hidden, false, "share button is visible");
        gBrowser.removeTab(tab);
        next();
      });
    });
  },
  testSharePage: function(next) {
    let provider = Social._getProviderFromOrigin(manifest.origin);
    let port = provider.getWorkerPort();
    ok(port, "provider has a port");
    let testTab;
    let testIndex = 0;
    let testData = corpus[testIndex++];

    function runOneTest() {
      addTab(testData.url, function(tab) {
        testTab = tab;
        SocialShare.sharePage(manifest.origin);
      });
    }

    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-share-data-message":
          gBrowser.removeTab(testTab);
          hasoptions(testData.options, e.data.result);
          testData = corpus[testIndex++];
          if (testData) {
            executeSoon(runOneTest);
          } else {
            SocialService.disableProvider(manifest.origin, next);
          }
          break;
      }
    }
    port.postMessage({topic: "test-init"});
    executeSoon(runOneTest);
  },
  testShareMicrodata: function(next) {
    SocialService.addProvider(manifest, function(provider) {
      let port = provider.getWorkerPort();
      let target, testTab;

      let expecting = JSON.stringify({
        "url": "https://example.com/browser/browser/base/content/test/social/microdata.html",
        "title": "My Blog",
        "previews": [],
        "microdata": {
          "items": [{
              "types": ["http://schema.org/BlogPosting"],
              "properties": {
                "headline": ["Progress report"],
                "datePublished": ["2013-08-29"],
                "url": ["https://example.com/browser/browser/base/content/test/social/microdata.html?comments=0"],
                "comment": [{
                    "types": ["http://schema.org/UserComments"],
                    "properties": {
                      "url": ["https://example.com/browser/browser/base/content/test/social/microdata.html#c1"],
                      "creator": [{
                          "types": ["http://schema.org/Person"],
                          "properties": {
                            "name": ["Greg"]
                          }
                        }
                      ],
                      "commentTime": ["2013-08-29"]
                    }
                  }, {
                    "types": ["http://schema.org/UserComments"],
                    "properties": {
                      "url": ["https://example.com/browser/browser/base/content/test/social/microdata.html#c2"],
                      "creator": [{
                          "types": ["http://schema.org/Person"],
                          "properties": {
                            "name": ["Charlotte"]
                          }
                        }
                      ],
                      "commentTime": ["2013-08-29"]
                    }
                  }
                ]
              }
            }
          ]
        }
      });

      port.onmessage = function (e) {
        let topic = e.data.topic;
        switch (topic) {
          case "got-share-data-message":
            is(JSON.stringify(e.data.result), expecting, "microdata data ok");
            gBrowser.removeTab(testTab);
            SocialService.disableProvider(manifest.origin, next);
            break;
        }
      }
      port.postMessage({topic: "test-init"});
  
      let url = "https://example.com/browser/browser/base/content/test/social/microdata.html"
      addTab(url, function(tab) {
        testTab = tab;
        let doc = tab.linkedBrowser.contentDocument;
        target = doc.getElementById("simple-hcard");
        SocialShare.sharePage(manifest.origin, null, target);
      });
    });
  },
  testSharePanelActivation: function(next) {
    let testTab;
    
    Services.prefs.setCharPref("social.directories", "https://example.com");
    Services.prefs.setBoolPref("social.share.activationPanelEnabled", true);
    
    SocialShare._createFrame();
    let iframe = SocialShare.iframe;

    promiseShareFrameEvent(iframe, "load").then(() => {
      let subframe = iframe.contentDocument.getElementById("activation-frame");
      waitForCondition(() => {
          
          
          return SocialShare.panel.state == "open"
                 && subframe.contentDocument
                 && subframe.contentDocument.readyState == "complete";
        }, () => {
        is(subframe.contentDocument.location.href, activationPage, "activation page loaded");
        promiseObserverNotified("social:provider-enabled").then(() => {
          let provider = Social._getProviderFromOrigin(manifest.origin);
          let port = provider.getWorkerPort();
          ok(!!port, "got port");
          port.onmessage = function (e) {
            let topic = e.data.topic;
            switch (topic) {
              case "got-share-data-message":
                ok(true, "share completed");
                gBrowser.removeTab(testTab);
                SocialService.uninstallProvider(manifest.origin, next);
                break;
            }
          }
          port.postMessage({topic: "test-init"});
        });
        sendActivationEvent(subframe);
      }, "share panel did not open and load share page");
    });
    addTab(activationPage, function(tab) {
      testTab = tab;
      SocialShare.sharePage();
    });
  }
}
