


















































gThreadManager = Cc["@mozilla.org/thread-manager;1"].createInstance();

function run_test()
{
  do_test_pending();
  tests.push(function testsComplete(_)
  {
    dumpn("******************\n" +
          "* TESTS COMPLETE *\n" +
          "******************");
    do_test_finished();
  });

  runNextTest();
}

function runNextTest()
{
  testIndex++;
  dumpn("*** runNextTest(), testIndex: " + testIndex);

  try
  {
    var test = tests[testIndex];
    test(runNextTest);
  }
  catch (e)
  {
    var msg = "exception running test " + testIndex + ": " + e;
    if (e && "stack" in e)
      msg += "\nstack follows:\n" + e.stack;
    do_throw(msg);
  }
}






const NOTHING = [];

const FIRST_SEGMENT = [1, 2, 3, 4];
const SECOND_SEGMENT = [5, 6, 7, 8];
const THIRD_SEGMENT = [9, 10, 11, 12];

const SEGMENT = FIRST_SEGMENT;
const TWO_SEGMENTS = [1, 2, 3, 4, 5, 6, 7, 8];
const THREE_SEGMENTS = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];

const SEGMENT_AND_HALF = [1, 2, 3, 4, 5, 6];

const QUARTER_SEGMENT = [1];
const HALF_SEGMENT = [1, 2];
const SECOND_HALF_SEGMENT = [3, 4];
const THREE_QUARTER_SEGMENT = [1, 2, 3];
const EXTRA_HALF_SEGMENT = [5, 6];
const MIDDLE_HALF_SEGMENT = [2, 3];
const LAST_QUARTER_SEGMENT = [4];
const FOURTH_HALF_SEGMENT = [7, 8];
const HALF_THIRD_SEGMENT = [9, 10];
const LATTER_HALF_THIRD_SEGMENT = [11, 12];

const TWO_HALF_SEGMENTS = [1, 2, 1, 2];






var tests =
  [
   sourceClosedWithoutWrite,
   writeOneSegmentThenClose,
   simpleWriteThenRead,
   writeLittleBeforeReading,
   writeMultipleSegmentsThenRead,
   writeLotsBeforeReading,
   writeLotsBeforeReading2,
   writeThenReadPartial,
   manyPartialWrites,
   partialRead,
   partialWrite,
   sinkClosedImmediately,
   sinkClosedWithReadableData,
   sinkClosedAfterWrite,
   sourceAndSinkClosed,
   sinkAndSourceClosed,
   sourceAndSinkClosedWithPendingData,
   sinkAndSourceClosedWithPendingData,
  ];
var testIndex = -1;

function sourceClosedWithoutWrite(next)
{
  var t = new CopyTest("sourceClosedWithoutWrite", next);

  t.closeSource(Cr.NS_OK);
  t.expect(Cr.NS_OK, [NOTHING]);
}

function writeOneSegmentThenClose(next)
{
  var t = new CopyTest("writeLittleBeforeReading", next);

  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.closeSource(Cr.NS_OK);
  t.makeSinkWritableAndWaitFor(SEGMENT.length, [SEGMENT]);
  t.expect(Cr.NS_OK, [SEGMENT]);
}

function simpleWriteThenRead(next)
{
  var t = new CopyTest("simpleWriteThenRead", next);

  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.makeSinkWritableAndWaitFor(SEGMENT.length, [SEGMENT]);
  t.closeSource(Cr.NS_OK);
  t.expect(Cr.NS_OK, [SEGMENT]);
}

function writeLittleBeforeReading(next)
{
  var t = new CopyTest("writeLittleBeforeReading", next);

  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.closeSource(Cr.NS_OK);
  t.makeSinkWritableAndWaitFor(SEGMENT.length, [SEGMENT]);
  t.makeSinkWritableAndWaitFor(SEGMENT.length, [SEGMENT]);
  t.expect(Cr.NS_OK, [SEGMENT, SEGMENT]);
}

function writeMultipleSegmentsThenRead(next)
{
  var t = new CopyTest("writeMultipleSegmentsThenRead", next);

  t.addToSource(TWO_SEGMENTS);
  t.makeSourceReadable(TWO_SEGMENTS.length);
  t.makeSinkWritableAndWaitFor(TWO_SEGMENTS.length,
                               [FIRST_SEGMENT, SECOND_SEGMENT]);
  t.closeSource(Cr.NS_OK);
  t.expect(Cr.NS_OK, [TWO_SEGMENTS]);
}

