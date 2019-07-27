

function e10s_init() {
  
  
  window.addEventListener("oop-browser-crashed", (event) => {
    let uri = event.target.currentURI;
    Cu.reportError("remote browser crashed while on " +
                   (uri ? uri.spec : "<unknown>") + "\n");
  }, true);
}
