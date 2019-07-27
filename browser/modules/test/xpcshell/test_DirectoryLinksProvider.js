


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
Cu.import("resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NewTabUtils",
  "resource://gre/modules/NewTabUtils.jsm");

do_get_profile();

const DIRECTORY_LINKS_FILE = "directoryLinks.json";
const DIRECTORY_FRECENCY = 1000;
const SUGGESTED_FRECENCY = Infinity;
const kURLData = {"directory": [{"url":"http://example.com","title":"LocalSource"}]};
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
kHttpHandlerData[kExamplePath] = {"directory": [{"url":"http://example.com","title":"RemoteSource"}]};

const BinaryInputStream = CC("@mozilla.org/binaryinputstream;1",
                              "nsIBinaryInputStream",
                              "setInputStream");

let gLastRequestPath;

let suggestedTile1 = {
  url: "http://turbotax.com",
  type: "affiliate",
  lastVisitDate: 4,
  frecent_sites: [
    "taxact.com",
    "hrblock.com",
    "1040.com",
    "taxslayer.com"
  ]
};
let suggestedTile2 = {
  url: "http://irs.gov",
  type: "affiliate",
  lastVisitDate: 3,
  frecent_sites: [
    "taxact.com",
    "hrblock.com",
    "freetaxusa.com",
    "taxslayer.com"
  ]
};
let suggestedTile3 = {
  url: "http://hrblock.com",
  type: "affiliate",
  lastVisitDate: 2,
  frecent_sites: [
    "taxact.com",
    "freetaxusa.com",
    "1040.com",
    "taxslayer.com"
  ]
};
let suggestedTile4 = {
  url: "http://sponsoredtile.com",
  type: "sponsored",
  lastVisitDate: 1,
  frecent_sites: [
    "sponsoredtarget.com"
  ]
}
let suggestedTile5 = {
  url: "http://eviltile.com",
  type: "affiliate",
  lastVisitDate: 5,
  explanation: "This is an evil tile <form><button formaction='javascript:alert(1)''>X</button></form> muhahaha",
  adgroup_name: "WE ARE EVIL <link rel='import' href='test.svg'/>",
  frecent_sites: [
    "eviltarget.com"
  ]
}
let someOtherSite = {url: "http://someothersite.com", title: "Not_A_Suggested_Site"};

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
    yield DirectoryLinksProvider._clearFrequencyCap();
    yield DirectoryLinksProvider._loadInadjacentSites();
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


function setTimeout(fun, timeout) {
  let timer = Components.classes["@mozilla.org/timer;1"]
                        .createInstance(Components.interfaces.nsITimer);
  var event = {
    notify: function (timer) {
      fun();
    }
  };
  timer.initWithCallback(event, timeout,
                         Components.interfaces.nsITimer.TYPE_ONE_SHOT);
  return timer;
}

add_task(function test_shouldUpdateSuggestedTile() {
  let suggestedLink = {
    targetedSite: "somesite.com"
  };

  
  do_check_eq(DirectoryLinksProvider._getCurrentTopSiteCount(), 0);
  isIdentical(NewTabUtils.getProviderLinks(), []);
  do_check_eq(DirectoryLinksProvider._shouldUpdateSuggestedTile(), false);

  
  let origGetProviderLinks = NewTabUtils.getProviderLinks;
  NewTabUtils.getProviderLinks = (provider) => [suggestedLink];

  do_check_eq(DirectoryLinksProvider._getCurrentTopSiteCount(), 0);
  isIdentical(NewTabUtils.getProviderLinks(), [suggestedLink]);
  do_check_eq(DirectoryLinksProvider._shouldUpdateSuggestedTile(), true);

  
  let origCurrentTopSiteCount = DirectoryLinksProvider._getCurrentTopSiteCount;
  DirectoryLinksProvider._getCurrentTopSiteCount = () => 8;

  do_check_eq(DirectoryLinksProvider._getCurrentTopSiteCount(), 8);
  isIdentical(NewTabUtils.getProviderLinks(), [suggestedLink]);
  do_check_eq(DirectoryLinksProvider._shouldUpdateSuggestedTile(), false);

  
  NewTabUtils.getProviderLinks = origGetProviderLinks;
  do_check_eq(DirectoryLinksProvider._getCurrentTopSiteCount(), 8);
  isIdentical(NewTabUtils.getProviderLinks(), []);
  do_check_eq(DirectoryLinksProvider._shouldUpdateSuggestedTile(), true);

  
  DirectoryLinksProvider._getCurrentTopSiteCount = origCurrentTopSiteCount;
});

