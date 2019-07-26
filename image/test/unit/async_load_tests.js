







const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

var server = new HttpServer();
server.registerDirectory("/", do_get_file(''));
server.registerContentType("sjs", "sjs");
server.start(8088);

load('image_load_helpers.js');

var requests = [];



function getCloneStopCallback(original_listener)
{
  return function cloneStop(listener) {
    do_check_eq(original_listener.state, listener.state);

    
    
    do_check_neq(original_listener, listener);
    do_test_finished();
  }
}



function checkClone(other_listener, aRequest)
{
  do_test_pending();

  
  var listener = new ImageListener(null, function(foo, bar) { do_test_finished(); } );
  listener.synchronous = false;
  var outer = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                .createScriptedObserver(listener);
  var clone = aRequest.clone(outer);
  requests.push(clone);
}


function checkSizeAndLoad(listener, aRequest)
{
  do_check_neq(listener.state & SIZE_AVAILABLE, 0);
  do_check_neq(listener.state & LOAD_COMPLETE, 0);

  do_test_finished();
}

function secondLoadDone(oldlistener, aRequest)
{
  do_test_pending();

  try {
    var staticrequest = aRequest.getStaticRequest();

    
    
    var listener = new ImageListener(null, checkSizeAndLoad);
    listener.synchronous = false;
    var outer = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                  .createScriptedObserver(listener);
    var staticrequestclone = staticrequest.clone(outer);
    requests.push(staticrequestclone);
  } catch(e) {
    
    
    do_test_finished();
  }

  run_loadImageWithChannel_tests();

  do_test_finished();
}



function checkSecondLoad()
{
  do_test_pending();

  var listener = new ImageListener(checkClone, secondLoadDone);
  var outer = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                .createScriptedObserver(listener);
  requests.push(gCurrentLoader.loadImageXPCOM(uri, null, null, null, null, outer, null, 0, null, null));
  listener.synchronous = false;
}

function firstLoadDone(oldlistener, aRequest)
{
  checkSecondLoad(uri);

  do_test_finished();
}



function getChannelLoadImageStartCallback(streamlistener)
{
  return function channelLoadStart(imglistener, aRequest) {
    
    
    
    do_check_eq(streamlistener.requestStatus & STOP_REQUEST, 0);

    checkClone(imglistener, aRequest);
  }
}



function getChannelLoadImageStopCallback(streamlistener, next)
{
  return function channelLoadStop(imglistener, aRequest) {

    next();

    do_test_finished();
  }
}



function checkSecondChannelLoad()
{
  do_test_pending();

  var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);  
  var channel = ioService.newChannelFromURI(uri);
  var channellistener = new ChannelListener();
  channel.asyncOpen(channellistener, null);

  var listener = new ImageListener(getChannelLoadImageStartCallback(channellistener),
                                   getChannelLoadImageStopCallback(channellistener,
                                                                   all_done_callback));
  var outer = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                .createScriptedObserver(listener);
  var outlistener = {};
  requests.push(gCurrentLoader.loadImageWithChannelXPCOM(channel, outer, null, outlistener));
  channellistener.outputListener = outlistener.value;

  listener.synchronous = false;
}

function run_loadImageWithChannel_tests()
{
  
  gCurrentLoader = Cc["@mozilla.org/image/loader;1"].createInstance(Ci.imgILoader);

  do_test_pending();

  var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);  
  var channel = ioService.newChannelFromURI(uri);
  var channellistener = new ChannelListener();
  channel.asyncOpen(channellistener, null);

  var listener = new ImageListener(getChannelLoadImageStartCallback(channellistener),
                                   getChannelLoadImageStopCallback(channellistener,
                                                                   checkSecondChannelLoad));
  var outer = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                .createScriptedObserver(listener);
  var outlistener = {};
  requests.push(gCurrentLoader.loadImageWithChannelXPCOM(channel, outer, null, outlistener));
  channellistener.outputListener = outlistener.value;

  listener.synchronous = false;
}

function all_done_callback()
{
  server.stop(function() { do_test_finished(); });
}

function startImageCallback(otherCb)
{
  return function(listener, request)
  {
    
    do_test_pending();
    var listener2 = new ImageListener(null, function(foo, bar) { do_test_finished(); });
    var outer = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                  .createScriptedObserver(listener2);
    requests.push(gCurrentLoader.loadImageXPCOM(uri, null, null, null, null, outer, null, 0, null, null));
    listener2.synchronous = false;

    
    otherCb(listener, request);
  }
}

var gCurrentLoader;

function cleanup()
{
  for (var i = 0; i < requests.length; ++i) {
    requests[i].cancelAndForgetObserver(0);
  }
}

function run_test()
{
  do_register_cleanup(cleanup);

  gCurrentLoader = Cc["@mozilla.org/image/loader;1"].createInstance(Ci.imgILoader);

  do_test_pending();
  var listener = new ImageListener(startImageCallback(checkClone), firstLoadDone);
  var outer = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                .createScriptedObserver(listener);
  var req = gCurrentLoader.loadImageXPCOM(uri, null, null, null, null, outer, null, 0, null, null);
  requests.push(req);

  
  req.lockImage();

  listener.synchronous = false;
}
