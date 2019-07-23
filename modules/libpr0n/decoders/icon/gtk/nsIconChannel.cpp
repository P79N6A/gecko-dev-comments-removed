




































#include <stdlib.h>
#include <unistd.h>


extern "C" {
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-icon-theme.h>
#include <libgnomeui/gnome-icon-lookup.h>
#include <libgnomeui/gnome-ui-init.h>

#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>
}

#include <gtk/gtkwidget.h>
#include <gtk/gtkiconfactory.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkfixed.h>
#include <gtk/gtkversion.h>

#include "nsIMIMEService.h"

#include "nsIStringBundle.h"

#include "nsNetUtil.h"
#include "nsIURL.h"

#include "nsIconChannel.h"

NS_IMPL_ISUPPORTS2(nsIconChannel,
                   nsIRequest,
                   nsIChannel)

static nsresult
moz_gdk_pixbuf_to_channel(GdkPixbuf* aPixbuf, nsIURI *aURI,
                          nsIChannel **aChannel)
{
  int width = gdk_pixbuf_get_width(aPixbuf);
  int height = gdk_pixbuf_get_height(aPixbuf);
  NS_ENSURE_TRUE(height < 256 && width < 256 && height > 0 && width > 0 &&
                 gdk_pixbuf_get_colorspace(aPixbuf) == GDK_COLORSPACE_RGB &&
                 gdk_pixbuf_get_bits_per_sample(aPixbuf) == 8 &&
                 gdk_pixbuf_get_has_alpha(aPixbuf) &&
                 gdk_pixbuf_get_n_channels(aPixbuf) == 4,
                 NS_ERROR_UNEXPECTED);

  const int n_channels = 4;
  gsize buf_size = 2 + n_channels * height * width;
  PRUint8 * const buf = (PRUint8*)NS_Alloc(buf_size);
  NS_ENSURE_TRUE(buf, NS_ERROR_OUT_OF_MEMORY);
  PRUint8 *out = buf;

  *(out++) = width;
  *(out++) = height;

  const guchar * const pixels = gdk_pixbuf_get_pixels(aPixbuf);
  int rowextra = gdk_pixbuf_get_rowstride(aPixbuf) - width * n_channels;

  
  const guchar * in = pixels;
  for (int y = 0; y < height; ++y, in += rowextra) {
    for (int x = 0; x < width; ++x) {
      PRUint8 r = *(in++);
      PRUint8 g = *(in++);
      PRUint8 b = *(in++);
      PRUint8 a = *(in++);
#define DO_PREMULTIPLY(c_) PRUint8(PRUint16(c_) * PRUint16(a) / PRUint16(255))
#ifdef IS_LITTLE_ENDIAN
      *(out++) = DO_PREMULTIPLY(b);
      *(out++) = DO_PREMULTIPLY(g);
      *(out++) = DO_PREMULTIPLY(r);
      *(out++) = a;
#else
      *(out++) = a;
      *(out++) = DO_PREMULTIPLY(r);
      *(out++) = DO_PREMULTIPLY(g);
      *(out++) = DO_PREMULTIPLY(b);
#endif
#undef DO_PREMULTIPLY
    }
  }

  NS_ASSERTION(out == buf + buf_size, "size miscalculation");

  nsresult rv;
  nsCOMPtr<nsIStringInputStream> stream =
    do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->AdoptData((char*)buf, buf_size);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewInputStreamChannel(aChannel, aURI, stream,
                                NS_LITERAL_CSTRING("image/icon"));
  return rv;
}

static GtkWidget *gProtoWindow = nsnull;
static GtkWidget *gStockImageWidget = nsnull;
static GnomeIconTheme *gIconTheme = nsnull;

#if GTK_CHECK_VERSION(2,4,0)
static GtkIconFactory *gIconFactory = nsnull;
#endif

static void
ensure_stock_image_widget()
{
  if (!gProtoWindow) {
    gProtoWindow = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_realize(gProtoWindow);
    GtkWidget* protoLayout = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(gProtoWindow), protoLayout);

    gStockImageWidget = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(protoLayout), gStockImageWidget);
    gtk_widget_realize(gStockImageWidget);
  }
}

