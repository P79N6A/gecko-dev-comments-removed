




































function createEmptyGroupItem(contentWindow, width, height, padding, noAnimation) {
  let pageBounds = contentWindow.Items.getPageBounds();
  pageBounds.inset(padding, padding);

  let box = new contentWindow.Rect(pageBounds);
  box.width = width;
  box.height = height;
  
  let immediately = noAnimation ? true: false;
  let emptyGroupItem = 
    new contentWindow.GroupItem([], { bounds: box, immediately: immediately });

  return emptyGroupItem;
}


function afterAllTabsLoaded(callback) {
  let stillToLoad = 0;

  function onLoad() {
    this.removeEventListener("load", onLoad, true);
    
    stillToLoad--;
    if (!stillToLoad)
      callback();
  }

  for (let a = 0; a < gBrowser.tabs.length; a++) {
    let browser = gBrowser.tabs[a].linkedBrowser;
    if (browser.contentDocument.readyState != "complete") {
      stillToLoad++;
      browser.addEventListener("load", onLoad, true);
    }
  }

  if (!stillToLoad)
    callback();
}
