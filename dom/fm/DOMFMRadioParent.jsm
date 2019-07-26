



"use strict"

let DEBUG = 0;
if (DEBUG)
  debug = function(s) { dump("-*- DOMFMRadioParent component: " + s + "\n");  };
else
  debug = function(s) {};

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

const MOZ_SETTINGS_CHANGED_OBSERVER_TOPIC  = "mozsettings-changed";
const PROFILE_BEFORE_CHANGE_OBSERVER_TOPIC = "profile-before-change";

const BAND_87500_108000_kHz = 1;
const BAND_76000_108000_kHz = 2;
const BAND_76000_90000_kHz  = 3;

const FM_BANDS = { };
FM_BANDS[BAND_76000_90000_kHz] = {
  lower: 76000,
  upper: 90000
};

FM_BANDS[BAND_87500_108000_kHz] = {
  lower: 87500,
  upper: 108000
};

FM_BANDS[BAND_76000_108000_kHz] = {
  lower: 76000,
  upper: 108000
};

const BAND_SETTING_KEY          = "fmRadio.band";
const CHANNEL_WIDTH_SETTING_KEY = "fmRadio.channelWidth";


const CHANNEL_WIDTH_200KHZ = 200;
const CHANNEL_WIDTH_100KHZ = 100;
const CHANNEL_WIDTH_50KHZ  = 50;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyGetter(this, "FMRadio", function() {
  return Cc["@mozilla.org/fmradio;1"].getService(Ci.nsIFMRadio);
});

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

this.EXPORTED_SYMBOLS = ["DOMFMRadioParent"];