function writeLotsBeforeReading(next)
{
  var t = new CopyTest("writeLotsBeforeReading", next);

  t.addToSource(TWO_SEGMENTS);
  t.makeSourceReadable(TWO_SEGMENTS.length);
  t.makeSinkWritableAndWaitFor(FIRST_SEGMENT.length, [FIRST_SEGMENT]);
  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.makeSinkWritableAndWaitFor(SECOND_SEGMENT.length, [SECOND_SEGMENT]);
  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.closeSource(Cr.NS_OK);
  t.makeSinkWritableAndWaitFor(2 * SEGMENT.length, [SEGMENT, SEGMENT]);
  t.expect(Cr.NS_OK, [TWO_SEGMENTS, SEGMENT, SEGMENT]);
}

function writeLotsBeforeReading2(next)
{
  var t = new CopyTest("writeLotsBeforeReading", next);

  t.addToSource(THREE_SEGMENTS);
  t.makeSourceReadable(THREE_SEGMENTS.length);
  t.makeSinkWritableAndWaitFor(FIRST_SEGMENT.length, [FIRST_SEGMENT]);
  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.makeSinkWritableAndWaitFor(SECOND_SEGMENT.length, [SECOND_SEGMENT]);
  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.makeSinkWritableAndWaitFor(THIRD_SEGMENT.length, [THIRD_SEGMENT]);
  t.closeSource(Cr.NS_OK);
  t.makeSinkWritableAndWaitFor(2 * SEGMENT.length, [SEGMENT, SEGMENT]);
  t.expect(Cr.NS_OK, [THREE_SEGMENTS, SEGMENT, SEGMENT]);
}

function writeThenReadPartial(next)
{
  var t = new CopyTest("writeThenReadPartial", next);

  t.addToSource(SEGMENT_AND_HALF);
  t.makeSourceReadable(SEGMENT_AND_HALF.length);
  t.makeSinkWritableAndWaitFor(SEGMENT.length, [SEGMENT]);
  t.closeSource(Cr.NS_OK);
  t.makeSinkWritableAndWaitFor(EXTRA_HALF_SEGMENT.length, [EXTRA_HALF_SEGMENT]);
  t.expect(Cr.NS_OK, [SEGMENT_AND_HALF]);
}

function manyPartialWrites(next)
{
  var t = new CopyTest("manyPartialWrites", next);

  t.addToSource(HALF_SEGMENT);
  t.makeSourceReadable(HALF_SEGMENT.length);

  t.addToSource(HALF_SEGMENT);
  t.makeSourceReadable(HALF_SEGMENT.length);
  t.makeSinkWritableAndWaitFor(2 * HALF_SEGMENT.length, [TWO_HALF_SEGMENTS]);
  t.closeSource(Cr.NS_OK);
  t.expect(Cr.NS_OK, [TWO_HALF_SEGMENTS]);
}

function partialRead(next)
{
  var t = new CopyTest("partialRead", next);

  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.addToSource(HALF_SEGMENT);
  t.makeSourceReadable(HALF_SEGMENT.length);
  t.makeSinkWritableAndWaitFor(SEGMENT.length, [SEGMENT]);
  t.closeSourceAndWaitFor(Cr.NS_OK, HALF_SEGMENT.length, [HALF_SEGMENT]);
  t.expect(Cr.NS_OK, [SEGMENT, HALF_SEGMENT]);
}

function partialWrite(next)
{
  var t = new CopyTest("partialWrite", next);

  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.makeSinkWritableByIncrementsAndWaitFor(SEGMENT.length,
                                           [QUARTER_SEGMENT,
                                            MIDDLE_HALF_SEGMENT,
                                            LAST_QUARTER_SEGMENT]);

  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.makeSinkWritableByIncrementsAndWaitFor(SEGMENT.length,
                                           [HALF_SEGMENT, SECOND_HALF_SEGMENT]);

  t.addToSource(THREE_SEGMENTS);
  t.makeSourceReadable(THREE_SEGMENTS.length);
  t.makeSinkWritableByIncrementsAndWaitFor(THREE_SEGMENTS.length,
                                           [HALF_SEGMENT, SECOND_HALF_SEGMENT,
                                            SECOND_SEGMENT,
                                            HALF_THIRD_SEGMENT,
                                            LATTER_HALF_THIRD_SEGMENT]);

  t.closeSource(Cr.NS_OK);
  t.expect(Cr.NS_OK, [SEGMENT, SEGMENT, THREE_SEGMENTS]);
}