#if GTK_CHECK_VERSION(2,4,0)
static void
ensure_icon_factory()
{
  if (!gIconFactory) {
    gIconFactory = gtk_icon_factory_new();
    gtk_icon_factory_add_default(gIconFactory);
    g_object_unref(gIconFactory);
  }
}
#endif

static GtkIconSize
moz_gtk_icon_size(const char *name)
{
  if (strcmp(name, "button") == 0)
    return GTK_ICON_SIZE_BUTTON;

  if (strcmp(name, "menu") == 0)
    return GTK_ICON_SIZE_MENU;

  if (strcmp(name, "toolbar") == 0)
    return GTK_ICON_SIZE_LARGE_TOOLBAR;

  if (strcmp(name, "toolbarsmall") == 0)
    return GTK_ICON_SIZE_SMALL_TOOLBAR;

  if (strcmp(name, "dialog") == 0)
    return GTK_ICON_SIZE_DIALOG;

  return GTK_ICON_SIZE_MENU;
}

nsresult
nsIconChannel::InitWithGnome(nsIMozIconURI *aIconURI)
{
  nsresult rv;
  
  if (!gnome_program_get()) {
    
    
    
    nsCOMPtr<nsIStringBundleService> bundleService = 
      do_GetService(NS_STRINGBUNDLE_CONTRACTID);

    NS_ASSERTION(bundleService, "String bundle service must be present!");

    nsCOMPtr<nsIStringBundle> bundle;
    bundleService->CreateBundle("chrome://branding/locale/brand.properties",
                                getter_AddRefs(bundle));
    nsAutoString appName;

    if (bundle) {
      bundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                getter_Copies(appName));
    } else {
      NS_WARNING("brand.properties not present, using default application name");
      appName.Assign(NS_LITERAL_STRING("Gecko"));
    }

    char* empty[] = { "" };
    gnome_init(NS_ConvertUTF16toUTF8(appName).get(), "1.0", 1, empty);
  }

  nsCAutoString iconSizeString;
  aIconURI->GetIconSize(iconSizeString);

  PRUint32 iconSize;

  if (iconSizeString.IsEmpty()) {
    rv = aIconURI->GetImageSize(&iconSize);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetImageSize failed");
  } else {
    int size;
    
    GtkIconSize icon_size = moz_gtk_icon_size(iconSizeString.get());
    gtk_icon_size_lookup(icon_size, &size, NULL);
    iconSize = size;
  }

  nsCAutoString type;
  aIconURI->GetContentType(type);

  GnomeVFSFileInfo fileInfo = {0};
  fileInfo.refcount = 1; 

  nsCAutoString spec;
  nsCOMPtr<nsIURI> fileURI;
  rv = aIconURI->GetIconFile(getter_AddRefs(fileURI));
  if (fileURI) {
    fileURI->GetAsciiSpec(spec);
    
    
    PRBool isFile;
    if (NS_SUCCEEDED(fileURI->SchemeIs("file", &isFile)) && isFile) {
      gnome_vfs_get_file_info(spec.get(), &fileInfo, GNOME_VFS_FILE_INFO_DEFAULT);
    }
    else {
      
      nsCOMPtr<nsIURL> url(do_QueryInterface(fileURI));
      if (url) {
        nsCAutoString name;
        
        
        
        
        
        url->GetFileName(name);
        fileInfo.name = g_strdup(name.get());
      }
      

      if (!type.IsEmpty()) {
        fileInfo.valid_fields = GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
        fileInfo.mime_type = g_strdup(type.get());
      }
    }
  }


  if (type.IsEmpty()) {
    nsCOMPtr<nsIMIMEService> ms(do_GetService("@mozilla.org/mime;1"));
    if (ms) {
      nsCAutoString fileExt;
      aIconURI->GetFileExtension(fileExt);
      ms->GetTypeFromExtension(fileExt, type);
    }
  }

  
  if (!gIconTheme) {
    gIconTheme = gnome_icon_theme_new();
    if (!gIconTheme) {
      gnome_vfs_file_info_clear(&fileInfo);
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  char* name = gnome_icon_lookup(gIconTheme, NULL, spec.get(), NULL, &fileInfo,
                                 type.get(), GNOME_ICON_LOOKUP_FLAGS_NONE,
                                 NULL);
  gnome_vfs_file_info_clear(&fileInfo);
  if (!name) {
    return NS_ERROR_NOT_AVAILABLE;
  }
 
  char* file = gnome_icon_theme_lookup_icon(gIconTheme, name, iconSize,
                                            NULL, NULL);
  g_free(name);
  if (!file)
    return NS_ERROR_NOT_AVAILABLE;

  
  GError *err = nsnull;
  GdkPixbuf* buf = gdk_pixbuf_new_from_file(file, &err);
  g_free(file);
  if (!buf) {
    if (err)
      g_error_free(err);
    return NS_ERROR_UNEXPECTED;
  }

  GdkPixbuf* scaled = buf;
  if (gdk_pixbuf_get_width(buf)  != iconSize &&
      gdk_pixbuf_get_height(buf) != iconSize) {
    
    scaled = gdk_pixbuf_scale_simple(buf, iconSize, iconSize,
                                     GDK_INTERP_BILINEAR);
    gdk_pixbuf_unref(buf);
    if (!scaled)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  rv = moz_gdk_pixbuf_to_channel(scaled, aIconURI,
                                 getter_AddRefs(mRealChannel));
  gdk_pixbuf_unref(scaled);
  return rv;
}

nsresult
nsIconChannel::Init(nsIURI* aURI)
{
  nsCOMPtr<nsIMozIconURI> iconURI = do_QueryInterface(aURI);
  NS_ASSERTION(iconURI, "URI is not an nsIMozIconURI");

  nsCAutoString stockIcon;
  iconURI->GetStockIcon(stockIcon);
  if (stockIcon.IsEmpty()) {
    return InitWithGnome(iconURI);
  }

  nsCAutoString iconSizeString;
  iconURI->GetIconSize(iconSizeString);

  nsCAutoString iconStateString;
  iconURI->GetIconState(iconStateString);

  GtkIconSize icon_size = moz_gtk_icon_size(iconSizeString.get());
   
  ensure_stock_image_widget();

  gboolean sensitive = strcmp(iconStateString.get(), "disabled");
  gtk_widget_set_sensitive (gStockImageWidget, sensitive);

  GdkPixbuf *icon = gtk_widget_render_icon(gStockImageWidget, stockIcon.get(),
                                           icon_size, NULL);
#if GTK_CHECK_VERSION(2,4,0)
  if (!icon) {
    ensure_icon_factory();
      
    GtkIconSet *icon_set = gtk_icon_set_new();
    GtkIconSource *icon_source = gtk_icon_source_new();
    
    gtk_icon_source_set_icon_name(icon_source, stockIcon.get());
    gtk_icon_set_add_source(icon_set, icon_source);
    gtk_icon_factory_add(gIconFactory, stockIcon.get(), icon_set);
    gtk_icon_set_unref(icon_set);
    gtk_icon_source_free(icon_source);

    icon = gtk_widget_render_icon(gStockImageWidget, stockIcon.get(),
                                  icon_size, NULL);
  }
#endif

  if (!icon)
    return NS_ERROR_NOT_AVAILABLE;
  
  nsresult rv = moz_gdk_pixbuf_to_channel(icon, iconURI,
                                          getter_AddRefs(mRealChannel));

  gdk_pixbuf_unref(icon);

  return rv;
}

void
nsIconChannel::Shutdown() {
  if (gProtoWindow) {
    gtk_widget_destroy(gProtoWindow);
    gProtoWindow = nsnull;
    gStockImageWidget = nsnull;
  }
  if (gIconTheme) {
    g_object_unref(G_OBJECT(gIconTheme));
    gIconTheme = nsnull;
  }
}
