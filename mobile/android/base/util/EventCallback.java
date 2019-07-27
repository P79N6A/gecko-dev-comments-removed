package org.mozilla.gecko.util;

import org.mozilla.gecko.mozglue.RobocopTarget;








@RobocopTarget
public interface EventCallback {
    





    public void sendSuccess(Object response);

    





    public void sendError(Object response);
}
