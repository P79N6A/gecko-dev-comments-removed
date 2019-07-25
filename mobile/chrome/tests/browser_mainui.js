

const Cc = Components.classes;
const Ci = Components.interfaces;



function test() {
 is(window.location.href, "chrome://browser/content/browser.xul", "Main window should be browser.xul");

 window.focus();

 let browser = Browser.selectedBrowser;
 isnot(browser, null, "Should have a browser");
   
 is(browser.currentURI.spec, Browser.selectedTab.browser.currentURI.spec, "selectedBrowser == selectedTab.browser");

 testContentContainerSize();
}

function testContentContainerSize() {
  let tiles = document.getElementById("tile-container");
  let oldtilesstyle = tiles.getAttribute("style");

  try {
    tiles.style.width = (window.innerWidth + 100) + "px";
    tiles.style.height = (window.innerHeight + 100) + "px";
    let container = document.getElementById("tile-container-container");
    let rect = container.getBoundingClientRect();
    ok(rect.width == window.innerWidth, "Content container is same width as window");
    ok(rect.height == window.innerHeight, "Content container is same height as window");
  }
  finally {
    tiles.setAttribute("style", oldtilesstyle);
  }
}
