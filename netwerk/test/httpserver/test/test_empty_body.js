








































var paths =
  [
   "http://localhost:4444/empty-body-unwritten",
   "http://localhost:4444/empty-body-written"
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
      do_check_true(ch.contentLength == 0);
    },
    onStopRequest: function(request, cx, status)
    {
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
  ch.asyncOpen(listener, null);
}

var srv;

function run_test()
{
  srv = createServer();

  
  srv.registerPathHandler("/empty-body-unwritten", emptyBodyUnwritten);
  srv.registerPathHandler("/empty-body-written", emptyBodyWritten);

  srv.start(4444);

  performNextTest();
}




function emptyBodyUnwritten(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
}


function emptyBodyWritten(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
  var body = "";
  response.bodyOutputStream.write(body, body.length);
}
