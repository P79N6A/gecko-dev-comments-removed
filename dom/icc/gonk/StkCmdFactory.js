



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);

const GONK_STKCMDFACTORY_CONTRACTID = "@mozilla.org/icc/stkcmdfactory;1";
const GONK_STKCMDFACTORY_CID = Components.ID("{7a663440-e336-11e4-8fd5-c3140a7ff307}");








function mapDurationToStkDuration(aDuration) {
  return (aDuration)
    ? new StkDuration(aDuration.timeUnit, aDuration.timeInterval)
    : null;
}




function mapIconInfoToStkIconInfo(aIconInfo) {
  let mapIconToStkIcon = function(aIcon) {
    return new StkIcon(aIcon.width, aIcon.height,
                       aIcon.codingScheme, aIcon.pixels);
  };

  return (aIconInfo &&
          aIconInfo.icons !== undefined &&
          aIconInfo.iconSelfExplanatory !== undefined)
    ? new StkIconInfo(aIconInfo.iconSelfExplanatory,
                      aIconInfo.icons.map(mapIconToStkIcon))
    : null;
}





function appendDuration(aTarget, aStkDuration) {
  aTarget.timeUnit = aStkDuration.timeUnit;
  aTarget.timeInterval = aStkDuration.timeInterval;
}

function appendIconInfo(aTarget, aStkIconInfo) {
  aTarget.iconSelfExplanatory = aStkIconInfo.iconSelfExplanatory;
  aTarget.icons = aStkIconInfo.getIcons().map(function(aStkIcon) {
    return {
      width: aStkIcon.width,
      height: aStkIcon.height,
      codingScheme: RIL.ICC_IMG_CODING_SCHEME_TO_GECKO[aStkIcon.codingScheme],
      pixels: aStkIcon.getPixels()
    };
  });
}






function StkDuration(aTimeUnit, aTimeInterval) {
  this.timeUnit = aTimeUnit;
  this.timeInterval = aTimeInterval;
}
StkDuration.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkDuration]),

  
  timeUnit: 0,
  timeInterval: 0
};

function StkIcon(aWidth, aHeight, aCodingScheme, aPixels) {
  this.width = aWidth;
  this.height = aHeight;
  this.codingScheme = this.IMG_CODING_SCHEME[aCodingScheme];
  this.pixels = aPixels.slice();
}
StkIcon.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkIcon]),

  
  pixels: null,

  
  IMG_CODING_SCHEME: {
    "basic": Ci.nsIStkIcon.CODING_SCHEME_BASIC,
    "color": Ci.nsIStkIcon.CODING_SCHEME_COLOR,
    "color-transparency": Ci.nsIStkIcon.CODING_SCHEME_COLOR_TRANSPARENCY
  },

  
  width: 0,
  height: 0,
  codingScheme: 0,
  getPixels: function(aCount) {
    if (!this.pixels) {
      if (aCount) {
        aCount.value = 0;
      }
      return null;
    }

    if (aCount) {
      aCount.value = this.pixels.length;
    }

    return this.pixels.slice();
  }
};

function StkIconInfo(aIconSelfExplanatory, aStkIcons) {
  this.iconSelfExplanatory = aIconSelfExplanatory;
  this.icons = aStkIcons;
}
StkIconInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkIconInfo]),

  
  icons: null,

  
  iconSelfExplanatory: false,

  getIcons: function(aCount) {
    if (!this.icons) {
      if (aCount) {
        aCount.value = 0;
      }
      return null;
    }

    if (aCount) {
      aCount.value = this.icons.length;
    }

    return this.icons.slice();
  }
};

function StkItem(aIdentifier, aText, aStkIconInfo) {
  this.identifier = aIdentifier;
  if (aText !== undefined) {
    this.text = aText;
  }
  this.iconInfo = aStkIconInfo;
}
StkItem.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkItem]),

  
  identifier: 0,
  text: null,
  iconInfo: null
};

function StkTimer(aTimerId, aTimerValue, aTimerAction) {
  this.timerId = aTimerId;
  if (aTimerValue !== undefined &&
      aTimerValue !== null) {
    this.timerValue = aTimerValue;
  }
  this.timerAction = aTimerAction;
}
StkTimer.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkTimer]),

  
  timerId: 0,
  timerValue: Ci.nsIStkTimer.TIMER_VALUE_INVALID,
  timerAction: Ci.nsIStkTimer.TIMER_ACTION_INVALID
};

function StkLocationInfo(aMcc, aMnc, aGsmLocationAreaCode, aGsmCellId) {
  this.mcc = aMcc;
  this.mnc = aMnc;
  this.gsmLocationAreaCode = aGsmLocationAreaCode;
  this.gsmCellId = aGsmCellId;
}
StkLocationInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkLocationInfo]),

  
  mcc: null,
  mnc: null,
  gsmLocationAreaCode: -1,
  gsmCellId: -1
};




function StkProactiveCommand(aCommandDetails) {
  this.commandNumber = aCommandDetails.commandNumber;
  this.typeOfCommand = aCommandDetails.typeOfCommand;
  this.commandQualifier = aCommandDetails.commandQualifier;
}
StkProactiveCommand.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd]),

  
  commandNumber: 0,
  typeOfCommand: 0,
  commandQualifier: 0
};

function StkCommandMessage(aStkProactiveCmd) {
  this.commandNumber = aStkProactiveCmd.commandNumber;
  this.typeOfCommand = aStkProactiveCmd.typeOfCommand;
  this.commandQualifier = aStkProactiveCmd.commandQualifier;
}
StkCommandMessage.prototype = {
  commandNumber: 0,
  typeOfCommand: 0,
  commandQualifier: 0,
  options: null
};

function StkPollIntervalCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);

  this.duration = mapDurationToStkDuration(aCommandDetails.options);
}
StkPollIntervalCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkPollIntervalCmd])
  },

  
  duration: { value: null, writable: true }
});

function StkPollIntervalMessage(aStkPollIntervalCmd) {
  
  StkCommandMessage.call(this, aStkPollIntervalCmd);

  this.options = {};
  appendDuration(this.options, aStkPollIntervalCmd.duration);
}
StkPollIntervalMessage.prototype = Object.create(StkCommandMessage.prototype);

function StkProvideLocalInfoCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);

  this.localInfoType = aCommandDetails.options.localInfoType;
}
StkProvideLocalInfoCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkProvideLocalInfoCmd])
  },

  
  localInfoType: { value: 0x00, writable: true }
});

