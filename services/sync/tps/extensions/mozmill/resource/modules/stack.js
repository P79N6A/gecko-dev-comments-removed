



var EXPORTED_SYMBOLS = ['findCallerFrame'];
















function findCallerFrame(aStartFrame) {
  let frame = Components.stack;
  let filename = frame.filename.replace(/(.*)-> /, "");

  
  
  if (aStartFrame) {
    filename = aStartFrame.filename.replace(/(.*)-> /, "");

    while (frame.caller &&
           frame.filename && (frame.filename.indexOf(filename) == -1)) {
      frame = frame.caller;
    }
  }

  
  while (frame.caller &&
         (!frame.filename || (frame.filename.indexOf(filename) != -1)))
    frame = frame.caller;

  return frame;
}
