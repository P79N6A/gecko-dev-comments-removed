









#ifndef WEBRTC_BASE_EXP_FILTER_H_
#define WEBRTC_BASE_EXP_FILTER_H_

namespace rtc {




class ExpFilter {
 public:
  static const float kValueUndefined;

  explicit ExpFilter(float alpha, float max = kValueUndefined)
      : max_(max) {
    Reset(alpha);
  }

  
  
  void Reset(float alpha);

  
  
  float Apply(float exp, float sample);

  
  float filtered() const { return filtered_; }

  
  void UpdateBase(float alpha);

 private:
  float alpha_;  
  float filtered_;  
  const float max_;
};
}  

#endif  