function StkProvideLocalInfoMessage(aStkProvideLocalInfoCmd) {
  
  StkCommandMessage.call(this, aStkProvideLocalInfoCmd);

  this.options = {
    localInfoType: aStkProvideLocalInfoCmd.localInfoType
  };
}
StkProvideLocalInfoMessage.prototype = Object.create(StkCommandMessage.prototype);

function StkSetupEventListCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);
  let eventList = aCommandDetails.options.eventList;
  if (eventList) {
    this.eventList = eventList.slice();
  }
}
StkSetupEventListCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkSetupEventListCmd])
  },

  
  eventList: { value: null, writable: true },

  
  getEventList: {
    value: function(aCount) {
      if (!this.eventList) {
        if (aCount) {
          aCount.value = 0;
        }
        return null;
      }

      if (aCount) {
        aCount.value = this.eventList.length;
      }

      return this.eventList.slice();
    }
  }
});

function StkSetupEventListMessage(aStkSetupEventListCmd) {
  
  StkCommandMessage.call(this, aStkSetupEventListCmd);

  this.options = {
    eventList: null
  };

  let eventList = aStkSetupEventListCmd.getEventList();

  if (eventList && eventList.length > 0) {
    this.options.eventList = eventList;
  }
}
StkSetupEventListMessage.prototype = Object.create(StkCommandMessage.prototype);

function StkSetUpMenuCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  if (options.title !== undefined) {
    this.title = options.title;
  }

  this.items = options.items.map(function(aItem) {
    
    
    
    return (aItem) ? new StkItem(aItem.identifier,
                                 aItem.text,
                                 mapIconInfoToStkIconInfo(aItem))
                   : null;
  });

  if (options.nextActionList) {
    this.nextActionList = options.nextActionList.slice();
  }

  this.iconInfo = mapIconInfoToStkIconInfo(options);

  this.isHelpAvailable = !!(options.isHelpAvailable);
}
StkSetUpMenuCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkSetUpMenuCmd])
  },

  
  items: { value: null, writable: true },

  
  nextActionList: { value: null, writable: true },

  
  title: { value: null, writable: true },

  getItems: {
    value: function(aCount) {
      if (!this.items) {
        if (aCount) {
          aCount.value = 0;
        }
        return null;
      }

      if (aCount) {
        aCount.value = this.items.length;
      }

      return this.items.slice();
    }
  },

  getNextActionList: {
    value: function(aCount) {
      if (!this.nextActionList) {
        if (aCount) {
          aCount.value = 0;
        }
        return null;
      }

      if (aCount) {
        aCount.value = this.nextActionList.length;
      }

      return this.nextActionList.slice();
    }
  },

  iconInfo: { value: null, writable: true },
  isHelpAvailable: { value: false, writable: true }
});

function StkSetUpMenuMessage(aStkSetUpMenuCmd) {
  
  StkCommandMessage.call(this, aStkSetUpMenuCmd);

  this.options = {
    items: aStkSetUpMenuCmd.getItems().map(function(aStkItem) {
      if (!aStkItem) {
        return null;
      }

      let item = {
        identifier: aStkItem.identifier,
        text: aStkItem.text
      };

      if (aStkItem.iconInfo) {
        appendIconInfo(item, aStkItem.iconInfo);
      }

      return item;
    }),
    isHelpAvailable: aStkSetUpMenuCmd.isHelpAvailable,
    title: aStkSetUpMenuCmd.title
  };

  let nextActionList = aStkSetUpMenuCmd.getNextActionList();
  if (nextActionList && nextActionList.length > 0) {
    this.options.nextActionList = nextActionList;
  }

  if (aStkSetUpMenuCmd.iconInfo) {
    appendIconInfo(this.options, aStkSetUpMenuCmd.iconInfo);
  }
}
StkSetUpMenuMessage.prototype = Object.create(StkCommandMessage.prototype);

function StkSelectItemCmd(aCommandDetails) {
  
  StkSetUpMenuCmd.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  this.presentationType = options.presentationType;

  if (options.defaultItem !== undefined &&
      options.defaultItem !== null) {
    this.defaultItem = options.defaultItem;
  }
}
StkSelectItemCmd.prototype = Object.create(StkSetUpMenuCmd.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkSetUpMenuCmd,
                                  Ci.nsIStkSelectItemCmd])
  },

  
  presentationType: {
    value: 0,
    writable: true
  },

  defaultItem: {
    value: Ci.nsIStkSelectItemCmd.DEFAULT_ITEM_INVALID,
    writable: true
  }
});

function StkSelectItemMessage(aStkSelectItemCmd) {
  
  StkSetUpMenuMessage.call(this, aStkSelectItemCmd);

  this.options.presentationType = aStkSelectItemCmd.presentationType;

  if (aStkSelectItemCmd.defaultItem !== Ci.nsIStkSelectItemCmd.DEFAULT_ITEM_INVALID) {
    this.options.defaultItem = aStkSelectItemCmd.defaultItem;
  }
}
StkSelectItemMessage.prototype = Object.create(StkSetUpMenuMessage.prototype);

function StkTextMessageCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  if (options.text !== undefined) {
    this.text = options.text;
  }

  this.iconInfo = mapIconInfoToStkIconInfo(options);
}
StkTextMessageCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkTextMessageCmd])
  },

  
  text: { value: null, writable: true },
  iconInfo: { value: null, writable: true }
});

function StkTextMessage(aStkTextMessageCmd) {
  
  StkCommandMessage.call(this, aStkTextMessageCmd);

  this.options = {
    text: aStkTextMessageCmd.text
  };

  if (aStkTextMessageCmd.iconInfo) {
    appendIconInfo(this.options, aStkTextMessageCmd.iconInfo);
  }
}
StkTextMessage.prototype = Object.create(StkCommandMessage.prototype);

function StkDisplayTextCmd(aCommandDetails) {
  
  StkTextMessageCmd.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  this.duration = mapDurationToStkDuration(options.duration);

  this.isHighPriority = !!(options.isHighPriority);
  this.userClear = !!(options.userClear);
  this.responseNeeded = !!(options.responseNeeded);
}
StkDisplayTextCmd.prototype = Object.create(StkTextMessageCmd.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkTextMessageCmd,
                                  Ci.nsIStkDisplayTextCmd])
  },

  
  duration: { value: null, writable: true },
  isHighPriority: { value: false, writable: true },
  userClear: { value: false, writable: true },
  responseNeeded: { value: false, writable: true }
});

