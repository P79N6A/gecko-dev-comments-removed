








thisTestLeaksUncaughtRejectionsAndShouldBeFixed("TypeError: this.docShell is null");

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsOAuthClient",
  "resource://gre/modules/FxAccountsOAuthClient.jsm");

const HTTP_PATH = "http://example.com";
const HTTP_ENDPOINT = "/browser/browser/base/content/test/general/browser_fxa_oauth.html";
const HTTP_ENDPOINT_WITH_KEYS = "/browser/browser/base/content/test/general/browser_fxa_oauth_with_keys.html";

let gTests = [
  {
    desc: "FxA OAuth - should open a new tab, complete OAuth flow",
    run: function () {
      return new Promise(function(resolve, reject) {
        let tabOpened = false;
        let properUrl = "http://example.com/browser/browser/base/content/test/general/browser_fxa_oauth.html";
        let queryStrings = [
          "action=signin",
          "client_id=client_id",
          "scope=",
          "state=state",
          "webChannelId=oauth_client_id"
        ];
        queryStrings.sort();

        waitForTab(function (tab) {
          Assert.ok("Tab successfully opened");

          
          let a1 = gBrowser.currentURI.spec.split('?');
          let a2 = a1[1].split('&');
          a2.sort();
          let match = a1.length == 2 &&
                      a1[0] == properUrl &&
                      a2.length == queryStrings.length;
          for (let i = 0; i < queryStrings.length; i++) {
            if (a2[i] !== queryStrings[i]) {
              match = false;
            }
          }
          Assert.ok(match);

          tabOpened = true;
        });

        let client = new FxAccountsOAuthClient({
          parameters: {
            state: "state",
            client_id: "client_id",
            oauth_uri: HTTP_PATH,
            content_uri: HTTP_PATH,
          },
          authorizationEndpoint: HTTP_ENDPOINT
        });

        client.onComplete = function(tokenData) {
          Assert.ok(tabOpened);
          Assert.equal(tokenData.code, "code1");
          Assert.equal(tokenData.state, "state");
          resolve();
        };

        client.onError = reject;

        client.launchWebFlow();
      });
    }
  },
  {
    desc: "FxA OAuth - should receive an error when there's a state mismatch",
    run: function () {
      return new Promise(function(resolve, reject) {
        let tabOpened = false;

        waitForTab(function (tab) {
          Assert.ok("Tab successfully opened");

          
          let queryString = gBrowser.currentURI.spec.split('?')[1];
          Assert.ok(queryString.indexOf('state=different-state') >= 0);

          tabOpened = true;
        });

        let client = new FxAccountsOAuthClient({
          parameters: {
            state: "different-state",
            client_id: "client_id",
            oauth_uri: HTTP_PATH,
            content_uri: HTTP_PATH,
          },
          authorizationEndpoint: HTTP_ENDPOINT
        });

        client.onComplete = reject;

        client.onError = function(err) {
          Assert.ok(tabOpened);
          Assert.equal(err.message, "OAuth flow failed. State doesn't match");
          resolve();
        };

        client.launchWebFlow();
      });
    }
  },
  {
    desc: "FxA OAuth - should be able to request keys during OAuth flow",
    run: function () {
      return new Promise(function(resolve, reject) {
        let tabOpened = false;

        waitForTab(function (tab) {
          Assert.ok("Tab successfully opened");

          
          let queryString = gBrowser.currentURI.spec.split('?')[1];
          Assert.ok(queryString.indexOf('keys=true') >= 0);

          tabOpened = true;
        });

        let client = new FxAccountsOAuthClient({
          parameters: {
            state: "state",
            client_id: "client_id",
            oauth_uri: HTTP_PATH,
            content_uri: HTTP_PATH,
            keys: true,
          },
          authorizationEndpoint: HTTP_ENDPOINT_WITH_KEYS
        });

        client.onComplete = function(tokenData, keys) {
          Assert.ok(tabOpened);
          Assert.equal(tokenData.code, "code1");
          Assert.equal(tokenData.state, "state");
          Assert.equal(keys.kAr, "kAr");
          Assert.equal(keys.kBr, "kBr");
          resolve();
        };

        client.onError = reject;

        client.launchWebFlow();
      });
    }
  },
  {
    desc: "FxA OAuth - should not receive keys if not explicitly requested",
    run: function () {
      return new Promise(function(resolve, reject) {
        let tabOpened = false;

        waitForTab(function (tab) {
          Assert.ok("Tab successfully opened");

          
          let queryString = gBrowser.currentURI.spec.split('?')[1];
          Assert.ok(queryString.indexOf('keys=true') == -1);

          tabOpened = true;
        });

        let client = new FxAccountsOAuthClient({
          parameters: {
            state: "state",
            client_id: "client_id",
            oauth_uri: HTTP_PATH,
            content_uri: HTTP_PATH
          },
          
          authorizationEndpoint: HTTP_ENDPOINT_WITH_KEYS
        });

        client.onComplete = function(tokenData, keys) {
          Assert.ok(tabOpened);
          Assert.equal(tokenData.code, "code1");
          Assert.equal(tokenData.state, "state");
          Assert.strictEqual(keys, undefined);
          resolve();
        };

        client.onError = reject;

        client.launchWebFlow();
      });
    }
  },
  {
    desc: "FxA OAuth - should receive an error if keys could not be obtained",
    run: function () {
      return new Promise(function(resolve, reject) {
        let tabOpened = false;

        waitForTab(function (tab) {
          Assert.ok("Tab successfully opened");

          
          let queryString = gBrowser.currentURI.spec.split('?')[1];
          Assert.ok(queryString.indexOf('keys=true') >= 0);

          tabOpened = true;
        });

        let client = new FxAccountsOAuthClient({
          parameters: {
            state: "state",
            client_id: "client_id",
            oauth_uri: HTTP_PATH,
            content_uri: HTTP_PATH,
            keys: true,
          },
          
          authorizationEndpoint: HTTP_ENDPOINT
        });

        client.onComplete = reject;

        client.onError = function(err) {
          Assert.ok(tabOpened);
          Assert.equal(err.message, "OAuth flow failed. Keys were not returned");
          resolve();
        };

        client.launchWebFlow();
      });
    }
  }
]; 

function waitForTab(aCallback) {
  let container = gBrowser.tabContainer;
  container.addEventListener("TabOpen", function tabOpener(event) {
    container.removeEventListener("TabOpen", tabOpener, false);
    gBrowser.addEventListener("load", function listener() {
      gBrowser.removeEventListener("load", listener, true);
      let tab = event.target;
      aCallback(tab);
    }, true);
  }, false);
}

function test() {
  waitForExplicitFinish();

  Task.spawn(function () {
    for (let test of gTests) {
      info("Running: " + test.desc);
      yield test.run();
    }
  }).then(finish, ex => {
    Assert.ok(false, "Unexpected Exception: " + ex);
    finish();
  });
}
