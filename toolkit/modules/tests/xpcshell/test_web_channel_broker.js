


"use strict";

const Cu = Components.utils;

Cu.import("resource://services-common/async.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/WebChannel.jsm");

const VALID_WEB_CHANNEL_ID = "id";
const URL_STRING = "http://example.com";
const VALID_WEB_CHANNEL_ORIGIN = Services.io.newURI(URL_STRING, null, null);

function run_test() {
  run_next_test();
}




add_test(function test_web_channel_broker_channel_map() {
  let channel = new Object();
  let channel2 = new Object();

  do_check_eq(WebChannelBroker._channelMap.size, 0);
  do_check_false(WebChannelBroker._messageListenerAttached);

  
  WebChannelBroker.registerChannel(channel);
  do_check_eq(WebChannelBroker._channelMap.size, 1);
  do_check_true(WebChannelBroker._messageListenerAttached);

  WebChannelBroker.registerChannel(channel2);
  do_check_eq(WebChannelBroker._channelMap.size, 2);

  WebChannelBroker.unregisterChannel(channel);
  do_check_eq(WebChannelBroker._channelMap.size, 1);

  
  do_check_false(WebChannelBroker._channelMap.has(channel));
  do_check_true(WebChannelBroker._channelMap.has(channel2));

  WebChannelBroker.unregisterChannel(channel2);
  do_check_eq(WebChannelBroker._channelMap.size, 0);

  run_next_test();
});





add_test(function test_web_channel_broker_listener() {
  let cb = Async.makeSpinningCallback();
  var channel = new Object({
    id: VALID_WEB_CHANNEL_ID,
    origin: VALID_WEB_CHANNEL_ORIGIN,
    deliver: function(data, sender) {
      do_check_eq(data.id, VALID_WEB_CHANNEL_ID);
      do_check_eq(data.message.command, "hello");
      WebChannelBroker.unregisterChannel(channel);
      cb();
      run_next_test();
    }
  });

  WebChannelBroker.registerChannel(channel);

  var mockEvent = {
    data: {
      id: VALID_WEB_CHANNEL_ID,
      message: {
        command: "hello"
      }
    },
    principal: {
      origin: URL_STRING
    }
  };

  WebChannelBroker._listener(mockEvent);
  cb.wait();
});
