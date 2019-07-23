








































var paths =
  [
   "http://localhost:4444/no/setstatusline",
   "http://localhost:4444/http1_0",
   "http://localhost:4444/http1_1",
   "http://localhost:4444/invalidVersion",
   "http://localhost:4444/invalidStatus",
   "http://localhost:4444/invalidDescription",
   "http://localhost:4444/crazyCode",
   "http://localhost:4444/nullVersion"
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
          checkStatusLine(ch, 1, 1, 200, "OK");
          break;

        case 1:
          checkStatusLine(ch, 1, 0, 200, "OK");
          break;

        case 2:
          checkStatusLine(ch, 1, 1, 200, "OK");
          break;

        case 3:
        case 4:
        case 5:
          checkStatusLine(ch, 1, 1, 200, "OK");
          do_check_eq(ch.getResponseHeader("Passed"), "true");
          break;

        case 6:
          checkStatusLine(ch, 1, 1, 617, "Crazy");
          break;

        case 7:
          
          checkStatusLine(ch, 1, 1, 255, "NULL");
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

  srv.registerPathHandler("/no/setstatusline", noSetstatusline);
  srv.registerPathHandler("/http1_0", http1_0);
  srv.registerPathHandler("/http1_1", http1_1);
  srv.registerPathHandler("/invalidVersion", invalidVersion);
  srv.registerPathHandler("/invalidStatus", invalidStatus);
  srv.registerPathHandler("/invalidDescription", invalidDescription);
  srv.registerPathHandler("/crazyCode", crazyCode);
  srv.registerPathHandler("/nullVersion", nullVersion);

  srv.start(4444);

  performNextTest();
}




function noSetstatusline(metadata, response)
{
}


function http1_0(metadata, response)
{
  response.setStatusLine("1.0", 200, "OK");
}


function http1_1(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
}


function invalidVersion(metadata, response)
{
  try
  {
    response.setStatusLine(" 1.0", 200, "FAILED");
  }
  catch (e)
  {
    response.setHeader("Passed", "true", false);
  }
}


function invalidStatus(metadata, response)
{
  try
  {
    response.setStatusLine("1.0", 1000, "FAILED");
  }
  catch (e)
  {
    response.setHeader("Passed", "true", false);
  }
}

function invalidDescription(metadata, response)
{
  try
  {
    response.setStatusLine("1.0", 200, "FAILED\x01");
  }
  catch (e)
  {
    response.setHeader("Passed", "true", false);
  }
}


function crazyCode(metadata, response)
{
  response.setStatusLine("1.1", 617, "Crazy");
}


function nullVersion(metadata, response)
{
  response.setStatusLine(null, 255, "NULL");
}
