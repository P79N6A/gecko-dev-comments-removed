








































var paths =
  [
   "http://localhost:4444/my/first-path/override/",
   "http://localhost:4444/my/second-path/override",
   "http://localhost:4444/my/second-path/override"
  ];
  
var contents =
  [
   "First content",
   "Second content",
   "" 
  ];

var contents =
  [
   "First content",
   "Second content",
   ""
  ];

var success =
  [
   true, true, false
  ];

var currPathIndex = 0;

var uploadListener =
  {
    
    onDataAvailable: function(request, cx, inputStream, offset, count)
    {
      makeBIS(inputStream).readByteArray(count); 
    },

    
    onStartRequest: function(request, cx)
    {
    },
    onStopRequest: function(request, cx, status)
    {
      do_check_true(Components.isSuccessCode(status));
      var ch = makeChannel(paths[currPathIndex]);
      ch.asyncOpen(downloadListener, null);
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

var downloadListener =
  {
    
    onDataAvailable: function(request, cx, inputStream, offset, count)
    {
      var content = makeBIS(inputStream).readByteArray(count); 

      if (success[currPathIndex])
      {
        for (var i=0; i<count; ++i)
          do_check_eq(contents[currPathIndex].charCodeAt(i), content[i])
      }
    },

    
    onStartRequest: function(request, cx)
    {
      if (success[currPathIndex])
      {
        
        var ch = request.QueryInterface(Ci.nsIHttpChannel)
                        .QueryInterface(Ci.nsIHttpChannelInternal);
        do_check_eq(ch.getResponseHeader("Content-Type"), "application/x-moz-put-test");
      }
    },
    onStopRequest: function(request, cx, status)
    {
      do_check_true(Components.isSuccessCode(status));

      var ch = request.QueryInterface(Ci.nsIHttpChannel)
                      .QueryInterface(Ci.nsIHttpChannelInternal);
      do_check_true(ch.requestSucceeded == success[currPathIndex]);

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
  var stream = Cc["@mozilla.org/io/string-input-stream;1"]
      .createInstance(Ci.nsIStringInputStream);
  stream.setData(contents[currPathIndex], contents[currPathIndex].length);
     
  var upch = ch.QueryInterface(Ci.nsIUploadChannel);
  upch.setUploadStream(stream, "application/x-moz-put-test", stream.available());
  ch.asyncOpen(uploadListener, null);
}

var srv, serverBasePath;

function run_test()
{
  srv = createServer();
  srv.start(4444);

  performNextTest();
}
