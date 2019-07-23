













































const kDNS_CONTRACTID = "@mozilla.org/network/dns-service;1";
const kPAC_CONTRACTID = "@mozilla.org/network/proxy-auto-config;1";
const kPAC_CID = Components.ID("{63ac8c66-1dd2-11b2-b070-84d00d3eaece}");

const nsISupports        = Components.interfaces.nsISupports;
const nsIProxyAutoConfig = Components.interfaces.nsIProxyAutoConfig;
const nsIDNSService      = Components.interfaces.nsIDNSService;


function nsProxyAutoConfig() {};

nsProxyAutoConfig.prototype = {
    
    _sandBox: null, 

    QueryInterface: function(iid) {
        if (iid.Equals(nsIProxyAutoConfig) ||
            iid.Equals(nsISupports))
            return this;
        throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    init: function(pacURI, pacText) {
        
        if (pacURI == "" || pacText == "") {
            this._sandBox = null;
            return;
        }

        
        this._sandBox = new Components.utils.Sandbox(pacURI);
        Components.utils.evalInSandbox(pacUtils, this._sandBox);

        
        Components.utils.importIntoSandbox(this._sandBox, myIpAddress);
        Components.utils.importIntoSandbox(this._sandBox, dnsResolve);
        Components.utils.importIntoSandbox(this._sandBox, proxyAlert, "alert");

        
        Components.utils.evalInSandbox(pacText, this._sandBox);
    },

    getProxyForURI: function(testURI, testHost) {
        if (!("FindProxyForURL" in this._sandBox))
            return null;

        
        var rval = this._sandBox.FindProxyForURL(testURI, testHost);
        return rval;
    }
}

function proxyAlert(msg) {
    try {
        
        var cns = Components.classes["@mozilla.org/consoleservice;1"]
                            .getService(Components.interfaces.nsIConsoleService);
        cns.logStringMessage("PAC-alert: "+msg);
    } catch (e) {
        dump("PAC: proxyAlert ERROR: "+e+"\n");
    }
}


function myIpAddress() {
    try {
        return dns.resolve(dns.myHostName, 0).getNextAddrAsString();
    } catch (e) {
        return '127.0.0.1';
    }
}


function dnsResolve(host) {
    try {
        return dns.resolve(host, 0).getNextAddrAsString();
    } catch (e) {
        return null;
    }
}

var pacModule = new Object();

pacModule.registerSelf =
    function (compMgr, fileSpec, location, type) {
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.registerFactoryLocation(kPAC_CID,
                                        "nsProxyAutoConfig",
                                        kPAC_CONTRACTID,
                                        fileSpec, 
                                        location, 
                                        type);
    }

pacModule.getClassObject =
function (compMgr, cid, iid) {
        if (!cid.equals(kPAC_CID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return pacFactory;
    }

pacModule.canUnload =
    function (compMgr) {
        return true;
    }

var pacFactory = new Object();
pacFactory.createInstance =
    function (outer, iid) {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;

        if (!iid.equals(nsIProxyAutoConfig) &&
            !iid.equals(Components.interfaces.nsISupports)) {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }
        return pac;
    }

function NSGetModule(compMgr, fileSpec) {
    return pacModule;
}

var pac = new nsProxyAutoConfig() ;
var dns = Components.classes[kDNS_CONTRACTID].getService(nsIDNSService);

var pacUtils = 
"function dnsDomainIs(host, domain) {\n" +
"    return (host.length >= domain.length &&\n" +
"            host.substring(host.length - domain.length) == domain);\n" +
"}\n" +

"function dnsDomainLevels(host) {\n" +
"    return host.split('.').length-1;\n" +
"}\n" +

"function convert_addr(ipchars) {\n"+
"    var bytes = ipchars.split('.');\n"+
"    var result = ((bytes[0] & 0xff) << 24) |\n"+
"                 ((bytes[1] & 0xff) << 16) |\n"+
"                 ((bytes[2] & 0xff) <<  8) |\n"+
"                  (bytes[3] & 0xff);\n"+
"    return result;\n"+
"}\n"+

"function isInNet(ipaddr, pattern, maskstr) {\n"+
"    var test = /^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$/(ipaddr);\n"+
"    if (test == null) {\n"+
"        ipaddr = dnsResolve(ipaddr);\n"+
"        if (ipaddr == null)\n"+
"            return false;\n"+
"    } else if (test[1] > 255 || test[2] > 255 || \n"+
"               test[3] > 255 || test[4] > 255) {\n"+
"        return false;    // not an IP address\n"+
"    }\n"+
"    var host = convert_addr(ipaddr);\n"+
"    var pat  = convert_addr(pattern);\n"+
"    var mask = convert_addr(maskstr);\n"+
"    return ((host & mask) == (pat & mask));\n"+
"    \n"+
"}\n"+

"function isPlainHostName(host) {\n" +
"    return (host.search('\\\\.') == -1);\n" +
"}\n" +

"function isResolvable(host) {\n" +
"    var ip = dnsResolve(host);\n" +
"    return (ip != null);\n" +
"}\n" +

"function localHostOrDomainIs(host, hostdom) {\n" +
"    return (host == hostdom) ||\n" +
"           (hostdom.lastIndexOf(host + '.', 0) == 0);\n" +
"}\n" +

"function shExpMatch(url, pattern) {\n" +
"   pattern = pattern.replace(/\\./g, '\\\\.');\n" +
"   pattern = pattern.replace(/\\*/g, '.*');\n" +
"   pattern = pattern.replace(/\\?/g, '.');\n" +
"   var newRe = new RegExp('^'+pattern+'$');\n" +
"   return newRe.test(url);\n" +
"}\n" +

"var wdays = new Array('SUN', 'MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT');\n" +

"var monthes = new Array('JAN', 'FEB', 'MAR', 'APR', 'MAY', 'JUN', 'JUL', 'AUG', 'SEP', 'OCT', 'NOV', 'DEC');\n"+

"function weekdayRange() {\n" +
"    function getDay(weekday) {\n" +
"        for (var i = 0; i < 6; i++) {\n" +
"            if (weekday == wdays[i]) \n" +
"                return i;\n" +
"        }\n" +
"        return -1;\n" +
"    }\n" +
"    var date = new Date();\n" +
"    var argc = arguments.length;\n" +
"    var wday;\n" +
"    if (argc < 1)\n" +
"        return false;\n" +
"    if (arguments[argc - 1] == 'GMT') {\n" +
"        argc--;\n" +
"        wday = date.getUTCDay();\n" +
"    } else {\n" +
"        wday = date.getDay();\n" +
"    }\n" +
"    var wd1 = getDay(arguments[0]);\n" +
"    var wd2 = (argc == 2) ? getDay(arguments[1]) : wd1;\n" +
"    return (wd1 == -1 || wd2 == -1) ? false\n" +
"                                    : (wd1 <= wday && wday <= wd2);\n" +
"}\n" +

"function dateRange() {\n" +
"    function getMonth(name) {\n" +
"        for (var i = 0; i < 6; i++) {\n" +
"            if (name == monthes[i])\n" +
"                return i;\n" +
"        }\n" +
"        return -1;\n" +
"    }\n" +
"    var date = new Date();\n" +
"    var argc = arguments.length;\n" +
"    if (argc < 1) {\n" +
"        return false;\n" +
"    }\n" +
"    var isGMT = (arguments[argc - 1] == 'GMT');\n" +
"\n" +
"    if (isGMT) {\n" +
"        argc--;\n" +
"    }\n" +
"    // function will work even without explict handling of this case\n" +
"    if (argc == 1) {\n" +
"        var tmp = parseInt(arguments[0]);\n" +
"        if (isNaN(tmp)) {\n" +
"            return ((isGMT ? date.getUTCMonth() : date.getMonth()) ==\n" +
"getMonth(arguments[0]));\n" +
"        } else if (tmp < 32) {\n" +
"            return ((isGMT ? date.getUTCDate() : date.getDate()) == tmp);\n" +
"        } else { \n" +
"            return ((isGMT ? date.getUTCFullYear() : date.getFullYear()) ==\n" +
"tmp);\n" +
"        }\n" +
"    }\n" +
"    var year = date.getFullYear();\n" +
"    var date1, date2;\n" +
"    date1 = new Date(year,  0,  1,  0,  0,  0);\n" +
"    date2 = new Date(year, 11, 31, 23, 59, 59);\n" +
"    var adjustMonth = false;\n" +
"    for (var i = 0; i < (argc >> 1); i++) {\n" +
"        var tmp = parseInt(arguments[i]);\n" +
"        if (isNaN(tmp)) {\n" +
"            var mon = getMonth(arguments[i]);\n" +
"            date1.setMonth(mon);\n" +
"        } else if (tmp < 32) {\n" +
"            adjustMonth = (argc <= 2);\n" +
"            date1.setDate(tmp);\n" +
"        } else {\n" +
"            date1.setFullYear(tmp);\n" +
"        }\n" +
"    }\n" +
"    for (var i = (argc >> 1); i < argc; i++) {\n" +
"        var tmp = parseInt(arguments[i]);\n" +
"        if (isNaN(tmp)) {\n" +
"            var mon = getMonth(arguments[i]);\n" +
"            date2.setMonth(mon);\n" +
"        } else if (tmp < 32) {\n" +
"            date2.setDate(tmp);\n" +
"        } else {\n" +
"            date2.setFullYear(tmp);\n" +
"        }\n" +
"    }\n" +
"    if (adjustMonth) {\n" +
"        date1.setMonth(date.getMonth());\n" +
"        date2.setMonth(date.getMonth());\n" +
"    }\n" +
"    if (isGMT) {\n" +
"    var tmp = date;\n" +
"        tmp.setFullYear(date.getUTCFullYear());\n" +
"        tmp.setMonth(date.getUTCMonth());\n" +
"        tmp.setDate(date.getUTCDate());\n" +
"        tmp.setHours(date.getUTCHours());\n" +
"        tmp.setMinutes(date.getUTCMinutes());\n" +
"        tmp.setSeconds(date.getUTCSeconds());\n" +
"        date = tmp;\n" +
"    }\n" +
"    return ((date1 <= date) && (date <= date2));\n" +
"}\n" +

"function timeRange() {\n" +
"    var argc = arguments.length;\n" +
"    var date = new Date();\n" +
"    var isGMT= false;\n"+
"\n" +
"    if (argc < 1) {\n" +
"        return false;\n" +
"    }\n" +
"    if (arguments[argc - 1] == 'GMT') {\n" +
"        isGMT = true;\n" +
"        argc--;\n" +
"    }\n" +
"\n" +
"    var hour = isGMT ? date.getUTCHours() : date.getHours();\n" +
"    var date1, date2;\n" +
"    date1 = new Date();\n" +
"    date2 = new Date();\n" +
"\n" +
"    if (argc == 1) {\n" +
"        return (hour == arguments[0]);\n" +
"    } else if (argc == 2) {\n" +
"        return ((arguments[0] <= hour) && (hour <= arguments[1]));\n" +
"    } else {\n" +
"        switch (argc) {\n" +
"        case 6:\n" +
"            date1.setSeconds(arguments[2]);\n" +
"            date2.setSeconds(arguments[5]);\n" +
"        case 4:\n" +
"            var middle = argc >> 1;\n" +
"            date1.setHours(arguments[0]);\n" +
"            date1.setMinutes(arguments[1]);\n" +
"            date2.setHours(arguments[middle]);\n" +
"            date2.setMinutes(arguments[middle + 1]);\n" +
"            if (middle == 2) {\n" +
"                date2.setSeconds(59);\n" +
"            }\n" +
"            break;\n" +
"        default:\n" +
"          throw 'timeRange: bad number of arguments'\n" +
"        }\n" +
"    }\n" +
"\n" +
"    if (isGMT) {\n" +
"        date.setFullYear(date.getUTCFullYear());\n" +
"        date.setMonth(date.getUTCMonth());\n" +
"        date.setDate(date.getUTCDate());\n" +
"        date.setHours(date.getUTCHours());\n" +
"        date.setMinutes(date.getUTCMinutes());\n" +
"        date.setSeconds(date.getUTCSeconds());\n" +
"    }\n" +
"    return ((date1 <= date) && (date <= date2));\n" +
"}\n"

