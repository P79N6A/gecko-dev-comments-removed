Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");

var httpserver = new HttpServer();
var currentTestIndex = 0;

XPCOMUtils.defineLazyGetter(this, "port", function() {
    return httpserver.identity.primaryPort;
});

XPCOMUtils.defineLazyGetter(this, "tests", function() {
    return [
            
            {url: "/assoc/assoctest?valid",
             responseheader: ["Assoc-Req: GET http://localhost:" + port +
                              "/assoc/assoctest?valid",
                              "Pragma: X-Verify-Assoc-Req"],
             flags: 0},

            
            {url: "/assoc/assoctest?invalid",
             responseheader: ["Assoc-Req: POST http://localhost:" + port +
                              "/assoc/assoctest?invalid",
                              "Pragma: X-Verify-Assoc-Req"],
             flags: CL_EXPECT_LATE_FAILURE},

            
            {url: "/assoc/assoctest?notvalid",
             responseheader: ["Assoc-Req: GET http://localhost:" + port +
                              "/wrongpath/assoc/assoctest?notvalid",
                              "Pragma: X-Verify-Assoc-Req"],
             flags: CL_EXPECT_LATE_FAILURE},

             
            {url: "/assoc/assoctest?invalid2",
             responseheader: ["Assoc-Req: GEThttp://localhost:" + port +
                              "/assoc/assoctest?invalid2",
                              "Pragma: X-Verify-Assoc-Req"],
             flags: CL_EXPECT_LATE_FAILURE},
    ];
});

var oldPrefVal;
var domBranch;

function setupChannel(url)
{
    var ios = Components.classes["@mozilla.org/network/io-service;1"].
                         getService(Ci.nsIIOService);
    var chan = ios.newChannel2("http://localhost:" + port + url,
                               "",
                               null,
                               null,      
                               Services.scriptSecurityManager.getSystemPrincipal(),
                               null,      
                               Ci.nsILoadInfo.SEC_NORMAL,
                               Ci.nsIContentPolicy.TYPE_OTHER);
    return chan;
}

function startIter()
{
    var channel = setupChannel(tests[currentTestIndex].url);
    channel.asyncOpen(new ChannelListener(completeIter,
                                          channel, tests[currentTestIndex].flags), null);
}

function completeIter(request, data, ctx)
{
    if (++currentTestIndex < tests.length ) {
        startIter();
    } else {
        domBranch.setBoolPref("enforce", oldPrefVal);
        httpserver.stop(do_test_finished);
    }
}

function run_test()
{
    var prefService =
        Components.classes["@mozilla.org/preferences-service;1"]
        .getService(Components.interfaces.nsIPrefService);
    domBranch = prefService.getBranch("network.http.assoc-req.");
    oldPrefVal = domBranch.getBoolPref("enforce");
    domBranch.setBoolPref("enforce", true);

    httpserver.registerPathHandler("/assoc/assoctest", handler);
    httpserver.start(-1);

    startIter();
    do_test_pending();
}

function handler(metadata, response)
{
    var body = "thequickbrownfox";
    response.setHeader("Content-Type", "text/plain", false);

    var header = tests[currentTestIndex].responseheader;
    if (header != undefined) {
        for (var i = 0; i < header.length; i++) {
            var splitHdr = header[i].split(": ");
            response.setHeader(splitHdr[0], splitHdr[1], false);
        }
    }
    
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.bodyOutputStream.write(body, body.length);
}
