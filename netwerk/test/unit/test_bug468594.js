














do_load_httpd_js();

var httpserver = new nsHttpServer();
var index = 0;
var tests = [
    {url: "/freshness",   server: "0", expected: "0"},
    {url: "/freshness",   server: "1", expected: "0"}, 

    
    {url: "/freshness?a", server: "2", expected: "2"},
    {url: "/freshness?a", server: "3", expected: "3"},

    
    {url: "/freshness?b", server: "4", expected: "4",
     responseheader: "Expires: "+getDateString(1)},
    {url: "/freshness?b", server: "5", expected: "4"},
    
    {url: "/freshness?c", server: "6", expected: "6",
     responseheader: "Cache-Control: max-age=3600"},
    {url: "/freshness?c", server: "7", expected: "6"}, 

    
    {url: "/freshness?d", server: "8", expected: "8",
     responseheader: "Expires: "+getDateString(-1)},
    {url: "/freshness?d", server: "9", expected: "9"},
    
    {url: "/freshness?e", server: "10", expected: "10",
     responseheader: "Cache-Control: max-age=0"},
    {url: "/freshness?e", server: "11", expected: "11"},

    {url: "/freshness",   server: "99", expected: "0"}, 
];

function getCacheService()
{
    return Components.classes["@mozilla.org/network/cache-service;1"].
                      getService(Components.interfaces.nsICacheService);
}

function logit(i, data) {
    dump(tests[i].url + "\t requested [" + tests[i].server + "]" +
         " got [" + data + "] expected [" + tests[i].expected + "]");
    if (tests[i].responseheader)
        dump("\t[" + tests[i].responseheader + "]");
    dump("\n");
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
    channel.asyncOpen(new ChannelListener(checkValueAndTrigger, null),null);
}

function checkValueAndTrigger(request, data, ctx) {
    logit(index, data);
    do_check_eq(tests[index].expected, data);

    if (index < tests.length-1) {
        index++;
        triggerNextTest();
    } else {
        httpserver.stop(do_test_finished);
    }
}

function run_test() {
    httpserver.registerPathHandler("/freshness", handler);
    httpserver.start(4444);

    
    getCacheService().
        evictEntries(Components.interfaces.nsICache.STORE_ANYWHERE);
    triggerNextTest();

    do_test_pending();
}

function handler(metadata, response) {
    var body = metadata.getHeader("x-request");
    response.setHeader("Content-Type", "text/plain", false);
    response.setHeader("Date", getDateString(0), false);
    
    var header = tests[index].responseheader;
    if (header == null) {
        response.setHeader("Last-Modified", getDateString(-1), false);
    } else {
        var splitHdr = header.split(": ");
        response.setHeader(splitHdr[0], splitHdr[1], false);
    }
    
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.bodyOutputStream.write(body, body.length);
}
 
function getDateString(yearDelta) {
    var months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
                  'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
    var days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

    var d = new Date();
    return days[d.getUTCDay()] + ", " +
            d.getUTCDate() + " " +
            months[d.getUTCMonth()] + " " +
            (d.getUTCFullYear() + yearDelta) + " " +
            d.getUTCHours() + ":" + d.getUTCMinutes() +":" +
            d.getUTCSeconds() + " UTC";
}
