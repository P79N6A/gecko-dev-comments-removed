





































#ifndef _MOZILLA_DECODER_H
#define _MOZILLA_DECODER_H

#include <pango/pangofc-decoder.h>

G_BEGIN_DECLS

#define MOZILLA_TYPE_DECODER (mozilla_decoder_get_type())
#define MOZILLA_DECODER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MOZILLA_TYPE_DECODER, MozillaDecoder))
#define MOZILLA_IS_DECODER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOZILLA_TYPE_DECODER))

typedef struct _MozillaDecoder      MozillaDecoder;
typedef struct _MozillaDecoderClass MozillaDecoderClass;

#define MOZILLA_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MOZILLA_TYPE_DECODER, MozillaDecoderClass))
#define MOZILLA_IS_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOZILLA_TYPE_DECODER))
#define MOZILLA_DECODER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MOZILLA_TYPE_DECODER, MozillaDecoderClass))

struct _MozillaDecoder
{
  PangoFcDecoder parent_instance;
};

struct _MozillaDecoderClass
{
  PangoFcDecoderClass parent_class;
};

GType           mozilla_decoder_get_type (void);
int             mozilla_decoders_init    (void);

G_END_DECLS

#endif 