add_task(function test_updateSuggestedTile() {
  let topSites = ["site0.com", "1040.com", "site2.com", "hrblock.com", "site4.com", "freetaxusa.com", "site6.com"];

  
  let data = {"suggested": [suggestedTile1, suggestedTile2, suggestedTile3], "directory": [someOtherSite]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);

  let testObserver = new TestFirstRun();
  DirectoryLinksProvider.addObserver(testObserver);

  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";

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

  let origCurrentTopSiteCount = DirectoryLinksProvider._getCurrentTopSiteCount;
  DirectoryLinksProvider._getCurrentTopSiteCount = () => 8;

  do_check_eq(DirectoryLinksProvider._updateSuggestedTile(), undefined);

  function TestFirstRun() {
    this.promise = new Promise(resolve => {
      this.onLinkChanged = (directoryLinksProvider, link) => {
        links.unshift(link);
        let possibleLinks = [suggestedTile1.url, suggestedTile2.url, suggestedTile3.url];

        isIdentical([...DirectoryLinksProvider._topSitesWithSuggestedLinks], ["hrblock.com", "1040.com", "freetaxusa.com"]);
        do_check_true(possibleLinks.indexOf(link.url) > -1);
        do_check_eq(link.frecency, SUGGESTED_FRECENCY);
        do_check_eq(link.type, "affiliate");
        resolve();
      };
    });
  }

  function TestChangingSuggestedTile() {
    this.count = 0;
    this.promise = new Promise(resolve => {
      this.onLinkChanged = (directoryLinksProvider, link) => {
        this.count++;
        let possibleLinks = [suggestedTile1.url, suggestedTile2.url, suggestedTile3.url];

        do_check_true(possibleLinks.indexOf(link.url) > -1);
        do_check_eq(link.type, "affiliate");
        do_check_true(this.count <= 2);

        if (this.count == 1) {
          
          do_check_eq(link.url, links.shift().url);
          do_check_eq(link.frecency, SUGGESTED_FRECENCY);
        } else {
          links.unshift(link);
          do_check_eq(link.frecency, SUGGESTED_FRECENCY);
        }
        isIdentical([...DirectoryLinksProvider._topSitesWithSuggestedLinks], ["hrblock.com", "freetaxusa.com"]);
        resolve();
      }
    });
  }

  function TestRemovingSuggestedTile() {
    this.count = 0;
    this.promise = new Promise(resolve => {
      this.onLinkChanged = (directoryLinksProvider, link) => {
        this.count++;

        do_check_eq(link.type, "affiliate");
        do_check_eq(this.count, 1);
        do_check_eq(link.frecency, SUGGESTED_FRECENCY);
        do_check_eq(link.url, links.shift().url);
        isIdentical([...DirectoryLinksProvider._topSitesWithSuggestedLinks], []);
        resolve();
      }
    });
  }

  
  yield testObserver.promise;
  DirectoryLinksProvider.removeObserver(testObserver);

  
  
  let removedTopsite = topSites.shift();
  do_check_eq(removedTopsite, "site0.com");
  do_check_false(NewTabUtils.isTopPlacesSite(removedTopsite));
  let updateSuggestedTile = DirectoryLinksProvider._handleLinkChanged({
    url: "http://" + removedTopsite,
    type: "history",
  });
  do_check_false(updateSuggestedTile);

  
  
  testObserver = new TestChangingSuggestedTile();
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
  testObserver = new TestRemovingSuggestedTile();
  DirectoryLinksProvider.addObserver(testObserver);
  DirectoryLinksProvider.onManyLinksChanged();
  yield testObserver.promise;

  
  yield promiseCleanDirectoryLinksProvider();
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  NewTabUtils.getProviderLinks = origGetProviderLinks;
  DirectoryLinksProvider._getCurrentTopSiteCount = origCurrentTopSiteCount;
});

add_task(function test_suggestedLinksMap() {
  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "testing map";

  let data = {"suggested": [suggestedTile1, suggestedTile2, suggestedTile3, suggestedTile4], "directory": [someOtherSite]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});
  let links = yield fetchData();

  
  do_check_eq(links.length, 1);
  let expected_data = [{url: "http://someothersite.com", title: "Not_A_Suggested_Site", frecency: DIRECTORY_FRECENCY, lastVisitDate: 1}];
  isIdentical(links, expected_data);

  
  expected_data = {
    "taxact.com": [suggestedTile1, suggestedTile2, suggestedTile3],
    "hrblock.com": [suggestedTile1, suggestedTile2],
    "1040.com": [suggestedTile1, suggestedTile3],
    "taxslayer.com": [suggestedTile1, suggestedTile2, suggestedTile3],
    "freetaxusa.com": [suggestedTile2, suggestedTile3],
    "sponsoredtarget.com": [suggestedTile4],
  };

  let suggestedSites = [...DirectoryLinksProvider._suggestedLinks.keys()];
  do_check_eq(suggestedSites.indexOf("sponsoredtarget.com"), 5);
  do_check_eq(suggestedSites.length, Object.keys(expected_data).length);

  DirectoryLinksProvider._suggestedLinks.forEach((suggestedLinks, site) => {
    let suggestedLinksItr = suggestedLinks.values();
    for (let link of expected_data[site]) {
      let linkCopy = JSON.parse(JSON.stringify(link));
      linkCopy.targetedName = "testing map";
      linkCopy.explanation = "";
      isIdentical(suggestedLinksItr.next().value, linkCopy);
    }
  })

  yield promiseCleanDirectoryLinksProvider();
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
});

add_task(function test_topSitesWithSuggestedLinks() {
  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";

  let topSites = ["site0.com", "1040.com", "site2.com", "hrblock.com", "site4.com", "freetaxusa.com", "site6.com"];
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = function(site) {
    return topSites.indexOf(site) >= 0;
  }

  
  let origGetProviderLinks = NewTabUtils.getProviderLinks;
  NewTabUtils.getProviderLinks = function(provider) {
    return [];
  }

  
  do_check_eq(DirectoryLinksProvider._topSitesWithSuggestedLinks.size, 0);

  let data = {"suggested": [suggestedTile1, suggestedTile2, suggestedTile3], "directory": [someOtherSite]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});
  let links = yield fetchData();

  
  do_check_eq(DirectoryLinksProvider._suggestedLinks.size, 5);

  
  let expectedTopSitesWithSuggestedLinks = ["hrblock.com", "1040.com", "freetaxusa.com"];
  DirectoryLinksProvider._handleManyLinksChanged();
  isIdentical([...DirectoryLinksProvider._topSitesWithSuggestedLinks], expectedTopSitesWithSuggestedLinks);

  
  let popped = topSites.pop();
  DirectoryLinksProvider._handleLinkChanged({url: "http://" + popped});
  isIdentical([...DirectoryLinksProvider._topSitesWithSuggestedLinks], expectedTopSitesWithSuggestedLinks);

  
  popped = topSites.pop();
  expectedTopSitesWithSuggestedLinks.pop();
  DirectoryLinksProvider._handleLinkChanged({url: "http://" + popped});
  isIdentical([...DirectoryLinksProvider._topSitesWithSuggestedLinks], expectedTopSitesWithSuggestedLinks);

  
  topSites.push(popped);
  expectedTopSitesWithSuggestedLinks.push(popped);
  DirectoryLinksProvider._handleLinkChanged({url: "http://" + popped});
  isIdentical([...DirectoryLinksProvider._topSitesWithSuggestedLinks], expectedTopSitesWithSuggestedLinks);

  
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  NewTabUtils.getProviderLinks = origGetProviderLinks;
});

