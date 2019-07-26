



"use strict"

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

function ActivitiesDialog() {
  this._id = 0;

  this.activities = [];
}

ActivitiesDialog.prototype = {
  run: function ap_run() {
    let id = "activity-choice" + this._id++;
    let activity = this.activities.shift();

    let choices = [];
    activity.list.forEach(function(item) {
      choices.push({ manifest: item.manifest, icon: item.icon });
    });


    
    
    
    let detail = {
      type: "activity-choice",
      id: id,
      name: activity.name,
      choices: choices
    };

    
    
    SystemAppProxy.addEventListener("mozContentEvent", function act_getChoice(evt) {
      if (evt.detail.id != id)
        return;

      SystemAppProxy.removeEventListener("mozContentEvent", act_getChoice);
      activity.callback.handleEvent(evt.detail.value !== undefined
                                      ? evt.detail.value
                                      : -1);
    });

    SystemAppProxy.dispatchEvent(detail);
  },

  chooseActivity: function ap_chooseActivity(aOptions, aActivities, aCallback) {
    this.activities.push({
      name: aOptions.name,
      list: aActivities,
      callback: aCallback
    });
    Services.tm.currentThread.dispatch(this, Ci.nsIEventTarget.DISPATCH_NORMAL);
  },

  classID: Components.ID("{3a54788b-48cc-4ab4-93d6-0d6a8ef74f8e}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIActivityUIGlue, Ci.nsIRunnable])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ActivitiesDialog]);

