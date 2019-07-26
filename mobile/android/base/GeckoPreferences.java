




package org.mozilla.gecko;

import org.mozilla.gecko.background.announcements.AnnouncementsConstants;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.GeckoPreferenceFragment;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONArray;
import org.json.JSONObject;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.preference.TwoStatePreference;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

public class GeckoPreferences
    extends PreferenceActivity
    implements OnPreferenceChangeListener, GeckoEventListener, GeckoActivityStatus
{
    private static final String LOGTAG = "GeckoPreferences";

    public static final String INTENT_EXTRA_RESOURCES = "resource";

    private static boolean sIsCharEncodingEnabled = false;
    private boolean mInitialized = false;

    private static final String NON_PREF_PREFIX = "android.not_a_preference.";
    
    private static String PREFS_ANNOUNCEMENTS_ENABLED = NON_PREF_PREFIX + "privacy.announcements.enabled";
    private static String PREFS_DATA_REPORTING_PREFERENCES = NON_PREF_PREFIX + "datareporting.preferences";
    private static String PREFS_TELEMETRY_ENABLED = "datareporting.telemetry.enabled";
    private static String PREFS_CRASHREPORTER_ENABLED = "datareporting.crashreporter.submitEnabled";
    private static String PREFS_HEALTHREPORT_UPLOAD_ENABLED = NON_PREF_PREFIX + "healthreport.uploadEnabled";
    private static String PREFS_MENU_CHAR_ENCODING = "browser.menu.showCharacterEncoding";
    private static String PREFS_MP_ENABLED = "privacy.masterpassword.enabled";
    private static String PREFS_UPDATER_AUTODOWNLOAD = "app.update.autodownload";
    private static String PREFS_TITLEBAR_MODE = "android.not_a_preference.privacy.titlebar";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB &&
            !getIntent().hasExtra(PreferenceActivity.EXTRA_SHOW_FRAGMENT)) {
            setupTopLevelFragmentIntent();
        }

        super.onCreate(savedInstanceState);

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
            Bundle intentExtras = getIntent().getExtras();
            if (intentExtras != null && intentExtras.containsKey(INTENT_EXTRA_RESOURCES)) {
                String resourceName = intentExtras.getString(INTENT_EXTRA_RESOURCES);
                int resource = getResources().getIdentifier(resourceName, "xml", getPackageName());
                addPreferencesFromResource(resource);
            } else {
                addPreferencesFromResource(R.xml.preferences_nonfragment);
            }
        }

        registerEventListener("Sanitize:Finished");

        if (Build.VERSION.SDK_INT >= 14)
            getActionBar().setHomeButtonEnabled(true);
    }

    


    private void setupTopLevelFragmentIntent() {
        Intent intent = getIntent();
        
        Bundle intentExtras = intent.getExtras();
        Bundle fragmentArgs = new Bundle();
        
        if (intentExtras != null && intentExtras.containsKey(INTENT_EXTRA_RESOURCES)) {
            String resource = intentExtras.getString(INTENT_EXTRA_RESOURCES);
            fragmentArgs.putString(INTENT_EXTRA_RESOURCES, resource);
        } else {
            
            if (!onIsMultiPane()) {
                fragmentArgs.putString(INTENT_EXTRA_RESOURCES, "preferences_main");
            } else {
                fragmentArgs.putString(INTENT_EXTRA_RESOURCES, "preferences_general");
            }
        }

        
        intent.putExtra(PreferenceActivity.EXTRA_SHOW_FRAGMENT, GeckoPreferenceFragment.class.getName());
        intent.putExtra(PreferenceActivity.EXTRA_SHOW_FRAGMENT_ARGUMENTS, fragmentArgs);
    }

    @Override
    public void onBuildHeaders(List<Header> target) {
        if (onIsMultiPane())
            loadHeadersFromResource(R.xml.preference_headers, target);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (!hasFocus || mInitialized)
            return;

        mInitialized = true;
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
            PreferenceScreen screen = getPreferenceScreen();
            setupPreferences(screen);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterEventListener("Sanitize:Finished");
    }

    @Override
    public void onPause() {
        super.onPause();

        if (getApplication() instanceof GeckoApplication) {
            ((GeckoApplication) getApplication()).onActivityPause(this);
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        if (getApplication() instanceof GeckoApplication) {
            ((GeckoApplication) getApplication()).onActivityResume(this);
        }
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Sanitize:Finished")) {
                boolean success = message.getBoolean("success");
                final int stringRes = success ? R.string.private_data_success : R.string.private_data_fail;
                final Context context = this;
                ThreadUtils.postToUiThread(new Runnable () {
                    @Override
                    public void run() {
                        Toast.makeText(context, stringRes, Toast.LENGTH_SHORT).show();
                    }
                });
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    public void setupPreferences(PreferenceGroup prefs) {
        ArrayList<String> list = new ArrayList<String>();
        setupPreferences(prefs, list);
        getGeckoPreferences(prefs, list);
    }

    private void setupPreferences(PreferenceGroup preferences, ArrayList<String> prefs) {
        for (int i = 0; i < preferences.getPreferenceCount(); i++) {
            Preference pref = preferences.getPreference(i);
            String key = pref.getKey();
            if (pref instanceof PreferenceGroup) {
                
                if (PREFS_DATA_REPORTING_PREFERENCES.equals(key)) {
                    if (!AppConstants.MOZ_DATA_REPORTING) {
                        preferences.removePreference(pref);
                        i--;
                        continue;
                    }
                }
                setupPreferences((PreferenceGroup) pref, prefs);
            } else {
                pref.setOnPreferenceChangeListener(this);
                if (PREFS_UPDATER_AUTODOWNLOAD.equals(key) && !AppConstants.MOZ_UPDATER) {
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (PREFS_TELEMETRY_ENABLED.equals(key) && !AppConstants.MOZ_TELEMETRY_REPORTING) {
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (PREFS_HEALTHREPORT_UPLOAD_ENABLED.equals(key) && !AppConstants.MOZ_SERVICES_HEALTHREPORT) {
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (PREFS_CRASHREPORTER_ENABLED.equals(key) && !AppConstants.MOZ_CRASHREPORTER) {
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (PREFS_TITLEBAR_MODE.equals(key)) {
                    setupTitlebarPref((ListPreference) pref);
                }

                
                
                
                
                
                
                if (key != null && !key.startsWith(NON_PREF_PREFIX)) {
                    prefs.add(key);
                }
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }

        return super.onOptionsItemSelected(item);
    }

    final private int DIALOG_CREATE_MASTER_PASSWORD = 0;
    final private int DIALOG_REMOVE_MASTER_PASSWORD = 1;

    public static void setCharEncodingState(boolean enabled) {
        sIsCharEncodingEnabled = enabled;
    }

    public static boolean getCharEncodingState() {
        return sIsCharEncodingEnabled;
    }

    




    public static void broadcastPrefAction(final Context context,
                                           final String action,
                                           final String pref,
                                           final boolean value) {
        final Intent intent = new Intent(action);
        intent.setAction(action);
        intent.putExtra("pref", pref);
        intent.putExtra("branch", GeckoApp.PREFS_NAME);
        intent.putExtra("enabled", value);
        Log.d(LOGTAG, "Broadcast: " + action + ", " + pref + ", " + GeckoApp.PREFS_NAME + ", " + value);
        context.sendBroadcast(intent);
    }

    



    public static void broadcastAnnouncementsPref(final Context context, final boolean value) {
        broadcastPrefAction(context,
                            AnnouncementsConstants.ACTION_ANNOUNCEMENTS_PREF,
                            PREFS_ANNOUNCEMENTS_ENABLED,
                            value);
    }

    



    public static void broadcastAnnouncementsPref(final Context context) {
        final boolean value = getBooleanPref(context, PREFS_ANNOUNCEMENTS_ENABLED, true);
        broadcastAnnouncementsPref(context, value);
    }

    











    public static boolean getBooleanPref(final Context context, final String name, boolean def) {
        final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        return prefs.getBoolean(name, def);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String prefName = preference.getKey();
        if (PREFS_MP_ENABLED.equals(prefName)) {
            showDialog((Boolean) newValue ? DIALOG_CREATE_MASTER_PASSWORD : DIALOG_REMOVE_MASTER_PASSWORD);
            return false;
        } else if (PREFS_MENU_CHAR_ENCODING.equals(prefName)) {
            setCharEncodingState(((String) newValue).equals("true"));
        } else if (PREFS_ANNOUNCEMENTS_ENABLED.equals(prefName)) {
            
            
            broadcastAnnouncementsPref(GeckoApp.mAppContext, ((Boolean) newValue).booleanValue());
        } else if (PREFS_UPDATER_AUTODOWNLOAD.equals(prefName)) {
            org.mozilla.gecko.updater.UpdateServiceHelper.registerForUpdates(GeckoApp.mAppContext, (String) newValue);
        } else if (PREFS_HEALTHREPORT_UPLOAD_ENABLED.equals(prefName)) {
            
            return true;
        }

        if (!TextUtils.isEmpty(prefName)) {
            PrefsHelper.setPref(prefName, newValue);
        }
        if (preference instanceof ListPreference) {
            
            int newIndex = ((ListPreference) preference).findIndexOfValue((String) newValue);
            CharSequence newEntry = ((ListPreference) preference).getEntries()[newIndex];
            ((ListPreference) preference).setSummary(newEntry);
        } else if (preference instanceof LinkPreference) {
            finish();
        } else if (preference instanceof FontSizePreference) {
            final FontSizePreference fontSizePref = (FontSizePreference) preference;
            fontSizePref.setSummary(fontSizePref.getSavedFontSizeName());
        }
        return true;
    }

    private EditText getTextBox(int aHintText) {
        EditText input = new EditText(GeckoApp.mAppContext);
        int inputtype = InputType.TYPE_CLASS_TEXT;
        inputtype |= InputType.TYPE_TEXT_VARIATION_PASSWORD | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
        input.setInputType(inputtype);

        String hint = getResources().getString(aHintText);
        input.setHint(aHintText);
        return input;
    }

    private class PasswordTextWatcher implements TextWatcher {
        EditText input1 = null;
        EditText input2 = null;
        AlertDialog dialog = null;

        PasswordTextWatcher(EditText aInput1, EditText aInput2, AlertDialog aDialog) {
            input1 = aInput1;
            input2 = aInput2;
            dialog = aDialog;
        }

        @Override
        public void afterTextChanged(Editable s) {
            if (dialog == null)
                return;

            String text1 = input1.getText().toString();
            String text2 = input2.getText().toString();
            boolean disabled = TextUtils.isEmpty(text1) || TextUtils.isEmpty(text2) || !text1.equals(text2);
            dialog.getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(!disabled);
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) { }
    }

    private class EmptyTextWatcher implements TextWatcher {
        EditText input = null;
        AlertDialog dialog = null;

        EmptyTextWatcher(EditText aInput, AlertDialog aDialog) {
            input = aInput;
            dialog = aDialog;
        }

        @Override
        public void afterTextChanged(Editable s) {
            if (dialog == null)
                return;

            String text = input.getText().toString();
            boolean disabled = TextUtils.isEmpty(text);
            dialog.getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(!disabled);
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) { }
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LinearLayout linearLayout = new LinearLayout(this);
        linearLayout.setOrientation(LinearLayout.VERTICAL);
        AlertDialog dialog = null;
        switch(id) {
            case DIALOG_CREATE_MASTER_PASSWORD:
                final EditText input1 = getTextBox(R.string.masterpassword_password);
                final EditText input2 = getTextBox(R.string.masterpassword_confirm);
                linearLayout.addView(input1);
                linearLayout.addView(input2);

                builder.setTitle(R.string.masterpassword_create_title)
                       .setView((View) linearLayout)
                       .setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener() {  
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                JSONObject jsonPref = new JSONObject();
                                try {
                                    jsonPref.put("name", PREFS_MP_ENABLED);
                                    jsonPref.put("type", "string");
                                    jsonPref.put("value", input1.getText().toString());
                    
                                    GeckoEvent event = GeckoEvent.createBroadcastEvent("Preferences:Set", jsonPref.toString());
                                    GeckoAppShell.sendEventToGecko(event);
                                } catch(Exception ex) {
                                    Log.e(LOGTAG, "Error setting master password", ex);
                                }
                                return;
                            }
                        })
                        .setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener() {  
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                return;
                            }
                        });
                        dialog = builder.create();
                        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
                            @Override
                            public void onShow(DialogInterface dialog) {
                                input1.setText("");
                                input2.setText("");
                                input1.requestFocus();
                            }
                        });

                        PasswordTextWatcher watcher = new PasswordTextWatcher(input1, input2, dialog);
                        input1.addTextChangedListener((TextWatcher) watcher);
                        input2.addTextChangedListener((TextWatcher) watcher);

                break;
            case DIALOG_REMOVE_MASTER_PASSWORD:
                final EditText input = getTextBox(R.string.masterpassword_password);
                linearLayout.addView(input);

                builder.setTitle(R.string.masterpassword_remove_title)
                       .setView((View) linearLayout)
                       .setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener() {  
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                PrefsHelper.setPref(PREFS_MP_ENABLED, input.getText().toString());
                            }
                        })
                        .setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener() {  
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                return;
                            }
                        });
                        dialog = builder.create();
                        dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                            @Override
                            public void onDismiss(DialogInterface dialog) {
                                input.setText("");
                            }
                        });
                        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
                            @Override
                            public void onShow(DialogInterface dialog) {
                                input.setText("");
                            }
                        });
                        input.addTextChangedListener(new EmptyTextWatcher(input, dialog));
                break;
            default:
                return null;
        }

        return dialog;
    }

    
    private void getGeckoPreferences(final PreferenceGroup screen, ArrayList<String> prefs) {
        JSONArray jsonPrefs = new JSONArray(prefs);

        PrefsHelper.getPrefs(jsonPrefs, new PrefsHelper.PrefHandlerBase() {
            private Preference getField(String prefName) {
                return screen.findPreference(prefName);
            }

            
            class CheckBoxPrefSetter {
                public void setBooleanPref(Preference preference, boolean value) {
                    if ((preference instanceof CheckBoxPreference) &&
                       ((CheckBoxPreference) preference).isChecked() != value) {
                        ((CheckBoxPreference) preference).setChecked(value);
                    }
                }
            }

            class TwoStatePrefSetter extends CheckBoxPrefSetter {
                @Override
                public void setBooleanPref(Preference preference, boolean value) {
                    if ((preference instanceof TwoStatePreference) &&
                       ((TwoStatePreference) preference).isChecked() != value) {
                        ((TwoStatePreference) preference).setChecked(value);
                    }
                }
            }

            @Override
            public void prefValue(String prefName, final boolean value) {
                final Preference pref = getField(prefName);
                final CheckBoxPrefSetter prefSetter;
                if (Build.VERSION.SDK_INT < 14) {
                    prefSetter = new CheckBoxPrefSetter();
                } else {
                    prefSetter = new TwoStatePrefSetter();
                }
                ThreadUtils.postToUiThread(new Runnable() {
                    public void run() {
                        prefSetter.setBooleanPref(pref, value);
                    }
                });
            }

            @Override
            public void prefValue(String prefName, final String value) {
                final Preference pref = getField(prefName);
                if (pref instanceof EditTextPreference) {
                    ThreadUtils.postToUiThread(new Runnable() {
                        @Override
                        public void run() {
                            ((EditTextPreference) pref).setText(value);
                        }
                    });
                } else if (pref instanceof ListPreference) {
                    ThreadUtils.postToUiThread(new Runnable() {
                        @Override
                        public void run() {
                            ((ListPreference) pref).setValue(value);
                            
                            CharSequence selectedEntry = ((ListPreference) pref).getEntry();
                            ((ListPreference) pref).setSummary(selectedEntry);
                        }
                    });
                } else if (pref instanceof FontSizePreference) {
                    final FontSizePreference fontSizePref = (FontSizePreference) pref;
                    fontSizePref.setSavedFontSize(value);
                    final String fontSizeName = fontSizePref.getSavedFontSizeName();
                    ThreadUtils.postToUiThread(new Runnable() {
                        @Override
                        public void run() {
                            fontSizePref.setSummary(fontSizeName); 
                        }
                    });
                }
            }

            @Override
            public void finish() {
                
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        screen.setEnabled(true);
                    }
                });
            }
        });
    }

    private void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, this);
    }

    private void unregisterEventListener(String event) {
        GeckoAppShell.getEventDispatcher().unregisterEventListener(event, this);
    }

    @Override
    public boolean isGeckoActivityOpened() {
        return false;
    }

    private void setupTitlebarPref(final ListPreference pref) {
        final SharedPreferences settings = getSharedPreferences(BrowserToolbar.PREFS_NAME, 0);
        boolean value = settings.getBoolean(BrowserToolbar.PREFS_SHOW_URL, false);

        final String[] entries = new String[] {
            getResources().getString(R.string.pref_titlebar_mode_url),
            getResources().getString(R.string.pref_titlebar_mode_title)
        };
        pref.setEntries(entries);
        pref.setEntryValues(entries);
        pref.setValueIndex(value ? 0 : 1);
        pref.setSummary(value ? entries[0] : entries[1]);

        pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, final Object newValue) {
                ThreadUtils.postToBackgroundThread(new Runnable() {
                    @Override
                    public void run() {
                        settings.edit()
                                .putBoolean(BrowserToolbar.PREFS_SHOW_URL, newValue.toString().equals(entries[0]))
                                .commit();
                    }
                });
                pref.setSummary(newValue.toString());
                return true;
            }
        });
    }
}
