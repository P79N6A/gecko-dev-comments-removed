














































var gFlasherRegistry = [];






function Flasher(aColor, aThickness, aDuration, aSpeed, aInvert)
{
  this.mShell = XPCU.getService("@mozilla.org/inspector/flasher;1", "inIFlasher");
  this.mShell.color = aColor;
  this.mShell.thickness = aThickness;
  this.mShell.invert = aInvert;
  this.duration = aDuration;
  this.mSpeed = aSpeed;
  
  this.register();
}

Flasher.prototype = 
{
  
  

  mFlashTimeout: null,
  mElement:null,
  mRegistryId: null,
  mFlashes: 0,
  mStartTime: 0,
  mDuration: 0,
  mSpeed: 0,

  
  

  get flashing() { return this.mFlashTimeout != null; },
  
  get element() { return this.mElement; },
  set element(val) 
  { 
    if (val && val.nodeType == Node.ELEMENT_NODE) {
      this.mElement = val; 
      this.mShell.scrollElementIntoView(val);
    } else 
      throw "Invalid node type.";
  },

  get color() { return this.mShell.color; },
  set color(aVal) { return this.mShell.color = aVal; },

  get thickness() { return this.mShell.thickness; },
  set thickness(aVal) { this.mShell.thickness = aVal; },

  get duration() { return this.mDuration; },
  set duration(aVal) { this.mDuration = aVal; },

  get speed() { return this.mSpeed; },
  set speed(aVal) { this.mSpeed = aVal; },

  get invert() { return this.mShell.invert; },
  set invert(aVal) { this.mShell.invert = aVal; },

  
  
  

  register: function()
  {
    var length = gFlasherRegistry.length;
    gFlasherRegistry[length] = this;
    this.mRegistryId = length;
  },

  start: function(aDuration, aSpeed, aHold)
  {
    this.mUDuration = aDuration ? aDuration*1000 : this.mDuration;
    this.mUSpeed = aSpeed ? aSpeed : this.mSpeed
    this.mHold = aHold;
    this.mFlashes = 0;
    this.mStartTime = new Date();
    this.doFlash();
  },

  doFlash: function()
  {
    if (this.mHold || this.mFlashes%2) {
      this.paintOn();
    } else {
      this.paintOff();
    }
    this.mFlashes++;

    if (this.mUDuration < 0 || new Date() - this.mStartTime < this.mUDuration) {
      this.mFlashTimeout = window.setTimeout("gFlasherRegistry["+this.mRegistryId+"].doFlash()", this.mUSpeed);
    } else {
      this.stop();
    }
},

  stop: function()
  {
    if (this.flashing) {
      window.clearTimeout(this.mFlashTimeout);
      this.mFlashTimeout = null;
      this.paintOff();
    }
  },

  paintOn: function()
  {
    this.mShell.drawElementOutline(this.mElement);
  },

  paintOff: function()
  {
    this.mShell.repaintElement(this.mElement);
  }

};