function StkDisplayTextMessage(aStkDisplayTextCmd) {
  
  StkTextMessage.call(this, aStkDisplayTextCmd);

  this.options.isHighPriority = aStkDisplayTextCmd.isHighPriority;
  this.options.userClear = aStkDisplayTextCmd.userClear;
  this.options.responseNeeded = aStkDisplayTextCmd.responseNeeded;

  if (aStkDisplayTextCmd.duration) {
    this.options.duration = {};
    appendDuration(this.options.duration, aStkDisplayTextCmd.duration);
  }
}
StkDisplayTextMessage.prototype = Object.create(StkTextMessage.prototype);

function StkInputCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  if (options.text !== undefined) {
    this.text = options.text;
  }

  this.duration = mapDurationToStkDuration(options.duration);

  if (options.defaultText !== undefined) {
    this.defaultText = options.defaultText;
  }

  this.isAlphabet = !!(options.isAlphabet);
  this.isUCS2 = !!(options.isUCS2);
  this.isHelpAvailable = !!(options.isHelpAvailable);

  this.iconInfo = mapIconInfoToStkIconInfo(options);
}
StkInputCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkInputCmd])
  },

  
  text: { value: null, writable: true },
  duration: { value: null, writable: true },
  minLength: { value: 1, writable: true },
  maxLength: { value: 1, writable: true },
  defaultText: { value: null, writable: true },
  isAlphabet: { value: false, writable: true },
  isUCS2: { value: false, writable: true },
  isHelpAvailable: { value: false, writable: true },
  iconInfo: { value: null, writable: true }
});

function StkInputMessage(aStkInputCmd) {
  
  StkCommandMessage.call(this, aStkInputCmd);

  this.options = {
    text: aStkInputCmd.text,
    minLength: aStkInputCmd.minLength,
    maxLength: aStkInputCmd.maxLength,
    isAlphabet: aStkInputCmd.isAlphabet,
    isUCS2: aStkInputCmd.isUCS2,
    isHelpAvailable: aStkInputCmd.isHelpAvailable,
    defaultText: aStkInputCmd.defaultText
  };

  if (aStkInputCmd.duration) {
    this.options.duration = {};
    appendDuration(this.options.duration, aStkInputCmd.duration);
  }

  if (aStkInputCmd.iconInfo) {
    appendIconInfo(this.options, aStkInputCmd.iconInfo);
  }
}
StkInputMessage.prototype = Object.create(StkCommandMessage.prototype);

function StkInputKeyCmd(aCommandDetails) {
  
  StkInputCmd.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  
  

  this.isYesNoRequested = !!(options.isYesNoRequested);
}
StkInputKeyCmd.prototype = Object.create(StkInputCmd.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkInputCmd,
                                  Ci.nsIStkInputKeyCmd])
  },

  
  isYesNoRequested: { value: false, writable: true }
});

function StkInputKeyMessage(aStkInputKeyCmd) {
  
  StkInputMessage.call(this, aStkInputKeyCmd);

  this.options.isYesNoRequested = aStkInputKeyCmd.isYesNoRequested;
}
StkInputKeyMessage.prototype = Object.create(StkInputMessage.prototype);

function StkInputTextCmd(aCommandDetails) {
  
  StkInputCmd.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  this.minLength = options.minLength;
  this.maxLength = options.maxLength;

  this.hideInput = !!(options.hideInput);
  this.isPacked = !!(options.isPacked);
}
StkInputTextCmd.prototype = Object.create(StkInputCmd.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkInputCmd,
                                  Ci.nsIStkInputTextCmd])
  },

  
  hideInput: { value: false, writable: true },
  isPacked: { value: false, writable: true }
});

function StkInputTextMessage(aStkInputTextCmd) {
  
  StkInputMessage.call(this, aStkInputTextCmd);

  this.options.hideInput = aStkInputTextCmd.hideInput;
  this.options.isPacked = aStkInputTextCmd.isPacked;
}
StkInputTextMessage.prototype = Object.create(StkInputMessage.prototype);

function StkSetUpCallCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  let confirmMessage = options.confirmMessage;
  let callMessage = options.callMessage;

  this.address = options.address;

  if(confirmMessage) {
    if (confirmMessage.text !== undefined) {
      this.confirmText = confirmMessage.text;
    }
    this.confirmIconInfo = mapIconInfoToStkIconInfo(confirmMessage);
  }

  if(callMessage) {
    if (callMessage.text !== undefined) {
      this.callText = callMessage.text;
    }
    this.callIconInfo = mapIconInfoToStkIconInfo(callMessage);
  }

  this.duration = mapDurationToStkDuration(options.duration);
}
StkSetUpCallCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkSetUpCallCmd])
  },

  
  address: { value: null, writable: true },
  confirmText: { value: null, writable: true },
  confirmIconInfo: { value: null, writable: true },
  callText: { value: null, writable: true },
  callIconInfo: { value: null, writable: true },
  duration: { value: null, writable: true }
});

function StkSetUpCallMessage(aStkSetUpCallCmd) {
  
  StkCommandMessage.call(this, aStkSetUpCallCmd);

  this.options = {
    address: aStkSetUpCallCmd.address
  };

  if (aStkSetUpCallCmd.confirmText !== null ||
      aStkSetUpCallCmd.confirmIconInfo) {
    let confirmMessage = {
      text: aStkSetUpCallCmd.confirmText
    };
    if (aStkSetUpCallCmd.confirmIconInfo) {
      appendIconInfo(confirmMessage, aStkSetUpCallCmd.confirmIconInfo);
    }
    this.options.confirmMessage = confirmMessage;
  }

  if (aStkSetUpCallCmd.callText !== null ||
      aStkSetUpCallCmd.callIconInfo) {
    let callMessage = {
      text: aStkSetUpCallCmd.callText
    };
    if (aStkSetUpCallCmd.callIconInfo) {
      appendIconInfo(callMessage, aStkSetUpCallCmd.callIconInfo);
    }
    this.options.callMessage = callMessage;
  }

  if (aStkSetUpCallCmd.duration) {
    this.options.duration = {};
    appendDuration(this.options.duration, aStkSetUpCallCmd.duration);
  }
}
StkSetUpCallMessage.prototype = Object.create(StkCommandMessage.prototype);

function StkBrowserSettingCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  this.url = options.url;

  this.mode = options.mode;

  let confirmMessage = options.confirmMessage;

  if(confirmMessage) {
    if (confirmMessage.text !== undefined) {
      this.confirmText = confirmMessage.text;
    }
    this.confirmIconInfo = mapIconInfoToStkIconInfo(confirmMessage);
  }
}
StkBrowserSettingCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkBrowserSettingCmd])
  },

  
  url: { value: null, writable: true },
  mode: { value: 0, writable: true },
  confirmText: { value: null, writable: true },
  confirmIconInfo: { value: null, writable: true }
});

