




Components.utils.import("resource://gre/modules/ForgetAboutSite.jsm");

const domains = [
  "mochi.test:8888",
  "www.example.com"
];

const addPath = "/browser/dom/indexedDB/test/browser_forgetThisSiteAdd.html";
const getPath = "/browser/dom/indexedDB/test/browser_forgetThisSiteGet.html";

const testPageURL1 = "http://" + domains[0] + addPath;
const testPageURL2 = "http://" + domains[1] + addPath;
const testPageURL3 = "http://" + domains[0] + getPath;
const testPageURL4 = "http://" + domains[1] + getPath;

function test()
{
  requestLongerTimeout(2);
  waitForExplicitFinish();
  
  setPermission(testPageURL1, "indexedDB", "unknown");
  setPermission(testPageURL2, "indexedDB", "unknown");
  executeSoon(test1);
}

function test1()
{
  
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function () {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    setFinishedCallback(function(result, exception) {
      ok(result == 11, "Set version on database in " + testPageURL1);
      ok(!exception, "No exception");
      gBrowser.removeCurrentTab();

      executeSoon(test2);
    });
  }, true);
  content.location = testPageURL1;
}

function test2()
{
  
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function () {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    setFinishedCallback(function(result, exception) {
      ok(result == 11, "Set version on database in " + testPageURL2);
      ok(!exception, "No exception");
      gBrowser.removeCurrentTab();

      executeSoon(test3);
    });
  }, true);
  content.location = testPageURL2;
}

function test3()
{
  
  ForgetAboutSite.removeDataFromDomain(domains[1]);
  setPermission(testPageURL4, "indexedDB", "unknown");
  executeSoon(test4);
}

function test4()
{
  
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function () {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    setFinishedCallback(function(result, exception) {
      ok(result == 11, "Got correct version on database in " + testPageURL3);
      ok(!exception, "No exception");
      gBrowser.removeCurrentTab();

      executeSoon(test5);
    });
  }, true);
  content.location = testPageURL3;
}

function test5()
{
  
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function () {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    setFinishedCallback(function(result, exception) {
      ok(result == 1, "Got correct version on database in " + testPageURL4);
      ok(!exception, "No exception");
      gBrowser.removeCurrentTab();

      executeSoon(finish);
    });
  }, true);
  content.location = testPageURL4;
}
