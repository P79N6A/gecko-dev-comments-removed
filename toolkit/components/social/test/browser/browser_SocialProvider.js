



let SocialProvider = Components.utils.import("resource://gre/modules/SocialProvider.jsm", {}).SocialProvider;

function test() {
  
  
  
  let provider = new SocialProvider({
    origin: 'http://example.com',
    name: "Example Provider",
    workerURL: "http://example.com/browser/toolkit/components/social/test/browser/worker_social.js"
  });

  ok(provider.enabled, "provider is initially enabled");
  ok(provider.port, "should be able to get a port from enabled provider");
  ok(provider.workerAPI, "should be able to get a workerAPI from enabled provider");

  provider.enabled = false;

  ok(!provider.enabled, "provider is now disabled");
  ok(!provider.port, "shouldn't be able to get a port from disabled provider");
  ok(!provider.workerAPI, "shouldn't be able to get a workerAPI from disabled provider");

  provider.enabled = true;

  ok(provider.enabled, "provider is re-enabled");
  ok(provider.port, "should be able to get a port from re-enabled provider");
  ok(provider.workerAPI, "should be able to get a workerAPI from re-enabled provider");

  
  provider.enabled = false;
}