function StkBrowserSettingMessage(aStkBrowserSettingCmd) {
  
  StkCommandMessage.call(this, aStkBrowserSettingCmd);

  this.options = {
    url: aStkBrowserSettingCmd.url,
    mode: aStkBrowserSettingCmd.mode
  };

  if (aStkBrowserSettingCmd.confirmText !== null ||
      aStkBrowserSettingCmd.confirmIconInfo) {
    let confirmMessage = {
      text: aStkBrowserSettingCmd.confirmText
    };
    if (aStkBrowserSettingCmd.confirmIconInfo) {
      appendIconInfo(confirmMessage, aStkBrowserSettingCmd.confirmIconInfo);
    }
    this.options.confirmMessage = confirmMessage;
  }
}
StkBrowserSettingMessage.prototype = Object.create(StkCommandMessage.prototype);

function StkPlayToneCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  if(options.text !== undefined) {
    this.text = options.text;
  }

  if (options.tone !== undefined &&
      options.tone !== null) {
    this.tone = options.tone;
  }

  if (options.isVibrate) {
    this.isVibrate = true;
  }

  this.duration = mapDurationToStkDuration(options.duration);

  this.iconInfo = mapIconInfoToStkIconInfo(options);
}
StkPlayToneCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkPlayToneCmd])
  },

  
  text: { value: null, writable: true },
  tone: { value: Ci.nsIStkPlayToneCmd.TONE_TYPE_INVALID, writable: true },
  duration: { value: null, writable: true },
  isVibrate: { value: false, writable: true },
  iconInfo: { value: null, writable: true }
});

function StkPlayToneMessage(aStkPlayToneCmd) {
  
  StkCommandMessage.call(this, aStkPlayToneCmd);

  this.options = {
    isVibrate: aStkPlayToneCmd.isVibrate,
    text: aStkPlayToneCmd.text
  };

  if (aStkPlayToneCmd.tone != Ci.nsIStkPlayToneCmd.TONE_TYPE_INVALID) {
    this.options.tone = aStkPlayToneCmd.tone;
  }

  if (aStkPlayToneCmd.duration) {
    this.options.duration = {};
    appendDuration(this.options.duration, aStkPlayToneCmd.duration);
  }

  if (aStkPlayToneCmd.iconInfo) {
    appendIconInfo(this.options, aStkPlayToneCmd.iconInfo);
  }
}
StkPlayToneMessage.prototype = Object.create(StkCommandMessage.prototype);

function StkTimerManagementCmd(aCommandDetails) {
  
  StkProactiveCommand.call(this, aCommandDetails);

  let options = aCommandDetails.options;

  this.timerInfo = new StkTimer(options.timerId,
                                options.timerValue,
                                options.timerAction);

}
StkTimerManagementCmd.prototype = Object.create(StkProactiveCommand.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkProactiveCmd,
                                  Ci.nsIStkTimerManagementCmd])
  },

  
  timerInfo: { value: null, writable: true }
});

function StkTimerMessage(aStkTimerManagementCmd) {
  
  StkCommandMessage.call(this, aStkTimerManagementCmd);

  let timerInfo = aStkTimerManagementCmd.timerInfo;

  this.options = {
    timerId: timerInfo.timerId,
    timerAction: timerInfo.timerAction
  };

  if (timerInfo.timerValue !== Ci.nsIStkTimer.TIMER_VALUE_INVALID) {
    this.options.timerValue = timerInfo.timerValue;
  }
}
StkTimerMessage.prototype = Object.create(StkCommandMessage.prototype);




let CmdPrototypes = {};
CmdPrototypes[RIL.STK_CMD_REFRESH] = StkProactiveCommand;
CmdPrototypes[RIL.STK_CMD_POLL_INTERVAL] = StkPollIntervalCmd;
CmdPrototypes[RIL.STK_CMD_POLL_OFF] = StkProactiveCommand;
CmdPrototypes[RIL.STK_CMD_PROVIDE_LOCAL_INFO] = StkProvideLocalInfoCmd;
CmdPrototypes[RIL.STK_CMD_SET_UP_EVENT_LIST] = StkSetupEventListCmd;
CmdPrototypes[RIL.STK_CMD_SET_UP_MENU] = StkSetUpMenuCmd;
CmdPrototypes[RIL.STK_CMD_SELECT_ITEM] = StkSelectItemCmd;
CmdPrototypes[RIL.STK_CMD_DISPLAY_TEXT] = StkDisplayTextCmd;
CmdPrototypes[RIL.STK_CMD_SET_UP_IDLE_MODE_TEXT] = StkTextMessageCmd;
CmdPrototypes[RIL.STK_CMD_SEND_SS] = StkTextMessageCmd;
CmdPrototypes[RIL.STK_CMD_SEND_USSD] = StkTextMessageCmd;
CmdPrototypes[RIL.STK_CMD_SEND_SMS] = StkTextMessageCmd;
CmdPrototypes[RIL.STK_CMD_SEND_DTMF] = StkTextMessageCmd;
CmdPrototypes[RIL.STK_CMD_GET_INKEY] = StkInputKeyCmd;
CmdPrototypes[RIL.STK_CMD_GET_INPUT] = StkInputTextCmd;
CmdPrototypes[RIL.STK_CMD_SET_UP_CALL] = StkSetUpCallCmd;
CmdPrototypes[RIL.STK_CMD_LAUNCH_BROWSER] = StkBrowserSettingCmd;
CmdPrototypes[RIL.STK_CMD_PLAY_TONE] = StkPlayToneCmd;
CmdPrototypes[RIL.STK_CMD_TIMER_MANAGEMENT] = StkTimerManagementCmd;
CmdPrototypes[RIL.STK_CMD_OPEN_CHANNEL] = StkTextMessageCmd;
CmdPrototypes[RIL.STK_CMD_CLOSE_CHANNEL] = StkTextMessageCmd;
CmdPrototypes[RIL.STK_CMD_SEND_DATA] = StkTextMessageCmd;
CmdPrototypes[RIL.STK_CMD_RECEIVE_DATA] = StkTextMessageCmd;




