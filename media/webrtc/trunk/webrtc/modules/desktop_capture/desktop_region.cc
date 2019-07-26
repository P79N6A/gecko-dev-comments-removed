









#include "webrtc/modules/desktop_capture/desktop_region.h"

#include <algorithm>
#include <cassert>

namespace webrtc {

DesktopRegion::RowSpan::RowSpan(int32_t left, int32_t right)
    : left(left), right(right) {
}

DesktopRegion::Row::Row(int32_t top, int32_t bottom)
    : top(top), bottom(bottom) {
}

DesktopRegion::DesktopRegion() {}

DesktopRegion::DesktopRegion(const DesktopRect& rect) {
  AddRect(rect);
}

DesktopRegion::DesktopRegion(const DesktopRect* rects, int count) {
  AddRects(rects, count);
}

DesktopRegion::DesktopRegion(const DesktopRegion& other) {
  *this = other;
}

DesktopRegion::~DesktopRegion() {
  Clear();
}

DesktopRegion& DesktopRegion::operator=(const DesktopRegion& other) {
  Clear();
  rows_ = other.rows_;
  for (Rows::iterator it = rows_.begin(); it != rows_.end(); ++it) {
    
    Row* row = it->second;
    it->second = new Row(*row);
  }
  return *this;
}

bool DesktopRegion::Equals(const DesktopRegion& region) const {
  
  Rows::const_iterator it1 = rows_.begin();
  Rows::const_iterator it2 = region.rows_.begin();
  while (it1 != rows_.end()) {
    if (it2 == region.rows_.end() ||
        it1->first != it2->first ||
        it1->second->top != it2->second->top ||
        it1->second->bottom != it2->second->bottom ||
        it1->second->spans != it2->second->spans) {
      return false;
    }
    ++it1;
    ++it2;
  }
  return it2 == region.rows_.end();
}

void DesktopRegion::Clear() {
  for (Rows::iterator row = rows_.begin(); row != rows_.end(); ++row) {
    delete row->second;
  }
  rows_.clear();
}

void DesktopRegion::SetRect(const DesktopRect& rect) {
  Clear();
  AddRect(rect);
}

void DesktopRegion::AddRect(const DesktopRect& rect) {
  if (rect.is_empty())
    return;

  
  
  int top = rect.top();

  
  
  Rows::iterator row = rows_.upper_bound(top);
  while (top < rect.bottom()) {
    if (row == rows_.end() || top < row->second->top)  {
      
      
      int32_t bottom = rect.bottom();
      if (row != rows_.end() && row->second->top < bottom)
        bottom = row->second->top;
      row = rows_.insert(
          row, Rows::value_type(bottom, new Row(top, bottom)));
    } else if (top > row->second->top) {
      
      
      
      assert(top <= row->second->bottom);
      Rows::iterator new_row = rows_.insert(
          row, Rows::value_type(top, new Row(row->second->top, top)));
      row->second->top = top;
      new_row->second->spans = row->second->spans;
    }

    if (rect.bottom() < row->second->bottom) {
      
      
      
      Rows::iterator new_row = rows_.insert(
          row, Rows::value_type(rect.bottom(), new Row(top, rect.bottom())));
      row->second->top = rect.bottom();
      new_row->second->spans = row->second->spans;
      row = new_row;
    }

    
    AddSpanToRow(row->second, rect.left(), rect.right());
    top = row->second->bottom;

    MergeWithPrecedingRow(row);

    
    row++;
  }

  if (row != rows_.end())
    MergeWithPrecedingRow(row);
}

void DesktopRegion::AddRects(const DesktopRect* rects, int count) {
  for (int i = 0; i < count; ++i) {
    AddRect(rects[i]);
  }
}

void DesktopRegion::MergeWithPrecedingRow(Rows::iterator row) {
  assert(row != rows_.end());

  if (row != rows_.begin()) {
    Rows::iterator previous_row = row;
    previous_row--;

    
    
    if (previous_row->second->bottom == row->second->top &&
        previous_row->second->spans == row->second->spans) {
      row->second->top = previous_row->second->top;
      delete previous_row->second;
      rows_.erase(previous_row);
    }
  }
}

void DesktopRegion::AddRegion(const DesktopRegion& region) {
  
  
  for (Iterator it(region); !it.IsAtEnd(); it.Advance()) {
    AddRect(it.rect());
  }
}

void DesktopRegion::Intersect(const DesktopRegion& region1,
                              const DesktopRegion& region2) {
  Clear();

  Rows::const_iterator it1 = region1.rows_.begin();
  Rows::const_iterator end1 = region1.rows_.end();
  Rows::const_iterator it2 = region2.rows_.begin();
  Rows::const_iterator end2 = region2.rows_.end();
  if (it1 == end1 || it2 == end2)
    return;

  while (it1 != end1 && it2 != end2) {
    
    if (it2->second->top < it1->second->top) {
      std::swap(it1, it2);
      std::swap(end1, end2);
    }

    
    if (it1->second->bottom <= it2->second->top) {
      ++it1;
      continue;
    }

    
    
    int32_t top = it2->second->top;
    int32_t bottom = std::min(it1->second->bottom, it2->second->bottom);

    Rows::iterator new_row = rows_.insert(
        rows_.end(), Rows::value_type(bottom, new Row(top, bottom)));
    IntersectRows(it1->second->spans, it2->second->spans,
                  &new_row->second->spans);
    if (new_row->second->spans.empty()) {
      delete new_row->second;
      rows_.erase(new_row);
    } else {
      MergeWithPrecedingRow(new_row);
    }

    
    if (it1->second->bottom == bottom)
      ++it1;
    
    if (it2->second->bottom == bottom)
      ++it2;
  }
}


void DesktopRegion::IntersectRows(const RowSpanSet& set1,
                                  const RowSpanSet& set2,
                                  RowSpanSet* output) {
  RowSpanSet::const_iterator it1 = set1.begin();
  RowSpanSet::const_iterator end1 = set1.end();
  RowSpanSet::const_iterator it2 = set2.begin();
  RowSpanSet::const_iterator end2 = set2.end();
  assert(it1 != end1 && it2 != end2);

  do {
    
    if (it2->left < it1->left) {
      std::swap(it1, it2);
      std::swap(end1, end2);
    }

    
    if (it1->right <= it2->left) {
      ++it1;
      continue;
    }

    int32_t left = it2->left;
    int32_t right = std::min(it1->right, it2->right);
    assert(left < right);

    output->push_back(RowSpan(left, right));

    
    if (it1->right == right)
      ++it1;
    
    if (it2->right == right)
      ++it2;
  } while (it1 != end1 && it2 != end2);
}

void DesktopRegion::IntersectWith(const DesktopRegion& region) {
  DesktopRegion old_region;
  Swap(&old_region);
  Intersect(old_region, region);
}

void DesktopRegion::IntersectWith(const DesktopRect& rect) {
  DesktopRegion region;
  region.AddRect(rect);
  IntersectWith(region);
}

void DesktopRegion::Translate(int32_t dx, int32_t dy) {
  Rows new_rows;

  for (Rows::iterator it = rows_.begin(); it != rows_.end(); ++it) {
    Row* row = it->second;

    row->top += dy;
    row->bottom += dy;

    if (dx != 0) {
      
      for (RowSpanSet::iterator span = row->spans.begin();
           span != row->spans.end(); ++span) {
        span->left += dx;
        span->right += dx;
      }
    }

    if (dy != 0)
      new_rows.insert(new_rows.end(), Rows::value_type(row->bottom, row));
  }

  if (dy != 0)
    new_rows.swap(rows_);
}

void DesktopRegion::Swap(DesktopRegion* region) {
  rows_.swap(region->rows_);
}


bool DesktopRegion::CompareSpanRight(const RowSpan& r, int32_t value) {
  return r.right < value;
}


bool DesktopRegion::CompareSpanLeft(const RowSpan& r, int32_t value) {
  return r.left < value;
}


void DesktopRegion::AddSpanToRow(Row* row, int left, int right) {
  
  
  
  if (row->spans.empty() || left > row->spans.back().right) {
    row->spans.push_back(RowSpan(left, right));
    return;
  }

  
  RowSpanSet::iterator start =
      std::lower_bound(row->spans.begin(), row->spans.end(), left,
                       CompareSpanRight);
  assert(start < row->spans.end());

  
  RowSpanSet::iterator end =
      std::lower_bound(start, row->spans.end(), right + 1, CompareSpanLeft);
  if (end == row->spans.begin()) {
    
    row->spans.insert(row->spans.begin(), RowSpan(left, right));
    return;
  }

  
  
  end--;

  
  
  if (end < start) {
    
    row->spans.insert(start, RowSpan(left, right));
    return;
  }

  left = std::min(left, start->left);
  right = std::max(right, end->right);

  
  *start = RowSpan(left, right);
  ++start;
  ++end;
  if (start < end)
    row->spans.erase(start, end);
}


bool DesktopRegion::IsSpanInRow(const Row& row, const RowSpan& span) {
  
  
  RowSpanSet::const_iterator it =
      std::lower_bound(row.spans.begin(), row.spans.end(), span.left,
                       CompareSpanLeft);
  return it != row.spans.end() && *it == span;
}

DesktopRegion::Iterator::Iterator(const DesktopRegion& region)
    : region_(region),
      row_(region.rows_.begin()),
      previous_row_(region.rows_.end()) {
  if (!IsAtEnd()) {
    assert(row_->second->spans.size() > 0);
    row_span_ = row_->second->spans.begin();
    UpdateCurrentRect();
  }
}

bool DesktopRegion::Iterator::IsAtEnd() const {
  return row_ == region_.rows_.end();
}

void DesktopRegion::Iterator::Advance() {
  assert(!IsAtEnd());

  while (true) {
    ++row_span_;
    if (row_span_ == row_->second->spans.end()) {
      previous_row_ = row_;
      ++row_;
      if (row_ != region_.rows_.end()) {
        assert(row_->second->spans.size() > 0);
        row_span_ = row_->second->spans.begin();
      }
    }

    if (IsAtEnd())
      return;

    
    
    
    if (previous_row_ != region_.rows_.end() &&
        previous_row_->second->bottom == row_->second->top &&
        IsSpanInRow(*previous_row_->second, *row_span_)) {
      continue;
    }

    break;
  }

  assert(!IsAtEnd());
  UpdateCurrentRect();
}

void DesktopRegion::Iterator::UpdateCurrentRect() {
  
  int bottom;
  Rows::const_iterator bottom_row = row_;
  Rows::const_iterator previous;
  do {
    bottom = bottom_row->second->bottom;
    previous = bottom_row;
    ++bottom_row;
  } while (bottom_row != region_.rows_.end() &&
           previous->second->bottom == bottom_row->second->top &&
           IsSpanInRow(*bottom_row->second, *row_span_));
  rect_ = DesktopRect::MakeLTRB(row_span_->left, row_->second->top,
                                row_span_->right, bottom);
}

}  
