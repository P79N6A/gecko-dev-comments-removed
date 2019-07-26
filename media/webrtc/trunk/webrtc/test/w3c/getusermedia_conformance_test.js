







setup({timeout:10000});


function failedCallback(test) {
  return test.step_func(function (error) {
    assert_unreached('Should not get an error callback');
  });
}
function invokeGetUserMedia(test, okCallback) {
  getUserMedia({ video: true, audio: true }, okCallback,
      failedCallback(test));
}


var mediaStreamTest = async_test('4.2 MediaStream');

function verifyMediaStream(stream) {
  
  test(function () {
    assert_own_property(stream, 'id');
    assert_true(typeof stream.id === 'string');
    assert_readonly(stream, 'id');
  }, '[MediaStream] id attribute');

  test(function () {
    assert_inherits(stream, 'getAudioTracks');
    assert_true(typeof stream.getAudioTracks === 'function');
  }, '[MediaStream] getAudioTracks function');

  test(function () {
    assert_inherits(stream, 'getVideoTracks');
    assert_true(typeof stream.getVideoTracks === 'function');
  }, '[MediaStream] getVideoTracks function');

  test(function () {
    assert_inherits(stream, 'getTrackById');
    assert_true(typeof stream.getTrackById === 'function');
  }, '[MediaStream] getTrackById function');

  test(function () {
    assert_inherits(stream, 'addTrack');
    assert_true(typeof stream.addTrack === 'function');
  }, '[MediaStream] addTrack function');

  test(function () {
    assert_inherits(stream, 'removeTrack');
    assert_true(typeof stream.removeTrack === 'function');
  }, '[MediaStream] removeTrack function');

  test(function () {
    
    assert_inherits(stream, 'clone');
    assert_true(typeof stream.clone === 'function');
  }, '[MediaStream] clone function');

  test(function () {
    assert_own_property(stream, 'ended');
    assert_true(typeof stream.ended === 'boolean');
    assert_readonly(stream, 'ended');
  }, '[MediaStream] ended attribute');

  test(function () {
    assert_own_property(stream, 'onended');
    assert_true(stream.onended === null);
  }, '[MediaStream] onended EventHandler');

  test(function () {
    assert_own_property(stream, 'onaddtrack');
    assert_true(stream.onaddtrack === null);
  }, '[MediaStream] onaddtrack EventHandler');

  test(function () {
    assert_own_property(stream, 'onremovetrack');
    assert_true(stream.onremovetrack === null);
  }, '[MediaStream] onremovetrack EventHandler');
}

mediaStreamTest.step(function() {
  var okCallback = mediaStreamTest.step_func(function (stream) {
    verifyMediaStream(stream);

    var videoTracks = stream.getVideoTracks();
    assert_true(videoTracks.length > 0);

    
    stream.onaddtrack = onAddTrackCallback
    stream.onremovetrack = onRemoveTrackCallback
    stream.removeTrack(videoTracks[0]);
    stream.addTrack(videoTracks[0]);
    mediaStreamTest.done();
  });
  var onAddTrackCallback = mediaStreamTest.step_func(function () {
    
    mediaStreamTest.done();
  });
  var onRemoveTrackCallback = mediaStreamTest.step_func(function () {
    
    mediaStreamTest.done();
  });
  invokeGetUserMedia(mediaStreamTest, okCallback);;
});


var mediaStreamTrackTest = async_test('4.3 MediaStreamTrack');

