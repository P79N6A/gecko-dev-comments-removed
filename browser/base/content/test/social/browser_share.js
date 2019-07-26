
let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

let baseURL = "https://example.com/browser/browser/base/content/test/social/";

let manifest = { 
  name: "provider 1",
  origin: "https://example.com",
  workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
  iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png",
  shareURL: "https://example.com/browser/browser/base/content/test/social/share.html"
};

function test() {
  waitForExplicitFinish();

  runSocialTests(tests);
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

function loadURLInTab(url, callback) {
  info("Loading tab with "+url);
  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  tab.linkedBrowser.addEventListener("load", function listener() {
    is(tab.linkedBrowser.currentURI.spec, url, "tab loaded")
    tab.linkedBrowser.removeEventListener("load", listener, true);
    executeSoon(function() { callback(tab) });
  }, true);
}

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
    
    
    is(gBrowser.contentDocument.location.href, "about:blank");
    SocialService.addProvider(manifest, function(provider) {
      is(SocialUI.enabled, true, "SocialUI is enabled");
      checkSocialUI();
      
      let shareButton = SocialShare.shareButton;
      is(shareButton.disabled, true, "share button is disabled");
      
      is(shareButton.getAttribute("disabled"), "true", "share button attribute is disabled");
      
      is(shareButton.hidden, false, "share button is visible");
      SocialService.removeProvider(manifest.origin, next);
    });
  },
  testShareEnabledOnActivation: function(next) {
    
    
    let testData = corpus[0];
    loadURLInTab(testData.url, function(tab) {
      SocialService.addProvider(manifest, function(provider) {
        is(SocialUI.enabled, true, "SocialUI is enabled");
        checkSocialUI();
        
        let shareButton = SocialShare.shareButton;
        is(shareButton.disabled, false, "share button is enabled");
        
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
      loadURLInTab(testData.url, function(tab) {
        testTab = tab;
        SocialShare.sharePage();
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
            SocialService.removeProvider(manifest.origin, next);
          }
          break;
      }
    }
    port.postMessage({topic: "test-init"});
    executeSoon(runOneTest);
  }
}
