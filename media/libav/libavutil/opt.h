




















#ifndef AVUTIL_OPT_H
#define AVUTIL_OPT_H






#include "rational.h"
#include "avutil.h"
#include "dict.h"
#include "log.h"
























































































































































































enum AVOptionType{
    AV_OPT_TYPE_FLAGS,
    AV_OPT_TYPE_INT,
    AV_OPT_TYPE_INT64,
    AV_OPT_TYPE_DOUBLE,
    AV_OPT_TYPE_FLOAT,
    AV_OPT_TYPE_STRING,
    AV_OPT_TYPE_RATIONAL,
    AV_OPT_TYPE_BINARY,  
    AV_OPT_TYPE_DICT,
    AV_OPT_TYPE_CONST = 128,
};




typedef struct AVOption {
    const char *name;

    



    const char *help;

    



    int offset;
    enum AVOptionType type;

    


    union {
        int64_t i64;
        double dbl;
        const char *str;
        
        AVRational q;
    } default_val;
    double min;                 
    double max;                 

    int flags;
#define AV_OPT_FLAG_ENCODING_PARAM  1   ///< a generic parameter which can be set by the user for muxing or encoding
#define AV_OPT_FLAG_DECODING_PARAM  2   ///< a generic parameter which can be set by the user for demuxing or decoding
#if FF_API_OPT_TYPE_METADATA
#define AV_OPT_FLAG_METADATA        4   ///< some data extracted or inserted into the file like title, comment, ...
#endif
#define AV_OPT_FLAG_AUDIO_PARAM     8
#define AV_OPT_FLAG_VIDEO_PARAM     16
#define AV_OPT_FLAG_SUBTITLE_PARAM  32



#define AV_OPT_FLAG_EXPORT          64




#define AV_OPT_FLAG_READONLY        128


    




    const char *unit;
} AVOption;










int av_opt_show2(void *obj, void *av_log_obj, int req_flags, int rej_flags);






void av_opt_set_defaults(void *s);

















int av_set_options_string(void *ctx, const char *opts,
                          const char *key_val_sep, const char *pairs_sep);




void av_opt_free(void *obj);









int av_opt_flag_is_set(void *obj, const char *field_name, const char *flag_name);















int av_opt_set_dict(void *obj, struct AVDictionary **options);















int av_opt_eval_flags (void *obj, const AVOption *o, const char *val, int        *flags_out);
int av_opt_eval_int   (void *obj, const AVOption *o, const char *val, int        *int_out);
int av_opt_eval_int64 (void *obj, const AVOption *o, const char *val, int64_t    *int64_out);
int av_opt_eval_float (void *obj, const AVOption *o, const char *val, float      *float_out);
int av_opt_eval_double(void *obj, const AVOption *o, const char *val, double     *double_out);
int av_opt_eval_q     (void *obj, const AVOption *o, const char *val, AVRational *q_out);




#define AV_OPT_SEARCH_CHILDREN   0x0001 /**< Search in possible children of the
                                             given object first. */






#define AV_OPT_SEARCH_FAKE_OBJ   0x0002























const AVOption *av_opt_find(void *obj, const char *name, const char *unit,
                            int opt_flags, int search_flags);






















const AVOption *av_opt_find2(void *obj, const char *name, const char *unit,
                             int opt_flags, int search_flags, void **target_obj);










const AVOption *av_opt_next(void *obj, const AVOption *prev);







void *av_opt_child_next(void *obj, void *prev);







const AVClass *av_opt_child_class_next(const AVClass *parent, const AVClass *prev);



























int av_opt_set         (void *obj, const char *name, const char *val, int search_flags);
int av_opt_set_int     (void *obj, const char *name, int64_t     val, int search_flags);
int av_opt_set_double  (void *obj, const char *name, double      val, int search_flags);
int av_opt_set_q       (void *obj, const char *name, AVRational  val, int search_flags);
int av_opt_set_bin     (void *obj, const char *name, const uint8_t *val, int size, int search_flags);




int av_opt_set_dict_val(void *obj, const char *name, const AVDictionary *val, int search_flags);



















int av_opt_get         (void *obj, const char *name, int search_flags, uint8_t   **out_val);
int av_opt_get_int     (void *obj, const char *name, int search_flags, int64_t    *out_val);
int av_opt_get_double  (void *obj, const char *name, int search_flags, double     *out_val);
int av_opt_get_q       (void *obj, const char *name, int search_flags, AVRational *out_val);




int av_opt_get_dict_val(void *obj, const char *name, int search_flags, AVDictionary **out_val);





#endif 
