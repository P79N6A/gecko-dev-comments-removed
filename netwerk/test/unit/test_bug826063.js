







const {classes: Cc, interfaces: Ci, utils: Cu} = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var URIs = [
  "http://example.org",
  "https://example.org",
  "ftp://example.org"
  ];

function LoadContext(usePrivateBrowsing) {
  this.usePrivateBrowsing = usePrivateBrowsing;
}
LoadContext.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsILoadContext, Ci.nsIInterfaceRequestor]),
  getInterface: XPCOMUtils.generateQI([Ci.nsILoadContext])
};

function getChannels() {
  for (let u of URIs) {
    yield Services.io.newChannel(u, null, null);
  }
}

function checkPrivate(channel, shouldBePrivate) {
  do_check_eq(channel.QueryInterface(Ci.nsIPrivateBrowsingChannel).isChannelPrivate,
              shouldBePrivate);
}





add_test(function test_plain() {
  for (let c of getChannels()) {
    checkPrivate(c, false);
  }
  run_next_test();
});




add_test(function test_setPrivate_private() {
  for (let c of getChannels()) {
    c.QueryInterface(Ci.nsIPrivateBrowsingChannel).setPrivate(true);
    checkPrivate(c, true);
  }
  run_next_test();
});




add_test(function test_setPrivate_regular() {
  for (let c of getChannels()) {
    c.QueryInterface(Ci.nsIPrivateBrowsingChannel).setPrivate(false);
    checkPrivate(c, false);
  }
  run_next_test();
});




add_test(function test_LoadContextPrivate() {
  let ctx = new LoadContext(true);
  for (let c of getChannels()) {
    c.notificationCallbacks = ctx;
    checkPrivate(c, true);
  }
  run_next_test();
});




add_test(function test_LoadContextRegular() {
  let ctx = new LoadContext(false);
  for (let c of getChannels()) {
    c.notificationCallbacks = ctx;
    checkPrivate(c, false);
  }
  run_next_test();
});







function run_test() {
    run_next_test();
}