let MsgPrototypes = {};
MsgPrototypes[RIL.STK_CMD_REFRESH] = StkCommandMessage;
MsgPrototypes[RIL.STK_CMD_POLL_INTERVAL] = StkPollIntervalMessage;
MsgPrototypes[RIL.STK_CMD_POLL_OFF] = StkCommandMessage;
MsgPrototypes[RIL.STK_CMD_PROVIDE_LOCAL_INFO] = StkProvideLocalInfoMessage;
MsgPrototypes[RIL.STK_CMD_SET_UP_EVENT_LIST] = StkSetupEventListMessage;
MsgPrototypes[RIL.STK_CMD_SET_UP_MENU] = StkSetUpMenuMessage;
MsgPrototypes[RIL.STK_CMD_SELECT_ITEM] = StkSelectItemMessage;
MsgPrototypes[RIL.STK_CMD_DISPLAY_TEXT] = StkDisplayTextMessage;
MsgPrototypes[RIL.STK_CMD_SET_UP_IDLE_MODE_TEXT] = StkTextMessage;
MsgPrototypes[RIL.STK_CMD_SEND_SS] = StkTextMessage;
MsgPrototypes[RIL.STK_CMD_SEND_USSD] = StkTextMessage;
MsgPrototypes[RIL.STK_CMD_SEND_SMS] = StkTextMessage;
MsgPrototypes[RIL.STK_CMD_SEND_DTMF] = StkTextMessage;
MsgPrototypes[RIL.STK_CMD_GET_INKEY] = StkInputKeyMessage;
MsgPrototypes[RIL.STK_CMD_GET_INPUT] = StkInputTextMessage;
MsgPrototypes[RIL.STK_CMD_SET_UP_CALL] = StkSetUpCallMessage;
MsgPrototypes[RIL.STK_CMD_LAUNCH_BROWSER] = StkBrowserSettingMessage;
MsgPrototypes[RIL.STK_CMD_PLAY_TONE] = StkPlayToneMessage;
MsgPrototypes[RIL.STK_CMD_TIMER_MANAGEMENT] = StkTimerMessage;
MsgPrototypes[RIL.STK_CMD_OPEN_CHANNEL] = StkTextMessage;
MsgPrototypes[RIL.STK_CMD_CLOSE_CHANNEL] = StkTextMessage;
MsgPrototypes[RIL.STK_CMD_SEND_DATA] = StkTextMessage;
MsgPrototypes[RIL.STK_CMD_RECEIVE_DATA] = StkTextMessage;




let QueriedIFs = {};
QueriedIFs[RIL.STK_CMD_REFRESH] = Ci.nsIStkProactiveCmd;
QueriedIFs[RIL.STK_CMD_POLL_INTERVAL] = Ci.nsIStkPollIntervalCmd;
QueriedIFs[RIL.STK_CMD_POLL_OFF] = Ci.nsIStkProactiveCmd;
QueriedIFs[RIL.STK_CMD_PROVIDE_LOCAL_INFO] = Ci.nsIStkProvideLocalInfoCmd;
QueriedIFs[RIL.STK_CMD_SET_UP_EVENT_LIST] = Ci.nsIStkSetupEventListCmd;
QueriedIFs[RIL.STK_CMD_SET_UP_MENU] = Ci.nsIStkSetUpMenuCmd;
QueriedIFs[RIL.STK_CMD_SELECT_ITEM] = Ci.nsIStkSelectItemCmd;
QueriedIFs[RIL.STK_CMD_DISPLAY_TEXT] = Ci.nsIStkDisplayTextCmd;
QueriedIFs[RIL.STK_CMD_SET_UP_IDLE_MODE_TEXT] = Ci.nsIStkTextMessageCmd;
QueriedIFs[RIL.STK_CMD_SEND_SS] = Ci.nsIStkTextMessageCmd;
QueriedIFs[RIL.STK_CMD_SEND_USSD] = Ci.nsIStkTextMessageCmd;
QueriedIFs[RIL.STK_CMD_SEND_SMS] = Ci.nsIStkTextMessageCmd;
QueriedIFs[RIL.STK_CMD_SEND_DTMF] = Ci.nsIStkTextMessageCmd;
QueriedIFs[RIL.STK_CMD_GET_INKEY] = Ci.nsIStkInputKeyCmd;
QueriedIFs[RIL.STK_CMD_GET_INPUT] = Ci.nsIStkInputTextCmd;
QueriedIFs[RIL.STK_CMD_SET_UP_CALL] = Ci.nsIStkSetUpCallCmd;
QueriedIFs[RIL.STK_CMD_LAUNCH_BROWSER] = Ci.nsIStkBrowserSettingCmd;
QueriedIFs[RIL.STK_CMD_PLAY_TONE] = Ci.nsIStkPlayToneCmd;
QueriedIFs[RIL.STK_CMD_TIMER_MANAGEMENT] = Ci.nsIStkTimerManagementCmd;
QueriedIFs[RIL.STK_CMD_OPEN_CHANNEL] = Ci.nsIStkTextMessageCmd;
QueriedIFs[RIL.STK_CMD_CLOSE_CHANNEL] = Ci.nsIStkTextMessageCmd;
QueriedIFs[RIL.STK_CMD_SEND_DATA] = Ci.nsIStkTextMessageCmd;
QueriedIFs[RIL.STK_CMD_RECEIVE_DATA] = Ci.nsIStkTextMessageCmd;




function StkTerminalResponse(aResponseMessage) {
  this.resultCode = aResponseMessage.resultCode;
  if (aResponseMessage.additionalInformation != undefined) {
    this.additionalInformation = aResponseMessage.additionalInformation;
  }
}
StkTerminalResponse.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkTerminalResponse]),

  
  resultCode: 0,
  additionalInformation: Ci.nsIStkTerminalResponse.ADDITIONAL_INFO_INVALID
};

function StkResponseMessage(aStkTerminalResponse) {
  this.resultCode = aStkTerminalResponse.resultCode;
  if (aStkTerminalResponse.additionalInformation
      !== Ci.nsIStkTerminalResponse.ADDITIONAL_INFO_INVALID) {
    this.additionalInformation = aStkTerminalResponse.additionalInformation;
  }
}
StkResponseMessage.prototype = {
  resultCode: Ci.nsIStkTerminalResponse.RESULT_OK
};

function StkSelectItemResponse(aStkSelectItemResponseMessage) {
  
  StkTerminalResponse.call(this, aStkSelectItemResponseMessage);
  this.itemIdentifier = aStkSelectItemResponseMessage.itemIdentifier;
}
StkSelectItemResponse.prototype = Object.create(StkTerminalResponse.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkTerminalResponse,
                                  Ci.nsIStkSelectItemResponse])
  },

  
  itemIdentifier: { value: 0, writable: true }
});

function StkSelectItemResponseMessage(aStkSelectItemResponse) {
  
  StkResponseMessage.call(this, aStkSelectItemResponse);

  this.itemIdentifier = aStkSelectItemResponse.itemIdentifier;
}
StkSelectItemResponseMessage.prototype = Object.create(StkResponseMessage.prototype);

