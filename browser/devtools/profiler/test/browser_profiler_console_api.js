


const URL = "data:text/html;charset=utf8,<p>JavaScript Profiler test</p>";

let gTab, gPanel;

function test() {
  waitForExplicitFinish();

  setUp(URL, (tab, browser, panel) => {
    gTab = tab;
    gPanel = panel;

    openConsole(tab, testConsoleProfile);
  });
}

function testConsoleProfile(hud) {
  hud.jsterm.clearOutput(true);

  
  
  

  let profilesStarted = 0;

  function profileEnd(_, uid) {
    let profile = gPanel.profiles.get(uid);

    profile.once("started", () => {
      if (++profilesStarted < 2)
        return;

      gPanel.off("profileCreated", profileEnd);
      gPanel.profiles.get(3).once("stopped", () => {
        openProfiler(gTab, checkProfiles);
      });

      hud.jsterm.execute("console.profileEnd()");
    });
  }

  gPanel.on("profileCreated", profileEnd);
  hud.jsterm.execute("console.profile()");
  hud.jsterm.execute("console.profile()");
}

function checkProfiles(toolbox) {
  let panel = toolbox.getPanel("jsprofiler");
  let getTitle = (uid) =>
    panel.document.querySelector("li#profile-" + uid + " > h1").textContent;

  is(getTitle(1), "Profile 1", "Profile 1 doesn't have a star next to it.");
  is(getTitle(2), "Profile 2 *", "Profile 2 doesn't have a star next to it.");
  is(getTitle(3), "Profile 3", "Profile 3 doesn't have a star next to it.");

  

  gPanel.profiles.get(2).once("stopped", () => {
    is(getTitle(2), "Profile 2", "Profile 2 doesn't have a star next to it.");
    tearDown(gTab, () => gTab = gPanel = null);
  });

  sendFromProfile(2, "stop");
}