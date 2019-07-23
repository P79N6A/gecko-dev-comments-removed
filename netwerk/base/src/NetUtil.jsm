







































let EXPORTED_SYMBOLS = [
  "NetUtil",
];








const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

const PR_UINT32_MAX = 0xffffffff;




const NetUtil = {
    
















    asyncCopy: function NetUtil_asyncCopy(aSource, aSink, aCallback)
    {
        if (!aSource || !aSink) {
            let exception = new Components.Exception(
                "Must have a source and a sink",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
        }

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
    },

    












    asyncFetch: function NetUtil_asyncOpen(aChannel, aCallback)
    {
        if (!aChannel || !aCallback) {
            let exception = new Components.Exception(
                "Must have a channel and a callback",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
        }

        
        
        let pipe = Cc["@mozilla.org/pipe;1"].
                   createInstance(Ci.nsIPipe);
        pipe.init(true, true, 0, PR_UINT32_MAX, null);

        
        let listener = Cc["@mozilla.org/network/simple-stream-listener;1"].
                       createInstance(Ci.nsISimpleStreamListener);
        listener.init(pipe.outputStream, {
            onStartRequest: function(aRequest, aContext) {},
            onStopRequest: function(aRequest, aContext, aStatusCode) {
                pipe.outputStream.close();
                aCallback(pipe.inputStream, aStatusCode);
            }
        });

        aChannel.asyncOpen(listener, null);
    },

    











    newURI: function NetUtil_newURI(aSpec, aOriginCharset, aBaseURI)
    {
        if (!aSpec) {
            let exception = new Components.Exception(
                "Must have a non-null spec",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
        }

        return this.ioService.newURI(aSpec, aOriginCharset, aBaseURI);
    },

    




    get ioService()
    {
        delete this.ioService;
        return this.ioService = Cc["@mozilla.org/network/io-service;1"].
                                getService(Ci.nsIIOService);
    },
};




Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


XPCOMUtils.defineLazyServiceGetter(this, "ioUtil", "@mozilla.org/io-util;1",
                                   "nsIIOUtil");