function StkGetInputResponse(aStkGetInputResponseMessage) {
  
  StkTerminalResponse.call(this, aStkGetInputResponseMessage);
  if (aStkGetInputResponseMessage.isYesNo !== undefined) {
    this.isYesNo = (aStkGetInputResponseMessage.isYesNo)
      ? Ci.nsIStkGetInputResponse.YES
      : Ci.nsIStkGetInputResponse.NO;
  }

  if (aStkGetInputResponseMessage.input !== undefined) {
    
    
    
    this.input = aStkGetInputResponseMessage.input || "";
  }

}
StkGetInputResponse.prototype = Object.create(StkTerminalResponse.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkTerminalResponse,
                                  Ci.nsIStkGetInputResponse])
  },

  
  isYesNo: { value: Ci.nsIStkGetInputResponse.YES_NO_INVALID, writable: true },
  input: { value: null, writable: true }
});

function StkGetInputResponseMessage(aStkGetInputResponse) {
  
  StkResponseMessage.call(this, aStkGetInputResponse);

  if (aStkGetInputResponse.isYesNo !== Ci.nsIStkGetInputResponse.YES_NO_INVALID) {
    this.isYesNo = !!aStkGetInputResponse.isYesNo;
  }

  if (aStkGetInputResponse.input !== null) {
    this.input = aStkGetInputResponse.input;
  }
}
StkGetInputResponseMessage.prototype = Object.create(StkResponseMessage.prototype);

function StkCallSetupResponse(aStkCallSetupResponseMessage) {
  
  StkTerminalResponse.call(this, aStkCallSetupResponseMessage);
  this.hasConfirmed = !! aStkCallSetupResponseMessage.hasConfirmed;
}
StkCallSetupResponse.prototype = Object.create(StkTerminalResponse.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkTerminalResponse,
                                  Ci.nsIStkCallSetupResponse])
  },

  
  hasConfirmed: { value: false, writable: true }
});

function StkCallSetupResponseMessage(aStkCallSetupResponse) {
  
  StkResponseMessage.call(this, aStkCallSetupResponse);

  this.hasConfirmed = aStkCallSetupResponse.hasConfirmed;
}
StkCallSetupResponseMessage.prototype = Object.create(StkResponseMessage.prototype);

function StkLocalInfoResponse(aStkLocalInfoResponseMessage) {
  
  StkTerminalResponse.call(this, aStkLocalInfoResponseMessage);

  let localInfo = aStkLocalInfoResponseMessage.localInfo;

  if (localInfo.imei) {
    this.imei = localInfo.imei;
    return;
  }

  if (localInfo.locationInfo) {
    let info = localInfo.locationInfo;

    this.locationInfo = new StkLocationInfo(info.mcc,
                                            info.mnc,
                                            info.gsmLocationAreaCode,
                                            info.gsmCellId);
    return;
  }

  if (localInfo.date) {
    if (localInfo.date instanceof Date) {
      this.date = localInfo.date.getTime();
    } else {
      
      
      
      
      this.date = new Date(localInfo.date).getTime();
    }

    return;
  }

  if (localInfo.language) {
    this.language = localInfo.language;
    return;
  }
}
StkLocalInfoResponse.prototype = Object.create(StkTerminalResponse.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkTerminalResponse,
                                  Ci.nsIStkLocalInfoResponse])
  },

  
  imei: { value: null, writable: true },
  locationInfo: { value: null, writable: true },
  date: { value: Ci.nsIStkLocalInfoResponse.DATE_INVALID, writable: true },
  language: { value: null, writable: true },
});

function StkLocalInfoResponseMessage(aStkLocalInfoResponse) {
  
  StkResponseMessage.call(this, aStkLocalInfoResponse);

  let localInfo = this.localInfo = {};

  if (aStkLocalInfoResponse.imei) {
    localInfo.imei = aStkLocalInfoResponse.imei;
    return;
  }

  if (aStkLocalInfoResponse.locationInfo) {
    let srcInfo = aStkLocalInfoResponse.locationInfo;
    let destInfo = localInfo.locationInfo = {};

    destInfo.mcc = srcInfo.mcc;
    destInfo.mnc = srcInfo.mnc;
    destInfo.gsmLocationAreaCode = srcInfo.gsmLocationAreaCode;
    destInfo.gsmCellId = srcInfo.gsmCellId;

    return;
  }

  if (aStkLocalInfoResponse.date !== Ci.nsIStkLocalInfoResponse.DATE_INVALID) {
    localInfo.date = new Date(aStkLocalInfoResponse.date);
    return;
  }

  if (aStkLocalInfoResponse.language) {
    localInfo.language = aStkLocalInfoResponse.language;
    return;
  }
}
StkLocalInfoResponseMessage.prototype = Object.create(StkResponseMessage.prototype);

function StkTimerResponse(aStkTimerResponseMessage) {
  
  StkTerminalResponse.call(this, aStkTimerResponseMessage);
  let timer = aStkTimerResponseMessage.timer;
  
  
  this.timer = new StkTimer(timer.timerId,
                            Math.floor(timer.timerValue),
                            Ci.nsIStkTimer.TIMER_ACTION_INVALID);
}
StkTimerResponse.prototype = Object.create(StkTerminalResponse.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkTerminalResponse,
                                  Ci.nsIStkTimerResponse])
  },

  
  timer: { value: null, writable: true }
});

function StkTimerResponseMessage(aStkTimerResponse) {
  
  StkResponseMessage.call(this, aStkTimerResponse);

  let timer = this.timer = {};
  
  timer.timerId = aStkTimerResponse.timer.timerId;
  timer.timerValue = aStkTimerResponse.timer.timerValue;
}
StkTimerResponseMessage.prototype = Object.create(StkResponseMessage.prototype);




function StkDownloadEvent(aEventMessage) {
  this.eventType = aEventMessage.eventType;
}
StkDownloadEvent.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkDownloadEvent]),

  
  eventType: 0
};

function StkEventMessage(aStkDownloadEvent) {
  this.eventType = aStkDownloadEvent.eventType;
}
StkEventMessage.prototype = {
  eventType: 0
};

function StkLocationEvent(aStkLocationEventMessage) {
  
  StkDownloadEvent.call(this, aStkLocationEventMessage);
  this.locationStatus = aStkLocationEventMessage.locationStatus;

  if (this.locationStatus == Ci.nsIStkLocationEvent.SERVICE_STATE_NORMAL &&
      aStkLocationEventMessage.locationInfo) {
    let info = aStkLocationEventMessage.locationInfo;

    this.locationInfo = new StkLocationInfo(info.mcc,
                                            info.mnc,
                                            info.gsmLocationAreaCode,
                                            info.gsmCellId);
  }
}
StkLocationEvent.prototype = Object.create(StkDownloadEvent.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkDownloadEvent,
                                  Ci.nsIStkLocationEvent])
  },

  
  locationStatus: { value: Ci.nsIStkLocationEvent.SERVICE_STATE_UNAVAILABLE, writable: true },
  locationInfo: { value: null, writable: true }
});