add_task(function test_suggestedAttributes() {
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = () => true;

  let origCurrentTopSiteCount = DirectoryLinksProvider._getCurrentTopSiteCount;
  DirectoryLinksProvider._getCurrentTopSiteCount = () => 8;

  let frecent_sites = "addons.mozilla.org,air.mozilla.org,blog.mozilla.org,bugzilla.mozilla.org,developer.mozilla.org,etherpad.mozilla.org,forums.mozillazine.org,hacks.mozilla.org,hg.mozilla.org,mozilla.org,planet.mozilla.org,quality.mozilla.org,support.mozilla.org,treeherder.mozilla.org,wiki.mozilla.org".split(",");
  let imageURI = "https://image/";
  let title = "the title";
  let type = "affiliate";
  let url = "http://test.url/";
  let data = {
    suggested: [{
      frecent_sites,
      imageURI,
      title,
      type,
      url
    }]
  };
  let dataURI = "data:application/json," + escape(JSON.stringify(data));

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  
  let gLinks = NewTabUtils.links;
  gLinks.addProvider(DirectoryLinksProvider);
  gLinks.populateCache();
  yield new Promise(resolve => {
    NewTabUtils.allPages.register({
      observe: _ => _,
      update() {
        NewTabUtils.allPages.unregister(this);
        resolve();
      }
    });
  });

  
  let link = gLinks.getLinks()[0];
  do_check_eq(link.imageURI, imageURI);
  do_check_eq(link.targetedName, "Mozilla");
  do_check_eq(link.targetedSite, frecent_sites[0]);
  do_check_eq(link.title, title);
  do_check_eq(link.type, type);
  do_check_eq(link.url, url);

  
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  DirectoryLinksProvider._getCurrentTopSiteCount = origCurrentTopSiteCount;
  gLinks.removeProvider(DirectoryLinksProvider);
  DirectoryLinksProvider.removeObserver(gLinks);
});

add_task(function test_frequencyCappedSites_views() {
  Services.prefs.setCharPref(kPingUrlPref, "");
  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = () => true;

  let origCurrentTopSiteCount = DirectoryLinksProvider._getCurrentTopSiteCount;
  DirectoryLinksProvider._getCurrentTopSiteCount = () => 8;

  let testUrl = "http://frequency.capped/link";
  let targets = ["top.site.com"];
  let data = {
    suggested: [{
      type: "affiliate",
      frecent_sites: targets,
      url: testUrl,
      frequency_caps: {daily: 5}
    }],
    directory: [{
      type: "organic",
      url: "http://directory.site/"
    }]
  };
  let dataURI = "data:application/json," + JSON.stringify(data);

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  
  let gLinks = NewTabUtils.links;
  gLinks.addProvider(DirectoryLinksProvider);
  gLinks.populateCache();
  yield new Promise(resolve => {
    NewTabUtils.allPages.register({
      observe: _ => _,
      update() {
        NewTabUtils.allPages.unregister(this);
        resolve();
      }
    });
  });

  function synthesizeAction(action) {
    DirectoryLinksProvider.reportSitesAction([{
      link: {
        targetedSite: targets[0],
        url: testUrl
      }
    }], action, 0);
  }

  function checkFirstTypeAndLength(type, length) {
    let links = gLinks.getLinks();
    do_check_eq(links[0].type, type);
    do_check_eq(links.length, length);
  }

  
  checkFirstTypeAndLength("affiliate", 2);
  synthesizeAction("view");
  checkFirstTypeAndLength("affiliate", 2);
  synthesizeAction("view");
  checkFirstTypeAndLength("affiliate", 2);
  synthesizeAction("view");
  checkFirstTypeAndLength("affiliate", 2);
  synthesizeAction("view");
  checkFirstTypeAndLength("affiliate", 2);
  synthesizeAction("view");
  checkFirstTypeAndLength("organic", 1);

  
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  DirectoryLinksProvider._getCurrentTopSiteCount = origCurrentTopSiteCount;
  gLinks.removeProvider(DirectoryLinksProvider);
  DirectoryLinksProvider.removeObserver(gLinks);
  Services.prefs.setCharPref(kPingUrlPref, kPingUrl);
});

