










































const PORT = 4444;

const PATH = "http://localhost:" + PORT + "/object-state.sjs";

var srv;

function run_test()
{
  srv = createServer();
  var sjsDir = do_get_file("data/sjs/");
  srv.registerDirectory("/", sjsDir);
  srv.registerContentType("sjs", "sjs");
  srv.start(PORT);

  do_test_pending();

  new HTTPTestLoader(PATH + "?state=initial", initialStart, initialStop);
}






























var initialStarted = false;
function initialStart(ch, cx)
{
  dumpn("*** initialStart");

  if (initialStarted)
    do_throw("initialStart: initialStarted is true?!?!");

  initialStarted = true;

  new HTTPTestLoader(PATH + "?state=intermediate",
                     intermediateStart, intermediateStop);
}

var initialStopped = false;
function initialStop(ch, cx, status, data)
{
  dumpn("*** initialStop");

  do_check_eq(data.map(function(v) { return String.fromCharCode(v); }).join(""),
              "done");

  do_check_eq(srv.getObjectState("object-state-test"), null);

  if (!initialStarted)
    do_throw("initialStop: initialStarted is false?!?!");
  if (initialStopped)
    do_throw("initialStop: initialStopped is true?!?!");
  if (!intermediateStarted)
    do_throw("initialStop: intermediateStarted is false?!?!");
  if (!intermediateStopped)
    do_throw("initialStop: intermediateStopped is false?!?!");

  initialStopped = true;

  checkForFinish();
}

var intermediateStarted = false;
function intermediateStart(ch, cx)
{
  dumpn("*** intermediateStart");

  do_check_neq(srv.getObjectState("object-state-test"), null);

  if (!initialStarted)
    do_throw("intermediateStart: initialStarted is false?!?!");
  if (intermediateStarted)
    do_throw("intermediateStart: intermediateStarted is true?!?!");

  intermediateStarted = true;
}

var intermediateStopped = false;
function intermediateStop(ch, cx, status, data)
{
  dumpn("*** intermediateStop");

  do_check_eq(data.map(function(v) { return String.fromCharCode(v); }).join(""),
              "intermediate");

  do_check_neq(srv.getObjectState("object-state-test"), null);

  if (!initialStarted)
    do_throw("intermediateStop: initialStarted is false?!?!");
  if (!intermediateStarted)
    do_throw("intermediateStop: intermediateStarted is false?!?!");
  if (intermediateStopped)
    do_throw("intermediateStop: intermediateStopped is true?!?!");

  intermediateStopped = true;

  new HTTPTestLoader(PATH + "?state=trigger", triggerStart,
                     triggerStop);
}

var triggerStarted = false;
function triggerStart(ch, cx)
{
  dumpn("*** triggerStart");

  if (!initialStarted)
    do_throw("triggerStart: initialStarted is false?!?!");
  if (!intermediateStarted)
    do_throw("triggerStart: intermediateStarted is false?!?!");
  if (!intermediateStopped)
    do_throw("triggerStart: intermediateStopped is false?!?!");
  if (triggerStarted)
    do_throw("triggerStart: triggerStarted is true?!?!");

  triggerStarted = true;
}

var triggerStopped = false;
function triggerStop(ch, cx, status, data)
{
  dumpn("*** triggerStop");

  do_check_eq(data.map(function(v) { return String.fromCharCode(v); }).join(""),
              "trigger");

  if (!initialStarted)
    do_throw("triggerStop: initialStarted is false?!?!");
  if (!intermediateStarted)
    do_throw("triggerStop: intermediateStarted is false?!?!");
  if (!intermediateStopped)
    do_throw("triggerStop: intermediateStopped is false?!?!");
  if (!triggerStarted)
    do_throw("triggerStop: triggerStarted is false?!?!");
  if (triggerStopped)
    do_throw("triggerStop: triggerStopped is false?!?!");

  triggerStopped = true;

  checkForFinish();
}

var finished = false;
function checkForFinish()
{
  if (finished)
  {
    try
    {
      do_throw("uh-oh, how are we being finished twice?!?!");
    }
    finally
    {
      quit(1);
    }
  }

  if (triggerStopped && initialStopped)
  {
    finished = true;
    try
    {
      do_check_eq(srv.getObjectState("object-state-test"), null);

      if (!initialStarted)
        do_throw("checkForFinish: initialStarted is false?!?!");
      if (!intermediateStarted)
        do_throw("checkForFinish: intermediateStarted is false?!?!");
      if (!intermediateStopped)
        do_throw("checkForFinish: intermediateStopped is false?!?!");
      if (!triggerStarted)
        do_throw("checkForFinish: triggerStarted is false?!?!");
    }
    finally
    {
      srv.stop(do_test_finished);
    }
  }
}







function HTTPTestLoader(path, start, stop)
{
  
  this._path = path;

  
  this._data = [];

  
  this._start = start;

  
  this._stop = stop;

  var channel = makeChannel(path);
  channel.asyncOpen(this, null);
}
HTTPTestLoader.prototype =
  {
    onStartRequest: function(request, cx)
    {
      dumpn("*** HTTPTestLoader.onStartRequest for " + this._path);

      var ch = request.QueryInterface(Ci.nsIHttpChannel)
                      .QueryInterface(Ci.nsIHttpChannelInternal);

      try
      {
        try
        {
          this._start(ch, cx);
        }
        catch (e)
        {
          do_throw(this._path + ": error in onStartRequest: " + e);
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
      dumpn("*** HTTPTestLoader.onDataAvailable for " + this._path);

      Array.prototype.push.apply(this._data,
                                 makeBIS(inputStream).readByteArray(count));
    },
    onStopRequest: function(request, cx, status)
    {
      dumpn("*** HTTPTestLoader.onStopRequest for " + this._path);

      var ch = request.QueryInterface(Ci.nsIHttpChannel)
                      .QueryInterface(Ci.nsIHttpChannelInternal);

      this._stop(ch, cx, status, this._data);
    },
    QueryInterface: function(aIID)
    {
      dumpn("*** QueryInterface: " + aIID);

      if (aIID.equals(Ci.nsIStreamListener) ||
          aIID.equals(Ci.nsIRequestObserver) ||
          aIID.equals(Ci.nsISupports))
        return this;
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
