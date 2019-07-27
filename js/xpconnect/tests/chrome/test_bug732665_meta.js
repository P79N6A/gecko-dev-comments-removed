var bottom = stackPointerInfo();
var top = bottom;

function nearNativeStackLimit() {
  function inner() {
    try {
      with ({}) { 
        top = stackPointerInfo();
        var stepsFromLimit = eval("inner()"); 
      }
      return stepsFromLimit + 1;
    } catch(e) {
      
      
      
      return 1;
    }
  }
  return inner();
}

var nbFrames = nearNativeStackLimit();
var frameSize = bottom - top;
print("Max stack size:", frameSize, "bytes",
      "\nMaximum number of frames:", nbFrames,
      "\nAverage frame size:", Math.ceil(frameSize / nbFrames), "bytes");
