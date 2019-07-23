







































const BASE = "http://localhost:4444";

var paths =
  [
 BASE + "/test_registerdirectory.js", 
 BASE + "/test_registerdirectory.js", 
 BASE + "/test_registerdirectory.js", 

 BASE + "/test_registerdirectory.js", 
 BASE + "/test_registerdirectory.js", 
 BASE + "/test_registerdirectory.js", 

 BASE + "/test_registerdirectory.js", 
 BASE + "/test_registerdirectory.js", 
 BASE + "/test_registerdirectory.js", 
 BASE + "/test_registerdirectory.js",  

 BASE + "/foo/test_registerdirectory.js", 
 BASE + "/foo/test_registerdirectory.js/test_registerdirectory.js", 
 BASE + "/foo/test_registerdirectory.js/test_registerdirectory.js", 
 BASE + "/foo/test_registerdirectory.js", 
 BASE + "/foo/test_registerdirectory.js", 
 BASE + "/foo/test_registerdirectory.js/test_registerdirectory.js", 
 BASE + "/foo/test_registerdirectory.js/test_registerdirectory.js", 
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
      var ch = request.QueryInterface(Ci.nsIHttpChannel);

      switch (currPathIndex)
      {
        case 0:
          do_check_eq(ch.responseStatus, 404);
          do_check_false(ch.requestSucceeded);
          break;

        case 1:
          do_check_eq(ch.responseStatus, 200);
          do_check_true(ch.requestSucceeded);

          var actualFile = serverBasePath.clone();
          actualFile.append("test_registerdirectory.js");
          do_check_eq(ch.getResponseHeader("Content-Length"),
                      actualFile.fileSize.toString());
          break;

        case 2:
          do_check_eq(ch.responseStatus, 404);
          do_check_false(ch.requestSucceeded);
          break;

        case 3:
          do_check_eq(ch.responseStatus, 404);
          do_check_false(ch.requestSucceeded);
          break;

        case 4:
          do_check_eq(ch.responseStatus, 200);
          do_check_eq(ch.responseStatusText, "OK");
          do_check_true(ch.requestSucceeded);
          do_check_eq(ch.getResponseHeader("Override-Succeeded"), "yes");
          break;

        case 5:
          do_check_eq(ch.responseStatus, 404);
          do_check_false(ch.requestSucceeded);
          break;

        case 6:
          do_check_eq(ch.responseStatus, 200);
          do_check_true(ch.requestSucceeded);

          var actualFile = serverBasePath.clone();
          actualFile.append("test_registerdirectory.js");
          do_check_eq(ch.getResponseHeader("Content-Length"),
                      actualFile.fileSize.toString());
          break;

        case 7:
        case 8:
          do_check_eq(ch.responseStatus, 200);
          do_check_eq(ch.responseStatusText, "OK");
          do_check_true(ch.requestSucceeded);
          do_check_eq(ch.getResponseHeader("Override-Succeeded"), "yes");
          break;

        case 9:
          do_check_eq(ch.responseStatus, 404);
          do_check_false(ch.requestSucceeded);
          break;

        case 10:
          do_check_eq(ch.responseStatus, 200);
          do_check_eq(ch.responseStatusText, "OK");
          break;

        case 11:
          do_check_eq(ch.responseStatus, 404);
          do_check_false(ch.requestSucceeded);
          break;

        case 12:
        case 13:
          do_check_eq(ch.responseStatus, 200);
          do_check_eq(ch.responseStatusText, "OK");
          break;

        case 14:
          do_check_eq(ch.responseStatus, 404);
          do_check_false(ch.requestSucceeded);
          break;

        case 15:
          do_check_eq(ch.responseStatus, 200);
          do_check_eq(ch.responseStatusText, "OK");
          break;

        case 16:
          do_check_eq(ch.responseStatus, 404);
          do_check_false(ch.requestSucceeded);
          break;
      }
    },
    onStopRequest: function(request, cx, status)
    {
      switch (currPathIndex)
      {
        case 0:
          
          serverBasePath = testsDirectory.clone();
          srv.registerDirectory("/", serverBasePath);
          break;

        case 1:
          
          serverBasePath = null;
          srv.registerDirectory("/", serverBasePath);
          break;

        case 3:
          
          srv.registerPathHandler("/test_registerdirectory.js",
                                  override_test_registerdirectory);
          break;

        case 4:
          
          srv.registerPathHandler("/test_registerdirectory.js", null);
          break;

        case 5:
          
          serverBasePath = testsDirectory.clone();
          srv.registerDirectory("/", serverBasePath);
          break;

        case 6:
          
          srv.registerPathHandler("/test_registerdirectory.js",
                                  override_test_registerdirectory);
          break;

        case 7:
          
          serverBasePath = null;
          srv.registerDirectory("/", serverBasePath);
          break;

        case 8:
          
          srv.registerPathHandler("/test_registerdirectory.js", null);
          break;

        case 9:
          
          serverBasePath = testsDirectory.clone();
          srv.registerDirectory("/foo/", serverBasePath);
          break;

        case 10:
          
          break;

        case 11:
          
          srv.registerDirectory("/foo/test_registerdirectory.js/", serverBasePath);
          break;

        case 12:
          
          break;

        case 13:
          srv.registerDirectory("/foo/", null);
          break;

        case 14:
          
          break;

        case 15:
          srv.registerDirectory("/foo/test_registerdirectory.js/", null);
          break;
      }

      if (!paths[++currPathIndex])
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

var srv;
var serverBasePath;
var testsDirectory;

function run_test()
{
  testsDirectory = do_get_file("netwerk/test/httpserver/test/");

  srv = createServer();
  srv.start(4444);

  performNextTest();
}




function override_test_registerdirectory(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
  response.setHeader("Override-Succeeded", "yes");

  var body = "success!";
  response.bodyOutputStream.write(body, body.length);
}
