


"use strict";





const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu, Constructor: CC } = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DirectoryLinksProvider.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Http.jsm");
Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/osfile.jsm")
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

do_get_profile();

const DIRECTORY_LINKS_FILE = "directoryLinks.json";
const DIRECTORY_FRECENCY = 1000;
const kURLData = {"en-US": [{"url":"http://example.com","title":"LocalSource"}]};
const kTestURL = 'data:application/json,' + JSON.stringify(kURLData);


const kLocalePref = DirectoryLinksProvider._observedPrefs.prefSelectedLocale;
const kSourceUrlPref = DirectoryLinksProvider._observedPrefs.linksURL;
const kReportClickUrlPref = "browser.newtabpage.directory.reportClickEndPoint";
const kTelemetryEnabledPref = "toolkit.telemetry.enabled";


var server;
const kDefaultServerPort = 9000;
const kBaseUrl = "http://localhost:" + kDefaultServerPort;
const kExamplePath = "/exampleTest/";
const kFailPath = "/fail/";
const kReportClickPath = "/reportClick/";
const kExampleURL = kBaseUrl + kExamplePath;
const kFailURL = kBaseUrl + kFailPath;
const kReportClickUrl = kBaseUrl + kReportClickPath;


Services.prefs.setCharPref(kLocalePref, "en-US");
Services.prefs.setCharPref(kSourceUrlPref, kTestURL);
Services.prefs.setCharPref(kReportClickUrlPref, kReportClickUrl);
Services.prefs.setBoolPref(kTelemetryEnabledPref, true);

const kHttpHandlerData = {};
kHttpHandlerData[kExamplePath] = {"en-US": [{"url":"http://example.com","title":"RemoteSource"}]};

const expectedBodyObject = {locale: DirectoryLinksProvider.locale};
const BinaryInputStream = CC("@mozilla.org/binaryinputstream;1",
                              "nsIBinaryInputStream",
                              "setInputStream");

function getHttpHandler(path) {
  let code = 200;
  let body = JSON.stringify(kHttpHandlerData[path]);
  if (path == kFailPath) {
    code = 204;
  }
  return function(aRequest, aResponse) {
    let bodyStream = new BinaryInputStream(aRequest.bodyInputStream);
    let bodyObject = JSON.parse(NetUtil.readInputStreamToString(bodyStream, bodyStream.available()));
    isIdentical(bodyObject, expectedBodyObject);

    aResponse.setStatusLine(null, code);
    aResponse.setHeader("Content-Type", "application/json");
    aResponse.write(body);
  };
}

function isIdentical(actual, expected) {
  if (expected == null) {
    do_check_eq(actual, expected);
  }
  else if (typeof expected == "object") {
    
    do_check_eq(Object.keys(actual).sort() + "", Object.keys(expected).sort());

    
    Object.keys(expected).forEach(key => {
      isIdentical(actual[key], expected[key]);
    });
  }
  else {
    do_check_eq(actual, expected);
  }
}

function fetchData() {
  let deferred = Promise.defer();

  DirectoryLinksProvider.getLinks(linkData => {
    deferred.resolve(linkData);
  });
  return deferred.promise;
}

function readJsonFile(jsonFile = DIRECTORY_LINKS_FILE) {
  let decoder = new TextDecoder();
  let directoryLinksFilePath = OS.Path.join(OS.Constants.Path.localProfileDir, jsonFile);
  return OS.File.read(directoryLinksFilePath).then(array => {
    let json = decoder.decode(array);
    return JSON.parse(json);
  }, () => { return "" });
}

function cleanJsonFile(jsonFile = DIRECTORY_LINKS_FILE) {
  let directoryLinksFilePath = OS.Path.join(OS.Constants.Path.localProfileDir, jsonFile);
  return OS.File.remove(directoryLinksFilePath);
}

function LinksChangeObserver() {
  this.deferred = Promise.defer();
  this.onManyLinksChanged = () => this.deferred.resolve();
  this.onDownloadFail = this.onManyLinksChanged;
}

function promiseDirectoryDownloadOnPrefChange(pref, newValue) {
  let oldValue = Services.prefs.getCharPref(pref);
  if (oldValue != newValue) {
    
    
    
    let observer = new LinksChangeObserver();
    DirectoryLinksProvider.addObserver(observer);
    Services.prefs.setCharPref(pref, newValue);
    return observer.deferred.promise;
  }
  return Promise.resolve();
}

