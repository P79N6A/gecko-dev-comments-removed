
#ifndef __gtkmozembed_MARSHAL_H__
#define __gtkmozembed_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS


extern void gtkmozembed_BOOLEAN__STRING (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING	gtkmozembed_BOOLEAN__STRING


extern void gtkmozembed_BOOLEAN__STRING_STRING (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING	gtkmozembed_BOOLEAN__STRING_STRING


extern void gtkmozembed_BOOLEAN__STRING_STRING_POINTER (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_POINTER	gtkmozembed_BOOLEAN__STRING_STRING_POINTER


extern void gtkmozembed_BOOLEAN__STRING_STRING_POINTER_INT (GClosure     *closure,
                                                            GValue       *return_value,
                                                            guint         n_param_values,
                                                            const GValue *param_values,
                                                            gpointer      invocation_hint,
                                                            gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_POINTER_INT	gtkmozembed_BOOLEAN__STRING_STRING_POINTER_INT


extern void gtkmozembed_BOOLEAN__STRING_STRING_POINTER_POINTER_STRING_POINTER (GClosure     *closure,
                                                                               GValue       *return_value,
                                                                               guint         n_param_values,
                                                                               const GValue *param_values,
                                                                               gpointer      invocation_hint,
                                                                               gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_POINTER_POINTER_STRING_POINTER	gtkmozembed_BOOLEAN__STRING_STRING_POINTER_POINTER_STRING_POINTER


extern void gtkmozembed_BOOLEAN__STRING_STRING_POINTER_STRING_POINTER (GClosure     *closure,
                                                                       GValue       *return_value,
                                                                       guint         n_param_values,
                                                                       const GValue *param_values,
                                                                       gpointer      invocation_hint,
                                                                       gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_POINTER_STRING_POINTER	gtkmozembed_BOOLEAN__STRING_STRING_POINTER_STRING_POINTER


extern void gtkmozembed_BOOLEAN__STRING_STRING_STRING_POINTER (GClosure     *closure,
                                                               GValue       *return_value,
                                                               guint         n_param_values,
                                                               const GValue *param_values,
                                                               gpointer      invocation_hint,
                                                               gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_STRING_POINTER	gtkmozembed_BOOLEAN__STRING_STRING_STRING_POINTER


extern void gtkmozembed_INT__STRING_INT_INT_INT_INT_INT (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);


extern void gtkmozembed_INT__STRING_STRING_INT_INT_INT_INT (GClosure     *closure,
                                                            GValue       *return_value,
                                                            guint         n_param_values,
                                                            const GValue *param_values,
                                                            gpointer      invocation_hint,
                                                            gpointer      marshal_data);


extern void gtkmozembed_INT__STRING_STRING_UINT_STRING_STRING_STRING_STRING_POINTER (GClosure     *closure,
                                                                                     GValue       *return_value,
                                                                                     guint         n_param_values,
                                                                                     const GValue *param_values,
                                                                                     gpointer      invocation_hint,
                                                                                     gpointer      marshal_data);


extern void gtkmozembed_INT__VOID (GClosure     *closure,
                                   GValue       *return_value,
                                   guint         n_param_values,
                                   const GValue *param_values,
                                   gpointer      invocation_hint,
                                   gpointer      marshal_data);


extern void gtkmozembed_STRING__STRING_STRING (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);


#define gtkmozembed_VOID__BOOLEAN	g_cclosure_marshal_VOID__BOOLEAN
#define gtkmozembed_VOID__BOOL	gtkmozembed_VOID__BOOLEAN


extern void gtkmozembed_VOID__INT_INT_BOOLEAN (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);
#define gtkmozembed_VOID__INT_INT_BOOL	gtkmozembed_VOID__INT_INT_BOOLEAN


extern void gtkmozembed_VOID__INT_STRING (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);


extern void gtkmozembed_VOID__INT_STRING_STRING (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);


extern void gtkmozembed_VOID__INT_UINT (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);


extern void gtkmozembed_VOID__POINTER_INT_POINTER (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);


extern void gtkmozembed_VOID__POINTER_INT_STRING_STRING_STRING_STRING_STRING_BOOLEAN_INT (GClosure     *closure,
                                                                                          GValue       *return_value,
                                                                                          guint         n_param_values,
                                                                                          const GValue *param_values,
                                                                                          gpointer      invocation_hint,
                                                                                          gpointer      marshal_data);


extern void gtkmozembed_VOID__POINTER_STRING_BOOLEAN_BOOLEAN (GClosure     *closure,
                                                              GValue       *return_value,
                                                              guint         n_param_values,
                                                              const GValue *param_values,
                                                              gpointer      invocation_hint,
                                                              gpointer      marshal_data);
#define gtkmozembed_VOID__POINTER_STRING_BOOL_BOOL	gtkmozembed_VOID__POINTER_STRING_BOOLEAN_BOOLEAN


extern void gtkmozembed_VOID__STRING_INT_INT (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);


extern void gtkmozembed_VOID__STRING_INT_UINT (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);


extern void gtkmozembed_VOID__STRING_STRING (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);


extern void gtkmozembed_VOID__STRING_STRING_POINTER (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);


extern void gtkmozembed_VOID__STRING_STRING_STRING_ULONG_INT (GClosure     *closure,
                                                              GValue       *return_value,
                                                              guint         n_param_values,
                                                              const GValue *param_values,
                                                              gpointer      invocation_hint,
                                                              gpointer      marshal_data);


extern void gtkmozembed_VOID__STRING_STRING_STRING_POINTER (GClosure     *closure,
                                                            GValue       *return_value,
                                                            guint         n_param_values,
                                                            const GValue *param_values,
                                                            gpointer      invocation_hint,
                                                            gpointer      marshal_data);


extern void gtkmozembed_VOID__UINT_INT_INT_STRING_STRING_STRING_STRING (GClosure     *closure,
                                                                        GValue       *return_value,
                                                                        guint         n_param_values,
                                                                        const GValue *param_values,
                                                                        gpointer      invocation_hint,
                                                                        gpointer      marshal_data);


extern void gtkmozembed_VOID__ULONG_ULONG_ULONG (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);


extern void gtkmozembed_BOOLEAN__POINTER_UINT (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);
#define gtkmozembed_BOOL__POINTER_UINT	gtkmozembed_BOOLEAN__POINTER_UINT


#define gtkmozembed_VOID__POINTER	g_cclosure_marshal_VOID__POINTER



G_END_DECLS

#endif 

