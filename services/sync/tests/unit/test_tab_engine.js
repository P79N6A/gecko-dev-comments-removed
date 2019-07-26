


Cu.import("resource://services-sync/engines/tabs.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/util.js");

function fakeSessionSvc() {
  let tabs = [];
  for(let i = 0; i < arguments.length; i++) {
    tabs.push({
      index: 1,
      entries: [{
        url: arguments[i],
        title: "title"
      }],
      attributes: {
        image: "image"
      }
    });
  }
  let obj = {windows: [{tabs: tabs}]};

  
  delete Svc.Session;
  Svc.Session = {
    getBrowserState: function() JSON.stringify(obj)
  };
}

function run_test() {

  _("test getOpenURLs");
  let engine = new TabEngine(Service);

  
  fakeSessionSvc("http://bar.com", "http://foo.com", "http://foobar.com");

  let matches;

  _("  test matching works (true)");
  let openurlsset = engine.getOpenURLs();
  matches = openurlsset.has("http://foo.com");
  do_check_true(matches);

  _("  test matching works (false)");
  matches = openurlsset.has("http://barfoo.com");
  do_check_false(matches);
}
