








































var paths =
  [
   "http://localhost:4444/throws/exception",
   "http://localhost:4444/this/file/does/not/exist/and/404s",
   "http://localhost:4444/attempts/404/fails/so/400/fails/so/500s"
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
          checkStatusLine(ch, 1, 1, 500, "Internal Server Error");
          break;

        case 1:
          checkStatusLine(ch, 1, 1, 400, "Bad Request");
          break;

        case 2:
          checkStatusLine(ch, 1, 1, 500, "Internal Server Error");
          break;
      }
    },
    onStopRequest: function(request, cx, status)
    {
      do_check_true(Components.isSuccessCode(status));

      switch (currPathIndex)
      {
        case 0:
          break;

        case 1:
          srv.registerErrorHandler(400, throwsException);
          break;

        case 2:
          break;
      }

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

function checkStatusLine(channel, httpMaxVer, httpMinVer, httpCode, statusText)
{
  do_check_eq(channel.responseStatus, httpCode);
  do_check_eq(channel.responseStatusText, statusText);

  var respMaj = {}, respMin = {};
  channel.getResponseVersion(respMaj, respMin);
  do_check_eq(respMaj.value, httpMaxVer);
  do_check_eq(respMin.value, httpMinVer);
}

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

  srv.registerErrorHandler(404, throwsException);
  srv.registerPathHandler("/throws/exception", throwsException);

  srv.start(4444);

  performNextTest();
}




function throwsException(metadata, response)
{
  throw "this shouldn't cause an exit...";
  do_throw("Not reached!");
}
