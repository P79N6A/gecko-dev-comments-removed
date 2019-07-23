





do_load_httpd_js();
const BUGID = "203271";

var httpserver = new nsHttpServer();
var index = 0;
var tests = [
    
    {url: "/precedence", server: "0", expected: "0",
     responseheader: [ "Expires: " + getDateString(-1),
                       "Cache-Control: max-age=3600"]},

    {url: "/precedence?0", server: "0", expected: "0",
     responseheader: [ "Cache-Control: max-age=3600",
                       "Expires: " + getDateString(-1)]},

    
    {url: "/precedence?1", server: "0", expected: "0",
     responseheader: [ "Expires: " + getDateString(1),
                       "Cache-Control: max-age=1"]},

    
    {url: "/precedence?2", server: "0", expected: "0",
     responseheader: [ "Expires: " + getDateString(0)]},

    
    {url: "/precedence?3", server: "0", expected: "0",
     responseheader: ["Cache-Control: max-age=1"]},

    
    
    
    
    
    {url: "/precedence?4", server: "0", expected: "0",
     responseheader: [ "Cache-Control: private, max-age=2592000",
                       "Expires: " + getDateString(+1)],
     explicitDate: getDateString(-1)},

    
    
    {url: "/precedence?5", server: "0", expected: "0",
     responseheader: [ "Cache-Control: max-age=1"],
     explicitDate: getDateString(1)},

    
    {url: "/precedence?6", server: "0", expected: "0",
     responseheader: [ "Cache-Control: max-age=60"],
     explicitDate: getDateString(1)},

    
    {url: "/precedence?999", server: "0", expected: "0", delay: "3000"},

    
    {url: "/precedence", server: "1", expected: "0"}, 
     
    {url: "/precedence?0", server: "1", expected: "0"}, 

    {url: "/precedence?1", server: "1", expected: "1"}, 

    {url: "/precedence?2", server: "1", expected: "1"}, 

    {url: "/precedence?3", server: "1", expected: "1"}, 

    {url: "/precedence?4", server: "1", expected: "1"}, 

    {url: "/precedence?5", server: "1", expected: "1"}, 
    
    {url: "/precedence?6", server: "1", expected: "0"}, 

];

function getCacheService()
{
    return Components.classes["@mozilla.org/network/cache-service;1"].
                      getService(Components.interfaces.nsICacheService);
}

function logit(i, data, ctx) {
    dump("requested [" + tests[i].server + "] " +
         "got [" + data + "] " +
         "expected [" + tests[i].expected + "]");

    if (tests[i].responseheader)
        dump("\t[" + tests[i].responseheader + "]");
    dump("\n");
    
    dump("\n===================================\n")
    ctx.visitResponseHeaders({
        visitHeader: function(key, val) {
            dump("\t" + key + ":"+val + "\n");
        }}
    );
    dump("===================================\n")
}

function setupChannel(suffix, value) {
    var ios = Components.classes["@mozilla.org/network/io-service;1"].
                         getService(Ci.nsIIOService);
    var chan = ios.newChannel("http://localhost:4444" + suffix, "", null);
    var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
    httpChan.requestMethod = "GET"; 
    httpChan.setRequestHeader("x-request", value, false);
    return httpChan;
}

function triggerNextTest() {
    var channel = setupChannel(tests[index].url, tests[index].server);
    channel.asyncOpen(new ChannelListener(checkValueAndTrigger, channel), null);
}

function checkValueAndTrigger(request, data, ctx) {
    logit(index, data, ctx);
    do_check_eq(tests[index].expected, data);

    if (index < tests.length - 1) {
        var delay = tests[index++].delay;
        if (delay) {
            do_timeout(delay, triggerNextTest);
        } else {
            triggerNextTest();
        }
    } else {
        httpserver.stop(do_test_finished);
    }
}

function run_test() {
    httpserver.registerPathHandler("/precedence", handler);
    httpserver.start(4444);

    
    getCacheService().
       evictEntries(Components.interfaces.nsICache.STORE_ANYWHERE);
    triggerNextTest();
    do_test_pending();
}

function handler(metadata, response) {
    var body = metadata.getHeader("x-request");
    response.setHeader("Content-Type", "text/plain", false);

    var date = tests[index].explicitDate;
    if (date == undefined) {
        response.setHeader("Date", getDateString(0), false);
    } else {
        response.setHeader("Date", date, false);
    }

    var header = tests[index].responseheader;
    if (header == undefined) {
        response.setHeader("Last-Modified", getDateString(-1), false);
    } else {
        for (var i = 0; i < header.length; i++) {
            var splitHdr = header[i].split(": ");
            response.setHeader(splitHdr[0], splitHdr[1], false);
        }
    }
    
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.bodyOutputStream.write(body, body.length);
}
 
function getDateString(yearDelta) {
    var months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug',
                  'Sep', 'Oct', 'Nov', 'Dec'];
    var days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

    var d = new Date();
    return days[d.getUTCDay()] + ", " +
            d.getUTCDate() + " " +
            months[d.getUTCMonth()] + " " +
            (d.getUTCFullYear() + yearDelta) + " " +
            d.getUTCHours() + ":" + d.getUTCMinutes() + ":" +
            d.getUTCSeconds() + " UTC";
}
