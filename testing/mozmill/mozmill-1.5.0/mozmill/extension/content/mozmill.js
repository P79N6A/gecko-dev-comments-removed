





































var mozmill = {}; Components.utils.import('resource://mozmill/modules/mozmill.js', mozmill);
var utils = {}; Components.utils.import('resource://mozmill/modules/utils.js', utils);

var updateOutput = function(){
  
  var pass = document.getElementById('outPass');
  var fail = document.getElementById('outFail');
  var info = document.getElementById('outTest');

  
  var passCollect = window.document.getElementsByClassName('pass');
  var failCollect = window.document.getElementsByClassName('fail');
  var infoCollect = window.document.getElementsByClassName('test');
  
  
  var setDisplay = function(item, collection){
    for (var i = 0; i < collection.length; i++){
      if (item.checked == true){
        collection[i].style.display = "block";
      } else {
        collection[i].style.display = "none";
      }
    }
  };
  
  setDisplay(pass, passCollect);
  setDisplay(fail, failCollect);
  setDisplay(info, infoCollect);
};

function cleanUp(){
  
  removeStateListeners();
  
  utils.setPreference("mozmill.screenX", window.screenX);
  utils.setPreference("mozmill.screenY", window.screenY);
  utils.setPreference("mozmill.width", window.document.documentElement.clientWidth);
  utils.setPreference("mozmill.height", window.document.documentElement.clientHeight);
}
