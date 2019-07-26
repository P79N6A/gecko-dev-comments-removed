




package org.mozilla.gecko.util;

import org.mozilla.gecko.mozglue.RobocopTarget;

@RobocopTarget
public interface NativeEventListener {
    void handleMessage(String event, NativeJSObject message);
}