add_task(function test_frequencyCappedSites_click() {
  Services.prefs.setCharPref(kPingUrlPref, "");
  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = () => true;

  let origCurrentTopSiteCount = DirectoryLinksProvider._getCurrentTopSiteCount;
  DirectoryLinksProvider._getCurrentTopSiteCount = () => 8;

  let testUrl = "http://frequency.capped/link";
  let targets = ["top.site.com"];
  let data = {
    suggested: [{
      type: "affiliate",
      frecent_sites: targets,
      url: testUrl
    }],
    directory: [{
      type: "organic",
      url: "http://directory.site/"
    }]
  };
  let dataURI = "data:application/json," + JSON.stringify(data);

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  
  let gLinks = NewTabUtils.links;
  gLinks.addProvider(DirectoryLinksProvider);
  gLinks.populateCache();
  yield new Promise(resolve => {
    NewTabUtils.allPages.register({
      observe: _ => _,
      update() {
        NewTabUtils.allPages.unregister(this);
        resolve();
      }
    });
  });

  function synthesizeAction(action) {
    DirectoryLinksProvider.reportSitesAction([{
      link: {
        targetedSite: targets[0],
        url: testUrl
      }
    }], action, 0);
  }

  function checkFirstTypeAndLength(type, length) {
    let links = gLinks.getLinks();
    do_check_eq(links[0].type, type);
    do_check_eq(links.length, length);
  }

  
  checkFirstTypeAndLength("affiliate", 2);
  synthesizeAction("view");
  checkFirstTypeAndLength("affiliate", 2);
  synthesizeAction("click");
  checkFirstTypeAndLength("organic", 1);

  
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  DirectoryLinksProvider._getCurrentTopSiteCount = origCurrentTopSiteCount;
  gLinks.removeProvider(DirectoryLinksProvider);
  DirectoryLinksProvider.removeObserver(gLinks);
  Services.prefs.setCharPref(kPingUrlPref, kPingUrl);
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

add_task(function test_DirectoryLinksProvider_getLinks_noDirectoryData() {
  let data = {
    "directory": [],
  };
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

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
  let data = {"directory": [
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

  
  do_check_eq(links[0].url, data["directory"][1].url);
  do_check_eq(links[1].url, data["directory"][3].url);
});

add_task(function test_DirectoryLinksProvider_getAllowedImages() {
  let data = {"directory": [
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

  
  do_check_eq(links[0].imageURI, data["directory"][3].imageURI);
  do_check_eq(links[1].imageURI, data["directory"][5].imageURI);
});

add_task(function test_DirectoryLinksProvider_getAllowedImages_base() {
  let data = {"directory": [
    {url: "http://example1.com", imageURI: "https://example.com"},
    {url: "http://example2.com", imageURI: "https://tiles.cdn.mozilla.net"},
    {url: "http://example3.com", imageURI: "https://tiles2.cdn.mozilla.net"},
    {url: "http://example4.com", enhancedImageURI: "https://mozilla.net"},
    {url: "http://example5.com", imageURI: "data:text/plain,hi"},
  ]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  
  DirectoryLinksProvider.__linksURLModified = false;

  let links = yield fetchData();
  do_check_eq(links.length, 4);

  
  do_check_eq(links[0].url, data["directory"][1].url);
  do_check_eq(links[1].url, data["directory"][2].url);
  do_check_eq(links[2].url, data["directory"][3].url);
  do_check_eq(links[3].url, data["directory"][4].url);
});

add_task(function test_DirectoryLinksProvider_getAllowedEnhancedImages() {
  let data = {"directory": [
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

  
  do_check_eq(links[0].enhancedImageURI, data["directory"][3].enhancedImageURI);
  do_check_eq(links[1].enhancedImageURI, data["directory"][5].enhancedImageURI);
});

add_task(function test_DirectoryLinksProvider_getEnhancedLink() {
  let data = {"enhanced": [
    {url: "http://example.net", enhancedImageURI: "data:,net1"},
    {url: "http://example.com", enhancedImageURI: "data:,com1"},
    {url: "http://example.com", enhancedImageURI: "data:,com2"},
  ]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  let links = yield fetchData();
  do_check_eq(links.length, 0); 

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

  
  data = {"enhanced": [
    {url: "http://example.com", enhancedImageURI: "data:,fresh"},
  ]};
  dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});
  links = yield fetchData();
  do_check_eq(links.length, 0); 
  checkEnhanced("http://example.net", undefined);
  checkEnhanced("http://example.com", "data:,fresh");
});

add_task(function test_DirectoryLinksProvider_enhancedURIs() {
  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = () => true;
  let origCurrentTopSiteCount = DirectoryLinksProvider._getCurrentTopSiteCount;
  DirectoryLinksProvider._getCurrentTopSiteCount = () => 8;

  let data = {
    "suggested": [
      {url: "http://example.net", enhancedImageURI: "data:,net1", title:"SuggestedTitle", frecent_sites: ["test.com"]}
    ],
    "directory": [
      {url: "http://example.net", enhancedImageURI: "data:,net2", title:"DirectoryTitle"}
    ]
  };
  let dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  
  let gLinks = NewTabUtils.links;
  gLinks.addProvider(DirectoryLinksProvider);
  gLinks.populateCache();
  yield new Promise(resolve => {
    NewTabUtils.allPages.register({
      observe: _ => _,
      update() {
        NewTabUtils.allPages.unregister(this);
        resolve();
      }
    });
  });

  
  let links = yield fetchData();
  do_check_eq(links.length, 1);
  do_check_eq(links[0].title, "DirectoryTitle");
  do_check_eq(links[0].enhancedImageURI, "data:,net2");

  
  links = gLinks.getLinks();
  do_check_eq(links.length, 1);
  do_check_eq(links[0].title, "SuggestedTitle");
  do_check_eq(links[0].enhancedImageURI, "data:,net1");

  
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  DirectoryLinksProvider._getCurrentTopSiteCount = origCurrentTopSiteCount;
  gLinks.removeProvider(DirectoryLinksProvider);
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

add_task(function test_timeSensetiveSuggestedTiles() {
  
  let testStartTime = Date.now();
  
  let startDate = new Date(testStartTime + 1000);
  
  let endDate = new Date(testStartTime + 3000);
  let suggestedTile = Object.assign({
    time_limits: {
      start: startDate.toISOString(),
      end: endDate.toISOString(),
    }
  }, suggestedTile1);

  
  let topSites = ["site0.com", "1040.com", "site2.com", "hrblock.com", "site4.com", "freetaxusa.com", "site6.com"];
  let data = {"suggested": [suggestedTile], "directory": [someOtherSite]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);

  let testObserver = new TestTimingRun();
  DirectoryLinksProvider.addObserver(testObserver);

  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";

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

  let origCurrentTopSiteCount = DirectoryLinksProvider._getCurrentTopSiteCount;
  DirectoryLinksProvider._getCurrentTopSiteCount = () => 8;

  do_check_eq(DirectoryLinksProvider._updateSuggestedTile(), undefined);

  
  
  function TestTimingRun() {
    this.promise = new Promise(resolve => {
      this.onLinkChanged = (directoryLinksProvider, link, ignoreFlag, deleteFlag) => {
        
        if (!deleteFlag) {
          links.unshift(link);
        }

        isIdentical([...DirectoryLinksProvider._topSitesWithSuggestedLinks], ["hrblock.com", "1040.com"]);
        do_check_eq(link.frecency, SUGGESTED_FRECENCY);
        do_check_eq(link.type, "affiliate");
        do_check_eq(link.url, suggestedTile.url);
        let timeDelta = Date.now() - testStartTime;
        if (!deleteFlag) {
          
          
          do_print("TESTING START timeDelta: " + timeDelta);
          do_check_true(timeDelta >= 1000 / 2); 
          do_check_eq(link.targetedSite, "hrblock.com");
          do_check_true(DirectoryLinksProvider._campaignTimeoutID);
        }
        else {
          
          
          do_print("TESTING END timeDelta: " + timeDelta);
          do_check_true(timeDelta >= 3000 / 2); 
          do_check_false(link.targetedSite);
          do_check_false(DirectoryLinksProvider._campaignTimeoutID);
          resolve();
        }
      };
    });
  }

  
  yield testObserver.promise;
  DirectoryLinksProvider.removeObserver(testObserver);

  
  do_check_eq(DirectoryLinksProvider._updateSuggestedTile(), undefined);

  
  links.shift();

  
  suggestedTile.time_limits.end = null;
  data = {"suggested": [suggestedTile], "directory": [someOtherSite]};
  dataURI = 'data:application/json,' + JSON.stringify(data);

  
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, dataURI);

  
  let deferred = Promise.defer();
  DirectoryLinksProvider.getLinks(() => {
    let link = DirectoryLinksProvider._updateSuggestedTile();
    
    do_check_eq(link.type, "affiliate");
    do_check_eq(link.url, suggestedTile.url);
    do_check_false(DirectoryLinksProvider._campaignTimeoutID);
    deferred.resolve();
  });
  yield deferred.promise;

  
  suggestedTile.time_limits.start = null;
  suggestedTile.time_limits.end = (new Date(Date.now() + 3000)).toISOString();

  data = {"suggested": [suggestedTile], "directory": [someOtherSite]};
  dataURI = 'data:application/json,' + JSON.stringify(data);

  
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, dataURI);

  
  deferred = Promise.defer();
  DirectoryLinksProvider.getLinks(() => {
    let link = DirectoryLinksProvider._updateSuggestedTile();
    
    do_check_eq(link.type, "affiliate");
    do_check_eq(link.url, suggestedTile.url);
    do_check_true(DirectoryLinksProvider._campaignTimeoutID);
    DirectoryLinksProvider._clearCampaignTimeout();
    deferred.resolve();
  });
  yield deferred.promise;

  
  yield promiseCleanDirectoryLinksProvider();
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  NewTabUtils.getProviderLinks = origGetProviderLinks;
  DirectoryLinksProvider._getCurrentTopSiteCount = origCurrentTopSiteCount;
});

add_task(function test_setupStartEndTime() {
  let currentTime = Date.now();
  let dt = new Date(currentTime);
  let link = {
    time_limits: {
      start: dt.toISOString()
    }
  };

  
  DirectoryLinksProvider._setupStartEndTime(link);
  do_check_eq(link.startTime, currentTime);

  
  let shiftedDate = new Date(currentTime - dt.getTimezoneOffset()*60*1000);
  link.time_limits.start = shiftedDate.toISOString().replace(/Z$/, "");

  DirectoryLinksProvider._setupStartEndTime(link);
  do_check_eq(link.startTime, currentTime);

  
  delete link.startTime;
  link.time_limits.start = "no date"
  DirectoryLinksProvider._setupStartEndTime(link);
  do_check_false(link.startTime);

  link.time_limits.start = "2015-99999-01T00:00:00"
  DirectoryLinksProvider._setupStartEndTime(link);
  do_check_false(link.startTime);

  link.time_limits.start = "20150501T00:00:00"
  DirectoryLinksProvider._setupStartEndTime(link);
  do_check_false(link.startTime);
});

add_task(function test_DirectoryLinksProvider_frequencyCapSetup() {
  yield promiseSetupDirectoryLinksProvider();
  yield DirectoryLinksProvider.init();

  yield promiseCleanDirectoryLinksProvider();
  yield DirectoryLinksProvider._readFrequencyCapFile();
  isIdentical(DirectoryLinksProvider._frequencyCaps, {});

  
  DirectoryLinksProvider._updateFrequencyCapSettings({
      url: "1",
  });
  DirectoryLinksProvider._updateFrequencyCapSettings({
      url: "2",
      frequency_caps: {daily: 1, total: 2}
  });
  DirectoryLinksProvider._updateFrequencyCapSettings({
      url: "3",
      frequency_caps: {total: 2}
  });
  DirectoryLinksProvider._updateFrequencyCapSettings({
      url: "4",
      frequency_caps: {daily: 1}
  });
  let freqCapsObject = DirectoryLinksProvider._frequencyCaps;
  let capObject = freqCapsObject["1"];
  let defaultDaily = capObject.dailyCap;
  let defaultTotal = capObject.totalCap;
  
  do_check_true(capObject.dailyCap > 0);
  do_check_true(capObject.totalCap > 0);
  
  do_check_eq(freqCapsObject["2"].dailyCap, 1);
  do_check_eq(freqCapsObject["2"].totalCap, 2);
  do_check_eq(freqCapsObject["3"].dailyCap, defaultDaily);
  do_check_eq(freqCapsObject["3"].totalCap, 2);
  do_check_eq(freqCapsObject["4"].dailyCap, 1);
  do_check_eq(freqCapsObject["4"].totalCap, defaultTotal);

  
  yield DirectoryLinksProvider._writeFrequencyCapFile();
  
  DirectoryLinksProvider._frequencyCaps = {};
  yield DirectoryLinksProvider._readFrequencyCapFile();
  
  do_check_eq(freqCapsObject["2"].dailyCap, 1);
  do_check_eq(freqCapsObject["2"].totalCap, 2);
  do_check_eq(freqCapsObject["3"].dailyCap, defaultDaily);
  do_check_eq(freqCapsObject["3"].totalCap, 2);
  do_check_eq(freqCapsObject["4"].dailyCap, 1);
  do_check_eq(freqCapsObject["4"].totalCap, defaultTotal);

  
  yield new Promise(resolve => {
    setTimeout(resolve, 1100);
  });

  
  DirectoryLinksProvider._updateFrequencyCapSettings({
      url: "3",
      frequency_caps: {daily: 1, total: 2}
  });
  DirectoryLinksProvider._updateFrequencyCapSettings({
      url: "7",
      frequency_caps: {daily: 1, total: 2}
  });
  
  DirectoryLinksProvider._pruneFrequencyCapUrls(1000);
  
  Object.keys(DirectoryLinksProvider._frequencyCaps).forEach(key => {
    do_check_true(key == "3" || key == "7");
  });

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_getFrequencyCapLogic() {
  yield promiseSetupDirectoryLinksProvider();
  yield DirectoryLinksProvider.init();

  
  DirectoryLinksProvider._updateFrequencyCapSettings({
    url: "1",
    frequency_caps: {daily: 2, total: 4}
  });

  do_check_true(DirectoryLinksProvider._testFrequencyCapLimits("1"));
  
  DirectoryLinksProvider._addFrequencyCapView("1")
  do_check_true(DirectoryLinksProvider._testFrequencyCapLimits("1"));
  DirectoryLinksProvider._addFrequencyCapView("1")
  do_check_false(DirectoryLinksProvider._testFrequencyCapLimits("1"));

  
  let _wasTodayOrig = DirectoryLinksProvider._wasToday;
  DirectoryLinksProvider._wasToday = function () {return false;}
  
  DirectoryLinksProvider._addFrequencyCapView("1")
  do_check_true(DirectoryLinksProvider._testFrequencyCapLimits("1"));
  DirectoryLinksProvider._addFrequencyCapView("1")
  
  do_check_false(DirectoryLinksProvider._testFrequencyCapLimits("1"));

  
  DirectoryLinksProvider._updateFrequencyCapSettings({
    url: "1",
    frequency_caps: {daily: 5, total: 10}
  });
  
  do_check_true(DirectoryLinksProvider._testFrequencyCapLimits("1"));

  
  DirectoryLinksProvider._setFrequencyCapClick("1");
  
  do_check_false(DirectoryLinksProvider._testFrequencyCapLimits("1"));

  
  DirectoryLinksProvider._addFrequencyCapView("nosuch.url");
  DirectoryLinksProvider._setFrequencyCapClick("nosuch.url");
  
  do_check_false(DirectoryLinksProvider._testFrequencyCapLimits("nosuch.url"));

  
  DirectoryLinksProvider._wasToday = _wasTodayOrig;
  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_getFrequencyCapReportSiteAction() {
  yield promiseSetupDirectoryLinksProvider();
  yield DirectoryLinksProvider.init();

  
  DirectoryLinksProvider._updateFrequencyCapSettings({
    url: "bar.com",
    frequency_caps: {daily: 2, total: 4}
  });

  do_check_true(DirectoryLinksProvider._testFrequencyCapLimits("bar.com"));
  
  yield DirectoryLinksProvider.reportSitesAction([{
    link: {
      targetedSite: "foo.com",
      url: "bar.com"
    },
    isPinned: function() {return false;},
  }], "view", 0);

  
  let data = yield readJsonFile(DirectoryLinksProvider._frequencyCapFilePath);
  do_check_eq(data["bar.com"].dailyViews, 1);
  do_check_eq(data["bar.com"].totalViews, 1);

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_ClickRemoval() {
  yield promiseSetupDirectoryLinksProvider();
  yield DirectoryLinksProvider.init();
  let landingUrl = "http://foo.com";

  
  DirectoryLinksProvider._updateFrequencyCapSettings({
    url: landingUrl,
    frequency_caps: {daily: 2, total: 4}
  });

  
  DirectoryLinksProvider._addFrequencyCapView(landingUrl)
  DirectoryLinksProvider._addFrequencyCapView(landingUrl)
  
  DirectoryLinksProvider._setFrequencyCapClick(landingUrl);

  
  do_check_eq(DirectoryLinksProvider._frequencyCaps[landingUrl].totalViews, 2);
  do_check_true(DirectoryLinksProvider._frequencyCaps[landingUrl].clicked);

  
  yield new Promise(resolve => {
    PlacesUtils.asyncHistory.updatePlaces(
      {
        uri: NetUtil.newURI(landingUrl),
        title: "HELLO",
        visits: [{
          visitDate: Date.now()*1000,
          transitionType: Ci.nsINavHistoryService.TRANSITION_LINK
        }]
      },
      {
        handleError: function () {do_check_true(false);},
        handleResult: function () {},
        handleCompletion: function () {resolve();}
      }
    );
  });

  function UrlDeletionTester() {
    this.promise = new Promise(resolve => {
      this.onDeleteURI = (directoryLinksProvider, link) => {
        resolve();
      };
      this.onClearHistory = (directoryLinksProvider) => {
        resolve();
      };
    });
  };

  let testObserver = new UrlDeletionTester();
  DirectoryLinksProvider.addObserver(testObserver);

  PlacesUtils.bhistory.removePage(NetUtil.newURI(landingUrl));
  yield testObserver.promise;
  DirectoryLinksProvider.removeObserver(testObserver);
  
  do_check_eq(DirectoryLinksProvider._frequencyCaps[landingUrl].totalViews, 2);
  do_check_false(DirectoryLinksProvider._frequencyCaps[landingUrl].hasOwnProperty("clicked"));

  
  let data = yield readJsonFile(DirectoryLinksProvider._frequencyCapFilePath);
  do_check_eq(data[landingUrl].totalViews, 2);
  do_check_false(data[landingUrl].hasOwnProperty("clicked"));

  
  DirectoryLinksProvider._updateFrequencyCapSettings({
    url: landingUrl,
    frequency_caps: {daily: 2, total: 4}
  });
  DirectoryLinksProvider._updateFrequencyCapSettings({
    url: "http://bar.com",
    frequency_caps: {daily: 2, total: 4}
  });

  DirectoryLinksProvider._setFrequencyCapClick(landingUrl);
  DirectoryLinksProvider._setFrequencyCapClick("http://bar.com");
  
  do_check_true(DirectoryLinksProvider._frequencyCaps[landingUrl].clicked);
  do_check_true(DirectoryLinksProvider._frequencyCaps["http://bar.com"].clicked);

  testObserver = new UrlDeletionTester();
  DirectoryLinksProvider.addObserver(testObserver);
  
  PlacesUtils.bhistory.removeAllPages();

  yield testObserver.promise;
  DirectoryLinksProvider.removeObserver(testObserver);
  
  do_check_false(DirectoryLinksProvider._frequencyCaps[landingUrl].hasOwnProperty("clicked"));
  do_check_false(DirectoryLinksProvider._frequencyCaps["http://bar.com"].hasOwnProperty("clicked"));

  
  data = yield readJsonFile(DirectoryLinksProvider._frequencyCapFilePath);
  do_check_false(data[landingUrl].hasOwnProperty("clicked"));
  do_check_false(data["http://bar.com"].hasOwnProperty("clicked"));

  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_DirectoryLinksProvider_anonymous() {
  do_check_true(DirectoryLinksProvider._newXHR().mozAnon);
});

add_task(function test_sanitizeExplanation() {
  
  

  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "testing map";

  let data = {"suggested": [suggestedTile5]};
  let dataURI = 'data:application/json,' + encodeURIComponent(JSON.stringify(data));

  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});
  let links = yield fetchData();

  let suggestedSites = [...DirectoryLinksProvider._suggestedLinks.keys()];
  do_check_eq(suggestedSites.indexOf("eviltarget.com"), 0);
  do_check_eq(suggestedSites.length, 1);

  let suggestedLink = [...DirectoryLinksProvider._suggestedLinks.get(suggestedSites[0]).values()][0];
  do_check_eq(suggestedLink.explanation, "This is an evil tile X muhahaha");
  do_check_eq(suggestedLink.targetedName, "WE ARE EVIL ");
});

add_task(function test_inadjecentSites() {
  let suggestedTile = Object.assign({
    check_inadjacency: true
  }, suggestedTile1);

  
  let topSites = ["1040.com", "site2.com", "hrblock.com", "site4.com", "freetaxusa.com", "site6.com"];
  let data = {"suggested": [suggestedTile], "directory": [someOtherSite]};
  let dataURI = 'data:application/json,' + JSON.stringify(data);

  let testObserver = new TestFirstRun();
  DirectoryLinksProvider.addObserver(testObserver);

  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";

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

  let origCurrentTopSiteCount = DirectoryLinksProvider._getCurrentTopSiteCount;
  DirectoryLinksProvider._getCurrentTopSiteCount = () => {
    origCurrentTopSiteCount.apply(DirectoryLinksProvider);
    return 8;
  };

  
  let origInadjacentSitesUrl = DirectoryLinksProvider._inadjacentSitesUrl;

  
  function setInadjacentSites(sites) {
    let badSiteB64 = [];
    sites.forEach(site => {
      badSiteB64.push(DirectoryLinksProvider._generateHash(site));
    });
    let theList = {"domains": badSiteB64};
    let dataURI = 'data:application/json,' + JSON.stringify(theList);
    DirectoryLinksProvider._inadjacentSitesUrl = dataURI;
    return DirectoryLinksProvider._loadInadjacentSites();
  };

  
  let gLinks = NewTabUtils.links;
  gLinks.addProvider(DirectoryLinksProvider);

  function updateNewTabCache() {
    gLinks.populateCache();
    return new Promise(resolve => {
      NewTabUtils.allPages.register({
        observe: _ => _,
        update() {
          NewTabUtils.allPages.unregister(this);
          resolve();
        }
      });
  });
  }

  
  do_check_eq(DirectoryLinksProvider._updateSuggestedTile(), undefined);
  
  do_check_true(DirectoryLinksProvider._avoidInadjacentSites);
  
  do_check_true(DirectoryLinksProvider._isInadjacentLink({baseDomain: "example.com"}));

  function TestFirstRun() {
    this.promise = new Promise(resolve => {
      this.onLinkChanged = (directoryLinksProvider, link) => {
        do_check_eq(link.url, suggestedTile.url);
        do_check_eq(link.type, "affiliate");
        resolve();
      };
    });
  }

  
  yield testObserver.promise;
  DirectoryLinksProvider.removeObserver(testObserver);

  
  yield updateNewTabCache();
  
  do_check_true(DirectoryLinksProvider._avoidInadjacentSites);

  
  let link = DirectoryLinksProvider._updateSuggestedTile();
  do_check_eq(link.url, "http://turbotax.com");
  
  do_check_true(link.check_inadjacency);

  
  yield setInadjacentSites(["someothersite.com"]);

  
  link = DirectoryLinksProvider._updateSuggestedTile();
  do_check_false(link);
  do_check_true(DirectoryLinksProvider._newTabHasInadjacentSite);

  
  do_check_true(DirectoryLinksProvider._handleLinkChanged({
    url: "http://someothersite.com",
    type: "history",
  }));
  
  do_check_false(DirectoryLinksProvider._handleLinkChanged({
    url: "http://foobar.com",
    type: "history",
  }));

  
  yield setInadjacentSites(["foo.com", "bar.com"]);

  link = DirectoryLinksProvider._updateSuggestedTile();
  
  do_check_true(link);
  do_check_eq(link.url, "http://turbotax.com");

  
  yield setInadjacentSites(["someothersite.com", "foo.com"]);
  
  link = DirectoryLinksProvider._updateSuggestedTile();
  do_check_false(link);
  do_check_true(DirectoryLinksProvider._newTabHasInadjacentSite);

  
  delete suggestedTile.check_inadjacency;
  data = {"suggested": [suggestedTile], "directory": [someOtherSite]};
  dataURI = 'data:application/json,' + JSON.stringify(data);
  yield promiseDirectoryDownloadOnPrefChange(kSourceUrlPref, dataURI);
  yield fetchData();

  
  do_check_false(DirectoryLinksProvider._avoidInadjacentSites);
  link = DirectoryLinksProvider._updateSuggestedTile();
  do_check_true(link);
  do_check_eq(link.url, "http://turbotax.com");
  do_check_false(DirectoryLinksProvider._newTabHasInadjacentSite);

  
  do_check_false(DirectoryLinksProvider._handleLinkChanged({
    url: "http://someothersite.com",
    type: "history",
  }));

  
  do_check_true(DirectoryLinksProvider._isInadjacentLink({baseDomain: "someothersite.com"}));
  do_check_false(DirectoryLinksProvider._isInadjacentLink({baseDomain: "bar.com"}));
  do_check_true(DirectoryLinksProvider._isInadjacentLink({url: "http://www.someothersite.com"}));
  do_check_false(DirectoryLinksProvider._isInadjacentLink({url: "http://www.bar.com"}));
  
  do_check_false(DirectoryLinksProvider._isInadjacentLink({baseDomain: ""}));
  do_check_false(DirectoryLinksProvider._isInadjacentLink({url: ""}));
  do_check_false(DirectoryLinksProvider._isInadjacentLink({url: "http://localhost:8081/"}));
  do_check_false(DirectoryLinksProvider._isInadjacentLink({url: "abracodabra"}));
  do_check_false(DirectoryLinksProvider._isInadjacentLink({}));

  
  do_check_true(DirectoryLinksProvider._checkForInadjacentSites());

  
  gLinks.removeProvider(DirectoryLinksProvider);
  DirectoryLinksProvider._inadjacentSitesUrl = origInadjacentSitesUrl;
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  NewTabUtils.getProviderLinks = origGetProviderLinks;
  DirectoryLinksProvider._getCurrentTopSiteCount = origCurrentTopSiteCount;
  yield promiseCleanDirectoryLinksProvider();
});

add_task(function test_reportPastImpressions() {
  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = () => true;
  let origCurrentTopSiteCount = DirectoryLinksProvider._getCurrentTopSiteCount;
  DirectoryLinksProvider._getCurrentTopSiteCount = () => 8;

  let testUrl = "http://frequency.capped/link";
  let targets = ["top.site.com"];
  let data = {
    suggested: [{
      type: "affiliate",
      frecent_sites: targets,
      url: testUrl
    }]
  };
  let dataURI = "data:application/json," + JSON.stringify(data);
  yield promiseSetupDirectoryLinksProvider({linksURL: dataURI});

  
  let loadPromise = Promise.defer();
  DirectoryLinksProvider.getLinks(_ => {loadPromise.resolve();});
  yield loadPromise.promise;

  
  let deferred, expectedPath, expectedAction, expectedImpressions;
  let done = false;
  server.registerPrefixHandler(kPingPath, (aRequest, aResponse) => {
    if (done) {
      return;
    }

    do_check_eq(aRequest.path, expectedPath);

    let bodyStream = new BinaryInputStream(aRequest.bodyInputStream);
    let bodyObject = JSON.parse(NetUtil.readInputStreamToString(bodyStream, bodyStream.available()));
    let expectedActionIndex = bodyObject[expectedAction];
    if (bodyObject.unpin) {
      
      do_check_false(bodyObject.tiles[expectedActionIndex].hasOwnProperty("past_impressions"));
    }
    else if (expectedImpressions) {
      do_check_eq(bodyObject.tiles[expectedActionIndex].past_impressions.total, expectedImpressions);
      do_check_eq(bodyObject.tiles[expectedActionIndex].past_impressions.daily, expectedImpressions);
    }
    else {
      do_check_eq(expectedPath, "/ping/view");
      do_check_false(bodyObject.tiles[expectedActionIndex].hasOwnProperty("past_impressions"));
    }

    deferred.resolve();
  });

  
  function sendPingAndTest(path, action, index) {
    deferred = Promise.defer();
    expectedPath = kPingPath + path;
    expectedAction = action;
    DirectoryLinksProvider.reportSitesAction(sites, action, index);
    return deferred.promise;
  }

  
  let site = {
    isPinned: _ => false,
    link: {
      directoryId: 1,
      frecency: 30000,
      frecent_sites: targets,
      targetedSite: targets[0],
      url: testUrl
    }
  };
  let sites = [,
    {
      isPinned: _ => false,
      link: {type: "history", url: "https://foo.com"}
    },
    site
  ];

  yield sendPingAndTest("view", "view", 2);
  yield sendPingAndTest("view", "view", 2);
  yield sendPingAndTest("view", "view", 2);

  expectedImpressions = DirectoryLinksProvider._frequencyCaps[testUrl].totalViews;
  do_check_eq(expectedImpressions, 3);

  
  sites.isPinned = _ => true;
  yield sendPingAndTest("click", "pin", 2);
  sites.isPinned = _ => false;
  yield sendPingAndTest("click", "unpin", 2);
  sites.isPinned = _ => false;
  yield sendPingAndTest("click", "click", 2);
  sites.isPinned = _ => false;
  yield sendPingAndTest("click", "block", 2);

  
  done = true;
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  DirectoryLinksProvider._getCurrentTopSiteCount = origCurrentTopSiteCount;
});
