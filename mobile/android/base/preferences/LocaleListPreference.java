



package org.mozilla.gecko.preferences;

import java.text.Collator;
import java.util.Arrays;
import java.util.Collection;
import java.util.Locale;

import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.R;

import android.content.Context;
import android.preference.ListPreference;
import android.util.AttributeSet;

public class LocaleListPreference extends ListPreference {
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
            this.nativeName = locale.getDisplayName(locale);
            this.tag = tag;
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

    private void buildList() {
        final LocaleDescriptor[] descriptors = getShippingLocales();
        final int count = descriptors.length;

        
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