function sinkClosedImmediately(next)
{
  var t = new CopyTest("sinkClosedImmediately", next);

  t.closeSink(Cr.NS_OK);
  t.expect(Cr.NS_ERROR_UNEXPECTED, [NOTHING]);
}

function sinkClosedWithReadableData(next)
{
  var t = new CopyTest("sinkClosedWithReadableData", next);

  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);
  t.closeSink(Cr.NS_OK);
  t.expect(Cr.NS_ERROR_UNEXPECTED, [NOTHING]);
}

function sinkClosedAfterWrite(next)
{
  var t = new CopyTest("sinkClosedAfterWrite", next);

  t.addToSource(TWO_SEGMENTS);
  t.makeSourceReadable(TWO_SEGMENTS.length);
  t.makeSinkWritableAndWaitFor(FIRST_SEGMENT.length, [FIRST_SEGMENT]);
  t.closeSink(Cr.NS_OK);
  t.expect(Cr.NS_ERROR_UNEXPECTED, [FIRST_SEGMENT]);
}

function sourceAndSinkClosed(next)
{
  var t = new CopyTest("sourceAndSinkClosed", next);

  t.closeSourceThenSink(Cr.NS_OK, Cr.NS_OK);
  t.expect(Cr.NS_OK, []);
}

function sinkAndSourceClosed(next)
{
  var t = new CopyTest("sinkAndSourceClosed", next);

  t.closeSinkThenSource(Cr.NS_OK, Cr.NS_OK);

  
  t.expect(Cr.NS_ERROR_UNEXPECTED, []);
}

function sourceAndSinkClosedWithPendingData(next)
{
  var t = new CopyTest("sourceAndSinkClosedWithPendingData", next);

  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);

  t.closeSourceThenSink(Cr.NS_OK, Cr.NS_OK);

  
  t.expect(Cr.NS_ERROR_UNEXPECTED, []);
}

function sinkAndSourceClosedWithPendingData(next)
{
  var t = new CopyTest("sinkAndSourceClosedWithPendingData", next);

  t.addToSource(SEGMENT);
  t.makeSourceReadable(SEGMENT.length);

  t.closeSinkThenSource(Cr.NS_OK, Cr.NS_OK);

  
  t.expect(Cr.NS_ERROR_UNEXPECTED, []);
}







function sum(arr)
{
  var sum = 0;
  for (var i = 0, sz = arr.length; i < sz; i++)
    sum += arr[i];
  return sum;
}















function createStreamReadyInterceptor(wrapperCallback, name)
{
  return function StreamReadyInterceptor(callback)
  {
    this.wrappedCallback = callback;
    this[name] = function streamReadyInterceptor(stream)
    {
      dumpn("*** StreamReadyInterceptor." + name);

      try
      {
        dumpn("*** calling original " + name + "...");
        callback[name](stream);
      }
      catch (e)
      {
        dumpn("!!! error running inner callback: " + e);
        throw e;
      }
      finally
      {
        dumpn("*** calling wrapper " + name + "...");
        wrapperCallback[name](stream);
      }
    }
  };
}





function note(m)
{
  m = m.toUpperCase();
  var asterisks = Array(m.length + 1 + 4).join("*");
  dumpn(asterisks + "\n* " + m + " *\n" + asterisks);
}











var BinaryInputStream = function BIS(stream) { return stream; };
var BinaryOutputStream = function BOS(stream) { return stream; };
Response.SEGMENT_SIZE = SEGMENT.length;











