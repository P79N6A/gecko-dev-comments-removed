




package org.mozilla.gecko.overlays.ui;

import java.net.URISyntaxException;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.Locales;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.LocalBrowserDB;
import org.mozilla.gecko.overlays.OverlayConstants;
import org.mozilla.gecko.overlays.service.OverlayActionService;
import org.mozilla.gecko.overlays.service.sharemethods.ParcelableClientRecord;
import org.mozilla.gecko.overlays.service.sharemethods.SendTab;
import org.mozilla.gecko.overlays.service.sharemethods.ShareMethod;
import org.mozilla.gecko.sync.setup.activities.WebURLFinder;
import org.mozilla.gecko.mozglue.ContextUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UIAsyncTask;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.Drawable;
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




public class ShareDialog extends Locales.LocaleAwareActivity implements SendTabTargetSelectedListener {

    private enum State {
        DEFAULT,
        DEVICES_ONLY 
    }

    private static final String LOGTAG = "GeckoShareDialog";

    
    public static final String INTENT_EXTRA_DEVICES_ONLY =
            AppConstants.ANDROID_PACKAGE_NAME + ".intent.extra.DEVICES_ONLY";

    
    private static final int MAXIMUM_INLINE_DEVICES = 2;

    private State state;

    private SendTabList sendTabList;
    private OverlayDialogButton readingListButton;
    private OverlayDialogButton bookmarkButton;

    
    private Drawable readingListButtonDrawable;

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

        ParcelableClientRecord[] clientrecords = (ParcelableClientRecord[]) intent.getParcelableArrayExtra(SendTab.EXTRA_CLIENT_RECORDS);

        
        
        
        if (state == State.DEVICES_ONLY &&
                (clientrecords == null || clientrecords.length == 0)) {
            Log.e(LOGTAG, "In state: " + State.DEVICES_ONLY + " and received 0 synced clients. Finishing...");
            Toast.makeText(this, getResources().getText(R.string.overlay_no_synced_devices), Toast.LENGTH_SHORT)
                 .show();
            finish();
            return;
        }

        sendTabList.setSyncClients(clientrecords);

