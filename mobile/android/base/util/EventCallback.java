package org.mozilla.gecko.util;








public interface EventCallback {
    





    public void sendSuccess(Object response);

    





    public void sendError(Object response);

    


    public void sendCancel();
}
