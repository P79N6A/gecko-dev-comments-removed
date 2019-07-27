


"use strict";





const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu, Constructor: CC } = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/DirectoryLinksProvider.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Http.jsm");
Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NewTabUtils",
  "resource://gre/modules/NewTabUtils.jsm");

do_get_profile();

const DIRECTORY_LINKS_FILE = "directoryLinks.json";
const DIRECTORY_FRECENCY = 1000;
const kURLData = {"en-US": [{"url":"http://example.com","title":"LocalSource"}]};
const kTestURL = 'data:application/json,' + JSON.stringify(kURLData);


const kLocalePref = DirectoryLinksProvider._observedPrefs.prefSelectedLocale;
const kSourceUrlPref = DirectoryLinksProvider._observedPrefs.linksURL;
const kPingUrlPref = "browser.newtabpage.directory.ping";
const kNewtabEnhancedPref = "browser.newtabpage.enhanced";


var server;
const kDefaultServerPort = 9000;
const kBaseUrl = "http://localhost:" + kDefaultServerPort;
const kExamplePath = "/exampleTest/";
const kFailPath = "/fail/";
const kPingPath = "/ping/";
const kExampleURL = kBaseUrl + kExamplePath;
const kFailURL = kBaseUrl + kFailPath;
const kPingUrl = kBaseUrl + kPingPath;


Services.prefs.setCharPref(kLocalePref, "en-US");
Services.prefs.setCharPref(kSourceUrlPref, kTestURL);
Services.prefs.setCharPref(kPingUrlPref, kPingUrl);
Services.prefs.setBoolPref(kNewtabEnhancedPref, true);

const kHttpHandlerData = {};
kHttpHandlerData[kExamplePath] = {"en-US": [{"url":"http://example.com","title":"RemoteSource"}]};

const BinaryInputStream = CC("@mozilla.org/binaryinputstream;1",
                              "nsIBinaryInputStream",
                              "setInputStream");

let gLastRequestPath;

let relatedTile1 = {
  url: "http://turbotax.com",
  type: "suggested",
  lastVisitDate: 4,
  suggested: [
    "taxact.com",
    "hrblock.com",
    "1040.com",
    "taxslayer.com"
  ]
};
let relatedTile2 = {
  url: "http://irs.gov",
  type: "suggested",
  lastVisitDate: 3,
  suggested: [
    "taxact.com",
    "hrblock.com",
    "freetaxusa.com",
    "taxslayer.com"
  ]
};
let relatedTile3 = {
  url: "http://hrblock.com",
  type: "suggested",
  lastVisitDate: 2,
  suggested: [
    "taxact.com",
    "freetaxusa.com",
    "1040.com",
    "taxslayer.com"
  ]
};
let someOtherSite = {url: "http://someothersite.com", title: "Not_A_Related_Site"};

