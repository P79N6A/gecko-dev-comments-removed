

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();

  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    let doc = gBrowser.contentDocument;
    let contentAreaContextMenu = document.getElementById("contentAreaContextMenu");
    let ellipsis = "\u2026";

    
    
    
    
    
    
    
    
    
    

    let testElement = function(opts) {
      let element = doc.getElementById(opts.id);
      document.popupNode = element;

      let selection = content.getSelection();
      selection.removeAllRanges();

      if(opts.isSelected) {
        selection.selectAllChildren(element);
      }

      let contextMenu = new nsContextMenu(contentAreaContextMenu);
      let menuItem = document.getElementById("context-searchselect");

      is(document.getElementById("context-searchselect").hidden, !opts.shouldBeShown, "search context menu item is shown for  '#" + opts.id + "' and selected is '" + opts.isSelected + "'");

      if(opts.shouldBeShown) {
        ok(menuItem.label.contains(opts.expectedLabelContents), "Menu item text '" + menuItem.label  + "' contains the correct search terms '" + opts.expectedLabelContents  + "'");
      }
    }

    testElement({
      id: "link",
      isSelected: true,
      shouldBeShown: true,
      expectedLabelContents: "I'm a link!",
    });
    testElement({
      id: "link",
      isSelected: false,
      shouldBeShown: true,
      expectedLabelContents: "I'm a link!",
    });

    testElement({
      id: "longLink",
      isSelected: true,
      shouldBeShown: true,
      expectedLabelContents: "I'm a really lo" + ellipsis,
    });
    testElement({
      id: "longLink",
      isSelected: false,
      shouldBeShown: true,
      expectedLabelContents: "I'm a really lo" + ellipsis,
    });

    testElement({
      id: "plainText",
      isSelected: true,
      shouldBeShown: true,
      expectedLabelContents: "Right clicking " + ellipsis,
    });
    testElement({
      id: "plainText",
      isSelected: false,
      shouldBeShown: false,
    });

    testElement({
      id: "mixedContent",
      isSelected: true,
      shouldBeShown: true,
      expectedLabelContents: "I'm some text, " + ellipsis,
    });
    testElement({
      id: "mixedContent",
      isSelected: false,
      shouldBeShown: false,
    });

    testElement({
      id: "partialLink",
      isSelected: true,
      shouldBeShown: true,
      expectedLabelContents: "link selection",
    });

    testElement({
      id: "partialLink",
      isSelected: false,
      shouldBeShown: true,
      expectedLabelContents: "A partial link " + ellipsis,
    });

    testElement({
      id: "surrogatePair",
      isSelected: true,
      shouldBeShown: true,
      expectedLabelContents: "This character\uD83D\uDD25" + ellipsis,
    });

    
    document.popupNode = null;
    gBrowser.removeCurrentTab();
    finish();
  }, true);

  content.location = "http://mochi.test:8888/browser/browser/base/content/test/general/browser_bug970746.xhtml";
}
