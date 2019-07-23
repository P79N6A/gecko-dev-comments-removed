





































do_load_httpd_js();


DEBUG = true;

const Timer = CC("@mozilla.org/timer;1", "nsITimer", "initWithCallback");








function createServer()
{
  return new nsHttpServer();
}







function makeChannel(url)
{
  var ios = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService);
  var chan = ios.newChannel(url, null, null)
                .QueryInterface(Ci.nsIHttpChannel);

  return chan;
}







function makeBIS(stream)
{
  return new BinaryInputStream(stream);
}










function fileContents(file)
{
  const PR_RDONLY = 0x01;
  var fis = new FileInputStream(file, PR_RDONLY, 0444,
                                Ci.nsIFileInputStream.CLOSE_ON_EOF);
  var sis = new ScriptableInputStream(fis);
  var contents = sis.read(file.fileSize);
  sis.close();
  return contents;
}











function LineIterator(data)
{
  var start = 0, index = 0;
  do
  {
    index = data.indexOf("\r\n");
    if (index >= 0)
      yield data.substring(0, index);
    else
      yield data;

    data = data.substring(index + 2);
  }
  while (index >= 0);
}












function expectLines(iter, expectedLines)
{
  var index = 0;
  for (var line in iter)
  {
    if (expectedLines.length == index)
      throw "Error: got more than " + expectedLines.length + " expected lines!";

    var expected = expectedLines[index++];
    if (expected !== line)
      throw "Error on line " + index + "!\n" +
            "  actual: '" + line + "',\n" +
            "  expect: '" + expected + "'";
  }

  if (expectedLines.length !== index)
  {
    throw "Expected more lines!  Got " + index +
          ", expected " + expectedLines.length;
  }
}









function writeDetails(request, response)
{
  response.write("Method:  " + request.method + "\r\n");
  response.write("Path:    " + request.path + "\r\n");
  response.write("Query:   " + request.queryString + "\r\n");
  response.write("Version: " + request.httpVersion + "\r\n");
  response.write("Scheme:  " + request.scheme + "\r\n");
  response.write("Host:    " + request.host + "\r\n");
  response.write("Port:    " + request.port);
}









function skipHeaders(iter)
{
  var line = iter.next();
  while (line !== "")
    line = iter.next();
}










function isException(e, code)
{
  if (e !== code && e.result !== code)
    do_throw("unexpected error: " + e);
}





var __pendingTimers = [];







const __timerFuzz = 15;











function callLater(msecs, callback)
{
  do_check_true(msecs >= 0);

  var start = Date.now();

  function checkTime()
  {
    var index = __pendingTimers.indexOf(timer);
    do_check_true(index >= 0); 
    __pendingTimers.splice(index, 1);
    do_check_eq(__pendingTimers.indexOf(timer), -1);

    
    
    
    var end = Date.now();
    var elapsed = end - start;
    if (elapsed >= msecs)
    {
      dumpn("*** TIMER FIRE " + elapsed + "ms (" + msecs + "ms requested)");
      try
      {
        callback();
      }
      catch (e)
      {
        do_throw("exception thrown from callLater callback: " + e);
      }
      return;
    }

    
    
    var newDelay = msecs - elapsed;
    dumpn("*** TIMER UNDERSHOOT " + newDelay + "ms " +
          "(" + msecs + "ms requested, delaying)");

    callLater(newDelay, callback);
  }

  var timer =
    new Timer(checkTime, msecs + __timerFuzz, Ci.nsITimer.TYPE_ONE_SHOT);
  __pendingTimers.push(timer);
}










function testComplete(srv)
{
  return function complete()
  {
    do_test_pending();
    srv.stop(function quit() { do_test_finished(); });
  };
}






















function Test(path, initChannel, onStartRequest, onStopRequest)
{
  function nil() { }

  this.path = path;
  this.initChannel = initChannel || nil;
  this.onStartRequest = onStartRequest || nil;
  this.onStopRequest = onStopRequest || nil;
}