function getHttpHandler(path) {
  let code = 200;
  let body = JSON.stringify(kHttpHandlerData[path]);
  if (path == kFailPath) {
    code = 204;
  }
  return function(aRequest, aResponse) {
    gLastRequestPath = aRequest.path;
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
    return observer.deferred.promise.then(() => {
      DirectoryLinksProvider.removeObserver(observer);
    });
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
  NewTabUtils.init();

  run_next_test();

  
  do_register_cleanup(function() {
    server.stop(function() { });
    DirectoryLinksProvider.reset();
    Services.prefs.clearUserPref(kLocalePref);
    Services.prefs.clearUserPref(kSourceUrlPref);
    Services.prefs.clearUserPref(kPingUrlPref);
    Services.prefs.clearUserPref(kNewtabEnhancedPref);
  });
}

add_task(function test_updateRelatedTile() {
  let topSites = ["site0.com", "1040.com", "site2.com", "hrblock.com", "site4.com", "freetaxusa.com", "site6.com"];

  
  let data = {"en-US": [relatedTile1, relatedTile2, relatedTile3, someOtherSite]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);

  let testObserver = new TestFirstRun();
  DirectoryLinksProvider.addObserver(testObserver);

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});
  let links = yield fetchData();

  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = function(site) {
    return topSites.indexOf(site) >= 0;
  }

  let origGetProviderLinks = NewTabUtils.getProviderLinks;
  NewTabUtils.getProviderLinks = function(provider) {
    return links;
  }

  do_check_eq(DirectoryLinksProvider._updateRelatedTile(), undefined);

  function TestFirstRun() {
    this.promise = new Promise(resolve => {
      this.onLinkChanged = (directoryLinksProvider, link) => {
        links.unshift(link);
        let possibleLinks = [relatedTile1.url, relatedTile2.url, relatedTile3.url];

        isIdentical([...DirectoryLinksProvider._topSitesWithRelatedLinks], ["hrblock.com", "1040.com", "freetaxusa.com"]);
        do_check_true(possibleLinks.indexOf(link.url) > -1);
        do_check_eq(link.frecency, Infinity);
        do_check_eq(link.type, "suggested");
        resolve();
      };
    });
  }

  function TestChangingRelatedTile() {
    this.count = 0;
    this.promise = new Promise(resolve => {
      this.onLinkChanged = (directoryLinksProvider, link) => {
        this.count++;
        let possibleLinks = [relatedTile1.url, relatedTile2.url, relatedTile3.url];

        do_check_true(possibleLinks.indexOf(link.url) > -1);
        do_check_eq(link.type, "suggested");
        do_check_true(this.count <= 2);

        if (this.count == 1) {
          
          do_check_eq(link.url, links.shift().url);
          do_check_eq(link.frecency, 0);
        } else {
          links.unshift(link);
          do_check_eq(link.frecency, Infinity);
        }
        isIdentical([...DirectoryLinksProvider._topSitesWithRelatedLinks], ["hrblock.com", "freetaxusa.com"]);
        resolve();
      }
    });
  }

  function TestRemovingRelatedTile() {
    this.count = 0;
    this.promise = new Promise(resolve => {
      this.onLinkChanged = (directoryLinksProvider, link) => {
        this.count++;

        do_check_eq(link.type, "suggested");
        do_check_eq(this.count, 1);
        do_check_eq(link.frecency, 0);
        do_check_eq(link.url, links.shift().url);
        isIdentical([...DirectoryLinksProvider._topSitesWithRelatedLinks], []);
        resolve();
      }
    });
  }

  
  yield testObserver.promise;
  DirectoryLinksProvider.removeObserver(testObserver);

  
  
  let removedTopsite = topSites.shift();
  do_check_eq(removedTopsite, "site0.com");
  do_check_false(NewTabUtils.isTopPlacesSite(removedTopsite));
  let updateRelatedTile = DirectoryLinksProvider._handleLinkChanged({
    url: "http://" + removedTopsite,
    type: "history",
  });
  do_check_false(updateRelatedTile);

  
  
  testObserver = new TestChangingRelatedTile();
  DirectoryLinksProvider.addObserver(testObserver);
  removedTopsite = topSites.shift();
  do_check_eq(removedTopsite, "1040.com");
  do_check_false(NewTabUtils.isTopPlacesSite(removedTopsite));
  DirectoryLinksProvider.onLinkChanged(DirectoryLinksProvider, {
    url: "http://" + removedTopsite,
    type: "history",
  });
  yield testObserver.promise;
  do_check_eq(testObserver.count, 2);
  DirectoryLinksProvider.removeObserver(testObserver);

  
  
  topSites = [];
  testObserver = new TestRemovingRelatedTile();
  DirectoryLinksProvider.addObserver(testObserver);
  DirectoryLinksProvider.onManyLinksChanged();
  yield testObserver.promise;

  
  yield promiseCleanDirectoryLinksProvider();
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  NewTabUtils.getProviderLinks = origGetProviderLinks;
});

