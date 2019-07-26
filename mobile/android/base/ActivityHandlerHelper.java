



package org.mozilla.gecko;

import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.util.ActivityResultHandlerMap;
import org.mozilla.gecko.util.ThreadUtils;

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
import android.provider.MediaStore;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.TimeUnit;

public class ActivityHandlerHelper {
    private static final String LOGTAG = "GeckoActivityHandlerHelper";

    private final ConcurrentLinkedQueue<String> mFilePickerResult;

    private final ActivityResultHandlerMap mActivityResultHandlerMap;
    private final FilePickerResultHandlerSync mFilePickerResultHandlerSync;
    private final AwesomebarResultHandler mAwesomebarResultHandler;
    private final CameraImageResultHandler mCameraImageResultHandler;
    private final CameraVideoResultHandler mCameraVideoResultHandler;

    @SuppressWarnings("serial")
    public ActivityHandlerHelper() {
        mFilePickerResult = new ConcurrentLinkedQueue<String>() {
            @Override public boolean offer(String e) {
                if (super.offer(e)) {
                    
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createNoOpEvent());
                    return true;
                }
                return false;
            }
        };
        mActivityResultHandlerMap = new ActivityResultHandlerMap();
        mFilePickerResultHandlerSync = new FilePickerResultHandlerSync(mFilePickerResult);
        mAwesomebarResultHandler = new AwesomebarResultHandler();
        mCameraImageResultHandler = new CameraImageResultHandler(mFilePickerResult);
        mCameraVideoResultHandler = new CameraVideoResultHandler(mFilePickerResult);
    }

    public int makeRequestCodeForAwesomebar() {
        return mActivityResultHandlerMap.put(mAwesomebarResultHandler);
    }

    public int makeRequestCode(ActivityResultHandler aHandler) {
        return mActivityResultHandlerMap.put(aHandler);
    }

    private int addIntentActivitiesToList(Context context, Intent intent, ArrayList<PromptService.PromptListItem> items, ArrayList<Intent> aIntents) {
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> lri = pm.queryIntentActivityOptions(GeckoApp.mAppContext.getComponentName(), null, intent, 0);

        if (lri == null) {
            return 0;
        }

        for (ResolveInfo ri : lri) {
            Intent rintent = new Intent(intent);
            rintent.setComponent(new ComponentName(
                    ri.activityInfo.applicationInfo.packageName,
                    ri.activityInfo.name));

            PromptService.PromptListItem item = new PromptService.PromptListItem(ri.loadLabel(pm).toString());
            item.icon = ri.loadIcon(pm);
            items.add(item);
            aIntents.add(rintent);
        }

        return lri.size();
    }

    private int addFilePickingActivities(Context context, ArrayList<PromptService.PromptListItem> aItems, String aType, ArrayList<Intent> aIntents) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType(aType);
        intent.addCategory(Intent.CATEGORY_OPENABLE);

        return addIntentActivitiesToList(context, intent, aItems, aIntents);
    }

    private PromptService.PromptListItem[] getItemsAndIntentsForFilePicker(Context context, String aMimeType, ArrayList<Intent> aIntents) {
        ArrayList<PromptService.PromptListItem> items = new ArrayList<PromptService.PromptListItem>();

        if (aMimeType.equals("audio/*")) {
            if (addFilePickingActivities(context, items, "audio/*", aIntents) <= 0) {
                addFilePickingActivities(context, items, "*/*", aIntents);
            }
        } else if (aMimeType.equals("image/*")) {
            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            intent.putExtra(MediaStore.EXTRA_OUTPUT,
                            Uri.fromFile(new File(Environment.getExternalStorageDirectory(),
                                                  CameraImageResultHandler.generateImageName())));
            addIntentActivitiesToList(context, intent, items, aIntents);

            if (addFilePickingActivities(context, items, "image/*", aIntents) <= 0) {
                addFilePickingActivities(context, items, "*/*", aIntents);
            }
        } else if (aMimeType.equals("video/*")) {
            Intent intent = new Intent(MediaStore.ACTION_VIDEO_CAPTURE);
            addIntentActivitiesToList(context, intent, items, aIntents);

            if (addFilePickingActivities(context, items, "video/*", aIntents) <= 0) {
                addFilePickingActivities(context, items, "*/*", aIntents);
            }
        } else {
            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            intent.putExtra(MediaStore.EXTRA_OUTPUT,
                            Uri.fromFile(new File(Environment.getExternalStorageDirectory(),
                                                  CameraImageResultHandler.generateImageName())));
            addIntentActivitiesToList(context, intent, items, aIntents);

            intent = new Intent(MediaStore.ACTION_VIDEO_CAPTURE);
            addIntentActivitiesToList(context, intent, items, aIntents);

            addFilePickingActivities(context, items, "*/*", aIntents);
        }

        return items.toArray(new PromptService.PromptListItem[] {});
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

    private Intent getFilePickerIntent(Context context, String aMimeType) {
        ArrayList<Intent> intents = new ArrayList<Intent>();
        final PromptService.PromptListItem[] items =
            getItemsAndIntentsForFilePicker(context, aMimeType, intents);

        if (intents.size() == 0) {
            Log.i(LOGTAG, "no activities for the file picker!");
            return null;
        }

        if (intents.size() == 1) {
            return intents.get(0);
        }

        final PromptService ps = GeckoApp.mAppContext.getPromptService();
        final String title = getFilePickerTitle(context, aMimeType);

        
        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override public void run() {
                ps.show(title, "", items, false);
            }
        });

        String promptServiceResult = ps.getResponse();
        int itemId = -1;
        try {
            itemId = new JSONObject(promptServiceResult).getInt("button");

            if (itemId == -1) {
                return null;
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "result from promptservice was invalid: ", e);
            return null;
        }

        return intents.get(itemId);
    }

    boolean showFilePicker(Activity parentActivity, String aMimeType, ActivityResultHandler handler) {
        Intent intent = getFilePickerIntent(parentActivity, aMimeType);

        if (intent == null) {
            return false;
        }
        parentActivity.startActivityForResult(intent, mActivityResultHandlerMap.put(handler));
        return true;
    }

    String showFilePicker(Activity parentActivity, String aMimeType) {
        Intent intent = getFilePickerIntent(parentActivity, aMimeType);

        if (intent == null) {
            return "";
        }

        if (intent.getAction().equals(MediaStore.ACTION_IMAGE_CAPTURE)) {
            parentActivity.startActivityForResult(intent, mActivityResultHandlerMap.put(mCameraImageResultHandler));
        } else if (intent.getAction().equals(MediaStore.ACTION_VIDEO_CAPTURE)) {
            parentActivity.startActivityForResult(intent, mActivityResultHandlerMap.put(mCameraVideoResultHandler));
        } else if (intent.getAction().equals(Intent.ACTION_GET_CONTENT)) {
            parentActivity.startActivityForResult(intent, mActivityResultHandlerMap.put(mFilePickerResultHandlerSync));
        } else {
            Log.e(LOGTAG, "We should not get an intent with another action!");
            return "";
        }

        String filePickerResult;
        while (null == (filePickerResult = mFilePickerResult.poll())) {
            GeckoAppShell.processNextNativeEvent(true);
        }
        return filePickerResult;
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
