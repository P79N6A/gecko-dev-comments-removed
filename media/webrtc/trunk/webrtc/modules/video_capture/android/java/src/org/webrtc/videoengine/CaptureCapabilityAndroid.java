









package org.webrtc.videoengine;

import org.mozilla.gecko.mozglue.WebRTCJNITarget;

@WebRTCJNITarget
public class CaptureCapabilityAndroid {
    public String name;
    public int width[];
    public int height[];
    public int minMilliFPS;
    public int maxMilliFPS;
    public boolean frontFacing;
    public int orientation;
}
