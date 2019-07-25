



let SocialProvider = Components.utils.import("resource://gre/modules/SocialProvider.jsm", {}).SocialProvider;

function test() {
  waitForExplicitFinish();

  
  
  
  let provider = new SocialProvider({
    origin: 'http://example.com',
    name: "Example Provider",
    workerURL: "http://example.com/browser/toolkit/components/social/test/browser/worker_social.js"
  });

  ok(provider.workerAPI, "provider has a workerAPI");
  is(provider.workerAPI.initialized, false, "workerAPI is not yet initialized");

  let port = provider.port;
  ok(port, "should be able to get a port from the provider");

  port.onmessage = function onMessage(event) {
    let {topic, data} = event.data;
    if (topic == "test-initialization-complete") {
      is(provider.workerAPI.initialized, true, "workerAPI is now initialized");
      
      provider.enabled = false;
      finish();
    }
  }
  port.postMessage({topic: "test-initialization"});
}