function StkLocationEventMessage(aStkLocationEvent) {
  
  StkEventMessage.call(this, aStkLocationEvent);
  this.locationStatus = aStkLocationEvent.locationStatus;
  if (aStkLocationEvent.locationInfo) {
    let info = aStkLocationEvent.locationInfo;

    this.locationInfo = new StkLocationInfo(info.mcc,
                                            info.mnc,
                                            info.gsmLocationAreaCode,
                                            info.gsmCellId);
  }
}
StkLocationEventMessage.prototype = Object.create(StkEventMessage.prototype);

function StkCallEvent(aStkCallEventMessage) {
  
  StkDownloadEvent.call(this, aStkCallEventMessage);

  if (aStkCallEventMessage.number) {
    this.number = aStkCallEventMessage.number;
  }

  this.isIssuedByRemote = !!aStkCallEventMessage.isIssuedByRemote;

  if (aStkCallEventMessage.error) {
    this.error = aStkCallEventMessage.error;
  }
}
StkCallEvent.prototype = Object.create(StkDownloadEvent.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkDownloadEvent,
                                  Ci.nsIStkCallEvent])
  },

  
  number: { value: null, writable: true },
  isIssuedByRemote: { value: false, writable: true },
  error: { value: null, writable: true }
});

function StkCallEventMessage(aStkCallEvent) {
  
  StkEventMessage.call(this, aStkCallEvent);
  this.number = aStkCallEvent.number;
  this.isIssuedByRemote = aStkCallEvent.isIssuedByRemote;
  this.error = aStkCallEvent.error;
}
StkCallEventMessage.prototype = Object.create(StkEventMessage.prototype);

function StkLanguageSelectionEvent(aStkLanguageSelectionEventMessage) {
  
  StkDownloadEvent.call(this, aStkLanguageSelectionEventMessage);

  if (aStkLanguageSelectionEventMessage.language) {
    this.language = aStkLanguageSelectionEventMessage.language;
  }
}
StkLanguageSelectionEvent.prototype = Object.create(StkDownloadEvent.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkDownloadEvent,
                                  Ci.nsIStkLanguageSelectionEvent])
  },

  
  language: { value: null, writable: true }
});

function StkLanguageSelectionEventMessage(aStkLanguageSelectionEvent) {
  
  StkEventMessage.call(this, aStkLanguageSelectionEvent);
  this.language = aStkLanguageSelectionEvent.language;
}
StkLanguageSelectionEventMessage.prototype = Object.create(StkEventMessage.prototype);

function StkBrowserTerminationEvent(aStkBrowserTerminationEventMessage) {
  
  StkDownloadEvent.call(this, aStkBrowserTerminationEventMessage);

  if (aStkBrowserTerminationEventMessage.terminationCause) {
    this.terminationCause = aStkBrowserTerminationEventMessage.terminationCause;
  }
}
StkBrowserTerminationEvent.prototype = Object.create(StkDownloadEvent.prototype, {
  QueryInterface: {
    value: XPCOMUtils.generateQI([Ci.nsIStkDownloadEvent,
                                  Ci.nsIStkBrowserTerminationEvent])
  },

  
  terminationCause: { value: Ci.nsIStkBrowserTerminationEvent.BROWSER_TERMINATION_CAUSE_USER, writable: true }
});

function StkBrowserTerminationEventMessage(aStkBrowserTerminationEvent) {
  
  StkEventMessage.call(this, aStkBrowserTerminationEvent);
  this.terminationCause = aStkBrowserTerminationEvent.terminationCause;
}
StkBrowserTerminationEventMessage.prototype = Object.create(StkEventMessage.prototype);




let EventPrototypes = {};
EventPrototypes[RIL.STK_EVENT_TYPE_USER_ACTIVITY] = StkDownloadEvent;
EventPrototypes[RIL.STK_EVENT_TYPE_IDLE_SCREEN_AVAILABLE] = StkDownloadEvent;
EventPrototypes[RIL.STK_EVENT_TYPE_MT_CALL] = StkCallEvent;
EventPrototypes[RIL.STK_EVENT_TYPE_CALL_CONNECTED] = StkCallEvent;
EventPrototypes[RIL.STK_EVENT_TYPE_CALL_DISCONNECTED] = StkCallEvent;
EventPrototypes[RIL.STK_EVENT_TYPE_LOCATION_STATUS] = StkLocationEvent;
EventPrototypes[RIL.STK_EVENT_TYPE_LANGUAGE_SELECTION] = StkLanguageSelectionEvent;
EventPrototypes[RIL.STK_EVENT_TYPE_BROWSER_TERMINATION] = StkBrowserTerminationEvent;




let EventMsgPrototypes = {};
EventMsgPrototypes[RIL.STK_EVENT_TYPE_USER_ACTIVITY] = StkEventMessage;
EventMsgPrototypes[RIL.STK_EVENT_TYPE_IDLE_SCREEN_AVAILABLE] = StkEventMessage;
EventMsgPrototypes[RIL.STK_EVENT_TYPE_MT_CALL] = StkCallEventMessage;
EventMsgPrototypes[RIL.STK_EVENT_TYPE_CALL_CONNECTED] = StkCallEventMessage;
EventMsgPrototypes[RIL.STK_EVENT_TYPE_CALL_DISCONNECTED] = StkCallEventMessage;
EventMsgPrototypes[RIL.STK_EVENT_TYPE_LOCATION_STATUS] = StkLocationEventMessage;
EventMsgPrototypes[RIL.STK_EVENT_TYPE_LANGUAGE_SELECTION] = StkLanguageSelectionEventMessage;
EventMsgPrototypes[RIL.STK_EVENT_TYPE_BROWSER_TERMINATION] = StkBrowserTerminationEventMessage;




