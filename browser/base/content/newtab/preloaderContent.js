



(function () { 

const ALLOW_BG_CAPTURES_MSG = "BrowserNewTabPreloader:allowBackgroundCaptures";

addMessageListener(ALLOW_BG_CAPTURES_MSG, function onMsg(msg) {
  removeMessageListener(ALLOW_BG_CAPTURES_MSG, onMsg);

  if (content.document.readyState == "complete") {
    setAllowBackgroundCaptures();
    return;
  }

  
  addEventListener("load", function onLoad(event) {
    if (event.target == content.document) {
      removeEventListener("load", onLoad, true);
      setAllowBackgroundCaptures();
    }
  }, true);
});

function setAllowBackgroundCaptures() {
  content.document.documentElement.setAttribute("allow-background-captures",
                                                "true");
}

})();
