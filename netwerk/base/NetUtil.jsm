





this.EXPORTED_SYMBOLS = [
  "NetUtil",
];








const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

const PR_UINT32_MAX = 0xffffffff;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");




this.NetUtil = {
    
















    asyncCopy: function NetUtil_asyncCopy(aSource, aSink,
                                          aCallback = null)
    {
        if (!aSource || !aSink) {
            let exception = new Components.Exception(
                "Must have a source and a sink",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
        }

        
        var copier = Cc["@mozilla.org/network/async-stream-copier;1"].
            createInstance(Ci.nsIAsyncStreamCopier2);
        copier.init(aSource, aSink,
                    null ,
                    0 ,
                    true, true );

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

        
        copier.QueryInterface(Ci.nsIAsyncStreamCopier).asyncCopy(observer, null);
        return copier;
    },

    

















    asyncFetch: function NetUtil_asyncFetch(aSource, aCallback)
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

        
        if (aSource instanceof Ci.nsIInputStream) {
            let pump = Cc["@mozilla.org/network/input-stream-pump;1"].
                       createInstance(Ci.nsIInputStreamPump);
            pump.init(aSource, -1, -1, 0, 0, true);
            pump.asyncRead(listener, null);
            return;
        }

        let channel = aSource;
        if (!(channel instanceof Ci.nsIChannel)) {
            channel = this.newChannel(aSource);
        }

        try {
            channel.asyncOpen(listener, null);
        }
        catch (e) {
            let exception = new Components.Exception(
                "Failed to open input source '" + channel.originalURI.spec + "'",
                e.result,
                Components.stack.caller,
                aSource,
                e
            );
            throw exception;
        }
    },

    


    asyncFetch2: function NetUtil_asyncFetch2(aSource,
                                              aCallback,
                                              aLoadingNode,
                                              aLoadingPrincipal,
                                              aTriggeringPrincipal,
                                              aSecurityFlags,
                                              aContentPolicyType)
    {
        if (!aSource || !aCallback) {
            let exception = new Components.Exception(
                "Must have a source and a callback",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
        }

        
        
        if (aSource instanceof Ci.nsIChannel) {
          if ((typeof aLoadingNode != "undefined") ||
              (typeof aLoadingPrincipal != "undefined") ||
              (typeof aTriggeringPrincipal != "undefined") ||
              (typeof aSecurityFlags != "undefined") ||
              (typeof aContentPolicyType != "undefined")) {

            let exception = new Components.Exception(
                "LoadInfo arguments must be undefined",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
          }
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

        
        if (aSource instanceof Ci.nsIInputStream) {
            let pump = Cc["@mozilla.org/network/input-stream-pump;1"].
                       createInstance(Ci.nsIInputStreamPump);
            pump.init(aSource, -1, -1, 0, 0, true);
            pump.asyncRead(listener, null);
            return;
        }

        let channel = aSource;
        if (!(channel instanceof Ci.nsIChannel)) {
            channel = this.newChannel2(aSource,
                                       "",   
                                       null, 
                                       aLoadingNode,
                                       aLoadingPrincipal,
                                       aTriggeringPrincipal,
                                       aSecurityFlags,
                                       aContentPolicyType);

        }

        try {
            channel.asyncOpen(listener, null);
        }
        catch (e) {
            let exception = new Components.Exception(
                "Failed to open input source '" + channel.originalURI.spec + "'",
                e.result,
                Components.stack.caller,
                aSource,
                e
            );
            throw exception;
        }
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
        
        if (typeof aWhatToLoad == "string" ||
            (aWhatToLoad instanceof Ci.nsIFile) ||
            (aWhatToLoad instanceof Ci.nsIURI)) {

            let uri = (aWhatToLoad instanceof Ci.nsIURI)
                      ? aWhatToLoad
                      : this.newURI(aWhatToLoad, aOriginCharset, aBaseURI);

            return this.ioService.newChannelFromURI(uri);
        }

        
        if (typeof aWhatToLoad != "object" ||
            aOriginCharset !== undefined ||
            aBaseURI !== undefined) {

            throw new Components.Exception(
                "newChannel requires a single object argument",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
        }

        let { uri,
              loadingNode,
              loadingPrincipal,
              loadUsingSystemPrincipal,
              triggeringPrincipal,
              securityFlags,
              contentPolicyType } = aWhatToLoad;

        if (!uri) {
            throw new Components.Exception(
                "newChannel requires the 'uri' property on the options object.",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
        }

        if (typeof uri == "string") {
            uri = this.newURI(uri);
        }

        if (!loadingNode && !loadingPrincipal && !loadUsingSystemPrincipal) {
            throw new Components.Exception(
                "newChannel requires at least one of the 'loadingNode'," +
                " 'loadingPrincipal', or 'loadUsingSystemPrincipal'" +
                " properties on the options object.",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
        }

        if (loadUsingSystemPrincipal === true) {
            if (loadingNode || loadingPrincipal) {
                throw new Components.Exception(
                    "newChannel does not accept 'loadUsingSystemPrincipal'" +
                    " if the 'loadingNode' or 'loadingPrincipal' properties" +
                    " are present on the options object.",
                    Cr.NS_ERROR_INVALID_ARG,
                    Components.stack.caller
                );
            }
            loadingPrincipal = Services.scriptSecurityManager
                                       .getSystemPrincipal();
        } else if (loadUsingSystemPrincipal !== undefined) {
            throw new Components.Exception(
                "newChannel requires the 'loadUsingSystemPrincipal'" +
                " property on the options object to be 'true' or 'undefined'.",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
        }

        if (securityFlags === undefined) {
            securityFlags = Ci.nsILoadInfo.SEC_NORMAL;
        }

        if (contentPolicyType === undefined) {
            if (!loadUsingSystemPrincipal) {
                throw new Components.Exception(
                    "newChannel requires the 'contentPolicyType' property on" +
                    " the options object unless loading from system principal.",
                    Cr.NS_ERROR_INVALID_ARG,
                    Components.stack.caller
                );
            }
            contentPolicyType = Ci.nsIContentPolicy.TYPE_OTHER;
        }

        return this.ioService.newChannelFromURI2(uri,
                                                 loadingNode || null,
                                                 loadingPrincipal || null,
                                                 triggeringPrincipal || null,
                                                 securityFlags,
                                                 contentPolicyType);
    },

    


    newChannel2: function NetUtil_newChannel2(aWhatToLoad,
                                              aOriginCharset,
                                              aBaseURI,
                                              aLoadingNode,
                                              aLoadingPrincipal,
                                              aTriggeringPrincipal,
                                              aSecurityFlags,
                                              aContentPolicyType)
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

        return this.ioService.newChannelFromURI2(uri,
                                                 aLoadingNode,
                                                 aLoadingPrincipal,
                                                 aTriggeringPrincipal,
                                                 aSecurityFlags,
                                                 aContentPolicyType);
    },

    






















    readInputStreamToString: function NetUtil_readInputStreamToString(aInputStream,
                                                                      aCount,
                                                                      aOptions)
    {
        if (!(aInputStream instanceof Ci.nsIInputStream)) {
            let exception = new Components.Exception(
                "First argument should be an nsIInputStream",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
        }

        if (!aCount) {
            let exception = new Components.Exception(
                "Non-zero amount of bytes must be specified",
                Cr.NS_ERROR_INVALID_ARG,
                Components.stack.caller
            );
            throw exception;
        }

        if (aOptions && "charset" in aOptions) {
          let cis = Cc["@mozilla.org/intl/converter-input-stream;1"].
                    createInstance(Ci.nsIConverterInputStream);
          try {
            
            
            if (!("replacement" in aOptions)) {
              
              
              
              aOptions.replacement = 0;
            }

            cis.init(aInputStream, aOptions.charset, aCount,
                     aOptions.replacement);
            let str = {};
            cis.readString(-1, str);
            cis.close();
            return str.value;
          }
          catch (e) {
            
            throw new Components.Exception(e.message, e.result,
                                           Components.stack.caller, e.data);
          }
        }

        let sis = Cc["@mozilla.org/scriptableinputstream;1"].
                  createInstance(Ci.nsIScriptableInputStream);
        sis.init(aInputStream);
        try {
            return sis.readBytes(aCount);
        }
        catch (e) {
            
            throw new Components.Exception(e.message, e.result,
                                           Components.stack.caller, e.data);
        }
    },

    




    get ioService()
    {
        delete this.ioService;
        return this.ioService = Cc["@mozilla.org/network/io-service;1"].
                                getService(Ci.nsIIOService);
    },
};
