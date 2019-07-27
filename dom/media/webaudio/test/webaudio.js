

function expectException(func, exceptionCode) {
  var threw = false;
  try {
    func();
  } catch (ex) {
    threw = true;
    ok(ex instanceof DOMException, "Expect a DOM exception");
    is(ex.code, exceptionCode, "Expect the correct exception code");
  }
  ok(threw, "The exception was thrown");
}

function expectNoException(func) {
  var threw = false;
  try {
    func();
  } catch (ex) {
    threw = true;
  }
  ok(!threw, "An exception was not thrown");
}

function expectTypeError(func) {
  var threw = false;
  try {
    func();
  } catch (ex) {
    threw = true;
    ok(ex instanceof TypeError, "Expect a TypeError");
  }
  ok(threw, "The exception was thrown");
}

function fuzzyCompare(a, b) {
  return Math.abs(a - b) < 9e-3;
}

function compareChannels(buf1, buf2,
                         length,
                         sourceOffset,
                         destOffset,
                         skipLengthCheck) {
  if (!skipLengthCheck) {
    is(buf1.length, buf2.length, "Channels must have the same length");
  }
  sourceOffset = sourceOffset || 0;
  destOffset = destOffset || 0;
  if (length == undefined) {
    length = buf1.length - sourceOffset;
  }
  var difference = 0;
  var maxDifference = 0;
  var firstBadIndex = -1;
  for (var i = 0; i < length; ++i) {
    if (!fuzzyCompare(buf1[i + sourceOffset], buf2[i + destOffset])) {
      difference++;
      maxDifference = Math.max(maxDifference, Math.abs(buf1[i + sourceOffset] - buf2[i + destOffset]));
      if (firstBadIndex == -1) {
        firstBadIndex = i;
      }
    }
  };

  is(difference, 0, "maxDifference: " + maxDifference +
     ", first bad index: " + firstBadIndex +
     " with test-data offset " + sourceOffset + " and expected-data offset " +
     destOffset + "; corresponding values " + buf1[firstBadIndex + sourceOffset] +
     " and " + buf2[firstBadIndex + destOffset] + " --- differences");
}

function compareBuffers(got, expected) {
  if (got.numberOfChannels != expected.numberOfChannels) {
    is(got.numberOfChannels, expected.numberOfChannels,
       "Correct number of buffer channels");
    return;
  }
  if (got.length != expected.length) {
    is(got.length, expected.length,
       "Correct buffer length");
    return;
  }
  if (got.sampleRate != expected.sampleRate) {
    is(got.sampleRate, expected.sampleRate,
       "Correct sample rate");
    return;
  }

  for (var i = 0; i < got.numberOfChannels; ++i) {
    compareChannels(got.getChannelData(i), expected.getChannelData(i),
                    got.length, 0, 0, true);
  }
}








function rms(audiobuffer, channel = 0, start = 0, end = audiobuffer.length) {
  var buffer= audiobuffer.getChannelData(channel);
  var rms = 0;
  for (var i = start; i < end; i++) {
    rms += buffer[i] * buffer[i];
  }

  rms /= buffer.length;
  rms = Math.sqrt(rms);
  return rms;
}

function getEmptyBuffer(context, length) {
  return context.createBuffer(gTest.numberOfChannels, length, context.sampleRate);
}

































function runTest()
{
  function done() {
    SimpleTest.finish();
  }

  SimpleTest.waitForExplicitFinish();
  function runTestFunction () {
    if (!gTest.numberOfChannels) {
      gTest.numberOfChannels = 2; 
    }

    var testLength;

    function runTestOnContext(context, callback, testOutput) {
      if (!gTest.createExpectedBuffers) {
        
        var expectedBuffers = getEmptyBuffer(context, gTest.length);
      } else {
        var expectedBuffers = gTest.createExpectedBuffers(context);
      }
      if (!(expectedBuffers instanceof Array)) {
        expectedBuffers = [expectedBuffers];
      }
      var expectedFrames = 0;
      for (var i = 0; i < expectedBuffers.length; ++i) {
        is(expectedBuffers[i].numberOfChannels, gTest.numberOfChannels,
           "Correct number of channels for expected buffer " + i);
        expectedFrames += expectedBuffers[i].length;
      }
      if (gTest.length && gTest.createExpectedBuffers) {
        is(expectedFrames, gTest.length, "Correct number of expected frames");
      }

      if (gTest.createGraphAsync) {
        gTest.createGraphAsync(context, function(nodeToInspect) {
          testOutput(nodeToInspect, expectedBuffers, callback);
        });
      } else {
        testOutput(gTest.createGraph(context), expectedBuffers, callback);
      }
    }

    function testOnNormalContext(callback) {
      function testOutput(nodeToInspect, expectedBuffers, callback) {
        testLength = 0;
        var sp = context.createScriptProcessor(expectedBuffers[0].length, gTest.numberOfChannels);
        nodeToInspect.connect(sp);
        sp.connect(context.destination);
        sp.onaudioprocess = function(e) {
          var expectedBuffer = expectedBuffers.shift();
          testLength += expectedBuffer.length;
          compareBuffers(e.inputBuffer, expectedBuffer);
          if (expectedBuffers.length == 0) {
            sp.onaudioprocess = null;
            callback();
          }
        };
      }
      var context = new AudioContext();
      runTestOnContext(context, callback, testOutput);
    }

    function testOnOfflineContext(callback, sampleRate) {
      function testOutput(nodeToInspect, expectedBuffers, callback) {
        nodeToInspect.connect(context.destination);
        context.oncomplete = function(e) {
          var samplesSeen = 0;
          while (expectedBuffers.length) {
            var expectedBuffer = expectedBuffers.shift();
            is(e.renderedBuffer.numberOfChannels, expectedBuffer.numberOfChannels,
               "Correct number of input buffer channels");
            for (var i = 0; i < e.renderedBuffer.numberOfChannels; ++i) {
              compareChannels(e.renderedBuffer.getChannelData(i),
                             expectedBuffer.getChannelData(i),
                             expectedBuffer.length,
                             samplesSeen,
                             undefined,
                             true);
            }
            samplesSeen += expectedBuffer.length;
          }
          callback();
        };
        context.startRendering();
      }

      var context = new OfflineAudioContext(gTest.numberOfChannels, testLength, sampleRate);
      runTestOnContext(context, callback, testOutput);
    }

    testOnNormalContext(function() {
      if (!gTest.skipOfflineContextTests) {
        testOnOfflineContext(function() {
          testOnOfflineContext(done, 44100);
        }, 48000);
      } else {
        done();
      }
    });
  };

  if (document.readyState !== 'complete') {
    addLoadEvent(runTestFunction);
  } else {
    runTestFunction();
  }
}
