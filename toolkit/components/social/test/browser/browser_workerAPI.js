



let provider;

function test() {
  waitForExplicitFinish();

  replaceAlertsService();
  registerCleanupFunction(restoreAlertsService);

  let manifest = {
    origin: 'http://example.com',
    name: "Example Provider",
    workerURL: "http://example.com/browser/toolkit/components/social/test/browser/worker_social.js"
  };

  SocialService.addProvider(manifest, function (p) {
    
    
    p.enabled = true;
    provider = p;
    runTests(tests, undefined, undefined, function () {
      SocialService.disableProvider(provider.origin, finish);
    });
  });
}

let tests = {
  testProfile: function(next) {
    let expect = {
      portrait: "https://example.com/portrait.jpg",
      userName: "trickster",
      displayName: "Kuma Lisa",
      profileURL: "http://en.wikipedia.org/wiki/Kuma_Lisa"
    }
    function ob(aSubject, aTopic, aData) {
      Services.obs.removeObserver(ob, "social:profile-changed");
      is(aData, provider.origin, "update of profile from our provider");
      let profile = provider.profile;
      is(profile.portrait, expect.portrait, "portrait is set");
      is(profile.userName, expect.userName, "userName is set");
      is(profile.displayName, expect.displayName, "displayName is set");
      is(profile.profileURL, expect.profileURL, "profileURL is set");
      next();
    }
    Services.obs.addObserver(ob, "social:profile-changed", false);
    let port = provider.getWorkerPort();
    port.postMessage({topic: "test-profile", data: expect});
    port.close();
  },

  testAmbientNotification: function(next) {
    let expect = {
      name: "test-ambient"
    }
    function ob(aSubject, aTopic, aData) {
      Services.obs.removeObserver(ob, "social:ambient-notification-changed");
      is(aData, provider.origin, "update is from our provider");
      let notif = provider.ambientNotificationIcons[expect.name];
      is(notif.name, expect.name, "ambientNotification reflected");

      next();
    }
    Services.obs.addObserver(ob, "social:ambient-notification-changed", false);
    let port = provider.getWorkerPort();
    port.postMessage({topic: "test-ambient", data: expect});
    port.close();
  },

  testProfileCleared: function(next) {
    let sent = {
      userName: ""
    };
    function ob(aSubject, aTopic, aData) {
      Services.obs.removeObserver(ob, "social:profile-changed");
      is(aData, provider.origin, "update of profile from our provider");
      is(Object.keys(provider.profile).length, 0, "profile was cleared by empty username");
      is(Object.keys(provider.ambientNotificationIcons).length, 0, "icons were cleared by empty username");

      next();
    }
    Services.obs.addObserver(ob, "social:profile-changed", false);
    provider.workerAPI._port.postMessage({topic: "test-profile", data: sent});
  },

  testNoCookies: function(next) {
    
    Services.cookies.removeAll();
    let port = provider.getWorkerPort();
    port.onmessage = function onMessage(event) {
      let {topic, data} = event.data;
      if (topic == "test.cookies-get-response") {
        is(data.length, 0, "got no cookies");
        port.close();
        next();
      }
    }
    port.postMessage({topic: "test-initialization"});
    port.postMessage({topic: "test.cookies-get"});
  },

  testCookies: function(next) {
    let port = provider.getWorkerPort();
    port.onmessage = function onMessage(event) {
      let {topic, data} = event.data;
      if (topic == "test.cookies-get-response") {
        is(data.length, 2, "got 2 cookies");
        is(data[0].name, "cheez", "cookie has the correct name");
        is(data[0].value, "burger", "cookie has the correct value");
        is(data[1].name, "moar", "cookie has the correct name");
        is(data[1].value, "bacon", "cookie has the correct value");
        Services.cookies.remove('.example.com', '/', 'cheez', false);
        Services.cookies.remove('.example.com', '/', 'moar', false);
        port.close();
        next();
      }
    }
    var MAX_EXPIRY = Math.pow(2, 62);
    Services.cookies.add('.example.com', '/', 'cheez', 'burger', false, false, true, MAX_EXPIRY);
    Services.cookies.add('.example.com', '/', 'moar', 'bacon', false, false, true, MAX_EXPIRY);
    port.postMessage({topic: "test-initialization"});
    port.postMessage({topic: "test.cookies-get"});
  },

  testWorkerReload: function(next) {
    let fw = {};
    Cu.import("resource://gre/modules/FrameWorker.jsm", fw);

    let worker = fw.getFrameWorkerHandle(provider.workerURL, undefined, "testWorkerReload");
    let port = provider.getWorkerPort();
    
    
    Services.obs.addObserver(function reloadObserver() {
      Services.obs.removeObserver(reloadObserver, "social:provider-reload");
      ok(port._closed, "old port was closed by the reload");
      let newWorker = fw.getFrameWorkerHandle(provider.workerURL, undefined, "testWorkerReload - worker2");
      let newPort = provider.getWorkerPort();
      newPort.onmessage = function (e) {
        let topic = e.data.topic;
        switch (topic) {
          case "test-initialization-complete":
            
            newPort.close();
            next();
            break;
        }
      }
      newPort.postMessage({topic: "test-initialization"});
    }, "social:provider-reload", false);

    port.onmessage = function (e) {
      let topic = e.data.topic;
      switch (topic) {
        case "test-initialization-complete":
          
          
          
          port.postMessage({topic: "test-reload-init"});
          
          break;
      }
    }
    port.postMessage({topic: "test-initialization"});
  },

  testNotificationLinks: function(next) {
    let port = provider.getWorkerPort();
    let data = {
      id: 'an id',
      body: 'the text',
      action: 'link',
      actionArgs: {} 
    }
    let testArgs = [
      
      ["http://example.com",      "http://example.com/", "tab"],
      
      ["https://example.com",     "http://example.com/", "tab"],
      
      ["https://mochitest:8888/", null,                 null]
    ];

    
    let oldopenUILinkIn = window.openUILinkIn;
    registerCleanupFunction(function () {
      
      window.openUILinkIn = oldopenUILinkIn;
    });
    let openLocation;
    let openWhere;
    window.openUILinkIn = function(location, where) {
      openLocation = location;
      openWhere = where;
    }

    
    let toURL, expectedLocation, expectedWhere;
    function nextTest() {
      if (testArgs.length == 0) {
        port.close();
        next(); 
        return;
      }
      openLocation = openWhere = null;
      [toURL, expectedLocation, expectedWhere] = testArgs.shift();
      data.actionArgs.toURL = toURL;
      port.postMessage({topic: 'test-notification-create', data: data});
    };

    port.onmessage = function(evt) {
      if (evt.data.topic == "did-notification-create") {
        is(openLocation, expectedLocation, "url actually opened was " + openLocation);
        is(openWhere, expectedWhere, "the url was opened in a " + expectedWhere);
        nextTest();
      }
    }
    
    port.postMessage({topic: "test-initialization"});
    nextTest();
  },

};