this.DOMFMRadioParent = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISettingsServiceCallback]),

  _initialized: false,

  
  _isEnabled: false,

  
  _enabling: false,

  
  _currentFrequency: 0,

  
  _currentBand: BAND_87500_108000_kHz,

  
  _currentWidth: CHANNEL_WIDTH_100KHZ,

  
  _antennaAvailable: true,

  _seeking: false,

  _seekingCallback: null,

  init: function() {
    if (this._initialized === true) {
      return;
    }
    this._initialized = true;

    this._messages = ["DOMFMRadio:enable", "DOMFMRadio:disable",
                      "DOMFMRadio:setFrequency", "DOMFMRadio:getCurrentBand",
                      "DOMFMRadio:getPowerState", "DOMFMRadio:getFrequency",
                      "DOMFMRadio:getAntennaState",
                      "DOMFMRadio:seekUp", "DOMFMRadio:seekDown",
                      "DOMFMRadio:cancelSeek"
                     ];
    this._messages.forEach(function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }.bind(this));

    Services.obs.addObserver(this, PROFILE_BEFORE_CHANGE_OBSERVER_TOPIC, false);
    Services.obs.addObserver(this, MOZ_SETTINGS_CHANGED_OBSERVER_TOPIC, false);

    this._updatePowerState();

    
    let lock = gSettingsService.createLock();
    lock.get(BAND_SETTING_KEY, this);
    lock.get(CHANNEL_WIDTH_SETTING_KEY, this);

    this._updateAntennaState();

    let self = this;
    FMRadio.onantennastatechange = function onantennachange() {
      self._updateAntennaState();
    };

    debug("Initialized");
  },

  
  handle: function(aName, aResult) {
    if (aName == BAND_SETTING_KEY) {
      this._updateBand(aResult);
    } else if (aName == CHANNEL_WIDTH_SETTING_KEY) {
      this._updateChannelWidth(aResult);
    }
  },

  handleError: function(aErrorMessage) {
    this._updateBand(BAND_87500_108000_kHz);
    this._updateChannelWidth(CHANNEL_WIDTH_100KHZ);
  },

  _updateAntennaState: function() {
    let antennaState = FMRadio.isAntennaAvailable;

    if (antennaState != this._antennaAvailable) {
      this._antennaAvailable = antennaState;
      ppmm.broadcastAsyncMessage("DOMFMRadio:antennaChange", { });
    }
  },

  _updateBand: function(band) {
      switch (parseInt(band)) {
        case BAND_87500_108000_kHz:
        case BAND_76000_108000_kHz:
        case BAND_76000_90000_kHz:
          this._currentBand = band;
          break;
      }
  },

  _updateChannelWidth: function(channelWidth) {
    switch (parseInt(channelWidth)) {
      case CHANNEL_WIDTH_50KHZ:
      case CHANNEL_WIDTH_100KHZ:
      case CHANNEL_WIDTH_200KHZ:
        this._currentWidth = channelWidth;
        break;
    }
  },

  




  _updateFrequency: function() {
    let frequency = FMRadio.frequency;

    if (frequency != this._currentFrequency) {
      this._currentFrequency = frequency;
      ppmm.broadcastAsyncMessage("DOMFMRadio:frequencyChange", { });
      return true;
    }

    return false;
  },

  



  _updatePowerState: function() {
    let enabled = FMRadio.enabled;

    if (this._isEnabled != enabled) {
      this._isEnabled = enabled;
      ppmm.broadcastAsyncMessage("DOMFMRadio:powerStateChange", { });

      
      if (enabled) {
        this._updateFrequency();
      }
    }
  },

  _onSeekComplete: function(success) {
    if (this._seeking) {
      this._seeking = false;

      if (this._seekingCallback) {
        this._seekingCallback(success);
        this._seekingCallback = null;
      }
    }
  },

  




  _seekStation: function(direction, aMessage) {
    let msg = aMessage.json || { };
    let messageName = aMessage.name + ":Return";

    
    if(!this._isEnabled) {
       this._sendMessage(messageName, false, null, msg);
       return;
    }

    let self = this;
    function callback(success) {
      debug("Seek completed.");
      if (!success) {
        self._sendMessage(messageName, false, null, msg);
      } else {
        
        self._updateFrequency();
        self._sendMessage(messageName, true, null, msg);
      }
    }

    if (this._seeking) {
      
      
      callback(false);
      return;
    }

    this._seekingCallback = callback;
    this._seeking = true;

    let self = this;
    FMRadio.seek(direction);
    FMRadio.addEventListener("seekcomplete", function FM_onSeekComplete() {
      FMRadio.removeEventListener("seekcomplete", FM_onSeekComplete);
      self._onSeekComplete(true);
    });
  },

  








  _roundFrequency: function(frequencyInKHz) {
    if (frequencyInKHz < FM_BANDS[this._currentBand].lower ||
        frequencyInKHz > FM_BANDS[this._currentBand].upper) {
      return null;
    }

    let partToBeRounded = frequencyInKHz - FM_BANDS[this._currentBand].lower;
    let roundedPart = Math.round(partToBeRounded / this._currentWidth) *
                        this._currentWidth;
    return FM_BANDS[this._currentBand].lower + roundedPart;
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case PROFILE_BEFORE_CHANGE_OBSERVER_TOPIC:
        this._messages.forEach(function(msgName) {
          ppmm.removeMessageListener(msgName, this);
        }.bind(this));

        Services.obs.removeObserver(this, PROFILE_BEFORE_CHANGE_OBSERVER_TOPIC);
        Services.obs.removeObserver(this, MOZ_SETTINGS_CHANGED_OBSERVER_TOPIC);

        ppmm = null;
        this._messages = null;
        break;
      case MOZ_SETTINGS_CHANGED_OBSERVER_TOPIC:
        let setting = JSON.parse(aData);
        this.handleMozSettingsChanged(setting);
        break;
    }
  },

  _sendMessage: function(message, success, data, msg) {
    msg.manager.sendAsyncMessage(message + (success ? ":OK" : ":NO"), {
      data: data,
      rid: msg.rid,
      mid: msg.mid
    });
  },

  handleMozSettingsChanged: function(settings) {
    switch (settings.key) {
      case BAND_SETTING_KEY:
        this._updateBand(settings.value);
        break;
      case CHANNEL_WIDTH_SETTING_KEY:
        this._updateChannelWidth(settings.value);
        break;
    }
  },

  _enableFMRadio: function(msg) {
    let frequencyInKHz = this._roundFrequency(msg.data * 1000);

    
    
    if (this._isEnabled || this._enabling || !frequencyInKHz) {
      this._sendMessage("DOMFMRadio:enable:Return", false, null, msg);
      return;
    }

    this._enabling = true;
    let self = this;

    FMRadio.addEventListener("enabled", function on_enabled() {
      debug("FM Radio is enabled!");
      self._enabling = false;

      FMRadio.removeEventListener("enabled", on_enabled);

      
      
      FMRadio.setFrequency(frequencyInKHz);

      
      
      
      self._currentFrequency = FMRadio.frequency;

      self._updatePowerState();
      self._sendMessage("DOMFMRadio:enable:Return", true, null, msg);

      
      
      ppmm.broadcastAsyncMessage("DOMFMRadio:frequencyChange", { });
    });

    FMRadio.enable({
      lowerLimit: FM_BANDS[self._currentBand].lower,
      upperLimit: FM_BANDS[self._currentBand].upper,
      channelWidth:  self._currentWidth   
    });
  },

  _disableFMRadio: function(msg) {
    
    if (!this._isEnabled) {
      this._sendMessage("DOMFMRadio:disable:Return", false, null, msg);
      return;
    }

    let self = this;
    FMRadio.addEventListener("disabled", function on_disabled() {
      debug("FM Radio is disabled!");
      FMRadio.removeEventListener("disabled", on_disabled);

      self._updatePowerState();
      self._sendMessage("DOMFMRadio:disable:Return", true, null, msg);

      
      
      self._onSeekComplete(false);
    });

    FMRadio.disable();
  },

  receiveMessage: function(aMessage) {
    let msg = aMessage.json || {};
    msg.manager = aMessage.target;

    let ret = 0;
    let self = this;
    switch (aMessage.name) {
      case "DOMFMRadio:enable":
        self._enableFMRadio(msg);
        break;
      case "DOMFMRadio:disable":
        self._disableFMRadio(msg);
        break;
      case "DOMFMRadio:setFrequency":
        let frequencyInKHz = self._roundFrequency(msg.data * 1000);

        
        
        if (!self._isEnabled || !frequencyInKHz) {
          self._sendMessage("DOMFMRadio:setFrequency:Return", false, null, msg);
        } else {
          FMRadio.setFrequency(frequencyInKHz);
          self._sendMessage("DOMFMRadio:setFrequency:Return", true, null, msg);
          this._updateFrequency();
        }
        break;
      case "DOMFMRadio:getCurrentBand":
        
        return {
          lower: FM_BANDS[self._currentBand].lower / 1000,   
          upper: FM_BANDS[self._currentBand].upper / 1000,   
          channelWidth: self._currentWidth / 1000            
        };
      case "DOMFMRadio:getPowerState":
        
        return self._isEnabled;
      case "DOMFMRadio:getFrequency":
        
        return self._isEnabled ? this._currentFrequency / 1000 : null; 
      case "DOMFMRadio:getAntennaState":
        
        return self._antennaAvailable;
      case "DOMFMRadio:seekUp":
        self._seekStation(Ci.nsIFMRadio.SEEK_DIRECTION_UP, aMessage);
        break;
      case "DOMFMRadio:seekDown":
        self._seekStation(Ci.nsIFMRadio.SEEK_DIRECTION_DOWN, aMessage);
        break;
      case "DOMFMRadio:cancelSeek":
        
        
        if (!self._isEnabled || !self._seeking) {
          self._sendMessage("DOMFMRadio:cancelSeek:Return", false, null, msg);
        } else {
          FMRadio.cancelSeek();
          
          
          this._onSeekComplete(false);
          
          
          this._updateFrequency();
          self._sendMessage("DOMFMRadio:cancelSeek:Return", true, null, msg);
        }
        break;
    }
  }
};

DOMFMRadioParent.init();

