



package org.mozilla.gecko;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Environment;
import android.os.Parcelable;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class FilePicker implements GeckoEventListener {
    private static final String LOGTAG = "GeckoFilePicker";
    private static FilePicker sFilePicker;
    private final Context context;

    public interface ResultHandler {
        public void gotFile(String filename);
    }

    public static void init(Context context) {
        if (sFilePicker == null) {
            sFilePicker = new FilePicker(context.getApplicationContext());
        }
    }

    protected FilePicker(Context context) {
        this.context = context;
        EventDispatcher.getInstance().registerGeckoThreadListener(this, "FilePicker:Show");
    }

    @Override
    public void handleMessage(String event, final JSONObject message) {
        if (event.equals("FilePicker:Show")) {
            String mimeType = "*/*";
            final String mode = message.optString("mode");
            final int tabId = message.optInt("tabId", -1);
            final String title = message.optString("title");

            if ("mimeType".equals(mode))
                mimeType = message.optString("mimeType");
            else if ("extension".equals(mode))
                mimeType = GeckoAppShell.getMimeTypeFromExtensions(message.optString("extensions"));

            showFilePickerAsync(title, mimeType, new ResultHandler() {
                public void gotFile(String filename) {
                    try {
                        message.put("file", filename);
                    } catch (JSONException ex) {
                        Log.i(LOGTAG, "Can't add filename to message " + filename);
                    }


                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(
                        "FilePicker:Result", message.toString()));
                }
            }, tabId);
        }
    }

    private void addActivities(Intent intent, HashMap<String, Intent> intents, HashMap<String, Intent> filters) {
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> lri = pm.queryIntentActivities(intent, 0);
        for (ResolveInfo ri : lri) {
            ComponentName cn = new ComponentName(ri.activityInfo.applicationInfo.packageName, ri.activityInfo.name);
            if (filters != null && !filters.containsKey(cn.toString())) {
                Intent rintent = new Intent(intent);
                rintent.setComponent(cn);
                intents.put(cn.toString(), rintent);
            }
        }
    }

    private Intent getIntent(String mimeType) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType(mimeType);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        return intent;
    }

    private List<Intent> getIntentsForFilePicker(final String mimeType,
                                                       final FilePickerResultHandler fileHandler) {
        
        
        Intent baseIntent;
        
        
        HashMap<String, Intent> baseIntents = new HashMap<String, Intent>();
        
        HashMap<String, Intent> intents = new HashMap<String, Intent> ();

        if ("audio/*".equals(mimeType)) {
            
            baseIntent = getIntent(mimeType);
            addActivities(baseIntent, baseIntents, null);
        } else if ("image/*".equals(mimeType)) {
            
            baseIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            baseIntent.putExtra(MediaStore.EXTRA_OUTPUT,
                            Uri.fromFile(new File(Environment.getExternalStorageDirectory(),
                                                  fileHandler.generateImageName())));
            addActivities(baseIntent, baseIntents, null);

            
            addActivities(getIntent(mimeType), intents, baseIntents);
        } else if ("video/*".equals(mimeType)) {
            
            baseIntent = new Intent(MediaStore.ACTION_VIDEO_CAPTURE);
            addActivities(baseIntent, baseIntents, null);

            
            addActivities(getIntent(mimeType), intents, baseIntents);
        } else {
            baseIntent = getIntent("*/*");
            addActivities(baseIntent, baseIntents, null);

            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            intent.putExtra(MediaStore.EXTRA_OUTPUT,
                            Uri.fromFile(new File(Environment.getExternalStorageDirectory(),
                                                  fileHandler.generateImageName())));
            addActivities(intent, intents, baseIntents);
            intent = new Intent(MediaStore.ACTION_VIDEO_CAPTURE);
            addActivities(intent, intents, baseIntents);
        }

        
        if (baseIntents.size() == 0 && intents.size() == 0) {
            intents.clear();

            baseIntent = getIntent("*/*");
            addActivities(baseIntent, baseIntents, null);
        }

        ArrayList<Intent> vals = new ArrayList<Intent>(intents.values());
        vals.add(0, baseIntent);
        return vals;
    }

    private String getFilePickerTitle(String mimeType) {
        if (mimeType.equals("audio/*")) {
            return context.getString(R.string.filepicker_audio_title);
        } else if (mimeType.equals("image/*")) {
            return context.getString(R.string.filepicker_image_title);
        } else if (mimeType.equals("video/*")) {
            return context.getString(R.string.filepicker_video_title);
        } else {
            return context.getString(R.string.filepicker_title);
        }
    }

    private interface IntentHandler {
        public void gotIntent(Intent intent);
    }

    




    private void getFilePickerIntentAsync(String title,
                                          final String mimeType,
                                          final FilePickerResultHandler fileHandler,
                                          final IntentHandler handler) {
        List<Intent> intents = getIntentsForFilePicker(mimeType, fileHandler);

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

        if (TextUtils.isEmpty(title)) {
            title = getFilePickerTitle(mimeType);
        }
        Intent chooser = Intent.createChooser(base, title);
        chooser.putExtra(Intent.EXTRA_INITIAL_INTENTS, intents.toArray(new Parcelable[]{}));
        handler.gotIntent(chooser);
    }

    



    protected void showFilePickerAsync(final String title, final String mimeType, final ResultHandler handler, final int tabId) {
        final FilePickerResultHandler fileHandler = new FilePickerResultHandler(handler, context, tabId);
        getFilePickerIntentAsync(title, mimeType, fileHandler, new IntentHandler() {
            @Override
            public void gotIntent(Intent intent) {
                if (handler == null) {
                    return;
                }

                if (intent == null) {
                    handler.gotFile("");
                    return;
                }

                ActivityHandlerHelper.startIntent(intent, fileHandler);
            }
        });
    }
}
