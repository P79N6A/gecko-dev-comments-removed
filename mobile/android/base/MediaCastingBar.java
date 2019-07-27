



package org.mozilla.gecko;

import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONObject;

import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageButton;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class MediaCastingBar extends RelativeLayout implements View.OnClickListener, GeckoEventListener  {
    private static final String LOGTAG = "GeckoMediaCastingBar";

    private TextView mCastingTo;
    private ImageButton mMediaPlay;
    private ImageButton mMediaPause;
    private ImageButton mMediaStop;

    private boolean mInflated;

    public MediaCastingBar(Context context, AttributeSet attrs) {
        super(context, attrs);

        EventDispatcher.getInstance().registerGeckoThreadListener(this,
            "Casting:Started",
            "Casting:Stopped");
    }

    public void inflateContent() {
        LayoutInflater inflater = LayoutInflater.from(getContext());
        View content = inflater.inflate(R.layout.media_casting, this);

        mMediaPlay = (ImageButton) content.findViewById(R.id.media_play);
        mMediaPlay.setOnClickListener(this);
        mMediaPause = (ImageButton) content.findViewById(R.id.media_pause);
        mMediaPause.setOnClickListener(this);
        mMediaStop = (ImageButton) content.findViewById(R.id.media_stop);
        mMediaStop.setOnClickListener(this);

        mCastingTo = (TextView) content.findViewById(R.id.media_sending_to);

        
        
        content.setOnClickListener(this);

        mInflated = true;
    }

    public void show() {
        if (!mInflated)
            inflateContent();

        setVisibility(VISIBLE);
    }

    public void hide() {
        setVisibility(GONE);
    }

    public void onDestroy() {
        EventDispatcher.getInstance().unregisterGeckoThreadListener(this,
            "Casting:Started",
            "Casting:Stopped");
    }

    
    @Override
    public void onClick(View v) {
        final int viewId = v.getId();

        if (viewId == R.id.media_play) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Casting:Play", ""));
            mMediaPlay.setVisibility(GONE);
            mMediaPause.setVisibility(VISIBLE);
        } else if (viewId == R.id.media_pause) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Casting:Pause", ""));
            mMediaPause.setVisibility(GONE);
            mMediaPlay.setVisibility(VISIBLE);
        } else if (viewId == R.id.media_stop) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Casting:Stop", ""));
        }
    }

    
    @Override
    public void handleMessage(final String event, final JSONObject message) {
        final String device = message.optString("device");

        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                if (event.equals("Casting:Started")) {
                    show();
                    if (!TextUtils.isEmpty(device)) {
                        mCastingTo.setText(device);
                    } else {
                        
                        mCastingTo.setText("");
                        Log.d(LOGTAG, "Device name is empty.");
                    }
                    mMediaPlay.setVisibility(GONE);
                    mMediaPause.setVisibility(VISIBLE);
                } else if (event.equals("Casting:Stopped")) {
                    hide();
                }
            }
        });
    }
}