function verifyTrack(type, track) {
  test(function () {
    assert_own_property(track, 'kind');
    assert_readonly(track, 'kind');
    assert_true(typeof track.kind === 'string',
        'kind is an object (DOMString)');
  }, '[MediaStreamTrack (' + type + ')] kind attribute');

  test(function () {
    assert_own_property(track, 'id');
    assert_readonly(track, 'id');
    assert_true(typeof track.id === 'string',
        'id is an object (DOMString)');
  }, '[MediaStreamTrack (' + type + ')] id attribute');

  test(function () {
    assert_own_property(track, 'label');
    assert_readonly(track, 'label');
    assert_true(typeof track.label === 'string',
        'label is an object (DOMString)');
  }, '[MediaStreamTrack (' + type + ')] label attribute');

  test(function () {
    assert_own_property(track, 'enabled');
    assert_true(typeof track.enabled === 'boolean');
    assert_true(track.enabled, 'enabled property must be true initially');
  }, '[MediaStreamTrack (' + type + ')] enabled attribute');

  test(function () {
    
    assert_own_property(track, 'muted');
    assert_readonly(track, 'muted');
    assert_true(typeof track.muted === 'boolean');
    assert_false(track.muted, 'muted property must be false initially');
  }, '[MediaStreamTrack (' + type + ')] muted attribute');

  test(function () {
    assert_own_property(track, 'onmute');
    assert_true(track.onmute === null);
  }, '[MediaStreamTrack (' + type + ')] onmute EventHandler');

  test(function () {
    assert_own_property(track, 'onunmute');
    assert_true(track.onunmute === null);
  }, '[MediaStreamTrack (' + type + ')] onunmute EventHandler');

  test(function () {
    
    assert_own_property(track, '_readonly');
    assert_readonly(track, '_readonly');
    assert_true(typeof track._readonly === 'boolean');
  }, '[MediaStreamTrack (' + type + ')] _readonly attribute');

  test(function () {
    
    assert_own_property(track, 'remote');
    assert_readonly(track, 'remote');
    assert_true(typeof track.remote === 'boolean');
  }, '[MediaStreamTrack (' + type + ')] remote attribute');

  test(function () {
    assert_own_property(track, 'readyState');
    assert_readonly(track, 'readyState');
    assert_true(typeof track.readyState === 'string');
    
  }, '[MediaStreamTrack (' + type + ')] readyState attribute');

  test(function () {
    
    assert_own_property(track, 'onstarted');
    assert_true(track.onstarted === null);
  }, '[MediaStreamTrack (' + type + ')] onstarted EventHandler');

  test(function () {
    assert_own_property(track, 'onended');
    assert_true(track.onended === null);
  }, '[MediaStreamTrack (' + type + ')] onended EventHandler');

  test(function () {
    
    assert_inherits(track, 'getSourceInfos');
    assert_true(typeof track.getSourceInfos === 'function');
  }, '[MediaStreamTrack (' + type + ')]: getSourceInfos function');

  test(function () {
    
    assert_inherits(track, 'constraints');
    assert_true(typeof track.constraints === 'function');
  }, '[MediaStreamTrack (' + type + ')]: constraints function');

  test(function () {
    
    assert_inherits(track, 'states');
    assert_true(typeof track.states === 'function');
  }, '[MediaStreamTrack (' + type + ')]: states function');

  test(function () {
    
    assert_inherits(track, 'capabilities');
    assert_true(typeof track.capabilities === 'function');
  }, '[MediaStreamTrack (' + type + ')]: capabilities function');

  test(function () {
    
    assert_inherits(track, 'applyConstraints');
    assert_true(typeof track.applyConstraints === 'function');
  }, '[MediaStreamTrack (' + type + ')]: applyConstraints function');

  test(function () {
    
    assert_own_property(track, 'onoverconstrained');
    assert_true(track.onoverconstrained === null);
  }, '[MediaStreamTrack (' + type + ')] onoverconstrained EventHandler');

  test(function () {
    
    assert_inherits(track, 'clone');
    assert_true(typeof track.clone === 'function');
  }, '[MediaStreamTrack (' + type + ')] clone function');

  test(function () {
    
    assert_inherits(track, 'stop');
    assert_true(typeof track.stop === 'function');
  }, '[MediaStreamTrack (' + type + ')] stop function');
};
mediaStreamTrackTest.step(function() {
  var okCallback = mediaStreamTrackTest.step_func(function (stream) {
    verifyTrack('audio', stream.getAudioTracks()[0]);
    verifyTrack('video', stream.getVideoTracks()[0]);
    mediaStreamTrackTest.done();
  });
  invokeGetUserMedia(mediaStreamTrackTest, okCallback);
});

mediaStreamTrackTest.step(function() {
  var okCallback = mediaStreamTrackTest.step_func(function (stream) {
    
    var track = stream.getVideoTracks()[0];
    track.onended = onendedCallback
    track.stop();
    mediaStreamTrackTest.done();
  });
  var onendedCallback = mediaStreamTrackTest.step_func(function () {
    assert_true(track.ended);
    mediaStreamTrackTest.done();
  });
  invokeGetUserMedia(mediaStreamTrackTest, okCallback);
});


var mediaStreamTrackEventTest = async_test('4.4 MediaStreamTrackEvent');
mediaStreamTrackEventTest.step(function() {
  var okCallback = mediaStreamTrackEventTest.step_func(function (stream) {
    
    mediaStreamTrackEventTest.done();
  });
  invokeGetUserMedia(mediaStreamTrackEventTest, okCallback);
});


var avTracksTest = async_test('4.5 Video and Audio Tracks');
avTracksTest.step(function() {
  var okCallback = avTracksTest.step_func(function (stream) {
    
    avTracksTest.done();
  });
  invokeGetUserMedia(avTracksTest, okCallback);
});













var createObjectURLTest = async_test('8.1 URL createObjectURL method');
createObjectURLTest.step(function() {
  var okCallback = createObjectURLTest.step_func(function (stream) {
    var url = webkitURL.createObjectURL(stream);
    assert_true(typeof url === 'string');
    createObjectURLTest.done();
  });
  invokeGetUserMedia(createObjectURLTest, okCallback);
});


var mediaElementsTest = async_test('9. MediaStreams as Media Elements');

