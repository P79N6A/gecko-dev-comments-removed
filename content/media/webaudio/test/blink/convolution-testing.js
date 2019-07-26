var sampleRate = 44100.0;

var renderLengthSeconds = 8;
var pulseLengthSeconds = 1;
var pulseLengthFrames = pulseLengthSeconds * sampleRate;

function createSquarePulseBuffer(context, sampleFrameLength) {
    var audioBuffer = context.createBuffer(1, sampleFrameLength, context.sampleRate);

    var n = audioBuffer.length;
    var data = audioBuffer.getChannelData(0);

    for (var i = 0; i < n; ++i)
        data[i] = 1;

    return audioBuffer;
}





function createTrianglePulseBuffer(context, sampleFrameLength) {
    var audioBuffer = context.createBuffer(1, sampleFrameLength, context.sampleRate);

    var n = audioBuffer.length;
    var halfLength = n / 2;
    var data = audioBuffer.getChannelData(0);
    
    for (var i = 0; i < halfLength; ++i)
        data[i] = i + 1;

    for (var i = halfLength; i < n; ++i)
        data[i] = n - i - 1;

    return audioBuffer;
}

function log10(x) {
  return Math.log(x)/Math.LN10;
}

function linearToDecibel(x) {
  return 20*log10(x);
}



function checkTriangularPulse(rendered, reference) {
    var match = true;
    var maxDelta = 0;
    var valueAtMaxDelta = 0;
    var maxDeltaIndex = 0;

    for (var i = 0; i < reference.length; ++i) {
        var diff = rendered[i] - reference[i];
        var x = Math.abs(diff);
        if (x > maxDelta) {
            maxDelta = x;
            valueAtMaxDelta = reference[i];
            maxDeltaIndex = i;
        }
    }

    
    
    
    
    var allowedDeviationDecibels = -129.4;
    var maxDeviationDecibels = linearToDecibel(maxDelta / valueAtMaxDelta);

    if (maxDeviationDecibels <= allowedDeviationDecibels) {
        testPassed("Triangular portion of convolution is correct.");
    } else {
        testFailed("Triangular portion of convolution is not correct.  Max deviation = " + maxDeviationDecibels + " dB at " + maxDeltaIndex);
        match = false;
    }

    return match;
}        



function checkTail1(data, reference, breakpoint) {
    var isZero = true;
    var tail1Max = 0;

    for (var i = reference.length; i < reference.length + breakpoint; ++i) {
        var mag = Math.abs(data[i]);
        if (mag > tail1Max) {
            tail1Max = mag;
        }
    }

    
    
    var refMax = 0;
    for (var i = 0; i < reference.length; ++i) {
      refMax = Math.max(refMax, Math.abs(reference[i]));
    }

    
    
    var threshold1 = -129.7;

    var tail1MaxDecibels = linearToDecibel(tail1Max/refMax);
    if (tail1MaxDecibels <= threshold1) {
        testPassed("First part of tail of convolution is sufficiently small.");
    } else {
        testFailed("First part of tail of convolution is not sufficiently small: " + tail1MaxDecibels + " dB");
        isZero = false;
    }

    return isZero;
}



function checkTail2(data, reference, breakpoint) {
    var isZero = true;
    var tail2Max = 0;
    
    
    var threshold2 = 0;
    for (var i = reference.length + breakpoint; i < data.length; ++i) {
        if (Math.abs(data[i]) > 0) {
            isZero = false; 
            break;
        }
    }

    if (isZero) {
        testPassed("Rendered signal after tail of convolution is silent.");
    } else {
        testFailed("Rendered signal after tail of convolution should be silent.");
    }

    return isZero;
}

function checkConvolvedResult(trianglePulse) {
    return function(event) {
        var renderedBuffer = event.renderedBuffer;

        var referenceData = trianglePulse.getChannelData(0);
        var renderedData = renderedBuffer.getChannelData(0);
    
        var success = true;
    
        

        success = success && checkTriangularPulse(renderedData, referenceData);
        
        
        
        
        
        

        
        
        
        
        
        
        
        
        var breakpoint = 12800;

        success = success && checkTail1(renderedData, referenceData, breakpoint);
        
        success = success && checkTail2(renderedData, referenceData, breakpoint);
        
        if (success) {
            testPassed("Test signal was correctly convolved.");
        } else {
            testFailed("Test signal was not correctly convolved.");
        }

        finishJSTest();
    }
}
