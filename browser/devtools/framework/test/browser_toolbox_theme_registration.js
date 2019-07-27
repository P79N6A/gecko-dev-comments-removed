



const CHROME_URL = "chrome://mochitests/content/browser/browser/devtools/framework/test/";

let toolbox;

function test()
{
  gBrowser.selectedTab = gBrowser.addTab();
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    gDevTools.showToolbox(target).then(testRegister);
  }, true);

  content.location = "data:text/html,test for dynamically registering and unregistering themes";
}

function testRegister(aToolbox)
{
  toolbox = aToolbox
  gDevTools.once("theme-registered", themeRegistered);

  gDevTools.registerTheme({
    id: "test-theme",
    label: "Test theme",
    stylesheets: [CHROME_URL + "doc_theme.css"],
    classList: ["theme-test"],
  });
}

function themeRegistered(event, themeId)
{
  is(themeId, "test-theme", "theme-registered event handler sent theme id");

  ok(gDevTools.getThemeDefinitionMap().has(themeId), "theme added to map");

  
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.showToolbox(target, "options").then(() => {
    let panel = toolbox.getCurrentPanel();
    let doc = panel.panelWin.frameElement.contentDocument;
    let themeOption = doc.querySelector("#devtools-theme-box > radio[value=test-theme]");

    ok(themeOption, "new theme exists in the Options panel");

    
    applyTheme();
  });
}

function applyTheme()
{
  let panelWin = toolbox.getCurrentPanel().panelWin;
  let doc = panelWin.frameElement.contentDocument;
  let testThemeOption = doc.querySelector("#devtools-theme-box > radio[value=test-theme]");
  let lightThemeOption = doc.querySelector("#devtools-theme-box > radio[value=light]");

  let color = panelWin.getComputedStyle(testThemeOption).color;
  isnot(color, "rgb(255, 0, 0)", "style unapplied");

  
  testThemeOption.click();

  color = panelWin.getComputedStyle(testThemeOption).color;
  is(color, "rgb(255, 0, 0)", "style applied");

  
  lightThemeOption.click();

  color = panelWin.getComputedStyle(testThemeOption).color;
  isnot(color, "rgb(255, 0, 0)", "style unapplied");

  
  testThemeOption.click();

  
  testUnregister();
}

function testUnregister()
{
  gDevTools.unregisterTheme("test-theme");

  ok(!gDevTools.getThemeDefinitionMap().has("test-theme"), "theme removed from map");

  let panelWin = toolbox.getCurrentPanel().panelWin;
  let doc = panelWin.frameElement.contentDocument;
  let themeBox = doc.querySelector("#devtools-theme-box");

  
  is(themeBox.selectedItem, themeBox.querySelector("[value=light]"),
    "theme light must be selected");

  
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let actor = target.activeTab.actor;
  target.client.attachTab(actor, (response) => {
    cleanup();
  });
}

function cleanup()
{
  toolbox.destroy().then(function() {
    toolbox = null;
    gBrowser.removeCurrentTab();
    finish();
  });
}
