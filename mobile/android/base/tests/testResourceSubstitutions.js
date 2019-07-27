




const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Promise.jsm"); 
Cu.import("resource://gre/modules/Services.jsm"); 
Cu.import("resource://gre/modules/NetUtil.jsm"); 

function readChannel(url) {
  let deferred = Promise.defer();

  let channel = NetUtil.newChannel(url);
  channel.contentType = "text/plain";

  NetUtil.asyncFetch(channel, function(inputStream, status) {
    if (!Components.isSuccessCode(status)) {
      deferred.reject();
      return;
    }

    let content = NetUtil.readInputStreamToString(inputStream, inputStream.available());
    deferred.resolve(content);
  });

  return deferred.promise;
}

add_task(function test_Android() {
    let protocolHandler = Services.io
       .getProtocolHandler("resource")
       .QueryInterface(Ci.nsIResProtocolHandler);

    do_check_true(protocolHandler.hasSubstitution("android"));

    
    let packageName = yield readChannel("resource://android/package-name.txt");

    
    
    let expectedPrefix = "org.mozilla.";
    do_check_eq(packageName.substring(0, expectedPrefix.length), expectedPrefix);
});

run_next_test();
