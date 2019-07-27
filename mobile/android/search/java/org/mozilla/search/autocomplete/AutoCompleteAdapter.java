



package org.mozilla.search.autocomplete;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;




class AutoCompleteAdapter extends ArrayAdapter<AutoCompleteModel> {

    private final AcceptsJumpTaps acceptsJumpTaps;

    public AutoCompleteAdapter(Context context, AcceptsJumpTaps acceptsJumpTaps) {
        
        
        super(context, 0);
        this.acceptsJumpTaps = acceptsJumpTaps;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        AutoCompleteRowView view;

        if (convertView == null) {
            view = new AutoCompleteRowView(getContext());
        } else {
            view = (AutoCompleteRowView) convertView;
        }

        view.setOnJumpListener(acceptsJumpTaps);


        AutoCompleteModel model = getItem(position);

        view.setMainText(model.getMainText());

        return view;
    }
}