function CustomPipe(name)
{
  var self = this;

  
  this._data = [];

  





  this._status = Cr.NS_OK;

  
  var input = this.inputStream =
    {
      
      name: name + " input",

      




      _readable: 0,

      



      _waiter: null,

      




      _event: null,

      




      _streamReadyInterceptCreator: null,

      



      interceptStreamReadyCallbacks: function(streamReadyInterceptCreator)
      {
        dumpn("*** [" + this.name + "].interceptStreamReadyCallbacks");

        do_check_true(this._streamReadyInterceptCreator === null,
                      "intercepting twice");
        this._streamReadyInterceptCreator = streamReadyInterceptCreator;
        if (this._waiter)
        {
          this._waiter.callback =
            new streamReadyInterceptCreator(this._waiter.callback);
        }
      },

      



      removeStreamReadyInterceptor: function()
      {
        dumpn("*** [" + this.name + "].removeStreamReadyInterceptor()");

        do_check_true(this._streamReadyInterceptCreator !== null,
                      "removing interceptor when none present?");
        this._streamReadyInterceptCreator = null;
        if (this._waiter)
          this._waiter.callback = this._waiter.callback.wrappedCallback;
      },

      
      
      
      asyncWait: function asyncWait(callback, flags, requestedCount, target)
      {
        dumpn("*** [" + this.name + "].asyncWait");

        do_check_true(callback && typeof callback !== "function");

        var closureOnly =
          (flags & Ci.nsIAsyncInputStream.WAIT_CLOSURE_ONLY) !== 0;

        do_check_true(this._waiter === null ||
                      (this._waiter.closureOnly && !closureOnly),
                      "asyncWait already called with a non-closure-only " +
                      "callback?  unexpected!");

        this._waiter =
          {
            callback:
              this._streamReadyInterceptCreator
              ? new this._streamReadyInterceptCreator(callback)
              : callback,
            closureOnly: closureOnly,
            requestedCount: requestedCount,
            eventTarget: target
          };

        if (!Components.isSuccessCode(self._status) ||
            (!closureOnly && this._readable >= requestedCount &&
             self._data.length >= requestedCount))
        {
          this._notify();
        }
      },

      
      
      
      closeWithStatus: function closeWithStatus(status)
      {
        dumpn("*** [" + this.name + "].closeWithStatus" +
              "(" + status + ")");

        if (!Components.isSuccessCode(self._status))
        {
          dumpn("*** ignoring second closure of [input " + this.name + "] " +
                "(status " + self._status + ")");
          return;
        }

        if (Components.isSuccessCode(status))
          status = Cr.NS_BASE_STREAM_CLOSED;

        self._status = status;

        if (this._waiter)
          this._notify();
        if (output._waiter)
          output._notify();
      },

      
      
      
      readByteArray: function readByteArray(count)
      {
        dumpn("*** [" + this.name + "].readByteArray(" + count + ")");

        if (self._data.length === 0)
        {
          throw Components.isSuccessCode(self._status)
              ? Cr.NS_BASE_STREAM_WOULD_BLOCK
              : self._status;
        }

        do_check_true(this._readable <= self._data.length ||
                      this._readable === Infinity,
                      "consistency check");

        if (this._readable < count || self._data.length < count)
          throw Cr.NS_BASE_STREAM_WOULD_BLOCK;
        this._readable -= count;
        return self._data.splice(0, count);
      },

      









      makeReadable: function makeReadable(count)
      {
        dumpn("*** [" + this.name + "].makeReadable(" + count + ")");

        do_check_true(Components.isSuccessCode(self._status), "errant call");
        do_check_true(this._readable + count <= self._data.length ||
                      this._readable === Infinity,
                      "increasing readable beyond written amount");

        this._readable += count;

        dumpn("readable: " + this._readable + ", data: " + self._data);

        var waiter = this._waiter;
        if (waiter !== null)
        {
          if (waiter.requestedCount <= this._readable && !waiter.closureOnly)
            this._notify();
        }
      },

      





      disableReadabilityLimit: function disableReadabilityLimit()
      {
        dumpn("*** [" + this.name + "].disableReadabilityLimit()");

        this._readable = Infinity;
      },

      
      
      
      available: function available()
      {
        dumpn("*** [" + this.name + "].available()");

        if (self._data.length === 0 && !Components.isSuccessCode(self._status))
          throw self._status;

        return Math.min(this._readable, self._data.length);
      },

      










      maybeNotifyFinally: function maybeNotifyFinally()
      {
        dumpn("*** [" + this.name + "].maybeNotifyFinally()");

        do_check_true(this._waiter !== null, "must be waiting now");

        if (self._data.length > 0)
        {
          dumpn("*** data still pending, normal notifications will signal " +
                "completion");
          return;
        }

        
        
        
        
        
        
        
        this._notify();
      },

      



      _notify: function _notify()
      {
        dumpn("*** [" + this.name + "]._notify()");

        var waiter = this._waiter;
        do_check_true(waiter !== null, "no waiter?");

        if (this._event === null)
        {
          var event = this._event =
            {
              run: function run()
              {
                input._waiter = null;
                input._event = null;
                try
                {
                  do_check_true(!Components.isSuccessCode(self._status) ||
                                input._readable >= waiter.requestedCount);
                  waiter.callback.onInputStreamReady(input);
                }
                catch (e)
                {
                  do_throw("error calling onInputStreamReady: " + e);
                }
              }
            };
          waiter.eventTarget.dispatch(event, Ci.nsIThread.DISPATCH_NORMAL);
        }
      },

      QueryInterface: function QueryInterface(iid)
      {
        if (iid.equals(Ci.nsIAsyncInputStream) ||
            iid.equals(Ci.nsIInputStream) ||
            iid.equals(Ci.nsISupports))
        {
          return this;
        }

        throw Cr.NS_ERROR_NO_INTERFACE;
      }
    };

  
  var output = this.outputStream =
    {
      
      name: name + " output",

      



      _writable: 0,

      









      _writableAmounts: [],

      



      _waiter: null,

      




      _event: null,

      




      _streamReadyInterceptCreator: null,

      



      interceptStreamReadyCallbacks: function(streamReadyInterceptCreator)
      {
        dumpn("*** [" + this.name + "].interceptStreamReadyCallbacks");

        do_check_true(this._streamReadyInterceptCreator !== null,
                      "intercepting onOutputStreamReady twice");
        this._streamReadyInterceptCreator = streamReadyInterceptCreator;
        if (this._waiter)
        {
          this._waiter.callback =
            new streamReadyInterceptCreator(this._waiter.callback);
        }
      },

      



      removeStreamReadyInterceptor: function()
      {
        dumpn("*** [" + this.name + "].removeStreamReadyInterceptor()");

        do_check_true(this._streamReadyInterceptCreator !== null,
                      "removing interceptor when none present?");
        this._streamReadyInterceptCreator = null;
        if (this._waiter)
          this._waiter.callback = this._waiter.callback.wrappedCallback;
      },

      
      
      
      asyncWait: function asyncWait(callback, flags, requestedCount, target)
      {
        dumpn("*** [" + this.name + "].asyncWait");

        do_check_true(callback && typeof callback !== "function");

        var closureOnly =
          (flags & Ci.nsIAsyncInputStream.WAIT_CLOSURE_ONLY) !== 0;

        do_check_true(this._waiter === null ||
                      (this._waiter.closureOnly && !closureOnly),
                      "asyncWait already called with a non-closure-only " +
                      "callback?  unexpected!");

        this._waiter =
          {
            callback:
              this._streamReadyInterceptCreator
              ? new this._streamReadyInterceptCreator(callback)
              : callback,
            closureOnly: closureOnly,
            requestedCount: requestedCount,
            eventTarget: target,
            toString: function toString()
            {
              return "waiter(" + (closureOnly ? "closure only, " : "") +
                      "requestedCount: " + requestedCount + ", target: " +
                      target + ")";
            }
          };

        if ((!closureOnly && this._writable >= requestedCount) ||
            !Components.isSuccessCode(this.status))
        {
          this._notify();
        }
      },

      
      
      
      closeWithStatus: function closeWithStatus(status)
      {
        dumpn("*** [" + this.name + "].closeWithStatus(" + status + ")");

        if (!Components.isSuccessCode(self._status))
        {
          dumpn("*** ignoring redundant closure of [input " + this.name + "] " +
                "because it's already closed (status " + self._status + ")");
          return;
        }

        if (Components.isSuccessCode(status))
          status = Cr.NS_BASE_STREAM_CLOSED;

        self._status = status;

        if (input._waiter)
          input._notify();
        if (this._waiter)
          this._notify();
      },

      
      
      
      writeByteArray: function writeByteArray(bytes, length)
      {
        dumpn("*** [" + this.name + "].writeByteArray" +
              "([" + bytes + "], " + length + ")");

        do_check_eq(bytes.length, length, "sanity");
        if (!Components.isSuccessCode(self._status))
          throw self._status;

        do_check_eq(this._writableAmounts.length, 0,
                    "writeByteArray can't support specified-length writes");

        if (this._writable < length)
          throw Cr.NS_BASE_STREAM_WOULD_BLOCK;

        self._data.push.apply(self._data, bytes);
        this._writable -= length;

        if (input._readable === Infinity && input._waiter &&
            !input._waiter.closureOnly)
        {
          input._notify();
        }
      },

      
      
      
      write: function write(str, length)
      {
        dumpn("*** [" + this.name + "].write");

        do_check_eq(str.length, length, "sanity");
        if (!Components.isSuccessCode(self._status))
          throw self._status;
        if (this._writable === 0)
          throw Cr.NS_BASE_STREAM_WOULD_BLOCK;

        var actualWritten;
        if (this._writableAmounts.length === 0)
        {
          actualWritten = Math.min(this._writable, length);
        }
        else
        {
          do_check_true(this._writable >= this._writableAmounts[0],
                        "writable amounts value greater than writable data?");
          do_check_eq(this._writable, sum(this._writableAmounts),
                      "total writable amount not equal to sum of writable " +
                      "increments");
          actualWritten = this._writableAmounts.shift();
        }

        var bytes = str.substring(0, actualWritten)
                       .split("")
                       .map(function(v) { return v.charCodeAt(0); });

        self._data.push.apply(self._data, bytes);
        this._writable -= actualWritten;

        if (input._readable === Infinity && input._waiter &&
            !input._waiter.closureOnly)
        {
          input._notify();
        }

        return actualWritten;
      },

      






      makeWritable: function makeWritable(count)
      {
        dumpn("*** [" + this.name + "].makeWritable(" + count + ")");

        do_check_true(Components.isSuccessCode(self._status));

        this._writable += count;

        var waiter = this._waiter;
        if (waiter && !waiter.closureOnly &&
            waiter.requestedCount <= this._writable)
        {
          this._notify();
        }
      },

      



















      makeWritableByIncrements: function makeWritableByIncrements(increments)
      {
        dumpn("*** [" + this.name + "].makeWritableByIncrements" +
              "([" + increments.join(", ") + "])");

        do_check_true(increments.length > 0, "bad increments");
        do_check_true(increments.every(function(v) { return v > 0; }),
                      "zero increment?");

        do_check_true(Components.isSuccessCode(self._status));

        this._writable += sum(increments);
        this._writableAmounts = increments;

        var waiter = this._waiter;
        if (waiter && !waiter.closureOnly &&
            waiter.requestedCount <= this._writable)
        {
          this._notify();
        }
      },

      



      _notify: function _notify()
      {
        dumpn("*** [" + this.name + "]._notify()");

        var waiter = this._waiter;
        do_check_true(waiter !== null, "no waiter?");

        if (this._event === null)
        {
          var event = this._event =
            {
              run: function run()
              {
                output._waiter = null;
                output._event = null;

                try
                {
                  waiter.callback.onOutputStreamReady(output);
                }
                catch (e)
                {
                  do_throw("error calling onOutputStreamReady: " + e);
                }
              }
            };
          waiter.eventTarget.dispatch(event, Ci.nsIThread.DISPATCH_NORMAL);
        }
      },

      QueryInterface: function QueryInterface(iid)
      {
        if (iid.equals(Ci.nsIAsyncOutputStream) ||
            iid.equals(Ci.nsIOutputStream) ||
            iid.equals(Ci.nsISupports))
        {
          return this;
        }

        throw Cr.NS_ERROR_NO_INTERFACE;
      }
    };
}








