do_load_httpd_js();

var httpserver = new nsHttpServer();
var index = 0;
var tests = [
    
    { url : "/bug468426", server : "0", expected : "0", cookie: null},

    
    
    { url : "/bug468426", server : "1", expected : "0", cookie: null},

    
    
    { url : "/bug468426", server : "2", expected : "2", cookie: "c=2"},

    
    
    { url : "/bug468426", server : "3", expected : "2", cookie: "c=2"},

    
    
    { url : "/bug468426", server : "4", expected : "4", cookie: "c=4"},

    
    
    { url : "/bug468426", server : "5", expected : "4", cookie: "c=4"},
    
    
    
    { url : "/bug468426", server : "6", expected : "6", cookie: null},

];

function getCacheService() {
    return Components.classes["@mozilla.org/network/cache-service;1"]
            .getService(Components.interfaces.nsICacheService);
}

function setupChannel(suffix, value, cookie) {
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
            .getService(Ci.nsIIOService);
    var chan = ios.newChannel("http://localhost:4444" + suffix, "", null);
    var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
    httpChan.requestMethod = "GET";
    httpChan.setRequestHeader("x-request", value, false);
    if (cookie != null)
        httpChan.setRequestHeader("Cookie", cookie, false);
    return httpChan;
}

function triggerNextTest() {
    var channel = setupChannel(tests[index].url, tests[index].server, tests[index].cookie);
    channel.asyncOpen(new ChannelListener(checkValueAndTrigger, null), null);
}

function checkValueAndTrigger(request, data, ctx) {
    do_check_eq(tests[index].expected, data);

    if (index < tests.length - 1) {
        index++;
        
        
        do_timeout(1, "triggerNextTest();");
    } else {
        httpserver.stop(do_test_finished);
    }
}

function run_test() {
    httpserver.registerPathHandler("/bug468426", handler);
    httpserver.start(4444);

    
    getCacheService().evictEntries(
            Components.interfaces.nsICache.STORE_ANYWHERE);
    triggerNextTest();

    do_test_pending();
}

function handler(metadata, response) {
    var body = "unset";
    try {
        body = metadata.getHeader("x-request");
    } catch(e) { }
    response.setStatusLine(metadata.httpVersion, 200, "Ok");
    response.setHeader("Content-Type", "text/plain", false);
    response.setHeader("Last-Modified", getDateString(-1), false);
    response.setHeader("Vary", "Cookie", false);
    response.bodyOutputStream.write(body, body.length);
}

function getDateString(yearDelta) {
    var months = [ 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug',
            'Sep', 'Oct', 'Nov', 'Dec' ];
    var days = [ 'Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat' ];

    var d = new Date();
    return days[d.getUTCDay()] + ", " + d.getUTCDate() + " "
            + months[d.getUTCMonth()] + " " + (d.getUTCFullYear() + yearDelta)
            + " " + d.getUTCHours() + ":" + d.getUTCMinutes() + ":"
            + d.getUTCSeconds() + " UTC";
}
