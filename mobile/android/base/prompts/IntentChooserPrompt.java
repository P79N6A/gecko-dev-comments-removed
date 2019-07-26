



package org.mozilla.gecko.prompts;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.GeckoActionProvider;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.widget.ListView;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;












public class IntentChooserPrompt {
    private static final String LOGTAG = "GeckoIntentChooser";

    private final ArrayList<PromptListItem> mItems;

    public IntentChooserPrompt(Context context, Intent[] intents) {
        mItems = getItems(context, intents);
    }

    public IntentChooserPrompt(Context context, GeckoActionProvider provider) {
        mItems = getItems(context, provider);
    }

    


    public void show(final String title, final Context context, final IntentHandler handler) {
        ThreadUtils.assertOnUiThread();

        if (mItems.isEmpty()) {
            Log.i(LOGTAG, "No activities for the intent chooser!");
            handler.onCancelled();
            return;
        }

        
        if (mItems.size() == 1) {
            handler.onIntentSelected(mItems.get(0).getIntent(), 0);
            return;
        }

        final Prompt prompt = new Prompt(context, new Prompt.PromptCallback() {
            @Override
            public void onPromptFinished(String promptServiceResult) {
                if (handler == null) {
                    return;
                }

                int itemId = -1;
                try {
                    itemId = new JSONObject(promptServiceResult).getInt("button");
                } catch (JSONException e) {
                    Log.e(LOGTAG, "result from promptservice was invalid: ", e);
                }

                if (itemId == -1) {
                    handler.onCancelled();
                } else {
                    handler.onIntentSelected(mItems.get(itemId).getIntent(), itemId);
                }
            }
        });

        PromptListItem[] arrays = new PromptListItem[mItems.size()];
        mItems.toArray(arrays);
        prompt.show(title, "", arrays, ListView.CHOICE_MODE_NONE);

        return;
    }

    
    public boolean hasActivities(Context context) {
        return mItems.isEmpty();
    }

    
    private ArrayList<PromptListItem> getItems(final Context context, Intent[] intents) {
        final ArrayList<PromptListItem> items = new ArrayList<PromptListItem>();

        
        for (final Intent intent : intents) {
            items.addAll(getItemsForIntent(context, intent));
        }

        return items;
    }

    
    private ArrayList<PromptListItem> getItems(final Context context, final GeckoActionProvider provider) {
        final ArrayList<PromptListItem> items = new ArrayList<PromptListItem>();

        
        final PackageManager packageManager = context.getPackageManager();
        final ArrayList<ResolveInfo> infos = provider.getSortedActivites();

        for (final ResolveInfo info : infos) {
            items.add(getItemForResolveInfo(info, packageManager, provider.getIntent()));
        }

        return items;
    }

    private PromptListItem getItemForResolveInfo(ResolveInfo info, PackageManager pm, Intent intent) {
        PromptListItem item = new PromptListItem(info.loadLabel(pm).toString());
        item.setIcon(info.loadIcon(pm));

        Intent i = new Intent(intent);
        
        i.setComponent(new ComponentName(info.activityInfo.applicationInfo.packageName,
                                         info.activityInfo.name));
        item.setIntent(new Intent(i));

        return item;
    }

    private ArrayList<PromptListItem> getItemsForIntent(Context context, Intent intent) {
        ArrayList<PromptListItem> items = new ArrayList<PromptListItem>();
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> lri = pm.queryIntentActivityOptions(GeckoAppShell.getGeckoInterface().getActivity().getComponentName(), null, intent, 0);

        
        if (lri == null) {
            return items;
        }

        
        for (ResolveInfo ri : lri) {
            items.add(getItemForResolveInfo(ri, pm, intent));
        }

        return items;
    }
}
