





"use strict";

module.metadata = {
  "stability": "stable"
};

const { Cc, Ci, Cr } = require("chrome");
const apiUtils = require("./deprecated/api-utils");
const errors = require("./deprecated/errors");

try {
  let alertServ = Cc["@mozilla.org/alerts-service;1"].
                  getService(Ci.nsIAlertsService);

  
  var notify = alertServ.showAlertNotification.bind(alertServ);
}
catch (err) {
  
  
  
  notify = notifyUsingConsole;
}

exports.notify = function notifications_notify(options) {
  let valOpts = validateOptions(options);
  let clickObserver = !valOpts.onClick ? null : {
    observe: function notificationClickObserved(subject, topic, data) {
      if (topic === "alertclickcallback")
        errors.catchAndLog(valOpts.onClick).call(exports, valOpts.data);
    }
  };
  function notifyWithOpts(notifyFn) {
    notifyFn(valOpts.iconURL, valOpts.title, valOpts.text, !!clickObserver,
             valOpts.data, clickObserver);
  }
  try {
    notifyWithOpts(notify);
  }
  catch (err if err instanceof Ci.nsIException &&
                err.result == Cr.NS_ERROR_FILE_NOT_FOUND) {
    console.warn("The notification icon named by " + valOpts.iconURL +
                 " does not exist.  A default icon will be used instead.");
    delete valOpts.iconURL;
    notifyWithOpts(notify);
  }
  catch (err) {
    notifyWithOpts(notifyUsingConsole);
  }
};

function notifyUsingConsole(iconURL, title, text) {
  title = title ? "[" + title + "]" : "";
  text = text || "";
  let str = [title, text].filter(function (s) s).join(" ");
  console.log(str);
}

function validateOptions(options) {
  return apiUtils.validateOptions(options, {
    data: {
      is: ["string", "undefined"]
    },
    iconURL: {
      is: ["string", "undefined"]
    },
    onClick: {
      is: ["function", "undefined"]
    },
    text: {
      is: ["string", "undefined"]
    },
    title: {
      is: ["string", "undefined"]
    }
  });
}
