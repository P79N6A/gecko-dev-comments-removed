



package org.mozilla.search.autocomplete;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import org.mozilla.search.AcceptsSearchQuery;
import org.mozilla.search.R;
import org.mozilla.search.autocomplete.SuggestionsFragment.Suggestion;

import java.util.List;




class AutoCompleteAdapter extends ArrayAdapter<Suggestion> {

    private final AcceptsSearchQuery searchListener;

    private final LayoutInflater inflater;

    public AutoCompleteAdapter(Context context) {
        
        
        super(context, 0);

        if (context instanceof AcceptsSearchQuery) {
            searchListener = (AcceptsSearchQuery) context;
        } else {
            throw new ClassCastException(context.toString() + " must implement AcceptsSearchQuery.");
        }

        
        setNotifyOnChange(false);

        inflater = LayoutInflater.from(context);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if (convertView == null) {
            convertView = inflater.inflate(R.layout.search_suggestions_row, null);
        }

        final Suggestion suggestion = getItem(position);

        final TextView textView = (TextView) convertView.findViewById(R.id.auto_complete_row_text);
        textView.setText(suggestion.display);

        final View jumpButton = convertView.findViewById(R.id.auto_complete_row_jump_button);
        jumpButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                searchListener.onSuggest(suggestion.value);
            }
        });

        return convertView;
    }

    




    public void update(List<Suggestion> suggestions) {
        clear();
        if (suggestions != null) {
            for (Suggestion s : suggestions) {
                add(s);
            }
        }
        notifyDataSetChanged();
    }
}
