









#ifndef WEBRTC_BASE_SIGSLOTREPEATER_H__
#define WEBRTC_BASE_SIGSLOTREPEATER_H__








#include "webrtc/base/sigslot.h"

namespace sigslot {

  template<class mt_policy = SIGSLOT_DEFAULT_MT_POLICY>
  class repeater0 : public signal0<mt_policy>,
                    public has_slots<mt_policy>
  {
  public:
    typedef signal0<mt_policy> base_type;
    typedef repeater0<mt_policy> this_type;

    repeater0() { }
    repeater0(const this_type& s) : base_type(s) { }

    void reemit() { signal0<mt_policy>::emit(); }
    void repeat(base_type &s) { s.connect(this, &this_type::reemit); }
    void stop(base_type &s) { s.disconnect(this); }
  };

  template<class arg1_type, class mt_policy = SIGSLOT_DEFAULT_MT_POLICY>
  class repeater1 : public signal1<arg1_type, mt_policy>,
                    public has_slots<mt_policy>
  {
  public:
    typedef signal1<arg1_type, mt_policy> base_type;
    typedef repeater1<arg1_type, mt_policy> this_type;

    repeater1() { }
    repeater1(const this_type& s) : base_type(s) { }

    void reemit(arg1_type a1) { signal1<arg1_type, mt_policy>::emit(a1); }
    void repeat(base_type& s) { s.connect(this, &this_type::reemit); }
    void stop(base_type &s) { s.disconnect(this); }
  };

  template<class arg1_type, class arg2_type, class mt_policy = SIGSLOT_DEFAULT_MT_POLICY>
  class repeater2 : public signal2<arg1_type, arg2_type, mt_policy>,
                    public has_slots<mt_policy>
  {
  public:
    typedef signal2<arg1_type, arg2_type, mt_policy> base_type;
    typedef repeater2<arg1_type, arg2_type, mt_policy> this_type;

    repeater2() { }
    repeater2(const this_type& s) : base_type(s) { }

    void reemit(arg1_type a1, arg2_type a2) { signal2<arg1_type, arg2_type, mt_policy>::emit(a1,a2); }
    void repeat(base_type& s) { s.connect(this, &this_type::reemit); }
    void stop(base_type &s) { s.disconnect(this); }
  };

  template<class arg1_type, class arg2_type, class arg3_type,
           class mt_policy = SIGSLOT_DEFAULT_MT_POLICY>
  class repeater3 : public signal3<arg1_type, arg2_type, arg3_type, mt_policy>,
                    public has_slots<mt_policy>
  {
  public:
    typedef signal3<arg1_type, arg2_type, arg3_type, mt_policy> base_type;
    typedef repeater3<arg1_type, arg2_type, arg3_type, mt_policy> this_type;

    repeater3() { }
    repeater3(const this_type& s) : base_type(s) { }

    void reemit(arg1_type a1, arg2_type a2, arg3_type a3) {
            signal3<arg1_type, arg2_type, arg3_type, mt_policy>::emit(a1,a2,a3);
    }
    void repeat(base_type& s) { s.connect(this, &this_type::reemit); }
    void stop(base_type &s) { s.disconnect(this); }
  };

}  

#endif  
