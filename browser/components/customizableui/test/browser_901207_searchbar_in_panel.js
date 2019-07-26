



let gTests = [
  {
    desc: "Ctrl+K should open the menu panel and focus the search bar if the search bar is in the panel.",
    run: function() {
      let searchbar = document.getElementById("searchbar");
      gCustomizeMode.addToPanel(searchbar);
      let placement = CustomizableUI.getPlacementOfWidget("search-container");
      is(placement.area, CustomizableUI.AREA_PANEL, "Should be in panel");

      let shownPanelPromise = promisePanelShown(window);
      EventUtils.synthesizeKey("k", { ctrlKey: true });
      yield shownPanelPromise;

      logActiveElement();
      is(document.activeElement, searchbar.textbox.inputField, "The searchbar should be focused");

      let hiddenPanelPromise = promisePanelHidden(window);
      EventUtils.synthesizeKey("VK_ESCAPE", {});
      yield hiddenPanelPromise;
    },
    teardown: function() {
      CustomizableUI.reset();
    }
  },
  {
    desc: "Ctrl+K should give focus to the searchbar when the searchbar is in the menupanel and the panel is already opened.",
    run: function() {
      let searchbar = document.getElementById("searchbar");
      gCustomizeMode.addToPanel(searchbar);
      let placement = CustomizableUI.getPlacementOfWidget("search-container");
      is(placement.area, CustomizableUI.AREA_PANEL, "Should be in panel");

      let shownPanelPromise = promisePanelShown(window);
      PanelUI.toggle({type: "command"});
      yield shownPanelPromise;

      EventUtils.synthesizeKey("k", { ctrlKey: true });
      logActiveElement();
      is(document.activeElement, searchbar.textbox.inputField, "The searchbar should be focused");

      let hiddenPanelPromise = promisePanelHidden(window);
      EventUtils.synthesizeKey("VK_ESCAPE", {});
      yield hiddenPanelPromise;
    },
    teardown: function() {
      CustomizableUI.reset();
    }
  },
  {
    desc: "Ctrl+K should open the overflow panel and focus the search bar if the search bar is overflowed.",
    setup: function() {
      this.originalWindowWidth = window.outerWidth;
      let navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);
      ok(!navbar.hasAttribute("overflowing"), "Should start with a non-overflowing toolbar.");
      ok(CustomizableUI.inDefaultState, "Should start in default state.");

      window.resizeTo(480, window.outerHeight);
      yield waitForCondition(() => navbar.hasAttribute("overflowing"));
      ok(!navbar.querySelector("#search-container"), "Search container should be overflowing");
    },
    run: function() {
      let searchbar = document.getElementById("searchbar");

      let shownPanelPromise = promiseOverflowShown(window);
      EventUtils.synthesizeKey("k", { ctrlKey: true });
      yield shownPanelPromise;

      let chevron = document.getElementById("nav-bar-overflow-button");
      yield waitForCondition(function() chevron.open);
      logActiveElement();
      is(document.activeElement, searchbar.textbox.inputField, "The searchbar should be focused");

      let hiddenPanelPromise = promiseOverflowHidden(window);
      EventUtils.synthesizeKey("VK_ESCAPE", {});
      yield hiddenPanelPromise;
    },
    teardown: function() {
      let navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);
      window.resizeTo(this.originalWindowWidth, window.outerHeight);
      yield waitForCondition(() => !navbar.hasAttribute("overflowing"));
      ok(!navbar.hasAttribute("overflowing"), "Should not have an overflowing toolbar.");
    }
  },
];

function test() {
  waitForExplicitFinish();
  runTests(gTests);
}

function logActiveElement() {
  let element = document.activeElement;
  info("Active element: " + element ?
    element + " (" + element.localName + "#" + element.id + "." + [...element.classList].join(".") + ")" :
    "null");
}
