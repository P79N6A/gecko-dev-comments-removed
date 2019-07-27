




package org.mozilla.gecko.overlays.service;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import android.view.View;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.overlays.service.sharemethods.AddBookmark;
import org.mozilla.gecko.overlays.service.sharemethods.AddToReadingList;
import org.mozilla.gecko.overlays.service.sharemethods.SendTab;
import org.mozilla.gecko.overlays.service.sharemethods.ShareMethod;
import org.mozilla.gecko.overlays.ui.OverlayToastHelper;
import org.mozilla.gecko.util.ThreadUtils;

import java.util.EnumMap;
import java.util.Map;

import static org.mozilla.gecko.overlays.OverlayConstants.ACTION_PREPARE_SHARE;
import static org.mozilla.gecko.overlays.OverlayConstants.ACTION_SHARE;
















public class OverlayActionService extends Service {
    private static final String LOGTAG = "GeckoOverlayService";

    
    final Map<ShareMethod.Type, ShareMethod> shareTypes = new EnumMap<>(ShareMethod.Type.class);

    
    
    
    
    

    
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent == null) {
            return START_NOT_STICKY;
        }

        
        String action = intent.getAction();

        switch (action) {
            case ACTION_SHARE:
                handleShare(intent);
                break;
            case ACTION_PREPARE_SHARE:
                initShareMethods(getApplicationContext());
                break;
            default:
                throw new IllegalArgumentException("Unsupported intent action: " + action);
        }

        return START_NOT_STICKY;
    }

    


    private void initShareMethods(final Context context) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                shareTypes.clear();

                shareTypes.put(ShareMethod.Type.ADD_BOOKMARK, new AddBookmark(context));
                shareTypes.put(ShareMethod.Type.ADD_TO_READING_LIST, new AddToReadingList(context));
                shareTypes.put(ShareMethod.Type.SEND_TAB, new SendTab(context));
            }
        });
    }

    public void handleShare(final Intent intent) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                ShareData shareData;
                try {
                    shareData = ShareData.fromIntent(intent);
                } catch (IllegalArgumentException e) {
                    Log.e(LOGTAG, "Error parsing share intent: ", e);
                    return;
                }

                ShareMethod shareMethod = shareTypes.get(shareData.shareMethodType);

                final ShareMethod.Result result = shareMethod.handle(shareData);
                
                switch (result) {
                    case SUCCESS:
                        
                        OverlayToastHelper.showSuccessToast(getApplicationContext(), shareMethod.getSuccessMessage());
                        break;
                    case TRANSIENT_FAILURE:
                        
                        View.OnClickListener retryListener = new View.OnClickListener() {
                            @Override
                            public void onClick(View view) {
                                handleShare(intent);
                            }
                        };

                        
                        OverlayToastHelper.showFailureToast(getApplicationContext(), shareMethod.getFailureMessage(), retryListener);
                        break;
                    case PERMANENT_FAILURE:
                        
                        OverlayToastHelper.showFailureToast(getApplicationContext(), shareMethod.getFailureMessage());
                        break;
                    default:
                        Assert.fail("Unknown share method result code: " + result);
                        break;
                }
            }
        });
    }
}
