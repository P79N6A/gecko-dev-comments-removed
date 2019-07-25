







do_load_httpd_js();

var server = new nsHttpServer();
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
  var clone = aRequest.clone(listener);
}


function checkAllCallbacks(listener, aRequest)
{
  do_check_eq(listener.state, ALL_BITS);

  do_test_finished();
}

function secondLoadDone(oldlistener, aRequest)
{
  do_test_pending();

  try {
    var staticrequest = aRequest.getStaticRequest();

    
    
    var listener = new ImageListener(null, checkAllCallbacks);
    listener.synchronous = false;
    var staticrequestclone = staticrequest.clone(listener);
  } catch(e) {
    
    
    do_test_finished();
  }

  run_loadImageWithChannel_tests();

  do_test_finished();
}



function checkSecondLoad()
{
  do_test_pending();

  var loader = Cc["@mozilla.org/image/loader;1"].getService(Ci.imgILoader);
  var listener = new ImageListener(checkClone, secondLoadDone);
  requests.push(loader.loadImage(uri, null, null, null, null, listener, null, 0, null, null, null));
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
    
    
    
    do_check_eq(streamlistener.requestStatus, 0);

    checkClone(imglistener, aRequest);
  }
}



function getChannelLoadImageStopCallback(streamlistener, next)
{
  return function channelLoadStop(imglistener, aRequest) {
    
    
    
    do_check_eq(streamlistener.requestStatus & STOP_REQUEST, 0);

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

  var loader = Cc["@mozilla.org/image/loader;1"].getService(Ci.imgILoader);
  var listener = new ImageListener(getChannelLoadImageStartCallback(channellistener),
                                   getChannelLoadImageStopCallback(channellistener,
                                                                   all_done_callback));
  var outlistener = {};
  requests.push(loader.loadImageWithChannel(channel, listener, null, outlistener));
  channellistener.outputListener = outlistener.value;

  listener.synchronous = false;
}

function run_loadImageWithChannel_tests()
{
  
  
  var loader = Cc["@mozilla.org/image/loader;1"].getService(Ci.imgILoader);
  loader.QueryInterface(Ci.imgICache);
  loader.clearCache(false);

  do_test_pending();

  var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);  
  var channel = ioService.newChannelFromURI(uri);
  var channellistener = new ChannelListener();
  channel.asyncOpen(channellistener, null);

  var loader = Cc["@mozilla.org/image/loader;1"].getService(Ci.imgILoader);
  var listener = new ImageListener(getChannelLoadImageStartCallback(channellistener),
                                   getChannelLoadImageStopCallback(channellistener,
                                                                   checkSecondChannelLoad));
  var outlistener = {};
  requests.push(loader.loadImageWithChannel(channel, listener, null, outlistener));
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
    var loader = Cc["@mozilla.org/image/loader;1"].getService(Ci.imgILoader);

    
    do_test_pending();
    var listener2 = new ImageListener(null, function(foo, bar) { do_test_finished(); });
    requests.push(loader.loadImage(uri, null, null, null, null, listener2, null, 0, null, null, null));
    listener2.synchronous = false;

    
    otherCb(listener, request);
  }
}

function run_test()
{
  var loader = Cc["@mozilla.org/image/loader;1"].getService(Ci.imgILoader);

  do_test_pending();
  var listener = new ImageListener(startImageCallback(checkClone), firstLoadDone);
  var req = loader.loadImage(uri, null, null, null, null, listener, null, 0, null, null, null);
  requests.push(req);

  
  req.lockImage();

  listener.synchronous = false;
}
