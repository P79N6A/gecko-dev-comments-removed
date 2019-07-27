



package org.mozilla.search.autocomplete;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;

import java.util.List;




class AutoCompleteAdapter extends ArrayAdapter<String> {

    private final AcceptsJumpTaps acceptsJumpTaps;

    public AutoCompleteAdapter(Context context, AcceptsJumpTaps acceptsJumpTaps) {
        
        
        super(context, 0);
        this.acceptsJumpTaps = acceptsJumpTaps;

        
        setNotifyOnChange(false);
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
        view.setMainText(getItem(position));

        return view;
    }

    




    public void update(List<String> suggestions) {
        clear();
        if (suggestions != null) {
            for (String s : suggestions) {
                add(s);
            }
        }
        notifyDataSetChanged();
    }
}
