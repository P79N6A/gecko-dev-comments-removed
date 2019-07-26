(function(global) {
  "use strict";

  function IDPJS() {
    this.domain = window.location.host;
    
    
    
    this.instructions = window.location.hash.replace("#", "").split(":");
    this.port = window.rtcwebIdentityPort;
    this.port.onmessage = this.receiveMessage.bind(this);
    this.sendResponse({
      type : "READY"
    });
  }

  IDPJS.prototype.getDelay = function() {
    
    
    var delay = 0;
    function addDelay(instruction) {
      var m = instruction.match(/^delay(\d+)$/);
      if (m) {
        delay += parseInt(m[1], 10);
      }
    }
    this.instructions.forEach(addDelay);
    return delay;
  };

  function is(target) {
    return function(instruction) {
      return instruction === target;
    };
  }

  IDPJS.prototype.sendResponse = function(response) {
    
    if (response.type === "READY" && !this.instructions.some(is("ready"))) {
      this.port.postMessage(response);
      return;
    }

    
    if (this.instructions.some(is("error"))) {
      response.type = "ERROR";
    }

    window.setTimeout(function() {
      this.port.postMessage(response);
    }.bind(this), this.getDelay());
  };

  IDPJS.prototype.receiveMessage = function(ev) {
    var message = ev.data;
    switch (message.type) {
    case "SIGN":
      this.sendResponse({
        type : "SUCCESS",
        id : message.id,
        message : {
          idp : {
            domain : this.domain,
            protocol : "idp.html"
          },
          assertion : JSON.stringify({
            identity : "someone@" + this.domain,
            contents : message.message
          })
        }
      });
      break;
    case "VERIFY":
      this.sendResponse({
        type : "SUCCESS",
        id : message.id,
        message : {
          identity : {
            name : "someone@" + this.domain,
            displayname : "Someone"
          },
          contents : JSON.parse(message.message).contents
        }
      });
      break;
    default:
      this.sendResponse({
        type : "ERROR",
        error : JSON.stringify(message)
      });
      break;
    }
  };

  global.idp = new IDPJS();
}(this));
