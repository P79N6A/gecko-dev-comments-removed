


"use strict";

(function (exports) {

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
let CC = Components.Constructor;

let { require } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
let { NetUtil } = Cu.import("resource://gre/modules/NetUtil.jsm", {});
let promise = require("promise");

let TEST_URL_ROOT = "http://example.com/browser/browser/devtools/shared/test/";
let ACTOR_URL = TEST_URL_ROOT + "test-actor.js";


exports.registerTestActor = Task.async(function* (client) {
  
  
  let deferred = promise.defer();
  client.listTabs(deferred.resolve);
  let response = yield deferred.promise;
  let { ActorRegistryFront } = require("devtools/server/actors/actor-registry");
  let registryFront = ActorRegistryFront(client, response);

  
  let options = {
    type: { tab: true },
    constructor: "TestActor",
    prefix: "testActor"
  };
  let testActorFront = yield registryFront.registerActor(ACTOR_URL, options);
  return testActorFront;
});



let _loadFront = Task.async(function* () {
  let sourceText = yield _request(ACTOR_URL);
  const principal = CC("@mozilla.org/systemprincipal;1", "nsIPrincipal")();
  const sandbox = Cu.Sandbox(principal);
  const exports = sandbox.exports = {};
  sandbox.require = require;
  Cu.evalInSandbox(sourceText, sandbox, "1.8", ACTOR_URL, 1);
  return sandbox.exports;
});

let _getUpdatedForm = function (client, tab) {
  return client.getTab({tab: tab})
               .then(response => response.tab);
};


exports.getTestActor = Task.async(function* (toolbox) {
  let client = toolbox.target.client;
  return _getTestActor(client, toolbox.target.tab, toolbox);
});




exports.getTestActorWithoutToolbox = Task.async(function* (tab) {
  let { DebuggerServer } = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});
  let { DebuggerClient } = Cu.import("resource://gre/modules/devtools/dbg-client.jsm", {});

  
  
  if (!DebuggerServer.initialized) {
    DebuggerServer.init();
    DebuggerServer.addBrowserActors();
  }
  let client = new DebuggerClient(DebuggerServer.connectPipe());

  let deferred = promise.defer();
  client.connect(deferred.resolve);
  yield deferred.promise;

  return _getTestActor(client, tab);
});


let _request = function (uri) {
  let deferred = promise.defer();
  try {
    uri = Services.io.newURI(uri, null, null);
  } catch (e) {
    deferred.reject(e);
  }

  NetUtil.asyncFetch(uri, (stream, status, req) => {
    if (!Components.isSuccessCode(status)) {
      deferred.reject(new Error("Request failed with status code = "
                       + status
                       + " after NetUtil.asyncFetch for url = "
                       + uri.spec));
      return;
    }

    let source = NetUtil.readInputStreamToString(stream, stream.available());
    stream.close();
    deferred.resolve(source);
  });
  return deferred.promise;
}

let _getTestActor = Task.async(function* (client, tab, toolbox) {
  
  
  let form = yield _getUpdatedForm(client, tab);

  let { TestActorFront } = yield _loadFront();

  return new TestActorFront(client, form, toolbox);
});

})(this);
