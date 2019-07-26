



'use strict';
Components.utils.import("resource://gre/modules/Services.jsm");
const ColorAnalyzer = Components.classes["@mozilla.org/places/colorAnalyzer;1"]
                .getService(Components.interfaces.mozIColorAnalyzer);

this.EXPORTED_SYMBOLS = ["ColorUtils"];

let ColorUtils = {
  



  getForegroundAndBackgroundIconColors: function getForegroundAndBackgroundIconColors(aIconURI, aSuccessCallback) {
    if (!aIconURI) {
      return;
    }

    let that = this;
    let wrappedIcon = aIconURI;
    ColorAnalyzer.findRepresentativeColor(wrappedIcon, function (success, color) {
      let foregroundColor = that.bestTextColorForContrast(color);
      let backgroundColor = that.convertDecimalToRgbColor(color);
      
      aSuccessCallback(foregroundColor, backgroundColor);
    }, this);
  },
  



  bestTextColorForContrast: function bestTextColorForContrast(aColor) {
    let r = (aColor & 0xff0000) >> 16;
    let g = (aColor & 0x00ff00) >> 8;
    let b = (aColor & 0x0000ff);

    let w3cContrastValue = ((r*299)+(g*587)+(b*114))/1000;
    w3cContrastValue = Math.round(w3cContrastValue);
    let textColor = "rgb(255,255,255)";

    if (w3cContrastValue > 125) {
      
      textColor = "rgb(0,0,0)";
    }
    return textColor;
  },

  


  convertDecimalToRgbColor: function convertDecimalToRgbColor(aColor) {
    let r = (aColor & 0xff0000) >> 16;
    let g = (aColor & 0x00ff00) >> 8;
    let b = (aColor & 0x0000ff);
    return "rgb("+r+","+g+","+b+")";
  }
};
