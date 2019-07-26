



package org.mozilla.gecko;

import org.mozilla.gecko.prompts.Prompt;
import org.mozilla.gecko.prompts.PromptService;
import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.util.ActivityResultHandlerMap;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Environment;
import android.os.Parcelable;
import android.provider.MediaStore;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.TimeUnit;

public class ActivityHandlerHelper implements GeckoEventListener {
    private static final String LOGTAG = "GeckoActivityHandlerHelper";


    private final ActivityResultHandlerMap mActivityResultHandlerMap;
    public interface ResultHandler {
        public void gotFile(String filename);
    }

    @SuppressWarnings("serial")
    public ActivityHandlerHelper() {
        mActivityResultHandlerMap = new ActivityResultHandlerMap();
        GeckoAppShell.getEventDispatcher().registerEventListener("FilePicker:Show", this);
    }

    @Override
    public void handleMessage(String event, final JSONObject message) {
        if (event.equals("FilePicker:Show")) {
            String mimeType = "*/*";
            String mode = message.optString("mode");

            if ("mimeType".equals(mode))
                mimeType = message.optString("mimeType");
            else if ("extension".equals(mode))
                mimeType = GeckoAppShell.getMimeTypeFromExtensions(message.optString("extensions"));

            showFilePickerAsync(GeckoAppShell.getGeckoInterface().getActivity(), mimeType, new ResultHandler() {
                public void gotFile(String filename) {
                    try {
                        message.put("file", filename);
                    } catch (JSONException ex) {
                        Log.i(LOGTAG, "Can't add filename to message " + filename);
                    }
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(
                        "FilePicker:Result", message.toString()));
                }
            });
        }
    }

    public int makeRequestCode(ActivityResultHandler aHandler) {
        return mActivityResultHandlerMap.put(aHandler);
    }

    public void startIntentForActivity (Activity activity, Intent intent, ActivityResultHandler activityResultHandler) {
        activity.startActivityForResult(intent, mActivityResultHandlerMap.put(activityResultHandler));
    }

    private void addActivities(Context context, Intent intent, HashMap<String, Intent> intents, HashMap<String, Intent> filters) {
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> lri = pm.queryIntentActivityOptions(GeckoAppShell.getGeckoInterface().getActivity().getComponentName(), null, intent, 0);
        for (ResolveInfo ri : lri) {
            ComponentName cn = new ComponentName(ri.activityInfo.applicationInfo.packageName, ri.activityInfo.name);
            if (filters != null && !filters.containsKey(cn.toString())) {
                Intent rintent = new Intent(intent);
                rintent.setComponent(cn);
                intents.put(cn.toString(), rintent);
            }
        }
    }

    private Intent getIntent(Context context, String mimeType) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType(mimeType);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        return intent;
    }

    private List<Intent> getIntentsForFilePicker(final Context context,
                                                       final String mimeType,
                                                       final FilePickerResultHandler fileHandler) {
        
        
        Intent baseIntent;
        
        
        HashMap<String, Intent> baseIntents = new HashMap<String, Intent>();
        
        HashMap<String, Intent> intents = new HashMap<String, Intent> ();

        if ("audio/*".equals(mimeType)) {
            
            baseIntent = getIntent(context, mimeType);
            addActivities(context, baseIntent, baseIntents, null);
        } else if ("image/*".equals(mimeType)) {
            
            baseIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            baseIntent.putExtra(MediaStore.EXTRA_OUTPUT,
                            Uri.fromFile(new File(Environment.getExternalStorageDirectory(),
                                                  fileHandler.generateImageName())));
            addActivities(context, baseIntent, baseIntents, null);

            
            addActivities(context, getIntent(context, mimeType), intents, baseIntents);
        } else if ("video/*".equals(mimeType)) {
            
            baseIntent = new Intent(MediaStore.ACTION_VIDEO_CAPTURE);
            addActivities(context, baseIntent, baseIntents, null);

            
            addActivities(context, getIntent(context, mimeType), intents, baseIntents);
        } else {
            
            baseIntent = getIntent(context, "*/*");
            addActivities(context, baseIntent, baseIntents, null);

            
            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            intent.putExtra(MediaStore.EXTRA_OUTPUT,
                            Uri.fromFile(new File(Environment.getExternalStorageDirectory(),
                                                  fileHandler.generateImageName())));
            addActivities(context, intent, intents, baseIntents);

            intent = new Intent(MediaStore.ACTION_VIDEO_CAPTURE);
            addActivities(context, intent, intents, baseIntents);
        }

        
        if (baseIntents.size() == 0 && intents.size() == 0) {
            intents.clear();

            baseIntent = getIntent(context, "*/*");
            addActivities(context, baseIntent, baseIntents, null);
        }

        ArrayList<Intent> vals = new ArrayList<Intent>(intents.values());
        vals.add(0, baseIntent);
        return vals;
    }

    private String getFilePickerTitle(Context context, String aMimeType) {
        if (aMimeType.equals("audio/*")) {
            return context.getString(R.string.filepicker_audio_title);
        } else if (aMimeType.equals("image/*")) {
            return context.getString(R.string.filepicker_image_title);
        } else if (aMimeType.equals("video/*")) {
            return context.getString(R.string.filepicker_video_title);
        } else {
            return context.getString(R.string.filepicker_title);
        }
    }

    private interface IntentHandler {
        public void gotIntent(Intent intent);
    }

    




    private void getFilePickerIntentAsync(final Context context,
                                          final String mimeType,
                                          final FilePickerResultHandler fileHandler,
                                          final IntentHandler handler) {
        List<Intent> intents = getIntentsForFilePicker(context, mimeType, fileHandler);

        if (intents.size() == 0) {
            Log.i(LOGTAG, "no activities for the file picker!");
            handler.gotIntent(null);
            return;
        }

        Intent base = intents.remove(0);
        if (intents.size() == 0) {
            handler.gotIntent(base);
            return;
        }

        Intent chooser = Intent.createChooser(base, getFilePickerTitle(context, mimeType));
        chooser.putExtra(Intent.EXTRA_INITIAL_INTENTS, intents.toArray(new Parcelable[]{}));
        handler.gotIntent(chooser);
    }

    



    public void showFilePickerAsync(final Activity parentActivity, String aMimeType, final ResultHandler handler) {
        final FilePickerResultHandler fileHandler = new FilePickerResultHandler(handler);
        getFilePickerIntentAsync(parentActivity, aMimeType, fileHandler, new IntentHandler() {
            @Override
            public void gotIntent(Intent intent) {
                if (handler == null) {
                    return;
                }

                if (intent == null) {
                    handler.gotFile("");
                    return;
                }

                parentActivity.startActivityForResult(intent, mActivityResultHandlerMap.put(fileHandler));
            }
        });
    }

    boolean handleActivityResult(int requestCode, int resultCode, Intent data) {
        ActivityResultHandler handler = mActivityResultHandlerMap.getAndRemove(requestCode);
        if (handler != null) {
            handler.onActivityResult(resultCode, data);
            return true;
        }
        return false;
    }
}
