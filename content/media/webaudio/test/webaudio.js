

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

function compareBuffers(buf1, buf2,
                         offset,
                         length,
                         sourceOffset,
                         destOffset) {
  is(buf1.length, buf2.length, "Buffers must have the same length");
  if (length == undefined) {
    length = buf1.length - (offset || 0);
  }
  sourceOffset = sourceOffset || 0;
  destOffset = destOffset || 0;
  var difference = 0;
  var maxDifference = 0;
  var firstBadIndex = -1;
  for (var i = offset || 0; i < Math.min(buf1.length, (offset || 0) + length); ++i) {
    if (!fuzzyCompare(buf1[i + sourceOffset], buf2[i + destOffset])) {
      difference++;
      maxDifference = Math.max(maxDifference, Math.abs(buf1[i + sourceOffset] - buf2[i + destOffset]));
      if (firstBadIndex == -1) {
        firstBadIndex = i;
      }
    }
  };

  is(difference, 0, "Found " + difference + " different samples, maxDifference: " +
     maxDifference + ", first bad index: " + firstBadIndex +
     " with source offset " + sourceOffset + " and desitnation offset " +
     destOffset);
}

function getEmptyBuffer(context, length) {
  return context.createBuffer(gTest.numberOfChannels, length, context.sampleRate);
}

























function runTest()
{
  function done() {
    SpecialPowers.clearUserPref("media.webaudio.enabled");
    SimpleTest.finish();
  }

  SimpleTest.waitForExplicitFinish();
  addLoadEvent(function() {
    SpecialPowers.setBoolPref("media.webaudio.enabled", true);

    if (!gTest.numberOfChannels) {
      gTest.numberOfChannels = 2; 
    }

    var context = new AudioContext();
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
    is(expectedFrames, gTest.length, "Correct number of expected frames");

    if (gTest.createGraphAsync) {
      gTest.createGraphAsync(context, function(nodeToInspect) {
        testOutput(nodeToInspect);
      });
    } else {
      testOutput(gTest.createGraph(context));
    }

    function testOutput(nodeToInspect) {
      var sp = context.createScriptProcessor(expectedBuffers[0].length, gTest.numberOfChannels);
      nodeToInspect.connect(sp);
      sp.connect(context.destination);
      sp.onaudioprocess = function(e) {
        var expectedBuffer = expectedBuffers.shift();
        is(e.inputBuffer.numberOfChannels, expectedBuffer.numberOfChannels,
           "Correct number of input buffer channels");
        for (var i = 0; i < e.inputBuffer.numberOfChannels; ++i) {
          compareBuffers(e.inputBuffer.getChannelData(i), expectedBuffer.getChannelData(i));
        }
        if (expectedBuffers.length == 0) {
          sp.onaudioprocess = null;
          done();
        }
      };
    }
  });
}
