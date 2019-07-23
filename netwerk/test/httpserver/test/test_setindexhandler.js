







































var paths =
  [
   "http://localhost:4444/",
   "http://localhost:4444/"
  ];
var currPathIndex = 0;

var listener =
  {
    
    onDataAvailable: function(request, cx, inputStream, offset, count)
    {
      makeBIS(inputStream).readByteArray(count); 
    },
    
    onStartRequest: function(request, cx)
    {
      var ch = request.QueryInterface(Ci.nsIHttpChannel)
                      .QueryInterface(Ci.nsIHttpChannelInternal);

      switch (currPathIndex)
      {
        case 0:
          do_check_eq(ch.getResponseHeader("Content-Length"), "10");
          srv.setIndexHandler(null);
          break;

        case 1:
          do_check_eq(ch.responseStatus, 200); 
          break;
      }
    },
    onStopRequest: function(request, cx, status)
    {
      do_check_true(Components.isSuccessCode(status));
      if (++currPathIndex == paths.length)
        srv.stop();
      else
        performNextTest();
      do_test_finished();
    },
    
    QueryInterface: function(aIID)
    {
      if (aIID.equals(Ci.nsIStreamListener) ||
          aIID.equals(Ci.nsIRequestObserver) ||
          aIID.equals(Ci.nsISupports))
        return this;
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };

function performNextTest()
{
  do_test_pending();

  var ch = makeChannel(paths[currPathIndex]);
  ch.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE; 
  ch.asyncOpen(listener, null);
}

var srv, serverBasePath;

function run_test()
{
  srv = createServer();
  serverBasePath = do_get_file("netwerk/test/httpserver/test/");
  srv.registerDirectory("/", serverBasePath);
  srv.setIndexHandler(myIndexHandler);
  srv.start(4444);

  performNextTest();
}



function myIndexHandler(metadata, response)
{
  var dir = metadata.getProperty("directory");
  do_check_true(dir != null);
  do_check_true(dir instanceof Ci.nsIFile);
  do_check_true(dir.equals(serverBasePath));

  response.write("directory!");
}