function promiseSetupDirectoryLinksProvider(options = {}) {
  return Task.spawn(function() {
    let linksURL = options.linksURL || kTestURL;
    yield DirectoryLinksProvider.init();
    yield promiseDirectoryDownloadOnPrefChange(kLocalePref, options.locale || "en-US");
    yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, linksURL);
    do_check_eq(DirectoryLinksProvider._linksURL, linksURL);
    DirectoryLinksProvider._lastDownloadMS = options.lastDownloadMS || 0;
  });
}

function promiseCleanDirectoryLinksProvider() {
  return Task.spawn(function() {
    yield promiseDirectoryDownloadOnPrefChange(kLocalePref, "en-US");
    yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, kTestURL);
    DirectoryLinksProvider._lastDownloadMS  = 0;
    DirectoryLinksProvider.reset();
  });
}

function run_test() {
  
  server = new HttpServer();
  server.registerPrefixHandler(kExamplePath, getHttpHandler(kExamplePath));
  server.registerPrefixHandler(kFailPath, getHttpHandler(kFailPath));
  server.start(kDefaultServerPort);

  run_next_test();

  
  do_register_cleanup(function() {
    server.stop(function() { });
    DirectoryLinksProvider.reset();
    Services.prefs.clearUserPref(kLocalePref);
    Services.prefs.clearUserPref(kSourceUrlPref);
    Services.prefs.clearUserPref(kReportClickUrlPref);
    Services.prefs.clearUserPref(kTelemetryEnabledPref);
  });
}

add_task(function test_reportLinkAction() {
  let link = 1;
  let action = "click";
  let tile = 2;
  let score = 3;
  let pin = 1;
  let expectedQuery = "list=&link=1&action=click&tile=2&score=3&pin=1"
  let expectedPath = kReportClickPath;

  let deferred = Promise.defer();
  server.registerPrefixHandler(kReportClickPath, (aRequest, aResponse) => {
    do_check_eq(aRequest.path, expectedPath);
    do_check_eq(aRequest.queryString, expectedQuery);
    deferred.resolve();
  });

  DirectoryLinksProvider.reportLinkAction({directoryIndex: link, frecency: score}, action, tile, pin);
  return deferred.promise;
});

add_task(function test_fetchAndCacheLinks_local() {
  yield DirectoryLinksProvider.init();
  yield cleanJsonFile();
  
  yield DirectoryLinksProvider._fetchAndCacheLinks(kTestURL);
  let data = yield readJsonFile();
  isIdentical(data, kURLData);
});

add_task(function test_fetchAndCacheLinks_remote() {
  yield DirectoryLinksProvider.init();
  yield cleanJsonFile();
  
  yield DirectoryLinksProvider._fetchAndCacheLinks(kExampleURL);
  let data = yield readJsonFile();
  isIdentical(data, kHttpHandlerData[kExamplePath]);
});

add_task(function test_fetchAndCacheLinks_malformedURI() {
  yield DirectoryLinksProvider.init();
  yield cleanJsonFile();
  let someJunk = "some junk";
  try {
    yield DirectoryLinksProvider._fetchAndCacheLinks(someJunk);
    do_throw("Malformed URIs should fail")
  } catch (e) {
    do_check_eq(e, "Error fetching " + someJunk)
  }

  
  let data = yield readJsonFile();
  isIdentical(data, "");
});

add_task(function test_fetchAndCacheLinks_unknownHost() {
  yield DirectoryLinksProvider.init();
  yield cleanJsonFile();
  let nonExistentServer = "http://localhost:56789/";
  try {
    yield DirectoryLinksProvider._fetchAndCacheLinks(nonExistentServer);
    do_throw("BAD URIs should fail");
  } catch (e) {
    do_check_true(e.startsWith("Fetching " + nonExistentServer + " results in error code: "))
  }

  
  let data = yield readJsonFile();
  isIdentical(data, "");
});

add_task(function test_fetchAndCacheLinks_non200Status() {
  yield DirectoryLinksProvider.init();
  yield cleanJsonFile();
  yield DirectoryLinksProvider._fetchAndCacheLinks(kFailURL);
  let data = yield readJsonFile();
  isIdentical(data, {});
});