let QueriedEventIFs = {};
QueriedEventIFs[RIL.STK_EVENT_TYPE_USER_ACTIVITY] = Ci.nsIStkDownloadEvent;
QueriedEventIFs[RIL.STK_EVENT_TYPE_IDLE_SCREEN_AVAILABLE] = Ci.nsIStkDownloadEvent;
QueriedEventIFs[RIL.STK_EVENT_TYPE_MT_CALL] = Ci.nsIStkCallEvent;
QueriedEventIFs[RIL.STK_EVENT_TYPE_CALL_CONNECTED] = Ci.nsIStkCallEvent;
QueriedEventIFs[RIL.STK_EVENT_TYPE_CALL_DISCONNECTED] = Ci.nsIStkCallEvent;
QueriedEventIFs[RIL.STK_EVENT_TYPE_LOCATION_STATUS] = Ci.nsIStkLocationEvent;
QueriedEventIFs[RIL.STK_EVENT_TYPE_LANGUAGE_SELECTION] = Ci.nsIStkLanguageSelectionEvent;
QueriedEventIFs[RIL.STK_EVENT_TYPE_BROWSER_TERMINATION] = Ci.nsIStkBrowserTerminationEvent;




function StkProactiveCmdFactory() {
}
StkProactiveCmdFactory.prototype = {
  classID: GONK_STKCMDFACTORY_CID,

  classInfo: XPCOMUtils.generateCI({classID: GONK_STKCMDFACTORY_CID,
                                    contractID: GONK_STKCMDFACTORY_CONTRACTID,
                                    classDescription: "StkProactiveCmdFactory",
                                    interfaces: [Ci.nsIStkCmdFactory],
                                    flags: Ci.nsIClassInfo.SINGLETON}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIStkCmdFactory]),

  


  createCommand: function(aCommandDetails) {
    let cmdType = CmdPrototypes[aCommandDetails.typeOfCommand];

    if (typeof cmdType != "function") {
      throw new Error("Unknown Command Type: " + aCommandDetails.typeOfCommand);
    }

    return new cmdType(aCommandDetails);
  },

  createCommandMessage: function(aStkProactiveCmd) {
    let cmd = null;

    let msgType = MsgPrototypes[aStkProactiveCmd.typeOfCommand];

    if (typeof msgType != "function") {
      throw new Error("Unknown Command Type: " + aStkProactiveCmd.typeOfCommand);
    }

    
    
    try {
      cmd = aStkProactiveCmd.QueryInterface(QueriedIFs[aStkProactiveCmd.typeOfCommand]);
    } catch (e) {
      throw new Error("Failed to convert command into concrete class: " + e);
    }

    return new msgType(cmd);
  },

  deflateCommand: function(aStkProactiveCmd) {
    return JSON.stringify(this.createCommandMessage(aStkProactiveCmd));
  },

  inflateCommand: function(aJSON) {
    return this.createCommand(JSON.parse(aJSON));
  },

  createResponse: function(aResponseMessage) {
    if (!aResponseMessage || aResponseMessage.resultCode === undefined) {
      throw new Error("Invalid response message: " + JSON.stringify(aResponseMessage));
    }

    if (aResponseMessage.itemIdentifier !== undefined) {
      return new StkSelectItemResponse(aResponseMessage);
    }

    if (aResponseMessage.input !== undefined ||
        aResponseMessage.isYesNo !== undefined) {
      return new StkGetInputResponse(aResponseMessage);
    }

    if (aResponseMessage.hasConfirmed !== undefined) {
      return new StkCallSetupResponse(aResponseMessage);
    }

    if (aResponseMessage.localInfo !== undefined) {
      return new StkLocalInfoResponse(aResponseMessage);
    }

    if (aResponseMessage.timer !== undefined) {
      return new StkTimerResponse(aResponseMessage);
    }

    return new StkTerminalResponse(aResponseMessage);
  },

  createResponseMessage: function(aStkTerminalResponse) {
    if (!aStkTerminalResponse) {
      throw new Error("Invalid terminal response: " + JSON.stringify(aStkTerminalResponse));
    }

    let response;
    if (aStkTerminalResponse instanceof Ci.nsIStkSelectItemResponse) {
      response = aStkTerminalResponse.QueryInterface(Ci.nsIStkSelectItemResponse);
      return new StkSelectItemResponseMessage(response);
    }

    if (aStkTerminalResponse instanceof Ci.nsIStkGetInputResponse) {
      response = aStkTerminalResponse.QueryInterface(Ci.nsIStkGetInputResponse);
      return new StkGetInputResponseMessage(response);
    }

    if (aStkTerminalResponse instanceof Ci.nsIStkCallSetupResponse) {
      response = aStkTerminalResponse.QueryInterface(Ci.nsIStkCallSetupResponse);
      return new StkCallSetupResponseMessage(response);
    }

    if (aStkTerminalResponse instanceof Ci.nsIStkLocalInfoResponse) {
      response = aStkTerminalResponse.QueryInterface(Ci.nsIStkLocalInfoResponse);
      return new StkLocalInfoResponseMessage(response);
    }

    if (aStkTerminalResponse instanceof Ci.nsIStkTimerResponse) {
      response = aStkTerminalResponse.QueryInterface(Ci.nsIStkTimerResponse);
      return new StkTimerResponseMessage(response);
    }

    return new StkResponseMessage(aStkTerminalResponse);
  },

  deflateResponse: function(aStkTerminalResponse) {
    return JSON.stringify(this.createResponseMessage(aStkTerminalResponse));
  },

  inflateResponse: function(aJSON) {
    return this.createResponse(JSON.parse(aJSON));
  },

  createEvent: function(aEventMessage) {
    let eventType = EventPrototypes[aEventMessage.eventType];

    if (typeof eventType != "function") {
      throw new Error("Unknown Event Type: " + aEventMessage.eventType);
    }

    return new eventType(aEventMessage);
  },

  createEventMessage: function(aStkDownloadEvent) {
    let event = null;

    let eventType = EventMsgPrototypes[aStkDownloadEvent.eventType];

    if (typeof eventType != "function") {
      throw new Error("Unknown Event Type: " + aStkDownloadEvent.eventType);
    }

    
    try {
      event = aStkDownloadEvent.QueryInterface(QueriedEventIFs[aStkDownloadEvent.eventType]);
    } catch (e) {
      throw new Error("Failed to convert event into concrete class: " + e);
    }

    return new eventType(event);
  },

  deflateDownloadEvent: function(aStkDownloadEvent) {
    return JSON.stringify(this.createEventMessage(aStkDownloadEvent));
  },

  inflateDownloadEvent: function(aJSON) {
    return this.createEvent(JSON.parse(aJSON));
  },

  createTimer: function(aStkTimerMessage) {
    if (!aStkTimerMessage ||
        aStkTimerMessage.timerId === undefined) {
      throw new Error("Invalid timer object: " + JSON.stringify(aStkTimerMessage));
    }

    
    
    return new StkTimer(aStkTimerMessage.timerId,
                        Math.floor(aStkTimerMessage.timerValue),
                        Ci.nsIStkTimer.TIMER_ACTION_INVALID);
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([StkProactiveCmdFactory]);