function CopyTest(name, next)
{
  
  this.name = name;

  
  this._done = next;

  var sourcePipe = new CustomPipe(name + "-source");

  
  this._source = sourcePipe.inputStream;

  


  this._copyableDataStream = sourcePipe.outputStream;

  var sinkPipe = new CustomPipe(name + "-sink");

  
  this._sink = sinkPipe.outputStream;

  
  this._copiedDataStream = sinkPipe.inputStream;

  this._copiedDataStream.disableReadabilityLimit();

  



  this._waitingForData = false;

  



  this._expectedData = undefined;

  
  this._receivedData = [];

  
  this._expectedStatus = -1;

  
  this._actualStatus = -1;

  
  this._lastQuantum = [];

  



  this._allDataWritten = false;

  



  this._copyingFinished = false;

  
  this._currentTask = 0;

  
  this._tasks = [];

  
  this._copier =
    new WriteThroughCopier(this._source, this._sink, this, null);

  
  this._waitForWrittenData();
}
CopyTest.prototype =
{
  





  addToSource: function addToSource(bytes)
  {
    var self = this;
    this._addToTasks(function addToSourceTask()
    {
      note("addToSourceTask");

      try
      {
        self._copyableDataStream.makeWritable(bytes.length);
        self._copyableDataStream.writeByteArray(bytes, bytes.length);
      }
      finally
      {
        self._stageNextTask();
      }
    });
  },

  






  makeSourceReadable: function makeSourceReadable(count)
  {
    var self = this;
    this._addToTasks(function makeSourceReadableTask()
    {
      note("makeSourceReadableTask");

      self._source.makeReadable(count);
      self._stageNextTask();
    });
  },

  










  makeSinkWritableAndWaitFor:
  function makeSinkWritableAndWaitFor(bytes, dataQuantums)
  {
    var self = this;

    do_check_eq(bytes,
                dataQuantums.reduce(function(partial, current)
                {
                  return partial + current.length;
                }, 0),
                "bytes/quantums mismatch");

    function increaseSinkSpaceTask()
    {
      
      self._sink.makeWritable(bytes);
    }

    this._waitForHelper("increaseSinkSpaceTask",
                        dataQuantums, increaseSinkSpaceTask);
  },

  










  makeSinkWritableByIncrementsAndWaitFor:
  function makeSinkWritableByIncrementsAndWaitFor(bytes, dataQuantums)
  {
    var self = this;

    var desiredAmounts = dataQuantums.map(function(v) { return v.length; });
    do_check_eq(bytes, sum(desiredAmounts), "bytes/quantums mismatch");

    function increaseSinkSpaceByIncrementsTask()
    {
      
      self._sink.makeWritableByIncrements(desiredAmounts);
    }

    this._waitForHelper("increaseSinkSpaceByIncrementsTask",
                        dataQuantums, increaseSinkSpaceByIncrementsTask);
  },

  






  closeSource: function closeSource(status)
  {
    var self = this;

    this._addToTasks(function closeSourceTask()
    {
      note("closeSourceTask");

      self._source.closeWithStatus(status);
      self._stageNextTask();
    });
  },

  











  closeSourceAndWaitFor:
  function closeSourceAndWaitFor(status, bytes, dataQuantums)
  {
    var self = this;

    do_check_eq(bytes, sum(dataQuantums.map(function(v) { return v.length; })),
                "bytes/quantums mismatch");

    function closeSourceAndWaitForTask()
    {
      self._sink.makeWritable(bytes);
      self._copyableDataStream.closeWithStatus(status);
    }

    this._waitForHelper("closeSourceAndWaitForTask",
                        dataQuantums, closeSourceAndWaitForTask);
  },

  






  closeSink: function closeSink(status)
  {
    var self = this;
    this._addToTasks(function closeSinkTask()
    {
      note("closeSinkTask");

      self._sink.closeWithStatus(status);
      self._stageNextTask();
    });
  },

  








  closeSourceThenSink: function closeSourceThenSink(sourceStatus, sinkStatus)
  {
    var self = this;
    this._addToTasks(function closeSourceThenSinkTask()
    {
      note("closeSourceThenSinkTask");

      self._source.closeWithStatus(sourceStatus);
      self._sink.closeWithStatus(sinkStatus);
      self._stageNextTask();
    });
  },

  








  closeSinkThenSource: function closeSinkThenSource(sinkStatus, sourceStatus)
  {
    var self = this;
    this._addToTasks(function closeSinkThenSourceTask()
    {
      note("closeSinkThenSource");

      self._sink.closeWithStatus(sinkStatus);
      self._source.closeWithStatus(sourceStatus);
      self._stageNextTask();
    });
  },

  











  expect: function expect(expectedStatus, receivedData)
  {
    this._expectedStatus = expectedStatus;
    this._expectedData = [];
    for (var i = 0, sz = receivedData.length; i < sz; i++)
      this._expectedData.push.apply(this._expectedData, receivedData[i]);

    this._stageNextTask();
  },

  















  _waitForHelper: function _waitForHelper(name, dataQuantums, trigger)
  {
    var self = this;
    this._addToTasks(function waitForHelperTask()
    {
      note(name);

      var quantumIndex = 0;

      



      var streamReadyCallback =
        {
          onInputStreamReady: function wrapperOnInputStreamReady(input)
          {
            dumpn("*** streamReadyCallback.onInputStreamReady" +
                  "(" + input.name + ")");

            do_check_eq(this, streamReadyCallback, "sanity");

            try
            {
              if (quantumIndex < dataQuantums.length)
              {
                var quantum = dataQuantums[quantumIndex++];
                var sz = quantum.length;
                do_check_eq(self._lastQuantum.length, sz,
                            "different quantum lengths");
                for (var i = 0; i < sz; i++)
                {
                  do_check_eq(self._lastQuantum[i], quantum[i],
                              "bad data at " + i);
                }

                dumpn("*** waiting to check remaining " +
                      (dataQuantums.length - quantumIndex) + " quantums...");
              }
            }
            finally
            {
              if (quantumIndex === dataQuantums.length)
              {
                dumpn("*** data checks completed!  next task...");
                self._copiedDataStream.removeStreamReadyInterceptor();
                self._stageNextTask();
              }
            }
          }
        };

      var interceptor =
        createStreamReadyInterceptor(streamReadyCallback, "onInputStreamReady");
      self._copiedDataStream.interceptStreamReadyCallbacks(interceptor);

      
      trigger();
    });
  },

  








  _waitForWrittenData: function _waitForWrittenData()
  {
    dumpn("*** _waitForWrittenData (" + this.name + ")");

    var self = this;
    var outputWrittenWatcher =
      {
        onInputStreamReady: function onInputStreamReady(input)
        {
          dumpn("*** outputWrittenWatcher.onInputStreamReady" +
                "(" + input.name + ")");

          if (self._allDataWritten)
          {
            do_throw("ruh-roh!  why are we getting notified of more data " +
                     "after we should have received all of it?");
          }

          self._waitingForData = false;

          try
          {
            var avail = input.available();
          }
          catch (e)
          {
            dumpn("*** available() threw!  error: " + e);
            if (self._completed)
            {
              dumpn("*** NB: this isn't a problem, because we've copied " +
                    "completely now, and this notify may have been expedited " +
                    "by maybeNotifyFinally such that we're being called when " +
                    "we can *guarantee* nothing is available any more");
            }
            avail = 0;
          }

          if (avail > 0)
          {
            var data = input.readByteArray(avail);
            do_check_eq(data.length, avail,
                        "readByteArray returned wrong number of bytes?");
            self._lastQuantum = data;
            self._receivedData.push.apply(self._receivedData, data);
          }

          if (avail === 0)
          {
            dumpn("*** all data received!");

            self._allDataWritten = true;

            if (self._copyingFinished)
            {
              dumpn("*** copying already finished, continuing to next test");
              self._testComplete();
            }
            else
            {
              dumpn("*** copying not finished, waiting for that to happen");
            }

            return;
          }

          self._waitForWrittenData();
        }
      };

    this._copiedDataStream.asyncWait(outputWrittenWatcher, 0, 1,
                                 gThreadManager.currentThread);
    this._waitingForData = true;
  },

  




  _testComplete: function _testComplete()
  {
    dumpn("*** CopyTest(" + this.name + ") complete!  " +
          "On to the next test...");

    try
    {
      do_check_true(this._allDataWritten, "expect all data written now!");
      do_check_true(this._copyingFinished, "expect copying finished now!");

      do_check_eq(this._actualStatus, this._expectedStatus,
                  "wrong final status");

      var expected = this._expectedData, received = this._receivedData;
      dumpn("received: [" + received + "], expected: [" + expected + "]");
      do_check_eq(received.length, expected.length, "wrong data");
      for (var i = 0, sz = expected.length; i < sz; i++)
        do_check_eq(received[i], expected[i], "bad data at " + i);
    }
    catch (e)
    {
      dumpn("!!! ERROR PERFORMING FINAL " + this.name + " CHECKS!  " + e);
      throw e;
    }
    finally
    {
      dumpn("*** CopyTest(" + this.name + ") complete!  " +
            "Invoking test-completion callback...");
      this._done();
    }
  },

  
  _stageNextTask: function _stageNextTask()
  {
    dumpn("*** CopyTest(" + this.name + ")._stageNextTask()");

    if (this._currentTask === this._tasks.length)
    {
      dumpn("*** CopyTest(" + this.name + ") tasks complete!");
      return;
    }

    var task = this._tasks[this._currentTask++];
    var self = this;
    var event =
      {
        run: function run()
        {
          try
          {
            task();
          }
          catch (e)
          {
            do_throw("exception thrown running task: " + e);
          }
        }
      };
    gThreadManager.currentThread.dispatch(event, Ci.nsIThread.DISPATCH_NORMAL);
  },

  





  _addToTasks: function _addToTasks(task)
  {
    this._tasks.push(task);
  },

  
  
  
  onStartRequest: function onStartRequest(self, _)
  {
    dumpn("*** CopyTest.onStartRequest (" + self.name + ")");

    do_check_true(_ === null);
    do_check_eq(this._receivedData.length, 0);
    do_check_eq(this._lastQuantum.length, 0);
  },

  
  
  
  onStopRequest: function onStopRequest(self, _, status)
  {
    dumpn("*** CopyTest.onStopRequest (" + self.name + ", " + status + ")");

    do_check_true(_ === null);
    this._actualStatus = status;

    this._copyingFinished = true;

    if (this._allDataWritten)
    {
      dumpn("*** all data written, continuing with remaining tests...");
      this._testComplete();
    }
    else
    {
      










      dumpn("*** not all data copied, waiting for that to happen...");

      if (!this._waitingForData)
        this._waitForWrittenData();

      this._copiedDataStream.maybeNotifyFinally();
    }
  }
};
