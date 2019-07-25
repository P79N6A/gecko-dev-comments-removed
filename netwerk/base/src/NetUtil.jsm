







































let EXPORTED_SYMBOLS = [
  "NetUtil",
];








const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

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

    

















    asyncFetch: function NetUtil_asyncOpen(aSource, aCallback)
    {
        if (!aSource || !aCallback) {
            let exception = new Components.Exception(
                "Must have a source and a callback",
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
                aCallback(pipe.inputStream, aStatusCode, aRequest);
            }
        });

        let channel = aSource;
        if (!(channel instanceof Ci.nsIChannel)) {
            channel = this.newChannel(aSource);
        }

        
        
        if (!channel.notificationCallbacks) {
          
          channel.notificationCallbacks = new BadCertHandler(true);
        }

        channel.asyncOpen(listener, null);
    },

    














    newURI: function NetUtil_newURI(aTarget, aOriginCharset, aBaseURI)
    {
        if (!aTarget) {
            let exception = new Components.Exception(
                "Must have a non-null string spec or nsIFile object",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
        }

        if (aTarget instanceof Ci.nsIFile) {
            return this.ioService.newFileURI(aTarget);
        }

        return this.ioService.newURI(aTarget, aOriginCharset, aBaseURI);
    },

    













    newChannel: function NetUtil_newChannel(aWhatToLoad, aOriginCharset,
                                            aBaseURI)
    {
        if (!aWhatToLoad) {
            let exception = new Components.Exception(
                "Must have a non-null string spec, nsIURI, or nsIFile object",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
        }

        let uri = aWhatToLoad;
        if (!(aWhatToLoad instanceof Ci.nsIURI)) {
            
            uri = this.newURI(aWhatToLoad, aOriginCharset, aBaseURI);
        }

        return this.ioService.newChannelFromURI(uri);
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

XPCOMUtils.defineLazyGetter(this, "BadCertHandler", function () {
  var obj = {};
  Cu.import("resource://gre/modules/CertUtils.jsm", obj);
  return obj.BadCertHandler;
});
