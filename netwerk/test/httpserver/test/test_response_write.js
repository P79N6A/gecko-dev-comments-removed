







































var paths =
  [
   "http://localhost:4444/writeString",
   "http://localhost:4444/writeInt"
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
          do_check_eq(ch.getResponseHeader("Content-Length"), "4");
          break;

        case 1:
          do_check_eq(ch.getResponseHeader("Content-Length"), "4");
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
  ch.asyncOpen(listener, null);
}

var srv;

function run_test()
{
  srv = createServer();

  srv.registerPathHandler("/writeString", writeString);
  srv.registerPathHandler("/writeInt", writeInt);
  srv.start(4444);

  performNextTest();
}



function writeString(metadata, response)
{
  response.write("1234");
}

function writeInt(metadata, response)
{
  response.write(1234);
}