        if (state == State.DEVICES_ONLY ||
                clientrecords == null ||
                clientrecords.length <= MAXIMUM_INLINE_DEVICES) {
            
            sendTabList.switchState(SendTabList.State.LIST);

            
            
            
            
            
            if (clientrecords == null || clientrecords.length == 0) {
                readingListButton.setBackgroundResource(
                        R.drawable.overlay_share_button_background_first);
            }
            return;
        }

        
        sendTabList.switchState(SendTabList.State.SHOW_DEVICES);
    }

    @Override
    protected void onDestroy() {
        
        
        
        
        LocalBroadcastManager.getInstance(this).unregisterReceiver(uiEventListener);

        super.onDestroy();
    }

    


    private void abortDueToNoURL() {
        Log.e(LOGTAG, "Unable to process shared intent. No URL found!");

        
        
        Toast toast = Toast.makeText(this, getResources().getText(R.string.overlay_share_no_url), Toast.LENGTH_SHORT);
        toast.show();
        finish();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.overlay_share_dialog);

        LocalBroadcastManager.getInstance(this).registerReceiver(uiEventListener,
                new IntentFilter(OverlayConstants.SHARE_METHOD_UI_EVENT));

        
        sendTabList = (SendTabList) findViewById(R.id.overlay_send_tab_btn);

        
        final SendTabDeviceListArrayAdapter adapter = new SendTabDeviceListArrayAdapter(this, this);
        sendTabList.setAdapter(adapter);
        sendTabList.setSendTabTargetSelectedListener(this);

        bookmarkButton = (OverlayDialogButton) findViewById(R.id.overlay_share_bookmark_btn);
        readingListButton = (OverlayDialogButton) findViewById(R.id.overlay_share_reading_list_btn);

        readingListButtonDrawable = readingListButton.getBackground();

        
        bookmarkButton = (OverlayDialogButton) findViewById(R.id.overlay_share_bookmark_btn);
        bookmarkButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                addBookmark();
            }
        });

        
        readingListButton = (OverlayDialogButton) findViewById(R.id.overlay_share_reading_list_btn);
        readingListButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                addToReadingList();
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        final Intent intent = getIntent();

        state = intent.getBooleanExtra(INTENT_EXTRA_DEVICES_ONLY, false) ?
                State.DEVICES_ONLY : State.DEFAULT;

        
        
        sendTabList.switchState(SendTabList.State.LOADING);
        readingListButton.setBackgroundDrawable(readingListButtonDrawable);

        
        final String extraText = ContextUtils.getStringExtra(intent, Intent.EXTRA_TEXT);
        if (TextUtils.isEmpty(extraText)) {
            abortDueToNoURL();
            return;
        }

        final String pageUrl = new WebURLFinder(extraText).bestWebURL();
        if (TextUtils.isEmpty(pageUrl)) {
            abortDueToNoURL();
            return;
        }

        
        
        Intent serviceStartupIntent = new Intent(this, OverlayActionService.class);
        serviceStartupIntent.setAction(OverlayConstants.ACTION_PREPARE_SHARE);
        startService(serviceStartupIntent);

        
        getWindow().setWindowAnimations(0);
        final Animation anim = AnimationUtils.loadAnimation(this, R.anim.overlay_slide_up);
        findViewById(R.id.sharedialog).startAnimation(anim);

        
        

        
        final String subjectText = intent.getStringExtra(Intent.EXTRA_SUBJECT);

        final String telemetryExtras = "title=" + (subjectText != null);
        if (subjectText != null) {
            ((TextView) findViewById(R.id.title)).setText(subjectText);
        }

        Telemetry.sendUIEvent(TelemetryContract.Event.SHOW, TelemetryContract.Method.SHARE_OVERLAY, telemetryExtras);

        title = subjectText;
        url = pageUrl;

        
        
        final TextView subtitleView = (TextView) findViewById(R.id.subtitle);
        subtitleView.setText(pageUrl);
        subtitleView.setEllipsize(TextUtils.TruncateAt.MARQUEE);
        subtitleView.setSingleLine(true);
        subtitleView.setMarqueeRepeatLimit(5);
        subtitleView.setSelected(true);

        final View titleView = findViewById(R.id.title);

        if (state == State.DEVICES_ONLY) {
            bookmarkButton.setVisibility(View.GONE);
            readingListButton.setVisibility(View.GONE);

            titleView.setOnClickListener(null);
            subtitleView.setOnClickListener(null);
            return;
        }

        bookmarkButton.setVisibility(View.VISIBLE);
        readingListButton.setVisibility(View.VISIBLE);

        
        final View.OnClickListener launchBrowser = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ShareDialog.this.launchBrowser();
            }
        };

        titleView.setOnClickListener(launchBrowser);
        subtitleView.setOnClickListener(launchBrowser);

        final LocalBrowserDB browserDB = new LocalBrowserDB(getCurrentProfile());
        setButtonState(url, browserDB);
    }

    @Override
    protected void onNewIntent(final Intent intent) {
        super.onNewIntent(intent);

        
        setIntent(intent);
    }

    



    private void setButtonState(final String pageURL, final LocalBrowserDB browserDB) {
        new UIAsyncTask.WithoutParams<Void>(ThreadUtils.getBackgroundHandler()) {
            
            boolean isBookmark;
            boolean isReadingListItem;

            @Override
            protected Void doInBackground() {
                final ContentResolver contentResolver = getApplicationContext().getContentResolver();

                isBookmark = browserDB.isBookmark(contentResolver, pageURL);
                isReadingListItem = browserDB.getReadingListAccessor().isReadingListItem(contentResolver, pageURL);

                return null;
            }

            @Override
            protected void onPostExecute(Void aVoid) {
                findViewById(R.id.overlay_share_bookmark_btn).setEnabled(!isBookmark);
                findViewById(R.id.overlay_share_reading_list_btn).setEnabled(!isReadingListItem);
            }
        }.execute();
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

    





    @Override
    public void onSendTabActionSelected() {
        
        Assert.isTrue(sendTabOverrideIntent != null);

        startActivity(sendTabOverrideIntent);
        finish();
    }

    @Override
    public void onSendTabTargetSelected(String targetGUID) {
        
        Assert.isTrue(targetGUID != null);

        Intent serviceIntent = getServiceIntent(ShareMethod.Type.SEND_TAB);

        
        Bundle extraParameters = new Bundle();

        
        extraParameters.putStringArray(SendTab.SEND_TAB_TARGET_DEVICES, new String[] { targetGUID });

        serviceIntent.putExtra(OverlayConstants.EXTRA_PARAMETERS, extraParameters);

        startService(serviceIntent);
        slideOut();

        Telemetry.sendUIEvent(TelemetryContract.Event.SHARE, TelemetryContract.Method.SHARE_OVERLAY, "sendtab");
    }

    public void addToReadingList() {
        startService(getServiceIntent(ShareMethod.Type.ADD_TO_READING_LIST));
        slideOut();

        Telemetry.sendUIEvent(TelemetryContract.Event.SAVE, TelemetryContract.Method.SHARE_OVERLAY, "reading_list");
    }

    public void addBookmark() {
        startService(getServiceIntent(ShareMethod.Type.ADD_BOOKMARK));
        slideOut();

        Telemetry.sendUIEvent(TelemetryContract.Event.SAVE, TelemetryContract.Method.SHARE_OVERLAY, "bookmark");
    }

    public void launchBrowser() {
        try {
            
            final Intent i = Intent.parseUri(url, Intent.URI_INTENT_SCHEME);
            i.setClassName(AppConstants.ANDROID_PACKAGE_NAME, AppConstants.BROWSER_INTENT_CLASS_NAME);
            startActivity(i);
        } catch (URISyntaxException e) {
            
        } finally {
            slideOut();
        }
    }

    private String getCurrentProfile() {
        return GeckoProfile.DEFAULT_PROFILE;
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
                
                ShareDialog.this.setVisible(false);

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
        Telemetry.sendUIEvent(TelemetryContract.Event.CANCEL, TelemetryContract.Method.SHARE_OVERLAY);
    }

    


    @Override
    public boolean onTouchEvent(MotionEvent event) {
        slideOut();
        Telemetry.sendUIEvent(TelemetryContract.Event.CANCEL, TelemetryContract.Method.SHARE_OVERLAY);
        return true;
    }
}
