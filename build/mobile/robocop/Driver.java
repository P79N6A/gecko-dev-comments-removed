



package org.mozilla.gecko;

import android.app.Activity;

public interface Driver {
    






    Element findElement(Activity activity, int id);

    


    void setupScrollHandling();

    int getPageHeight();
    int getScrollHeight();
    int getHeight();
    int getGeckoTop();
    int getGeckoLeft();
    int getGeckoWidth();
    int getGeckoHeight();

    void startFrameRecording();
    int stopFrameRecording();

    void startCheckerboardRecording();
    float stopCheckerboardRecording();

    




    PaintedSurface getPaintedSurface();
}
