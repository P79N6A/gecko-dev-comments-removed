



#include "chrome/common/thumbnail_score.h"

#include "base/logging.h"

using base::Time;
using base::TimeDelta;

const TimeDelta ThumbnailScore::kUpdateThumbnailTime = TimeDelta::FromDays(1);
const double ThumbnailScore::kThumbnailMaximumBoringness = 0.94;
const double ThumbnailScore::kThumbnailDegradePerHour = 0.01;




static int GetThumbnailType(bool good_clipping, bool at_top) {
  if (good_clipping && at_top) {
    return 0;
  } else if (good_clipping && !at_top) {
    return 1;
  } else if (!good_clipping && at_top) {
    return 2;
  } else if (!good_clipping && !at_top) {
    return 3;
  } else {
    NOTREACHED();
    return -1;
  }
}

ThumbnailScore::ThumbnailScore()
    : boring_score(1.0),
      good_clipping(false),
      at_top(false),
      time_at_snapshot(Time::Now()) {
}

ThumbnailScore::ThumbnailScore(double score, bool clipping, bool top)
    : boring_score(score),
      good_clipping(clipping),
      at_top(top),
      time_at_snapshot(Time::Now()) {
}

ThumbnailScore::ThumbnailScore(double score, bool clipping, bool top,
                               const Time& time)
    : boring_score(score),
      good_clipping(clipping),
      at_top(top),
      time_at_snapshot(time) {
}

ThumbnailScore::~ThumbnailScore() {
}

bool ThumbnailScore::Equals(const ThumbnailScore& rhs) const {
  
  
  
  return boring_score == rhs.boring_score &&
      good_clipping == rhs.good_clipping &&
      at_top == rhs.at_top &&
      time_at_snapshot.ToTimeT() == rhs.time_at_snapshot.ToTimeT();
}

bool ShouldReplaceThumbnailWith(const ThumbnailScore& current,
                                const ThumbnailScore& replacement) {
  int current_type = GetThumbnailType(current.good_clipping, current.at_top);
  int replacement_type = GetThumbnailType(replacement.good_clipping,
                                          replacement.at_top);
  if (replacement_type < current_type) {
    
    
    return replacement.boring_score <
        ThumbnailScore::kThumbnailMaximumBoringness;
  } else if (replacement_type == current_type) {
    if (replacement.boring_score < current.boring_score) {
      
      return true;
    }

    
    
    TimeDelta since_last_thumbnail =
        replacement.time_at_snapshot - current.time_at_snapshot;
    double degraded_boring_score = current.boring_score +
        (since_last_thumbnail.InHours() *
         ThumbnailScore::kThumbnailDegradePerHour);

    if (degraded_boring_score > ThumbnailScore::kThumbnailMaximumBoringness)
      degraded_boring_score = ThumbnailScore::kThumbnailMaximumBoringness;
    if (replacement.boring_score < degraded_boring_score)
      return true;
  }

  
  
  
  return current.boring_score >= ThumbnailScore::kThumbnailMaximumBoringness &&
      replacement.boring_score < ThumbnailScore::kThumbnailMaximumBoringness;
}
