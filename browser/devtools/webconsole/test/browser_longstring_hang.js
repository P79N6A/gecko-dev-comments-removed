





function test()
{
  waitForExplicitFinish();

  let DebuggerServer = Cu.import("resource://gre/modules/devtools/dbg-server.jsm",
                                 {}).DebuggerServer;

  addTab("http://example.com/browser/browser/devtools/webconsole/test/test-bug-859170-longstring-hang.html");

  let hud = null;

  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openConsole(null, performTest);
  }, true);

  function performTest(aHud)
  {
    hud = aHud;

    info("wait for the initial long string");

    waitForMessages({
      webconsole: hud,
      messages: [
        {
          name: "find 'foobar', no 'foobaz', in long string output",
          text: "foobar",
          noText: "foobaz",
          category: CATEGORY_WEBDEV,
          longString: true,
        },
      ],
    }).then(onInitialString);
  }

  function onInitialString(aResults)
  {
    let clickable = aResults[0].longStrings[0];
    ok(clickable, "long string ellipsis is shown");

    scrollOutputToNode(clickable);

    executeSoon(() => {
      EventUtils.synthesizeMouse(clickable, 2, 2, {}, hud.iframeWindow);

      info("wait for long string expansion");

      waitForMessages({
        webconsole: hud,
        messages: [
          {
            name: "find 'foobaz' after expand, but no 'boom!' at the end",
            text: "foobaz",
            noText: "boom!",
            category: CATEGORY_WEBDEV,
            longString: false,
          },
          {
            text: "too long to be displayed",
            longString: false,
          },
        ],
      }).then(finishTest);
    });
  }
}
