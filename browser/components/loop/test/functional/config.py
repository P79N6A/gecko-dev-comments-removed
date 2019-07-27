
CONTENT_SERVER_PORT = 3001
LOOP_SERVER_PORT = 5001
LOOP_SERVER_URL = "http://localhost:" + str(LOOP_SERVER_PORT)
FIREFOX_PREFERENCES = {
    "loop.server": LOOP_SERVER_URL + "/v0",
    "browser.dom.window.dump.enabled": True,
    
    "media.peerconnection.default_iceservers": "[]",
    "media.peerconnection.use_document_iceservers": False,
    "media.peerconnection.ice.loopback": True,
    "devtools.chrome.enabled": True,
    "devtools.debugger.prompt-connection": False,
    "devtools.debugger.remote-enabled": True,
    "media.volume_scale": "0",
    "loop.gettingStarted.seen": True,
    "loop.seenToS": "seen",

    
    "media.navigator.permission.disabled": True,
    
    "media.navigator.streams.fake": True
}
