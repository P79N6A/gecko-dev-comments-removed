# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

let TrackingProtection = {
  MAX_INTROS: 0,
  PREF_ENABLED_GLOBALLY: "privacy.trackingprotection.enabled",
  PREF_ENABLED_IN_PRIVATE_WINDOWS: "privacy.trackingprotection.pbmode.enabled",
  enabledGlobally: false,
  enabledInPrivateWindows: false,

  init() {
    let $ = selector => document.querySelector(selector);
    this.container = $("#tracking-protection-container");
    this.content = $("#tracking-protection-content");
    this.icon = $("#tracking-protection-icon");

    this.updateEnabled();
    Services.prefs.addObserver(this.PREF_ENABLED_GLOBALLY, this, false);
    Services.prefs.addObserver(this.PREF_ENABLED_IN_PRIVATE_WINDOWS, this, false);

    this.enabledHistogram.add(this.enabledGlobally);
  },

  uninit() {
    Services.prefs.removeObserver(this.PREF_ENABLED_GLOBALLY, this);
    Services.prefs.removeObserver(this.PREF_ENABLED_IN_PRIVATE_WINDOWS, this);
  },

  observe() {
    this.updateEnabled();
  },

  get enabled() {
    return this.enabledGlobally ||
           (this.enabledInPrivateWindows &&
            PrivateBrowsingUtils.isWindowPrivate(window));
  },

  updateEnabled() {
    this.enabledGlobally =
      Services.prefs.getBoolPref(this.PREF_ENABLED_GLOBALLY);
    this.enabledInPrivateWindows =
      Services.prefs.getBoolPref(this.PREF_ENABLED_IN_PRIVATE_WINDOWS);
    this.container.hidden = !this.enabled;
  },

  get enabledHistogram() {
    return Services.telemetry.getHistogramById("TRACKING_PROTECTION_ENABLED");
  },

  get eventsHistogram() {
    return Services.telemetry.getHistogramById("TRACKING_PROTECTION_EVENTS");
  },

  onSecurityChange(state) {
    if (!this.enabled) {
      return;
    }

    let {
      STATE_BLOCKED_TRACKING_CONTENT, STATE_LOADED_TRACKING_CONTENT
    } = Ci.nsIWebProgressListener;

    for (let element of [this.icon, this.content]) {
      if (state & STATE_BLOCKED_TRACKING_CONTENT) {
        element.setAttribute("state", "blocked-tracking-content");
      } else if (state & STATE_LOADED_TRACKING_CONTENT) {
        element.setAttribute("state", "loaded-tracking-content");
      } else {
        element.removeAttribute("state");
      }
    }

    if (state & STATE_BLOCKED_TRACKING_CONTENT) {
      
      let introCount = gPrefService.getIntPref("privacy.trackingprotection.introCount");
      if (introCount < TrackingProtection.MAX_INTROS) {
        gPrefService.setIntPref("privacy.trackingprotection.introCount", ++introCount);
        gPrefService.savePrefFile(null);
        this.showIntroPanel();
      }
    }

    
    this.eventsHistogram.add(0);
  },

  disableForCurrentPage() {
    
    
    
    let normalizedUrl = Services.io.newURI(
      "https://" + gBrowser.selectedBrowser.currentURI.hostPort,
      null, null);

    
    
    
    Services.perms.add(normalizedUrl,
      "trackingprotection", Services.perms.ALLOW_ACTION);

    
    this.eventsHistogram.add(1);

    BrowserReload();
  },

  enableForCurrentPage() {
    
    
    
    let normalizedUrl = Services.io.newURI(
      "https://" + gBrowser.selectedBrowser.currentURI.hostPort,
      null, null);

    Services.perms.remove(normalizedUrl,
      "trackingprotection");

    
    this.eventsHistogram.add(2);

    BrowserReload();
  },

  showIntroPanel: Task.async(function*() {
    let mm = gBrowser.selectedBrowser.messageManager;
    let brandBundle = document.getElementById("bundle_brand");
    let brandShortName = brandBundle.getString("brandShortName");

    let openStep2 = () => {
      
      
      gPrefService.setIntPref("privacy.trackingprotection.introCount",
                              this.MAX_INTROS);
      gPrefService.savePrefFile(null);

      let nextURL = Services.urlFormatter.formatURLPref("privacy.trackingprotection.introURL") +
                    "#step2";
      switchToTabHavingURI(nextURL, true, {
        
        
        
        ignoreFragment: true,
      });
    };

    let buttons = [
      {
        label: gNavigatorBundle.getString("trackingProtection.intro.step1of3"),
        style: "text",
      },
      {
        callback: openStep2,
        label: gNavigatorBundle.getString("trackingProtection.intro.nextButton.label"),
        style: "primary",
      },
    ];

    let panelTarget = yield UITour.getTarget(window, "trackingProtection");
    UITour.initForBrowser(gBrowser.selectedBrowser);
    UITour.showInfo(window, mm, panelTarget,
                    gNavigatorBundle.getString("trackingProtection.intro.title"),
                    gNavigatorBundle.getFormattedString("trackingProtection.intro.description",
                                                        [brandShortName]),
                    undefined, buttons);
  }),
};
