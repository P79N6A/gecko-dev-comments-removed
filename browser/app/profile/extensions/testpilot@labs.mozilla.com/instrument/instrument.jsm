let EXPORTED_SYMBOLS = ["Instrument"];

let data = {};










let track = function(obj, func, name) {
  
  data[name] = 0;

  
  let orig = obj[func];
  obj[func] = function() {
    
    data[name]++;

    
    return orig.apply(this, arguments);
  };
}







function Instrument(window) {
  let $ = function(id) window.document.getElementById(id);

  track(window.gURLBar, "showHistoryPopup", "dropdown");
  track($("back-button"), "_handleClick", "back");
  track($("forward-button"), "_handleClick", "forward");
}


Instrument.report = function() JSON.stringify(data);
