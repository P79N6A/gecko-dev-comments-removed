



let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

let manifest = { 
  name: "provider test1",
  origin: "https://test1.example.com",
  workerURL: "https://test1.example.com/browser/browser/base/content/test/social/social_worker.js",
  markURL: "https://test1.example.com/browser/browser/base/content/test/social/social_mark.html?url=%{url}",
  markedIcon: "https://test1.example.com/browser/browser/base/content/test/social/unchecked.jpg",
  unmarkedIcon: "https://test1.example.com/browser/browser/base/content/test/social/checked.jpg",

  iconURL: "https://test1.example.com/browser/browser/base/content/test/general/moz.png",
  version: 1
};

function test() {
  waitForExplicitFinish();

  runSocialTestWithProvider(manifest, function (finishcb) {
    runSocialTests(tests, undefined, undefined, function () {
      finishcb();
    });
  });
}

var tests = {
  testMarkMicrodata: function(next) {
    
    let provider = Social._getProviderFromOrigin(manifest.origin);
    let port = provider.getWorkerPort();
    let target, testTab;

    
    
    let expecting = JSON.stringify({
      "url": "https://example.com/browser/browser/base/content/test/social/microdata.html",
      "microdata": {
        "items": [{
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
    });

    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "got-share-data-message":
          is(JSON.stringify(e.data.result), expecting, "microdata data ok");
          gBrowser.removeTab(testTab);
          port.close();
          next();
          break;
      }
    }
    port.postMessage({topic: "test-init"});

    let url = "https://example.com/browser/browser/base/content/test/social/microdata.html"
    addTab(url, function(tab) {
      testTab = tab;
      let doc = tab.linkedBrowser.contentDocument;
      target = doc.getElementById("test-comment");
      SocialMarks.markLink(manifest.origin, url, target);
    });
  }
}