add_task(function test_relatedLinksMap() {
  let data = {"en-US": [relatedTile1, relatedTile2, relatedTile3, someOtherSite]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});
  let links = yield fetchData();

  
  do_check_eq(links.length, 1);
  let expected_data = [{url: "http://someothersite.com", title: "Not_A_Related_Site", frecency: DIRECTORY_FRECENCY, lastVisitDate: 1}];
  isIdentical(links, expected_data);

  
  expected_data = {
    "taxact.com": [relatedTile1, relatedTile2, relatedTile3],
    "hrblock.com": [relatedTile1, relatedTile2],
    "1040.com": [relatedTile1, relatedTile3],
    "taxslayer.com": [relatedTile1, relatedTile2, relatedTile3],
    "freetaxusa.com": [relatedTile2, relatedTile3],
  };

  DirectoryLinksProvider._relatedLinks.forEach((relatedLinks, site) => {
    let relatedLinksItr = relatedLinks.values();
    for (let link of expected_data[site]) {
      isIdentical(relatedLinksItr.next().value, link);
    }
  })

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_topSitesWithRelatedLinks() {
  let topSites = ["site0.com", "1040.com", "site2.com", "hrblock.com", "site4.com", "freetaxusa.com", "site6.com"];
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = function(site) {
    return topSites.indexOf(site) >= 0;
  }

  
  let origGetProviderLinks = NewTabUtils.getProviderLinks;
  NewTabUtils.getProviderLinks = function(provider) {
    return [];
  }

  
  do_check_eq(DirectoryLinksProvider._topSitesWithRelatedLinks.size, 0);

  let data = {"en-US": [relatedTile1, relatedTile2, relatedTile3, someOtherSite]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});
  let links = yield fetchData();

  
  do_check_eq(DirectoryLinksProvider._relatedLinks.size, 5);

  
  let expectedTopSitesWithRelatedLinks = ["hrblock.com", "1040.com", "freetaxusa.com"];
  DirectoryLinksProvider._handleManyLinksChanged();
  isIdentical([...DirectoryLinksProvider._topSitesWithRelatedLinks], expectedTopSitesWithRelatedLinks);

  
  let popped = topSites.pop();
  DirectoryLinksProvider._handleLinkChanged({url: "http://" + popped});
  isIdentical([...DirectoryLinksProvider._topSitesWithRelatedLinks], expectedTopSitesWithRelatedLinks);

  
  popped = topSites.pop();
  expectedTopSitesWithRelatedLinks.pop();
  DirectoryLinksProvider._handleLinkChanged({url: "http://" + popped});
  isIdentical([...DirectoryLinksProvider._topSitesWithRelatedLinks], expectedTopSitesWithRelatedLinks);

  
  topSites.push(popped);
  expectedTopSitesWithRelatedLinks.push(popped);
  DirectoryLinksProvider._handleLinkChanged({url: "http://" + popped});
  isIdentical([...DirectoryLinksProvider._topSitesWithRelatedLinks], expectedTopSitesWithRelatedLinks);

  
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  NewTabUtils.getProviderLinks = origGetProviderLinks;
});

