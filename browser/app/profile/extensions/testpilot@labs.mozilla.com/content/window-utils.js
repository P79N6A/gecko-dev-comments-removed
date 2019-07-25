




































var TestPilotWindowUtils;

(function() {
  const ALL_STUDIES_WINDOW_NAME = "TestPilotAllStudiesWindow";
  const ALL_STUDIES_WINDOW_TYPE = "extensions:testpilot:all_studies_window";

  TestPilotWindowUtils = {
    openAllStudiesWindow: function() {
      
      
      let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].
                 getService(Components.interfaces.nsIWindowMediator);
      let allStudiesWindow = wm.getMostRecentWindow(ALL_STUDIES_WINDOW_TYPE);

      if (allStudiesWindow) {
        allStudiesWindow.focus();
      } else {
        allStudiesWindow = window.openDialog(
          "chrome://testpilot/content/all-studies-window.xul",
          ALL_STUDIES_WINDOW_NAME,
          "chrome,titlebar,centerscreen,dialog=no");
      }
    },

    openInTab: function(url) {
      let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                 .getService(Components.interfaces.nsIWindowMediator);
      let enumerator = wm.getEnumerator("navigator:browser");
      let found = false;

      while(enumerator.hasMoreElements()) {
        let win = enumerator.getNext();
        let tabbrowser = win.getBrowser();

        
        let numTabs = tabbrowser.browsers.length;
        for (let i = 0; i < numTabs; i++) {
          let currentBrowser = tabbrowser.getBrowserAtIndex(i);
          if (url == currentBrowser.currentURI.spec) {
            tabbrowser.selectedTab = tabbrowser.tabContainer.childNodes[i];
            found = true;
            win.focus();
            break;
          }
        }
      }

      if (!found) {
        let win = wm.getMostRecentWindow("navigator:browser");
        if (win) {
          let browser = win.getBrowser();
          let tab = browser.addTab(url);
          browser.selectedTab = tab;
          win.focus();
        } else {
          window.open(url);
        }
      }
    },

    getCurrentTabUrl: function() {
      let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                 .getService(Components.interfaces.nsIWindowMediator);
      let win = wm.getMostRecentWindow("navigator:browser");
      let tabbrowser = win.getBrowser();
      let currentBrowser = tabbrowser.selectedBrowser;
      return currentBrowser.currentURI.spec;
    },

    openHomepage: function() {
      let Application = Cc["@mozilla.org/fuel/application;1"]
                          .getService(Ci.fuelIApplication);
      let url = Application.prefs.getValue("extensions.testpilot.homepageURL",
                                           "");
      this.openInTab(url);
    },

    openFeedbackPage: function(aIsHappy) {
      Components.utils.import("resource://testpilot/modules/feedback.js");
      FeedbackManager.setCurrUrl(this.getCurrentTabUrl());
      if (aIsHappy) {
        this.openInTab(FeedbackManager.happyUrl);
      } else {
        this.openInTab(FeedbackManager.sadUrl);
      }
    },

    openChromeless: function(url) {
      




      
      

      let screenWidth = window.screen.availWidth;
      let screenHeight = window.screen.availHeight;
      let width = screenWidth >= 1200 ? 1000 : screenWidth - 200;
      let height = screenHeight >= 1000 ? 800 : screenHeight - 200;

      let win = window.open(url, "TestPilotStudyDetailWindow",
                           "chrome,centerscreen,resizable=yes,scrollbars=yes," +
                           "status=no,width=" + width + ",height=" + height);
      win.focus();
    }
  };
}());
