




package org.mozilla.gecko;

import android.annotation.TargetApi;
import android.app.Presentation;
import android.content.Context;
import android.os.Bundle;
import android.support.v7.media.MediaRouter;
import android.util.Log;
import android.view.Display;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.view.WindowManager;

import org.mozilla.gecko.AppConstants.Versions;




@TargetApi(17)
public class PresentationMediaPlayerManager extends MediaPlayerManager {

    private static final String LOGTAG = "Gecko" + PresentationMediaPlayerManager.class.getSimpleName();

    private GeckoPresentation presentation;

    public PresentationMediaPlayerManager() {
        if (!Versions.feature17Plus) {
            throw new IllegalStateException(PresentationMediaPlayerManager.class.getSimpleName() +
                    " does not support < API 17");
        }
    }

    @Override
    public void onStop() {
        super.onStop();
        if (presentation != null) {
            presentation.dismiss();
            presentation = null;
        }
    }

    @Override
    protected void updatePresentation() {
        if (mediaRouter == null) {
            return;
        }

        MediaRouter.RouteInfo route = mediaRouter.getSelectedRoute();
        Display display = route != null ? route.getPresentationDisplay() : null;

        if (display != null) {
            if ((presentation != null) && (presentation.getDisplay() != display)) {
                presentation.dismiss();
                presentation = null;
            }

            if (presentation == null) {
                presentation = new GeckoPresentation(getActivity(), display);

                try {
                    presentation.show();
                } catch (WindowManager.InvalidDisplayException ex) {
                    Log.w(LOGTAG, "Couldn't show presentation!  Display was removed in "
                            + "the meantime.", ex);
                    presentation = null;
                }
            }
        } else if (presentation != null) {
            presentation.dismiss();
            presentation = null;
        }
    }

    private final static class GeckoPresentation extends Presentation {
        private SurfaceView mView;
        public GeckoPresentation(Context context, Display display) {
            super(context, display);
        }

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            mView = new SurfaceView(getContext());
            setContentView(mView, new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));
            mView.getHolder().addCallback(new SurfaceListener());
        }
    }

    private static class SurfaceListener implements SurfaceHolder.Callback {
        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width,
                                   int height) {
            
            GeckoAppShell.invalidateAndScheduleComposite();
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            GeckoAppShell.addPresentationSurface(holder.getSurface());
            GeckoAppShell.invalidateAndScheduleComposite();
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            GeckoAppShell.removePresentationSurface(holder.getSurface());
        }
    }
}
