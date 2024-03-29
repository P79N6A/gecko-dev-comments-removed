



function onOK() {
  window.arguments[0].out = {
    video: document.getElementById("video").selectedItem.value,
    audio: document.getElementById("audio").selectedItem.value
  };

  return true;
}

function onLoad() {
  let videoDevices = window.arguments[0].videoDevices;
  if (videoDevices.length) {
    let videoMenu = document.getElementById("video");
    for (let i = 0; i < videoDevices.length; i++) {
      videoMenu.appendItem(videoDevices[i].name, i);
    }
    videoMenu.selectedIndex = 1;
  } else {
    document.getElementById("videoGroup").hidden = true;
  }

  let audioDevices = window.arguments[0].audioDevices;
  if (audioDevices.length) {
    let audioMenu = document.getElementById("audio");
    for (let i = 0; i < audioDevices.length; i++) {
      audioMenu.appendItem(audioDevices[i].name, i);
    }
    audioMenu.selectedIndex = 1;
  } else {
    document.getElementById("audioGroup").hidden = true;
  }

  window.sizeToContent();
}
