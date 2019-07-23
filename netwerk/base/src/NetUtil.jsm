






































let EXPORTED_SYMBOLS = [
  "NetUtil",
];





const Ci = Components.interfaces;
const Cc = Components.classes;

const NetUtil = {
    














    asyncCopy: function _asyncCopy(aSource, aSink, aCallback) {
        if (!aSource || !aSink) {
            throw "Must have a source and a sink";
        }

        const ioUtil = Cc["@mozilla.org/io-util;1"].getService(Ci.nsIIOUtil);
        var sourceBuffered = ioUtil.inputStreamIsBuffered(aSource);
        var sinkBuffered = ioUtil.outputStreamIsBuffered(aSink);

        var ostream = aSink;
        if (!sourceBuffered && !sinkBuffered) {
            
            ostream = Cc["@mozilla.org/network/buffered-output-stream;1"].
                      createInstance(Ci.nsIBufferedOutputStream);
            ostream.init(aSink, 0x8000);
            sinkBuffered = true;
        }

        
        var copier = Cc["@mozilla.org/network/async-stream-copier;1"].
            createInstance(Ci.nsIAsyncStreamCopier);

        
        
        
        
        copier.init(aSource, ostream, null, sourceBuffered, sinkBuffered,
                    0x8000, true, true);

        var observer;
        if (aCallback) {
            observer = {
                onStartRequest: function(aRequest, aContext) {},
                onStopRequest: function(aRequest, aContext, aStatusCode) {
                    aCallback(aStatusCode);
                }
            }
        } else {
            observer = null;
        }

        
        copier.asyncCopy(observer, null);
        return copier;
    }
};
