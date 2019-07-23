










































const Ci = Components.interfaces;
const Cc = Components.classes;

var NetUtil = {
    














    asyncCopy: function _asyncCopy(aSource, aSink, aCallback) {
        if (!aSource || !aSink) {
            throw "Must have a source and a sink";
        }

        const ioUtil = Cc["@mozilla.org/io-util;1"].getService(Ci.nsIIOUtil);
        var sourceBuffered = ioUtil.inputStreamIsBuffered(aSource);
        var sinkBuffered = ioUtil.outputStreamIsBuffered(aSink);

        if (!sourceBuffered && !sinkBuffered) {
            
            var bostream = Cc["@mozilla.org/network/buffered-output-stream;1"].
                createInstance(Ci.nsIBufferedOutputStream);
            bostream.init(aSink, 0x8000);
            sinkBuffered = true;
        }

        
        var copier = Cc["@mozilla.org/network/async-stream-copier;1"].
            createInstance(Ci.nsIAsyncStreamCopier);

        
        
        
        
        copier.init(aSource, bostream, null, sourceBuffered, sinkBuffered,
                    0x8000, true, true);

        var observer;
        if (aCallback) {
            observer = {
            onStartRequest: function(aRequest, aContext) {},
            onStopRequest: function(aRequest, aContext, aStatusCode) {
              aCallback(aStatusCode);
            }
        } else {
            observer = null;
        }

        
        copier.asyncCopy(observer, null);
        return copier;
    }
};
