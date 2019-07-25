



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'PrivateBrowsingAPI',
                       'TabbedBrowsingAPI', 'UtilsAPI'
                      ];

const gDelay = 0;
const gTimeout = 7000;

const PREF_GEO_TOKEN = "geo.wifi.access_token";

const localTestFolder = collector.addHttpResource('./files');

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  pb = new PrivateBrowsingAPI.privateBrowsing(controller);
  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);
}

var teardownModule = function(module)
{
  pb.reset();
}




var testTabRestoration = function()
{
  var available = false;
  var tokens = { };

  
  pb.enabled = false;
  pb.showPrompt = false;

  
  pb.start();

  
  controller.open(localTestFolder + "geolocation.html");
  controller.waitForPageLoad();

  var shortcut = UtilsAPI.getProperty("chrome://browser/locale/browser.properties",
                                      "geolocation.shareLocation.accesskey");
  controller.keypress(null, shortcut, {ctrlKey: mozmill.isMac, altKey: !mozmill.isMac});

  try {
    var result = new elementslib.ID(controller.tabs.activeTab, "result");
    controller.waitForEval("subject.innerHTML != 'undefined'", gTimeout, 100,
                           result.getNode());
    available = true;
  } catch (ex) {}

  
  if (available) {
    PrefsAPI.preferences.branch.getChildList(PREF_GEO_TOKEN, tokens);
    controller.assertJS("subject.hasGeoTokens == true",
                        {hasGeoTokens: tokens.value > 0});
  }

  
  pb.stop();

  
  PrefsAPI.preferences.branch.getChildList(PREF_GEO_TOKEN, tokens);
  controller.assertJS("subject.hasNoGeoTokens == true",
                      {hasNoGeoTokens: tokens.value == 0});
}
