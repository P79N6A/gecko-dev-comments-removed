




package org.mozilla.gecko.overlays.ui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.content.LocalBroadcastManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.TextView;
import android.widget.Toast;

import org.mozilla.gecko.Assert;
import org.mozilla.gecko.R;
import org.mozilla.gecko.overlays.OverlayConstants;
import org.mozilla.gecko.overlays.service.OverlayActionService;
import org.mozilla.gecko.overlays.service.sharemethods.ParcelableClientRecord;
import org.mozilla.gecko.overlays.service.sharemethods.SendTab;
import org.mozilla.gecko.overlays.service.sharemethods.ShareMethod;
import org.mozilla.gecko.LocaleAware;
import org.mozilla.gecko.sync.setup.activities.WebURLFinder;




public class ShareDialog extends LocaleAware.LocaleAwareActivity implements SendTabTargetSelectedListener {
    private static final String LOGTAG = "GeckoShareDialog";

    private String url;
    private String title;

    
    private Intent sendTabOverrideIntent;

    
    private boolean isAnimating;

    
    private final BroadcastReceiver uiEventListener = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            ShareMethod.Type originShareMethod = intent.getParcelableExtra(OverlayConstants.EXTRA_SHARE_METHOD);
            switch (originShareMethod) {
                case SEND_TAB:
                    handleSendTabUIEvent(intent);
                    break;
                default:
                    throw new IllegalArgumentException("UIEvent broadcast from ShareMethod that isn't thought to support such broadcasts.");
            }
        }
    };

    


    protected void handleSendTabUIEvent(Intent intent) {
        sendTabOverrideIntent = intent.getParcelableExtra(SendTab.OVERRIDE_INTENT);

        SendTabList sendTabList = (SendTabList) findViewById(R.id.overlay_send_tab_btn);

        ParcelableClientRecord[] clientrecords = (ParcelableClientRecord[]) intent.getParcelableArrayExtra(SendTab.EXTRA_CLIENT_RECORDS);
        sendTabList.setSyncClients(clientrecords);
    }

    @Override
    protected void onDestroy() {
        
        
        
        
        LocalBroadcastManager.getInstance(this).unregisterReceiver(uiEventListener);

        super.onDestroy();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setWindowAnimations(0);

        Intent intent = getIntent();

        
        String extraText = intent.getStringExtra(Intent.EXTRA_TEXT);
        String pageUrl = new WebURLFinder(extraText).bestWebURL();

        if (TextUtils.isEmpty(pageUrl)) {
            Log.e(LOGTAG, "Unable to process shared intent. No URL found!");

            
            
            Toast toast = Toast.makeText(this, getResources().getText(R.string.overlay_share_no_url), Toast.LENGTH_SHORT);
            toast.show();
            finish();

            return;
        }

        setContentView(R.layout.overlay_share_dialog);

        LocalBroadcastManager.getInstance(this).registerReceiver(uiEventListener,
                                                                 new IntentFilter(OverlayConstants.SHARE_METHOD_UI_EVENT));

        
        
        Intent serviceStartupIntent = new Intent(this, OverlayActionService.class);
        serviceStartupIntent.setAction(OverlayConstants.ACTION_PREPARE_SHARE);
        startService(serviceStartupIntent);

        
        
        
        String subjectText = intent.getStringExtra(Intent.EXTRA_SUBJECT);
        if (subjectText != null) {
            ((TextView) findViewById(R.id.title)).setText(subjectText);
        }

        title = subjectText;
        url = pageUrl;

        
        
        TextView subtitleView = (TextView) findViewById(R.id.subtitle);
        subtitleView.setText(pageUrl);
        subtitleView.setEllipsize(TextUtils.TruncateAt.MARQUEE);
        subtitleView.setSingleLine(true);
        subtitleView.setMarqueeRepeatLimit(5);
        subtitleView.setSelected(true);

        
        Animation anim = AnimationUtils.loadAnimation(this, R.anim.overlay_slide_up);
        findViewById(R.id.sharedialog).startAnimation(anim);

        

        findViewById(R.id.overlay_share_bookmark_btn).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                addBookmark();
            }
        });

        findViewById(R.id.overlay_share_reading_list_btn).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                addToReadingList();
            }
        });

        
        SendTabList sendTabList = (SendTabList) findViewById(R.id.overlay_send_tab_btn);

        
        SendTabDeviceListArrayAdapter adapter = new SendTabDeviceListArrayAdapter(this, this);
        sendTabList.setAdapter(adapter);
        sendTabList.setSendTabTargetSelectedListener(this);
    }

    


    private Intent getServiceIntent(ShareMethod.Type method) {
        final Intent serviceIntent = new Intent(this, OverlayActionService.class);
        serviceIntent.setAction(OverlayConstants.ACTION_SHARE);

        serviceIntent.putExtra(OverlayConstants.EXTRA_SHARE_METHOD, (Parcelable) method);
        serviceIntent.putExtra(OverlayConstants.EXTRA_URL, url);
        serviceIntent.putExtra(OverlayConstants.EXTRA_TITLE, title);

        return serviceIntent;
    }

    @Override
    public void finish() {
        super.finish();

        
        overridePendingTransition(0, 0);
    }

    





    public void sendTab(String targetGUID) {
        
        if (sendTabOverrideIntent != null) {
            startActivity(sendTabOverrideIntent);
            finish();
            return;
        }

        
        Assert.isTrue(targetGUID != null);

        Intent serviceIntent = getServiceIntent(ShareMethod.Type.SEND_TAB);

        
        Bundle extraParameters = new Bundle();

        
        extraParameters.putStringArray(SendTab.SEND_TAB_TARGET_DEVICES, new String[] { targetGUID });

        serviceIntent.putExtra(OverlayConstants.EXTRA_PARAMETERS, extraParameters);

        startService(serviceIntent);
        slideOut();
    }

    @Override
    public void onSendTabTargetSelected(String targetGUID) {
        sendTab(targetGUID);
    }

    public void addToReadingList() {
        startService(getServiceIntent(ShareMethod.Type.ADD_TO_READING_LIST));
        slideOut();
    }

    public void addBookmark() {
        startService(getServiceIntent(ShareMethod.Type.ADD_BOOKMARK));
        slideOut();
    }

    


    private void slideOut() {
        if (isAnimating) {
            return;
        }

        isAnimating = true;
        Animation anim = AnimationUtils.loadAnimation(this, R.anim.overlay_slide_down);
        findViewById(R.id.sharedialog).startAnimation(anim);

        anim.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation animation) {
                
            }

            @Override
            public void onAnimationEnd(Animation animation) {
                finish();
            }

            @Override
            public void onAnimationRepeat(Animation animation) {
                
            }
        });
    }

    


    @Override
    public void onBackPressed() {
        slideOut();
    }

    


    @Override
    public boolean onTouchEvent(MotionEvent event) {
        slideOut();
        return true;
    }
}
