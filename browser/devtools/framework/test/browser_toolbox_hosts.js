



let temp = {}
Cu.import("resource:///modules/devtools/gDevTools.jsm", temp);
let DevTools = temp.DevTools;

Cu.import("resource:///modules/devtools/Toolbox.jsm", temp);
let Toolbox = temp.Toolbox;

Cu.import("resource:///modules/devtools/Target.jsm", temp);
let TargetFactory = temp.TargetFactory;

let toolbox;

function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    openToolbox(testBottomHost);
  }, true);

  content.location = "data:text/html,test for opening toolbox in different hosts";
}

function openToolbox(callback)
{
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.toggleToolboxForTarget(target);

  toolbox = gDevTools.getToolboxForTarget(target);
  toolbox.once("ready", callback);
}

function testBottomHost()
{
  checkHostType(Toolbox.HostType.BOTTOM);

  
  let iframe = document.getElementById("devtools-toolbox-bottom-iframe");
  ok(iframe, "toolbox bottom iframe exists");

  checkToolboxLoaded(iframe);

  toolbox.once("host-changed", testSidebarHost);
  toolbox.hostType = Toolbox.HostType.SIDE;
}

function testSidebarHost()
{
  checkHostType(Toolbox.HostType.SIDE);

  
  let bottom = document.getElementById("devtools-toolbox-bottom-iframe");
  ok(!bottom, "toolbox bottom iframe doesn't exist");

  let iframe = document.getElementById("devtools-toolbox-side-iframe");
  ok(iframe, "toolbox side iframe exists");

  checkToolboxLoaded(iframe);

  toolbox.once("host-changed", testWindowHost);
  toolbox.hostType = Toolbox.HostType.WINDOW;
}

function testWindowHost()
{
  checkHostType(Toolbox.HostType.WINDOW);

  let sidebar = document.getElementById("devtools-toolbox-side-iframe");
  ok(!sidebar, "toolbox sidebar iframe doesn't exist");

  let win = Services.wm.getMostRecentWindow("devtools:toolbox");
  ok(win, "toolbox separate window exists");

  let iframe = win.document.getElementById("toolbox-iframe");
  checkToolboxLoaded(iframe);

  testToolSelect();
}

function testToolSelect()
{
  
  toolbox.once("inspector-ready", testDestroy);
  toolbox.selectTool("inspector");
}

function testDestroy()
{
  toolbox.once("destroyed", function() {
    openToolbox(testRememberHost);
  });

  toolbox.destroy();
}

function testRememberHost()
{
  
  is(toolbox.hostType, Toolbox.HostType.WINDOW, "host remembered");

  let win = Services.wm.getMostRecentWindow("devtools:toolbox");
  ok(win, "toolbox separate window exists");

  cleanup();
}

function checkHostType(hostType)
{
  is(toolbox.hostType, hostType, "host type is " + hostType);

  let pref = Services.prefs.getCharPref("devtools.toolbox.host");
  is(pref, hostType, "host pref is " + hostType);
}

function checkToolboxLoaded(iframe)
{
  let tabs = iframe.contentDocument.getElementById("toolbox-tabs");
  ok(tabs, "toolbox UI has been loaded into iframe");
}

function cleanup()
{
  Services.prefs.setCharPref("devtools.toolbox.host", Toolbox.HostType.BOTTOM);

  toolbox.destroy();
  DevTools = Toolbox = toolbox = null;
  gBrowser.removeCurrentTab();
  finish();
}
