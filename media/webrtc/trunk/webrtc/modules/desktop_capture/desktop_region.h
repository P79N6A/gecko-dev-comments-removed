









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_REGION_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_REGION_H_

#include <map>
#include <vector>

#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {





class DesktopRegion {
 private:
  
  

  
  struct RowSpan {
    RowSpan(int32_t left, int32_t right);

    
    bool operator==(const RowSpan& that) const {
      return left == that.left && right == that.right;
    }

    int32_t left;
    int32_t right;
  };

  typedef std::vector<RowSpan> RowSpanSet;

  
  
  struct Row {
    Row(int32_t top, int32_t bottom);
    ~Row();

    int32_t top;
    int32_t bottom;

    RowSpanSet spans;
  };

  
  
  
  typedef std::map<int, Row*> Rows;

 public:
  
  
  class Iterator {
   public:
    explicit Iterator(const DesktopRegion& target);

    bool IsAtEnd() const;
    void Advance();

    const DesktopRect& rect() const { return rect_; }

   private:
    const DesktopRegion& region_;

    
    
    
    void UpdateCurrentRect();

    Rows::const_iterator row_;
    Rows::const_iterator previous_row_;
    RowSpanSet::const_iterator row_span_;
    DesktopRect rect_;
  };

  DesktopRegion();
  explicit DesktopRegion(const DesktopRect& rect);
  DesktopRegion(const DesktopRect* rects, int count);
  DesktopRegion(const DesktopRegion& other);
  ~DesktopRegion();

  DesktopRegion& operator=(const DesktopRegion& other);

  bool is_empty() const { return rows_.empty(); }

  bool Equals(const DesktopRegion& region) const;

  
  void Clear();

  
  void SetRect(const DesktopRect& rect);

  
  void AddRect(const DesktopRect& rect);
  void AddRects(const DesktopRect* rects, int count);
  void AddRegion(const DesktopRegion& region);

  
  void Intersect(const DesktopRegion& region1, const DesktopRegion& region2);

  
  void IntersectWith(const DesktopRegion& region);

  
  void IntersectWith(const DesktopRect& rect);

  
  void Subtract(const DesktopRegion& region);

  
  void Subtract(const DesktopRect& rect);

  
  void Translate(int32_t dx, int32_t dy);

  void Swap(DesktopRegion* region);

 private:
  
  
  static bool CompareSpanLeft(const RowSpan& r, int32_t value);
  static bool CompareSpanRight(const RowSpan& r, int32_t value);

  
  static void AddSpanToRow(Row* row, int32_t left, int32_t right);

  
  static bool IsSpanInRow(const Row& row, const RowSpan& rect);

  
  static void IntersectRows(const RowSpanSet& set1,
                            const RowSpanSet& set2,
                            RowSpanSet* output);

  static void SubtractRows(const RowSpanSet& set_a,
                           const RowSpanSet& set_b,
                           RowSpanSet* output);

  
  
  
  
  void MergeWithPrecedingRow(Rows::iterator row);

  Rows rows_;
};

}  

#endif  

