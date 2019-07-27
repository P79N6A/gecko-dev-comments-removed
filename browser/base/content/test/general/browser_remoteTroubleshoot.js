



let {WebChannel} = Cu.import("resource://gre/modules/WebChannel.jsm", {});

const TEST_URL_TAIL = "example.com/browser/browser/base/content/test/general/test_remoteTroubleshoot.html"
const TEST_URI_GOOD = Services.io.newURI("https://" + TEST_URL_TAIL, null, null);
const TEST_URI_BAD = Services.io.newURI("http://" + TEST_URL_TAIL, null, null);


function promiseChannelResponse(channelID, originOrPermission) {
  return new Promise((resolve, reject) => {
    let channel = new WebChannel(channelID, originOrPermission);
    channel.listen((id, data, target) => {
      channel.stopListening();
      resolve(data);
    });
  });
};



function promiseNewChannelResponse(uri) {
  let channelPromise = promiseChannelResponse("test-remote-troubleshooting-backchannel",
                                              uri);
  let tab = gBrowser.loadOneTab(uri.spec, { inBackground: false });
  return promiseTabLoaded(tab).then(
    () => channelPromise
  ).then(data => {
    gBrowser.removeTab(tab);
    return data;
  });
}

add_task(function*() {
  
  let got = yield promiseNewChannelResponse(TEST_URI_GOOD);
  
  Assert.ok(got.message === undefined, "should have failed to get any data");

  
  Services.perms.add(TEST_URI_GOOD,
                     "remote-troubleshooting",
                     Services.perms.ALLOW_ACTION);
  registerCleanupFunction(() => {
    Services.perms.remove(TEST_URI_GOOD, "remote-troubleshooting");
  });

  
  got = yield promiseNewChannelResponse(TEST_URI_GOOD);

  
  Assert.ok(got.message.extensions, "should have extensions");
  Assert.ok(got.message.graphics, "should have graphics");

  
  Assert.equal(got.message.application.buildID, Services.appinfo.appBuildID,
               "should have correct build ID");

  let updateChannel = null;
  try {
    updateChannel = Cu.import("resource://gre/modules/UpdateChannel.jsm", {}).UpdateChannel.get();
  } catch (ex) {}
  if (!updateChannel) {
    Assert.ok(!('updateChannel' in got.message.application),
                "should not have update channel where not available.");
  } else {
    Assert.equal(got.message.application.updateChannel, updateChannel,
                 "should have correct update channel.");
  }


  
  Assert.ok(!got.message.modifiedPreferences, "should not have a modifiedPreferences key");
  Assert.ok(!got.message.crashes, "should not have crash info");

  
  got = yield promiseNewChannelResponse(TEST_URI_BAD);
  Assert.ok(got.message === undefined, "should have failed to get any data");
});
