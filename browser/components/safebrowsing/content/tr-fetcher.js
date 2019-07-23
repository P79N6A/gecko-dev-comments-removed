

















































function PROT_TRFetcher(opt_noCrypto) {
  this.debugZone = "trfetcher";
  this.useCrypto_ = !opt_noCrypto;
  this.protocol4Parser_ = new G_Protocol4Parser();

  
  
  
  
  
}

PROT_TRFetcher.TRY_REKEYING_RESPONSE = "pleaserekey";








PROT_TRFetcher.prototype.getRequestURL_ = function(url) {

  if (!this.urlCrypto_)
    this.urlCrypto_ = new PROT_UrlCrypto();

  G_Debug(this, "Fetching for " + url);
    
  var requestURL = gDataProvider.getLookupURL();
  if (!requestURL)
    return null;

  if (this.useCrypto_) {
    var maybeCryptedParams = this.urlCrypto_.maybeCryptParams({ "q": url});
    
    for (var param in maybeCryptedParams) 
      requestURL += param + "=" + 
                    encodeURIComponent(maybeCryptedParams[param]) + "&";
  } else {
    requestURL += "q=" + encodeURIComponent(url);
  }

  G_Debug(this, "Request URL: " + requestURL);

  return requestURL;
};








PROT_TRFetcher.prototype.get = function(forPage, callback) {
  
  var url = this.getRequestURL_(forPage);
  if (!url) {
    G_Debug(this, "No remote lookup url.");
    return;
  }
  var closure = BindToObject(this.onFetchComplete_, this, callback);
  (new PROT_XMLFetcher()).get(url, closure);
};









PROT_TRFetcher.prototype.onFetchComplete_ = function(callback, responseText,
                                                     httpStatus) {
  
  var responseObj = this.extractResponse_(responseText);

  
  
  

  if (responseObj[PROT_TRFetcher.TRY_REKEYING_RESPONSE] == "1" &&
      this.urlCrypto_) {
    G_Debug(this, "We're supposed to re-key. Trying.");
    var manager = this.urlCrypto_.getManager();
    if (manager)
      manager.maybeReKey();
  }

  G_Debug(this, "TR Response:");
  for (var field in responseObj)
    G_Debug(this, field + "=" + responseObj[field]);

  callback(responseObj, httpStatus);
};









PROT_TRFetcher.prototype.extractResponse_ = function(responseText) {
  return this.protocol4Parser_.parse(responseText);
};

#ifdef DEBUG

function TEST_PROT_TRFetcher() {
  if (G_GDEBUG) {
    var z = "trfetcher UNITTEST";
    G_debugService.enableZone(z);
    G_Debug(z, "Starting");

    
    
    var fetcher = new PROT_TRFetcher();
    var fakeResponse = "foo:3:foo\nbar:3:bar\nbaz:1:b";
    var fields = fetcher.extractResponse_(fakeResponse);
    G_Assert(z, fields["foo"] == "foo", "Bad parse: foo");
    G_Assert(z, fields["bar"] == "bar", "Bad parse: bar");
    G_Assert(z, fields["baz"] == "b", "Bad parse: baz");
    G_Assert(z, !fields["yourmom"], "Bad parse: yourmom");

    

    var calledRekey = false;
    var fakeManager = {
      maybeReKey: function() {
        calledRekey = true;
      }
    };

    fetcher.urlCrypto_ = {};
    fetcher.urlCrypto_.getManager = function() { return fakeManager; };
    var rekeyText = "foo:1:bar\n" + 
                    PROT_TRFetcher.TRY_REKEYING_RESPONSE + ":1:1";
    fetcher.onFetchComplete_(function() {}, rekeyText);
    G_Assert(z, calledRekey, "rekey didn't trigger call to maybeReKey()");
    
    G_Debug(z, "PASSED");
  }
}
#endif
