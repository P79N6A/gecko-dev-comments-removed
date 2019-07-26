



const EXPORTED_SYMBOLS = ["httpRequest", "percentEncode"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;




function percentEncode(aString)
  encodeURIComponent(aString).replace(/[!'()]/g, escape).replace(/\*/g, "%2A");



















function httpRequest(aUrl, aOptions) {
  let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
              .createInstance(Ci.nsIXMLHttpRequest);
  xhr.mozBackgroundRequest = true; 
  let hasPostData = "postData" in aOptions && aOptions.postData;
  xhr.open("method" in aOptions ? aOptions.method :
           (hasPostData ? "POST" : "GET"), aUrl);
  xhr.channel.loadFlags = Ci.nsIChannel.LOAD_ANONYMOUS | 
                          Ci.nsIChannel.LOAD_BYPASS_CACHE |
                          Ci.nsIChannel.INHIBIT_CACHING;
  xhr.onerror = function(aProgressEvent) {
    if ("onError" in aOptions) {
      
      let request = aProgressEvent.target;
      let status;
      try {
        
        status = request.status;
      }
      catch (e) {
        request = request.channel.QueryInterface(Ci.nsIRequest);
        status = request.status;
      }
      
      let statusText = status ? request.statusText : "offline";
      aOptions.onError(statusText, null, this);
    }
  };
  xhr.onload = function (aRequest) {
    try {
      let target = aRequest.target;
      if ("logger" in aOptions)
        aOptions.logger.debug("Received response: " + target.responseText);
      if (target.status < 200 || target.status >= 300) {
        let errorText = target.responseText;
        if (!errorText || /<(ht|\?x)ml\b/i.test(errorText))
          errorText = target.statusText;
        throw target.status + " - " + errorText;
      }
      if ("onLoad" in aOptions)
        aOptions.onLoad(target.responseText, this);
    } catch (e) {
      Cu.reportError(e);
      if ("onError" in aOptions)
        aOptions.onError(e, aRequest.target.responseText, this);
    }
  };

  if ("headers" in aOptions) {
    aOptions.headers.forEach(function(header) {
      xhr.setRequestHeader(header[0], header[1]);
    });
  }

  
  let POSTData = hasPostData ? aOptions.postData : "";
  if (Array.isArray(POSTData)) {
    xhr.setRequestHeader("Content-Type",
                         "application/x-www-form-urlencoded; charset=utf-8");
    POSTData = POSTData.map(function(p) p[0] + "=" + percentEncode(p[1]))
                       .join("&");
  }

  if ("logger" in aOptions) {
    aOptions.logger.log("sending request to " + aUrl + " (POSTData = " +
                        POSTData + ")");
  }
  xhr.send(POSTData);
  return xhr;
}
