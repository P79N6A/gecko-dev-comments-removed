



package org.mozilla.gecko.preferences;

import java.text.Collator;
import java.util.Arrays;
import java.util.Collection;
import java.util.Locale;

import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.R;

import android.content.Context;
import android.preference.ListPreference;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;

public class LocaleListPreference extends ListPreference {
    private static final String LOG_TAG = "GeckoLocaleList";

    private volatile Locale entriesLocale;

    public LocaleListPreference(Context context) {
        this(context, null);
    }

    public LocaleListPreference(Context context, AttributeSet attributes) {
        super(context, attributes);
        buildList();
    }

    private static final class LocaleDescriptor implements Comparable<LocaleDescriptor> {
        
        private static final Collator COLLATOR = Collator.getInstance(Locale.US);

        public final String tag;
        private final String nativeName;

        public LocaleDescriptor(String tag) {
            this(BrowserLocaleManager.parseLocaleCode(tag), tag);
        }

        public LocaleDescriptor(Locale locale, String tag) {
            this.tag = tag;

            final String displayName = locale.getDisplayName(locale);
            if (TextUtils.isEmpty(displayName)) {
                
                Log.w(LOG_TAG, "Display name is empty. Using " + locale.toString());
                this.nativeName = locale.toString();
                return;
            }

            
            
            
            final byte directionality = Character.getDirectionality(displayName.charAt(0));
            if (directionality == Character.DIRECTIONALITY_LEFT_TO_RIGHT) {
                this.nativeName = displayName.substring(0, 1).toUpperCase(locale) +
                                  displayName.substring(1);
                return;
            }

            this.nativeName = displayName;
        }

        public String getTag() {
            return this.tag;
        }

        public String getDisplayName() {
            return this.nativeName;
        }

        @Override
        public String toString() {
            return this.nativeName;
        }


        @Override
        public int compareTo(LocaleDescriptor another) {
            
            return COLLATOR.compare(this.nativeName, another.nativeName);
        }
    }

    private LocaleDescriptor[] getShippingLocales() {
        Collection<String> shippingLocales = BrowserLocaleManager.getPackagedLocaleTags(getContext());

        
        if (shippingLocales == null) {
            final String fallbackTag = BrowserLocaleManager.getFallbackLocaleTag();
            return new LocaleDescriptor[] { new LocaleDescriptor(fallbackTag) };
        }

        final int count = shippingLocales.size();
        final LocaleDescriptor[] descriptors = new LocaleDescriptor[count];

        int i = 0;
        for (String tag : shippingLocales) {
            descriptors[i++] = new LocaleDescriptor(tag);
        }

        Arrays.sort(descriptors, 0, count);
        return descriptors;
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        
        super.onDialogClosed(positiveResult);

        
        
        
        final Locale selectedLocale = getSelectedLocale();
        final Context context = getContext();
        BrowserLocaleManager.getInstance().updateConfiguration(context, selectedLocale);
    }

    private Locale getSelectedLocale() {
        final String tag = getValue();
        if (tag == null || tag.equals("")) {
            return Locale.getDefault();
        }
        return BrowserLocaleManager.parseLocaleCode(tag);
    }

    @Override
    public CharSequence getSummary() {
        final String value = getValue();

        if (TextUtils.isEmpty(value)) {
            return getContext().getString(R.string.locale_system_default);
        }

        
        
        final Locale loc = new Locale(value);
        return loc.getDisplayName(loc);
    }

    private void buildList() {
        final Locale currentLocale = Locale.getDefault();
        Log.d(LOG_TAG, "Building locales list. Current locale: " + currentLocale);

        if (currentLocale.equals(this.entriesLocale) &&
            getEntries() != null) {
            Log.v(LOG_TAG, "No need to build list.");
            return;
        }

        final LocaleDescriptor[] descriptors = getShippingLocales();
        final int count = descriptors.length;

        this.entriesLocale = currentLocale;

        
        final String[] entries = new String[count + 1];
        final String[] values = new String[count + 1];

        entries[0] = getContext().getString(R.string.locale_system_default);
        values[0] = "";

        for (int i = 0; i < count; ++i) {
            entries[i + 1] = descriptors[i].getDisplayName();
            values[i + 1] = descriptors[i].getTag();
        }

        setEntries(entries);
        setEntryValues(values);
    }
}