add_task(function test_reportSitesAction() {
  yield DirectoryLinksProvider.init();
  let deferred, expectedPath, expectedPost;
  let done = false;
  server.registerPrefixHandler(kPingPath, (aRequest, aResponse) => {
    if (done) {
      return;
    }

    do_check_eq(aRequest.path, expectedPath);

    let bodyStream = new BinaryInputStream(aRequest.bodyInputStream);
    let bodyObject = JSON.parse(NetUtil.readInputStreamToString(bodyStream, bodyStream.available()));
    isIdentical(bodyObject, expectedPost);

    deferred.resolve();
  });

  function sendPingAndTest(path, action, index) {
    deferred = Promise.defer();
    expectedPath = kPingPath + path;
    DirectoryLinksProvider.reportSitesAction(sites, action, index);
    return deferred.promise;
  }

  
  let sites = [,,{
    isPinned: _ => true,
    link: {
      directoryId: 1,
      frecency: 30000,
      url: "http://directory1/",
    },
  }];

  
  
  expectedPost = JSON.parse(JSON.stringify({
    click: 0,
    locale: "en-US",
    tiles: [{
      id: 1,
      pin: 1,
      pos: 2,
      score: 3,
      url: undefined,
    }],
  }));
  yield sendPingAndTest("click", "click", 2);

  
  delete expectedPost.click;
  expectedPost.pin = 0;
  yield sendPingAndTest("click", "pin", 2);

  
  delete expectedPost.pin;
  expectedPost.block = 0;
  yield sendPingAndTest("click", "block", 2);

  
  delete expectedPost.block;
  expectedPost.view = 0;
  yield sendPingAndTest("view", "view", 2);

  
  delete sites[2].link.directoryId;
  delete expectedPost.tiles[0].id;
  yield sendPingAndTest("view", "view", 2);

  
  sites[0] = {
    isPinned: _ => false,
    link: {
      directoryId: 1234,
      frecency: 1000,
      url: "http://directory/",
    }
  };
  expectedPost.tiles.unshift(JSON.parse(JSON.stringify({
    id: 1234,
    pin: undefined,
    pos: undefined,
    score: undefined,
    url: undefined,
  })));
  expectedPost.view = 1;
  yield sendPingAndTest("view", "view", 2);

  
  sites[2].enhancedId = "id from enhanced";
  expectedPost.tiles[1].id = "id from enhanced";
  expectedPost.tiles[1].url = "";
  yield sendPingAndTest("view", "view", 2);

  
  delete expectedPost.view;
  expectedPost.click = 0;
  yield sendPingAndTest("click", "click", 0);

  
  expectedPost.click = 1;
  yield sendPingAndTest("click", "click", 2);

  done = true;
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
  
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, kExampleURL + "%LOCALE%");
  do_check_eq(gLastRequestPath, kExamplePath + "en-US");
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
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, kFailURL);
  do_check_eq(gLastRequestPath, kFailPath);
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
  expected_data = [{url: "http://example.com", title: "US", frecency: DIRECTORY_FRECENCY, lastVisitDate: 1}];
  isIdentical(links, expected_data);

  yield promiseDirectoryDownloadOnPrefChange("general.useragent.locale", "zh-CN");

  links = yield fetchData();
  do_check_eq(links.length, 2)
  expected_data = [
    {url: "http://example.net", title: "CN", frecency: DIRECTORY_FRECENCY, lastVisitDate: 2},
    {url: "http://example.net/2", title: "CN2", frecency: DIRECTORY_FRECENCY, lastVisitDate: 1}
  ];
  isIdentical(links, expected_data);

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider__prefObserver_url() {
  yield promiseSetupDirectoryLinksProvider({linksURL: kTestURL});

  let links = yield fetchData();
  do_check_eq(links.length, 1);
  let expectedData =  [{url: "http://example.com", title: "LocalSource", frecency: DIRECTORY_FRECENCY, lastVisitDate: 1}];
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

add_task(function test_DirectoryLinksProvider_getLinks_badData() {
  let data = {
    "en-US": {
      "en-US": [{url: "http://example.com", title: "US"}],
    },
  };
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  
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
  do_check_eq(gLastRequestPath, kExamplePath);
  let data = yield readJsonFile();
  isIdentical(data, kHttpHandlerData[kExamplePath]);

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_fetchDirectoryOnShow() {
  yield promiseSetupDirectoryLinksProvider();

  
  DirectoryLinksProvider._lastDownloadMS = 0;
  do_check_true(DirectoryLinksProvider._needsDownload);

  
  yield DirectoryLinksProvider.reportSitesAction([], "view");
  do_check_true(DirectoryLinksProvider._lastDownloadMS != 0);

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

add_task(function test_DirectoryLinksProvider_getAllowedLinks() {
  let data = {"en-US": [
    {url: "ftp://example.com"},
    {url: "http://example.net"},
    {url: "javascript:5"},
    {url: "https://example.com"},
    {url: "httpJUNKjavascript:42"},
    {url: "data:text/plain,hi"},
    {url: "http/bork:eh"},
  ]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  let links = yield fetchData();
  do_check_eq(links.length, 2);

  
  do_check_eq(links[0].url, data["en-US"][1].url);
  do_check_eq(links[1].url, data["en-US"][3].url);
});

add_task(function test_DirectoryLinksProvider_getAllowedImages() {
  let data = {"en-US": [
    {url: "http://example.com", imageURI: "ftp://example.com"},
    {url: "http://example.com", imageURI: "http://example.net"},
    {url: "http://example.com", imageURI: "javascript:5"},
    {url: "http://example.com", imageURI: "https://example.com"},
    {url: "http://example.com", imageURI: "httpJUNKjavascript:42"},
    {url: "http://example.com", imageURI: "data:text/plain,hi"},
    {url: "http://example.com", imageURI: "http/bork:eh"},
  ]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  let links = yield fetchData();
  do_check_eq(links.length, 2);

  
  do_check_eq(links[0].imageURI, data["en-US"][3].imageURI);
  do_check_eq(links[1].imageURI, data["en-US"][5].imageURI);
});

add_task(function test_DirectoryLinksProvider_getAllowedEnhancedImages() {
  let data = {"en-US": [
    {url: "http://example.com", enhancedImageURI: "ftp://example.com"},
    {url: "http://example.com", enhancedImageURI: "http://example.net"},
    {url: "http://example.com", enhancedImageURI: "javascript:5"},
    {url: "http://example.com", enhancedImageURI: "https://example.com"},
    {url: "http://example.com", enhancedImageURI: "httpJUNKjavascript:42"},
    {url: "http://example.com", enhancedImageURI: "data:text/plain,hi"},
    {url: "http://example.com", enhancedImageURI: "http/bork:eh"},
  ]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  let links = yield fetchData();
  do_check_eq(links.length, 2);

  
  do_check_eq(links[0].enhancedImageURI, data["en-US"][3].enhancedImageURI);
  do_check_eq(links[1].enhancedImageURI, data["en-US"][5].enhancedImageURI);
});

add_task(function test_DirectoryLinksProvider_getEnhancedLink() {
  let data = {"en-US": [
    {url: "http://example.net", enhancedImageURI: "data:,net1"},
    {url: "http://example.com", enhancedImageURI: "data:,com1"},
    {url: "http://example.com", enhancedImageURI: "data:,com2"},
  ]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  let links = yield fetchData();
  do_check_eq(links.length, 3);

  function checkEnhanced(url, image) {
    let enhanced = DirectoryLinksProvider.getEnhancedLink({url: url});
    do_check_eq(enhanced && enhanced.enhancedImageURI, image);
  }

  
  checkEnhanced("http://example.net/", "data:,net1");
  checkEnhanced("http://example.net/path", "data:,net1");
  checkEnhanced("https://www.example.net/", "data:,net1");
  checkEnhanced("https://www3.example.net/", "data:,net1");

  
  checkEnhanced("http://example.com", "data:,com2");

  
  let inline = DirectoryLinksProvider.getEnhancedLink({
    url: "http://example.com/echo",
    enhancedImageURI: "data:,echo",
  });
  do_check_eq(inline.enhancedImageURI, "data:,echo");
  do_check_eq(inline.url, "http://example.com/echo");

  
  checkEnhanced("http://sub.example.net/", undefined);
  checkEnhanced("http://example.org", undefined);
  checkEnhanced("http://localhost", undefined);
  checkEnhanced("http://127.0.0.1", undefined);

  
  data = {"en-US": [
    {url: "http://example.com", enhancedImageURI: "data:,fresh"},
  ]};
  dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});
  links = yield fetchData();
  do_check_eq(links.length, 1);
  checkEnhanced("http://example.net", undefined);
  checkEnhanced("http://example.com", "data:,fresh");
});

add_task(function test_DirectoryLinksProvider_setDefaultEnhanced() {
  function checkDefault(expected) {
    Services.prefs.clearUserPref(kNewtabEnhancedPref);
    do_check_eq(Services.prefs.getBoolPref(kNewtabEnhancedPref), expected);
  }

  
  Services.prefs.clearUserPref("privacy.donottrackheader.enabled");
  checkDefault(true);

  
  Services.prefs.setBoolPref("privacy.donottrackheader.enabled", true);
  checkDefault(false);

  
  Services.prefs.clearUserPref("privacy.donottrackheader.enabled");
  checkDefault(true);

  
  Services.prefs.clearUserPref("privacy.donottrackheader.value");
});
