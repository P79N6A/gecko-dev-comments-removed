




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

do_get_profile();

const prefService = Cc["@mozilla.org/preferences-service;1"]
                       .getService(Ci.nsIPrefBranch);

const httpserver = new HttpServer();


function repeatToLargerThan1K(data) {
    while(data.length <= 1024)
        data += data;
    return data;
}

function setupChannel(suffix, value) {
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
            .getService(Ci.nsIIOService);
    var chan = ios.newChannel("http://localhost:4444" + suffix, "", null);
    var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
    httpChan.setRequestHeader("x-request", value, false);
    
    return httpChan;
}

var tests = [
             new InitializeCacheDevices(true, false), 
             new TestCacheEntrySize(
                 function() { prefService.setIntPref("browser.cache.memory.max_entry_size", 1); },
                              "012345", "9876543210", "012345"), 
             new TestCacheEntrySize(
                 function() { prefService.setIntPref("browser.cache.memory.max_entry_size", 1); },
                              "0123456789a", "9876543210", "9876543210"), 
             new TestCacheEntrySize(
                 function() { prefService.setIntPref("browser.cache.memory.max_entry_size", -1); },
                              "0123456789a", "9876543210", "0123456789a"), 

             new InitializeCacheDevices(false, true), 
             new TestCacheEntrySize(
                 function() { prefService.setIntPref("browser.cache.disk.max_entry_size", 1); },
                              "012345", "9876543210", "012345"), 
             new TestCacheEntrySize(
                 function() { prefService.setIntPref("browser.cache.disk.max_entry_size", 1); },
                              "0123456789a", "9876543210", "9876543210"), 
             new TestCacheEntrySize(
                 function() { prefService.setIntPref("browser.cache.disk.max_entry_size", -1); },
                              "0123456789a", "9876543210", "0123456789a"), 
            ];

function nextTest() {
    
    
    syncWithCacheIOThread(function() {
        evict_cache_entries();
        syncWithCacheIOThread(runNextTest);
    });
}

function runNextTest() {
    var aTest = tests.shift();
    if (!aTest) {
        httpserver.stop(do_test_finished);
        return;
    }
    do_execute_soon(function() { aTest.start(); } );
}


function InitializeCacheDevices(memDevice, diskDevice) {
    this.start = function() {
        prefService.setBoolPref("browser.cache.memory.enable", memDevice);
        if (memDevice) {
            try {
                cap = prefService.getIntPref("browser.cache.memory.capacity");
            }
            catch(ex) {
                cap = 0;
            }
            if (cap == 0) {
                prefService.setIntPref("browser.cache.memory.capacity", 1024);
            }
        }
        prefService.setBoolPref("browser.cache.disk.enable", diskDevice);
        if (diskDevice) {
            try {
                cap = prefService.getIntPref("browser.cache.disk.capacity");
            }
            catch(ex) {
                cap = 0;
            }
            if (cap == 0) {
                prefService.setIntPref("browser.cache.disk.capacity", 1024);
            }
        }
        var channel = setupChannel("/bug650995", "Initial value");
        channel.asyncOpen(new ChannelListener(
            nextTest, null),
            null);
    }
}

function TestCacheEntrySize(setSizeFunc, firstRequest, secondRequest, secondExpectedReply) {

    
    
    
    if (firstRequest.length > 10)
        firstRequest = repeatToLargerThan1K(firstRequest);
    if (secondExpectedReply.length > 10)
        secondExpectedReply = repeatToLargerThan1K(secondExpectedReply);

    this.start = function() {
        setSizeFunc();
        var channel = setupChannel("/bug650995", firstRequest);
        channel.asyncOpen(new ChannelListener(this.initialLoad, this), null);
    },

    this.initialLoad = function(request, data, ctx) {
        do_check_eq(firstRequest, data);
        var channel = setupChannel("/bug650995", secondRequest);
        do_execute_soon(function() {
            channel.asyncOpen(new ChannelListener(ctx.testAndTriggerNext, ctx), null);
            });
    },

    this.testAndTriggerNext = function(request, data, ctx) {
        do_check_eq(secondExpectedReply, data);
        do_execute_soon(nextTest);
    }
}

function run_test()
{
    httpserver.registerPathHandler("/bug650995", handler);
    httpserver.start(4444);

    prefService.setBoolPref("browser.cache.offline.enable", false);

    nextTest();
    do_test_pending();
}

function handler(metadata, response) {
    var body = "BOOM!";
    try {
        body = metadata.getHeader("x-request");
    } catch(e) {}

    response.setStatusLine(metadata.httpVersion, 200, "Ok");
    response.setHeader("Content-Type", "text/plain", false);
    response.setHeader("Cache-Control", "max-age=3600", false);
    response.bodyOutputStream.write(body, body.length);
}
