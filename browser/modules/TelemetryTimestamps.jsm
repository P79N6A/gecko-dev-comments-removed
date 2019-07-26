



this.EXPORTED_SYMBOLS = ["TelemetryTimestamps"];










let timeStamps = {};

this.TelemetryTimestamps = {
  








  add: function TT_add(name, value) {
    
    if (value == null)
      value = Date.now();

    if (isNaN(value))
      throw new Error("Value must be a timestamp");

    
    if (timeStamps.hasOwnProperty(name))
      return;

    timeStamps[name] = value;
  },

  




  get: function TT_get() {
    
    return JSON.parse(JSON.stringify(timeStamps));
  }
};
