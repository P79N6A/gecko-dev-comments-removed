var RTCPeerConnection = null;
var getUserMedia = null;
var attachMediaStream = null;

if (navigator.mozGetUserMedia) {
  console.log("This appears to be Firefox");

  
  RTCPeerConnection = mozRTCPeerConnection;

  
  
  getUserMedia = navigator.mozGetUserMedia.bind(navigator);

  
  attachMediaStream = function(element, stream) {
    console.log("Attaching media stream");
    element.mozSrcObject = stream;
    element.play();
  };
} else if (navigator.webkitGetUserMedia) {
  console.log("This appears to be Chrome");

  
  RTCPeerConnection = webkitRTCPeerConnection;
  
  
  
  getUserMedia = navigator.webkitGetUserMedia.bind(navigator);

  
  attachMediaStream = function(element, stream) {
    element.src = webkitURL.createObjectURL(stream);
  };

  
  
  if (!webkitMediaStream.prototype.getVideoTracks) {
      webkitMediaStream.prototype.getVideoTracks = function() {
      return this.videoTracks;
    } 
  } 
  
  if (!webkitMediaStream.prototype.getAudioTracks) {
      webkitMediaStream.prototype.getAudioTracks = function() {
      return this.audioTracks;
    }
  } 
} else {
  console.log("Browser does not appear to be WebRTC-capable");
}