function verifyVideoTagWithStream(videoTag) {
  test(function () {
    assert_equals(videoTag.buffered.length, 0);
  }, '[Video tag] buffered attribute');

  test(function () {
    
    assert_true(videoTag.currentTime >= 0);
    assert_throws('InvalidStateError',
                  function () { videoTag.currentTime = 1234; },
                  'Attempts to modify currentTime shall throw ' +
                      'InvalidStateError');
  }, '[Video tag] currentTime attribute');

  test(function () {
    assert_equals(videoTag.duration, Infinity, 'videoTag.duration');
  }, '[Video tag] duration attribute');

  test(function () {
    assert_false(videoTag.seeking, 'videoTag.seeking');
  }, '[Video tag] seeking attribute');

  test(function () {
    assert_equals(videoTag.defaultPlaybackRate, 1.0);
    assert_throws('DOMException',
                  function () { videoTag.defaultPlaybackRate = 2.0; },
                  'Attempts to alter videoTag.defaultPlaybackRate MUST fail');
  }, '[Video tag] defaultPlaybackRate attribute');

  test(function () {
    assert_equals(videoTag.playbackRate, 1.0);
    assert_throws('DOMException',
      function () { videoTag.playbackRate = 2.0; },
      'Attempts to alter videoTag.playbackRate MUST fail');
  }, '[Video tag] playbackRate attribute');

  test(function () {
    assert_equals(videoTag.played.length, 1, 'videoTag.played.length');
    assert_equals(videoTag.played.start(0), 0);
    assert_true(videoTag.played.end(0) >= videoTag.currentTime);
  }, '[Video tag] played attribute');

  test(function () {
    assert_equals(videoTag.seekable.length, 0);
    assert_equals(videoTag.seekable.start(), videoTag.currentTime);
    assert_equals(videoTag.seekable.end(), videoTag.currentTime);
    assert_equals(videoTag.startDate, NaN, 'videoTag.startDate');
  }, '[Video tag] seekable attribute');

  test(function () {
    assert_false(videoTag.loop);
  }, '[Video tag] loop attribute');
};

mediaElementsTest.step(function() {
  var okCallback = mediaElementsTest.step_func(function (stream) {
    var videoTag = document.getElementById('local-view');
    
    attachMediaStream(videoTag, stream);
    verifyVideoTagWithStream(videoTag);
    mediaElementsTest.done();
  });
  invokeGetUserMedia(mediaElementsTest, okCallback);
});




var getUserMediaTest = async_test('11.1 NavigatorUserMedia');
getUserMediaTest.step(function() {
  var okCallback = getUserMediaTest.step_func(function (stream) {
    assert_true(stream !== null);
    getUserMediaTest.done();
  });

  
  getUserMedia({ video: true, audio: true }, okCallback);
  getUserMedia({ video: true, audio: false }, okCallback);
  getUserMedia({ video: false, audio: true }, okCallback);

  
  getUserMedia({ video: true, audio: true }, okCallback,
      failedCallback(getUserMediaTest));
  getUserMedia({ video: true, audio: false }, okCallback,
      failedCallback(getUserMediaTest));
  getUserMedia({ video: false, audio: true }, okCallback,
      failedCallback(getUserMediaTest));
});


var constraintsTest = async_test('11.2 MediaStreamConstraints');
constraintsTest.step(function() {
  var okCallback = constraintsTest.step_func(function (stream) {
    assert_true(stream !== null);
    constraintsTest.done();
  });

  
  
  
  var constraints = {};
  constraints.audio = true;
  constraints.video = { mandatory: {}, optional: [] };
  constraints.video.mandatory.minWidth = 640;
  constraints.video.mandatory.minHeight = 480;
  constraints.video.mandatory.minFrameRate = 15;

  getUserMedia(constraints, okCallback, failedCallback(constraintsTest));
});


var successCallbackTest =
  async_test('11.3 NavigatorUserMediaSuccessCallback');
successCallbackTest.step(function() {
  var okCallback = successCallbackTest.step_func(function (stream) {
    assert_true(stream !== null);
    successCallbackTest.done();
  });
  invokeGetUserMedia(successCallbackTest, okCallback);
});


var errorCallbackTest = async_test('11.4 NavigatorUserMediaError and ' +
                                   'NavigatorUserMediaErrorCallback');
errorCallbackTest.step(function() {
  var okCallback = errorCallbackTest.step_func(function (stream) {
    assert_unreached('Should not get a success callback');
  });
  var errorCallback = errorCallbackTest.step_func(function (error) {
    assert_own_property(error, 'name');
    assert_readonly(error.name);
    assert_true(typeof error.name === 'string');
    assert_equals(error.name, 'ConstraintNotSatisfiedError', 'error.name');
    errorCallbackTest.done();
  });
  
  
  

  
  
  getUserMedia({ video: false, audio: false }, okCallback, errorCallback);
});
