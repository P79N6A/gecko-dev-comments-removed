




package org.mozilla.gecko.preferences;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.Checkable;

import org.mozilla.gecko.R;






class ListCheckboxPreference extends MultiChoicePreference implements Checkable {
    private static final String LOGTAG = "GeckoListCheckboxPreference";
    private boolean checked;

    public ListCheckboxPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setWidgetLayoutResource(R.layout.preference_checkbox);
    }

    @Override
    public boolean isChecked() {
        return checked;
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        View checkboxView = view.findViewById(R.id.checkbox);
        if (checkboxView != null && checkboxView instanceof Checkable) {
            ((Checkable) checkboxView).setChecked(checked);
        }
    }

    @Override
    public void setChecked(boolean checked) {
        boolean changed = checked != this.checked;
        this.checked = checked;
        if (changed) {
            notifyDependencyChange(shouldDisableDependents());
            notifyChanged();
        }
    }

    @Override
    public void toggle() {
        checked = !checked;
    }
}
