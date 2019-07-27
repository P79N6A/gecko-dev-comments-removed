
































Cu.import('resource://gre/modules/LoadContextInfo.jsm');
Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");



var packagedAppRequestsMade = 0;


function packagedAppContentHandler(metadata, response)
{
  packagedAppRequestsMade++;
  response.setHeader("Content-Type", 'application/package');
  var body = testData.getData();
  response.bodyOutputStream.write(body, body.length);
}



var testData = {
  content: [
   { headers: ["Content-Location: /index.html", "Content-Type: text/html"], data: "<html>\r\n  <head>\r\n    <script src=\"/scripts/app.js\"></script>\r\n    ...\r\n  </head>\r\n  ...\r\n</html>\r\n", type: "text/html" },
   { headers: ["Content-Location: /scripts/app.js", "Content-Type: text/javascript"], data: "module Math from '/scripts/helpers/math.js';\r\n...\r\n", type: "text/javascript" },
   { headers: ["Content-Location: /scripts/helpers/math.js", "Content-Type: text/javascript"], data: "export function sum(nums) { ... }\r\n...\r\n", type: "text/javascript" }
  ],
  token : "gc0pJq0M:08jU534c0p",
  getData: function() {
    var str = "";
    for (var i in this.content) {
      str += "--" + this.token + "\r\n";
      for (var j in this.content[i].headers) {
        str += this.content[i].headers[j] + "\r\n";
      }
      str += "\r\n";
      str += this.content[i].data + "\r\n";
    }

    str += "--" + this.token + "--";
    return str;
  }
}

XPCOMUtils.defineLazyGetter(this, "uri", function() {
  return "http://localhost:" + httpserver.identity.primaryPort;
});


var httpserver = null;

var paservice = null;


var packagePath = null;

function run_test()
{
  
  httpserver = new HttpServer();
  httpserver.registerPathHandler("/package", packagedAppContentHandler);
  httpserver.registerPathHandler("/304Package", packagedAppContentHandler);
  httpserver.registerPathHandler("/badPackage", packagedAppBadContentHandler);
  httpserver.start(-1);

  paservice = Cc["@mozilla.org/network/packaged-app-service;1"]
                     .getService(Ci.nsIPackagedAppService);
  ok(!!paservice, "test service exists");

  add_test(test_bad_args);

  add_test(test_callback_gets_called);
  add_test(test_same_content);
  add_test(test_request_number);

  add_test(test_package_does_not_exist);
  add_test(test_file_does_not_exist);

  add_test(test_bad_package);
  add_test(test_bad_package_404);

  
  run_next_test();
}


var metadataListener = {
  onMetaDataElement: function(key, value) {
    if (key == 'response-head')
      equal(value, "HTTP/1.1 200 \r\nContent-Location: /index.html\r\nContent-Type: text/html\r\n");
    else if (key == 'request-method')
      equal(value, "GET");
    else
      ok(false, "unexpected metadata key")
  }
}




var cacheListener = {
  onCacheEntryCheck: function() { return Ci.nsICacheEntryOpenCallback.ENTRY_WANTED; },
  onCacheEntryAvailable: function (entry, isnew, appcache, status) {
    ok(!!entry, "Needs to have an entry");
    equal(status, Cr.NS_OK, "status is NS_OK");
    equal(entry.key, uri + packagePath + "!//index.html", "Check entry has correct name");
    entry.visitMetaData(metadataListener);
    var inputStream = entry.openInputStream(0);
    pumpReadStream(inputStream, function(read) {
        inputStream.close();
        equal(read,"<html>\r\n  <head>\r\n    <script src=\"/scripts/app.js\"></script>\r\n    ...\r\n  </head>\r\n  ...\r\n</html>\r\n"); 
    });
    run_next_test();
  }
};




function test_bad_args() {
  Assert.throws(() => { paservice.requestURI(createURI("http://test.com"), LoadContextInfo.default, cacheListener); }, "url's with no !// aren't allowed");
  Assert.throws(() => { paservice.requestURI(createURI("http://test.com/package!//test"), LoadContextInfo.default, null); }, "should have a callback");
  Assert.throws(() => { paservice.requestURI(null, LoadContextInfo.default, cacheListener); }, "should have a URI");
  Assert.throws(() => { paservice.requestURI(createURI("http://test.com/package!//test"), null, cacheListener); }, "should have a LoadContextInfo");
  run_next_test();
}




function test_callback_gets_called() {
  packagePath = "/package";
  paservice.requestURI(createURI(uri + packagePath + "!//index.html"), LoadContextInfo.default, cacheListener);
}


function test_same_content() {
  packagePath = "/package";
  paservice.requestURI(createURI(uri + packagePath + "!//index.html"), LoadContextInfo.default, cacheListener);
}


function test_request_number() {
  equal(packagedAppRequestsMade, 1, "only one request should be made. Second should be loaded from cache");
  run_next_test();
}






var listener404 = {
  onCacheEntryCheck: function() { return Ci.nsICacheEntryOpenCallback.ENTRY_WANTED; },
  onCacheEntryAvailable: function (entry, isnew, appcache, status) {
    
    
    
    notEqual(status, Cr.NS_OK, "NOT FOUND");
    ok(!entry, "There should be no entry");
    run_next_test();
  }
};


function test_package_does_not_exist() {
  packagePath = "/package_non_existent";
  paservice.requestURI(createURI(uri + packagePath + "!//index.html"), LoadContextInfo.default, listener404);
}


function test_file_does_not_exist() {
  packagePath = "/package"; 
  paservice.requestURI(createURI(uri + packagePath + "!//file_non_existent.html"), LoadContextInfo.default, listener404);
}





var badTestData = {
  content: [
   { headers: ["Content-Type: text/javascript"], data: "module Math from '/scripts/helpers/math.js';\r\n...\r\n", type: "text/javascript" },
   { headers: ["Content-Location: /index.html", "Content-Type: text/html"], data: "<html>\r\n  <head>\r\n    <script src=\"/scripts/app.js\"></script>\r\n    ...\r\n  </head>\r\n  ...\r\n</html>\r\n", type: "text/html" },
   { headers: ["Content-Type: text/javascript"], data: "export function sum(nums) { ... }\r\n...\r\n", type: "text/javascript" }
  ],
  token : "gc0pJq0M:08jU534c0p",
  getData: function() {
    var str = "";
    for (var i in this.content) {
      str += "--" + this.token + "\r\n";
      for (var j in this.content[i].headers) {
        str += this.content[i].headers[j] + "\r\n";
      }
      str += "\r\n";
      str += this.content[i].data + "\r\n";
    }

    str += "--" + this.token + "--";
    return str;
  }
}


function packagedAppBadContentHandler(metadata, response)
{
  response.setHeader("Content-Type", 'application/package');
  var body = badTestData.getData();
  response.bodyOutputStream.write(body, body.length);
}


function test_bad_package() {
  packagePath = "/badPackage";
  paservice.requestURI(createURI(uri + packagePath + "!//index.html"), LoadContextInfo.default, cacheListener);
}


function test_bad_package_404() {
  packagePath = "/badPackage";
  paservice.requestURI(createURI(uri + packagePath + "!//file_non_existent.html"), LoadContextInfo.default, listener404);
}


