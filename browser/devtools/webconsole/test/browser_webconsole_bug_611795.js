



const TEST_URI = 'data:text/html;charset=utf-8,<div style="-moz-opacity:0;">test repeated' +
                 ' css warnings</div><p style="-moz-opacity:0">hi</p>';

function onContentLoaded()
{
  browser.removeEventListener("load", onContentLoaded, true);

  let HUD = HUDService.getHudByWindow(content);

  let cssWarning = "Unknown property '-moz-opacity'.  Declaration dropped.";

  waitForMessages({
    webconsole: HUD,
    messages: [{
      text: cssWarning,
      category: CATEGORY_CSS,
      severity: SEVERITY_WARNING,
      repeats: 2,
    }],
  }).then(testConsoleLogRepeats);
}

function testConsoleLogRepeats()
{
  let HUD = HUDService.getHudByWindow(content);
  let jsterm = HUD.jsterm;

  jsterm.clearOutput();

  jsterm.setInputValue("for (let i = 0; i < 10; ++i) console.log('this is a line of reasonably long text that I will use to verify that the repeated text node is of an appropriate size.');");
  jsterm.execute();

  waitForMessages({
    webconsole: HUD,
    messages: [{
      text: "this is a line of reasonably long text",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
      repeats: 10,
    }],
  }).then(finishTest);
}





function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, function(aHud) {
      
      aHud.jsterm.clearOutput(true);
      browser.addEventListener("load", onContentLoaded, true);
      content.location.reload();
    });
  }, true);
}
