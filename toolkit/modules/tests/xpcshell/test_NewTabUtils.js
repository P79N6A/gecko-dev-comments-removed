




const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;
Cu.import("resource://gre/modules/NewTabUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

function run_test() {
  run_next_test();
}

add_test(function multipleProviders() {
  
  
  let evenLinks = makeLinks(0, 2 * NewTabUtils.links.maxNumLinks, 2);
  let evenProvider = new TestProvider(done => done(evenLinks));
  let oddLinks = makeLinks(0, 2 * NewTabUtils.links.maxNumLinks - 1, 2);
  let oddProvider = new TestProvider(done => done(oddLinks));

  NewTabUtils.initWithoutProviders();
  NewTabUtils.links.addProvider(evenProvider);
  NewTabUtils.links.addProvider(oddProvider);

  
  NewTabUtils.links.populateCache(function () {}, false);

  let links = NewTabUtils.links.getLinks();
  let expectedLinks = makeLinks(NewTabUtils.links.maxNumLinks,
                                2 * NewTabUtils.links.maxNumLinks,
                                1);
  do_check_eq(links.length, NewTabUtils.links.maxNumLinks);
  do_check_links(links, expectedLinks);

  NewTabUtils.links.removeProvider(evenProvider);
  NewTabUtils.links.removeProvider(oddProvider);
  run_next_test();
});

add_test(function changeLinks() {
  let expectedLinks = makeLinks(0, 20, 2);
  let provider = new TestProvider(done => done(expectedLinks));

  NewTabUtils.initWithoutProviders();
  NewTabUtils.links.addProvider(provider);

  
  NewTabUtils.links.populateCache(function () {}, false);

  do_check_links(NewTabUtils.links.getLinks(), expectedLinks);

  
  let newLink = makeLink(19);
  expectedLinks.splice(1, 0, newLink);
  provider.notifyLinkChanged(newLink);
  do_check_links(NewTabUtils.links.getLinks(), expectedLinks);

  
  newLink.frecency = 17;
  expectedLinks.splice(1, 1);
  expectedLinks.splice(2, 0, newLink);
  provider.notifyLinkChanged({
    url: newLink.url,
    frecency: 17,
  });
  do_check_links(NewTabUtils.links.getLinks(), expectedLinks);

  
  newLink.title = "My frecency is now 17";
  provider.notifyLinkChanged({
    url: newLink.url,
    title: newLink.title,
  });
  do_check_links(NewTabUtils.links.getLinks(), expectedLinks);

  
  provider.maxNumLinks = expectedLinks.length;
  newLink = makeLink(21);
  expectedLinks.unshift(newLink);
  expectedLinks.pop();
  do_check_eq(expectedLinks.length, provider.maxNumLinks); 
  provider.notifyLinkChanged(newLink);
  do_check_links(NewTabUtils.links.getLinks(), expectedLinks);

  
  expectedLinks = makeLinks(0, 3, 1);
  provider.notifyManyLinksChanged();
  
  
  do_check_links(NewTabUtils.links.getLinks(), expectedLinks);

  NewTabUtils.links.removeProvider(provider);
  run_next_test();
});

add_task(function oneProviderAlreadyCached() {
  let links1 = makeLinks(0, 10, 1);
  let provider1 = new TestProvider(done => done(links1));

  NewTabUtils.initWithoutProviders();
  NewTabUtils.links.addProvider(provider1);

  
  NewTabUtils.links.populateCache(function () {}, false);
  do_check_links(NewTabUtils.links.getLinks(), links1);

  let links2 = makeLinks(10, 20, 1);
  let provider2 = new TestProvider(done => done(links2));
  NewTabUtils.links.addProvider(provider2);

  NewTabUtils.links.populateCache(function () {}, false);
  do_check_links(NewTabUtils.links.getLinks(), links2.concat(links1));

  NewTabUtils.links.removeProvider(provider1);
  NewTabUtils.links.removeProvider(provider2);
});

add_task(function newLowRankedLink() {
  
  let links = makeLinks(0, 10, 1);
  let provider = new TestProvider(done => done(links));
  provider.maxNumLinks = links.length;

  NewTabUtils.initWithoutProviders();
  NewTabUtils.links.addProvider(provider);

  
  NewTabUtils.links.populateCache(function () {}, false);
  do_check_links(NewTabUtils.links.getLinks(), links);

  
  let newLink = makeLink(0);
  provider.notifyLinkChanged(newLink);
  do_check_links(NewTabUtils.links.getLinks(), links);

  
  provider.notifyLinkChanged({
    url: newLink.url,
    title: "a new title",
  });
  do_check_links(NewTabUtils.links.getLinks(), links);

  NewTabUtils.links.removeProvider(provider);
});

add_task(function extractSite() {
  
  [ "mozilla.org",
    "m.mozilla.org",
    "mobile.mozilla.org",
    "www.mozilla.org",
    "www3.mozilla.org",
  ].forEach(host => {
    let url = "http://" + host;
    do_check_eq(NewTabUtils.extractSite(url), "mozilla.org", "extracted same " + host);
  });

  
  [ "bugzilla.mozilla.org",
    "www.bugzilla.mozilla.org",
  ].forEach(host => {
    let url = "http://" + host;
    do_check_eq(NewTabUtils.extractSite(url), "bugzilla.mozilla.org", "extracted eTLD+2 " + host);
  });

  
  [ "bugzilla.mozilla.org",
    "bug123.bugzilla.mozilla.org",
    "too.many.levels.bugzilla.mozilla.org",
    "m2.mozilla.org",
    "mobile30.mozilla.org",
    "ww.mozilla.org",
    "ww2.mozilla.org",
    "wwwww.mozilla.org",
    "wwwww50.mozilla.org",
    "wwws.mozilla.org",
    "secure.mozilla.org",
    "secure10.mozilla.org",
    "many.levels.deep.mozilla.org",
    "just.check.in",
    "192.168.0.1",
    "localhost",
  ].forEach(host => {
    let url = "http://" + host;
    do_check_neq(NewTabUtils.extractSite(url), "mozilla.org", "extracted diff " + host);
  });

  
  [ "about:blank",
    "file:///Users/user/file",
    "chrome://browser/something",
    "ftp://ftp.mozilla.org/",
  ].forEach(url => {
    do_check_neq(NewTabUtils.extractSite(url), "mozilla.org", "extracted diff url " + url);
  });
});

function TestProvider(getLinksFn) {
  this.getLinks = getLinksFn;
  this._observers = new Set();
}

TestProvider.prototype = {
  addObserver: function (observer) {
    this._observers.add(observer);
  },
  notifyLinkChanged: function (link) {
    this._notifyObservers("onLinkChanged", link);
  },
  notifyManyLinksChanged: function () {
    this._notifyObservers("onManyLinksChanged");
  },
  _notifyObservers: function (observerMethodName, arg) {
    for (let obs of this._observers) {
      if (obs[observerMethodName])
        obs[observerMethodName](this, arg);
    }
  },
};

function do_check_links(actualLinks, expectedLinks) {
  do_check_true(Array.isArray(actualLinks));
  do_check_eq(actualLinks.length, expectedLinks.length);
  for (let i = 0; i < expectedLinks.length; i++) {
    let expected = expectedLinks[i];
    let actual = actualLinks[i];
    do_check_eq(actual.url, expected.url);
    do_check_eq(actual.title, expected.title);
    do_check_eq(actual.frecency, expected.frecency);
    do_check_eq(actual.lastVisitDate, expected.lastVisitDate);
  }
}

function makeLinks(frecRangeStart, frecRangeEnd, step) {
  let links = [];
  
  for (let i = frecRangeEnd; i > frecRangeStart; i -= step) {
    links.push(makeLink(i));
  }
  return links;
}

function makeLink(frecency) {
  return {
    url: "http://example" + frecency + ".com/",
    title: "My frecency is " + frecency,
    frecency: frecency,
    lastVisitDate: 0,
  };
}
