  SimpleTest.waitForExplicitFinish();

  function frameLoaded() {
    var testframe = document.getElementById('testframe');
    var embed = document.getElementsByTagName('embed')[0];
    try {
      var content = testframe.contentDocument.body.innerHTML;
      if (!content.length)
        return;

      var filename = embed.getAttribute("src") ||
          embed.getAttribute("geturl") ||
          embed.getAttribute("geturlnotify");
      
      var req = new XMLHttpRequest();
      req.open('GET', filename, false);
      req.overrideMimeType('text/plain; charset=x-user-defined');
      req.send(null);
      is(req.status, 200, "bad XMLHttpRequest status");
      is(content, req.responseText.replace(/\r\n/g, "\n"),
         "content doesn't match");
    }
    catch (e) {
      
      
      

      
      
      
      ok(e.message.indexOf("Permission denied") > -1 &&
         e.message.indexOf("access property 'body'") > -1,
         "Unexpected exception thrown: " + e.message);
    }
    is(embed.getError(), "pass", "plugin reported error");
    SimpleTest.finish();
  }