add_task(function test_DirectoryLinksProvider__linkObservers() {
  yield DirectoryLinksProvider.init();

  let testObserver = new LinksChangeObserver();
  DirectoryLinksProvider.addObserver(testObserver);
  do_check_eq(DirectoryLinksProvider._observers.size, 1);
  DirectoryLinksProvider._fetchAndCacheLinksIfNecessary(true);

  yield testObserver.deferred.promise;
  DirectoryLinksProvider._removeObservers();
  do_check_eq(DirectoryLinksProvider._observers.size, 0);

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_linksURL_locale() {
  let data = {
    "en-US": [{url: "http://example.com", title: "US"}],
    "zh-CN": [
              {url: "http://example.net", title: "CN"},
              {url:"http://example.net/2", title: "CN2"}
    ],
  };
  let dataURI = 'data:application/json,' + JSON.stringify(data);

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  let links;
  let expected_data;

  links = yield fetchData();
  do_check_eq(links.length, 1);
  expected_data = [{url: "http://example.com", title: "US", frecency: DIRECTORY_FRECENCY, lastVisitDate: 1, directoryIndex: 0}];
  isIdentical(links, expected_data);

  yield promiseDirectoryDownloadOnPrefChange("general.useragent.locale", "zh-CN");

  links = yield fetchData();
  do_check_eq(links.length, 2)
  expected_data = [
    {url: "http://example.net", title: "CN", frecency: DIRECTORY_FRECENCY, lastVisitDate: 2, directoryIndex: 0},
    {url: "http://example.net/2", title: "CN2", frecency: DIRECTORY_FRECENCY, lastVisitDate: 1, directoryIndex: 1}
  ];
  isIdentical(links, expected_data);

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider__prefObserver_url() {
  yield promiseSetupDirectoryLinksProvider({linksURL: kTestURL});

  let links = yield fetchData();
  do_check_eq(links.length, 1);
  let expectedData =  [{url: "http://example.com", title: "LocalSource", frecency: DIRECTORY_FRECENCY, lastVisitDate: 1, directoryIndex: 0}];
  isIdentical(links, expectedData);

  
  
  
  let exampleUrl = 'http://localhost:56789/bad';
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, exampleUrl);
  do_check_eq(DirectoryLinksProvider._linksURL, exampleUrl);

  
  let newLinks = yield fetchData();
  isIdentical(newLinks, expectedData);

  
  yield cleanJsonFile();
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, exampleUrl + " ");
  
  newLinks = yield fetchData();
  isIdentical(newLinks, []);

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_getLinks_noLocaleData() {
  yield promiseSetupDirectoryLinksProvider({locale: 'zh-CN'});
  let links = yield fetchData();
  do_check_eq(links.length, 0);
  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_needsDownload() {
  
  DirectoryLinksProvider._lastDownloadMS = 0;
  do_check_true(DirectoryLinksProvider._needsDownload);
  DirectoryLinksProvider._lastDownloadMS = Date.now();
  do_check_false(DirectoryLinksProvider._needsDownload);
  DirectoryLinksProvider._lastDownloadMS = Date.now() - (60*60*24 + 1)*1000;
  do_check_true(DirectoryLinksProvider._needsDownload);
  DirectoryLinksProvider._lastDownloadMS = 0;
});

add_task(function test_DirectoryLinksProvider_fetchAndCacheLinksIfNecessary() {
  yield DirectoryLinksProvider.init();
  yield cleanJsonFile();
  
  yield promiseSetupDirectoryLinksProvider({linksURL: kTestURL+" "});
  yield DirectoryLinksProvider._fetchAndCacheLinksIfNecessary();

  
  let lastDownloadMS = DirectoryLinksProvider._lastDownloadMS;
  do_check_true((Date.now() - lastDownloadMS) < 5000);

  
  let data = yield readJsonFile();
  isIdentical(data, kURLData);

  
  yield DirectoryLinksProvider._fetchAndCacheLinksIfNecessary();
  do_check_eq(DirectoryLinksProvider._lastDownloadMS, lastDownloadMS);

  
  yield cleanJsonFile();
  yield DirectoryLinksProvider._fetchAndCacheLinksIfNecessary(true);
  data = yield readJsonFile();
  isIdentical(data, kURLData);

  
  lastDownloadMS = DirectoryLinksProvider._lastDownloadMS;
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, "http://");
  yield DirectoryLinksProvider._fetchAndCacheLinksIfNecessary(true);
  data = yield readJsonFile();
  isIdentical(data, kURLData);
  do_check_eq(DirectoryLinksProvider._lastDownloadMS, lastDownloadMS);

  
  let downloadPromise = DirectoryLinksProvider._fetchAndCacheLinksIfNecessary(true);
  let anotherPromise = DirectoryLinksProvider._fetchAndCacheLinksIfNecessary(true);
  do_check_true(downloadPromise === anotherPromise);
  yield downloadPromise;

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_fetchDirectoryOnPrefChange() {
  yield DirectoryLinksProvider.init();

  let testObserver = new LinksChangeObserver();
  DirectoryLinksProvider.addObserver(testObserver);

  yield cleanJsonFile();
  
  do_check_false(DirectoryLinksProvider._needsDownload);

  
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, kExampleURL);
  
  yield testObserver.deferred.promise;
  let data = yield readJsonFile();
  isIdentical(data, kHttpHandlerData[kExamplePath]);

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_fetchDirectoryOnShowCount() {
  yield promiseSetupDirectoryLinksProvider();

  
  DirectoryLinksProvider._lastDownloadMS = 0;
  do_check_true(DirectoryLinksProvider._needsDownload);

  
  let directoryCount = {sponsored: 0};
  yield DirectoryLinksProvider.reportShownCount(directoryCount);
  
  do_check_eq(DirectoryLinksProvider._lastDownloadMS, 0);

  
  directoryCount.sponsored = 1;
  yield DirectoryLinksProvider.reportShownCount(directoryCount);
  do_check_true(DirectoryLinksProvider._lastDownloadMS != 0);

  
  expectedBodyObject.directoryCount = directoryCount;
  
  
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, kExampleURL);
  
  delete expectedBodyObject.directoryCount;

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_fetchDirectoryOnInit() {
  
  yield promiseSetupDirectoryLinksProvider();
  
  yield promiseCleanDirectoryLinksProvider();

  yield cleanJsonFile();
  yield DirectoryLinksProvider.init();
  let data = yield readJsonFile();
  isIdentical(data, kURLData);

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_getLinksFromCorruptedFile() {
  yield promiseSetupDirectoryLinksProvider();

  
  let directoryLinksFilePath = OS.Path.join(OS.Constants.Path.profileDir, DIRECTORY_LINKS_FILE);
  yield OS.File.writeAtomic(directoryLinksFilePath, '{"en-US":');
  let data = yield fetchData();
  isIdentical(data, []);

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_getEnhancedLink() {
  let data = {"en-US": [
    {url: "http://example.net", enhancedImageURI: "net1"},
    {url: "http://example.com", enhancedImageURI: "com1"},
    {url: "http://example.com", enhancedImageURI: "com2"},
  ]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  let links = yield fetchData();
  do_check_eq(links.length, 3);

  function checkEnhanced(url, image) {
    let enhanced = DirectoryLinksProvider.getEnhancedLink({url: url});
    do_check_eq(enhanced && enhanced.enhancedImageURI, image);
  }

  
  checkEnhanced("http://example.net/", "net1");
  checkEnhanced("http://sub.example.net/", "net1");
  checkEnhanced("http://example.net/path", "net1");
  checkEnhanced("https://www.example.net/", "net1");

  
  checkEnhanced("http://example.com", "com2");

  
  let inline = DirectoryLinksProvider.getEnhancedLink({
    url: "http://example.com/echo",
    enhancedImageURI: "echo",
  });
  do_check_eq(inline.enhancedImageURI, "echo");
  do_check_eq(inline.url, "http://example.com/echo");

  
  checkEnhanced("http://example.org", undefined);
  checkEnhanced("http://localhost", undefined);
  checkEnhanced("http://127.0.0.1", undefined);

  
  data = {"en-US": [
    {url: "http://example.com", enhancedImageURI: "fresh"},
  ]};
  dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});
  links = yield fetchData();
  do_check_eq(links.length, 1);
  checkEnhanced("http://example.net", undefined);
  checkEnhanced("http://example.com", "fresh");
});