function runHttpTests(testArray, done)
{
  
  function performNextTest()
  {
    if (++testIndex == testArray.length)
    {
      try
      {
        done();
      }
      catch (e)
      {
        do_throw("error running test-completion callback: " + e);
      }
      return;
    }

    do_test_pending();

    var test = testArray[testIndex];
    var ch = makeChannel(test.path);
    try
    {
      test.initChannel(ch);
    }
    catch (e)
    {
      try
      {
        do_throw("testArray[" + testIndex + "].initChannel(ch) failed: " + e);
      }
      catch (e) {  }
    }

    ch.asyncOpen(listener, null);
  }

  
  var testIndex = -1;

  
  var listener =
    {
      
      _data: [],

      onStartRequest: function(request, cx)
      {
        var ch = request.QueryInterface(Ci.nsIHttpChannel)
                        .QueryInterface(Ci.nsIHttpChannelInternal);

        this._data.length = 0;
        try
        {
          try
          {
            testArray[testIndex].onStartRequest(ch, cx);
          }
          catch (e)
          {
            do_throw("testArray[" + testIndex + "].onStartRequest: " + e);
          }
        }
        catch (e)
        {
          dumpn("!!! swallowing onStartRequest exception so onStopRequest is " +
                "called...");
        }
      },
      onDataAvailable: function(request, cx, inputStream, offset, count)
      {
        Array.prototype.push.apply(this._data,
                                   makeBIS(inputStream).readByteArray(count));
      },
      onStopRequest: function(request, cx, status)
      {
        var ch = request.QueryInterface(Ci.nsIHttpChannel)
                        .QueryInterface(Ci.nsIHttpChannelInternal);

        
        
        
        
        try
        {
          testArray[testIndex].onStopRequest(ch, cx, status, this._data);
        }
        finally
        {
          try
          {
            performNextTest();
          }
          finally
          {
            do_test_finished();
          }
        }
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

  performNextTest();
}
























function RawTest(host, port, data, responseCheck)
{
  if (0 > port || 65535 < port || port % 1 !== 0)
    throw "bad port";
  if (!(data instanceof Array))
    data = [data];
  if (data.length <= 0)
    throw "bad data length";
  if (!data.every(function(v) { return /^[\x00-\xff]*$/.test(data); }))
    throw "bad data contained non-byte-valued character";

  this.host = host;
  this.port = port;
  this.data = data;
  this.responseCheck = responseCheck;
}









function runRawTests(testArray, done)
{
  do_test_pending();

  var sts = Cc["@mozilla.org/network/socket-transport-service;1"]
              .getService(Ci.nsISocketTransportService);

  var currentThread = Cc["@mozilla.org/thread-manager;1"]
                        .getService()
                        .currentThread;
  
  
  function performNextTest()
  {
    if (++testIndex == testArray.length)
    {
      do_test_finished();
      try
      {
        done();
      }
      catch (e)
      {
        do_throw("error running test-completion callback: " + e);
      }
      return;
    }


    var rawTest = testArray[testIndex];

    var transport =
      sts.createTransport(null, 0, rawTest.host, rawTest.port, null);

    var inStream = transport.openInputStream(0, 0, 0);
    var outStream  = transport.openOutputStream(0, 0, 0);

    
    dataIndex = 0;
    received = "";

    waitForMoreInput(inStream);
    waitToWriteOutput(outStream);
  }

  function waitForMoreInput(stream)
  {
    stream = stream.QueryInterface(Ci.nsIAsyncInputStream);
    stream.asyncWait(reader, 0, 0, currentThread);
  }

  function waitToWriteOutput(stream)
  {
    
    
    
    stream = stream.QueryInterface(Ci.nsIAsyncOutputStream);
    stream.asyncWait(writer, 0, testArray[testIndex].data[dataIndex].length,
                     currentThread);
  }

  
  var testIndex = -1;

  



  var dataIndex = 0;

  
  var received = "";

  
  var reader =
    {
      onInputStreamReady: function(stream)
      {
        var bis = new BinaryInputStream(stream);

        var av = 0;
        try
        {
          av = bis.available();
        }
        catch (e) {  }

        if (av > 0)
        {
          received += String.fromCharCode.apply(null, bis.readByteArray(av));
          waitForMoreInput(stream);
          return;
        }

        var rawTest = testArray[testIndex];
        try
        {
          rawTest.responseCheck(received);
        }
        catch (e)
        {
          do_throw("error thrown by responseCheck: " + e);
        }
        finally
        {
          stream.close();
          performNextTest();
        }
      }
    };

  
  var writer = 
    {
      onOutputStreamReady: function(stream)
      {
        var data = testArray[testIndex].data[dataIndex];

        var written = 0;
        try
        {
          written = stream.write(data, data.length);
          if (written == data.length)
            dataIndex++;
          else
            testArray[testIndex].data = data.substring(written);
        }
        catch (e) {  }

        
        if (written != 0)
          waitToWriteOutput(stream);
        else
          stream.close();
      }
    };

  performNextTest();
}
