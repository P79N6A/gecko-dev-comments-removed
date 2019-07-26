


















#include "pngpriv.h"

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)

void 
png_destroy_png_struct(png_structrp png_ptr)
{
   if (png_ptr != NULL)
   {
      


      png_struct dummy_struct = *png_ptr;
      memset(png_ptr, 0, (sizeof *png_ptr));
      png_free(&dummy_struct, png_ptr);

#     ifdef PNG_SETJMP_SUPPORTED
         
         png_free_jmpbuf(&dummy_struct);
#     endif
   }
}







PNG_FUNCTION(png_voidp,PNGAPI
png_calloc,(png_const_structrp png_ptr, png_alloc_size_t size),PNG_ALLOCATED)
{
   png_voidp ret;

   ret = png_malloc(png_ptr, size);

   if (ret != NULL)
      memset(ret, 0, size);

   return ret;
}






PNG_FUNCTION(png_voidp ,
png_malloc_base,(png_const_structrp png_ptr, png_alloc_size_t size),
   PNG_ALLOCATED)
{
   




#ifdef PNG_USER_MEM_SUPPORTED
   PNG_UNUSED(png_ptr)
#endif
   if (size > 0 && size <= PNG_SIZE_MAX
#     ifdef PNG_MAX_MALLOC_64K
         && size <= 65536U
#     endif
      )
   {
#ifdef PNG_USER_MEM_SUPPORTED
      if (png_ptr != NULL && png_ptr->malloc_fn != NULL)
         return png_ptr->malloc_fn(png_constcast(png_structrp,png_ptr), size);

      else
#endif
         return malloc((size_t)size); 
   }

   else
      return NULL;
}





static png_voidp
png_malloc_array_checked(png_const_structrp png_ptr, int nelements,
   size_t element_size)
{
   png_alloc_size_t req = nelements; 

   if (req <= PNG_SIZE_MAX/element_size)
      return png_malloc_base(png_ptr, req * element_size);

   
   return NULL;
}

PNG_FUNCTION(png_voidp ,
png_malloc_array,(png_const_structrp png_ptr, int nelements,
   size_t element_size),PNG_ALLOCATED)
{
   if (nelements <= 0 || element_size == 0)
      png_error(png_ptr, "internal error: array alloc");

   return png_malloc_array_checked(png_ptr, nelements, element_size);
}

PNG_FUNCTION(png_voidp ,
png_realloc_array,(png_const_structrp png_ptr, png_const_voidp old_array,
   int old_elements, int add_elements, size_t element_size),PNG_ALLOCATED)
{
   
   if (add_elements <= 0 || element_size == 0 || old_elements < 0 ||
      (old_array == NULL && old_elements > 0))
      png_error(png_ptr, "internal error: array realloc");

   


   if (add_elements <= INT_MAX - old_elements)
   {
      png_voidp new_array = png_malloc_array_checked(png_ptr,
         old_elements+add_elements, element_size);

      if (new_array != NULL)
      {
         


         if (old_elements > 0)
            memcpy(new_array, old_array, element_size*(unsigned)old_elements);

         memset((char*)new_array + element_size*(unsigned)old_elements, 0,
            element_size*(unsigned)add_elements);

         return new_array;
      }
   }

   return NULL; 
}





PNG_FUNCTION(png_voidp,PNGAPI
png_malloc,(png_const_structrp png_ptr, png_alloc_size_t size),PNG_ALLOCATED)
{
   png_voidp ret;

   if (png_ptr == NULL)
      return NULL;

   ret = png_malloc_base(png_ptr, size);

   if (ret == NULL)
       png_error(png_ptr, "Out of memory"); 

   return ret;
}

#ifdef PNG_USER_MEM_SUPPORTED
PNG_FUNCTION(png_voidp,PNGAPI
png_malloc_default,(png_const_structrp png_ptr, png_alloc_size_t size),
   PNG_ALLOCATED PNG_DEPRECATED)
{
   png_voidp ret;

   if (png_ptr == NULL)
      return NULL;

   
   ret = png_malloc_base(NULL, size);

   if (ret == NULL)
      png_error(png_ptr, "Out of Memory"); 

   return ret;
}
#endif 





PNG_FUNCTION(png_voidp,PNGAPI
png_malloc_warn,(png_const_structrp png_ptr, png_alloc_size_t size),
   PNG_ALLOCATED)
{
   if (png_ptr != NULL)
   {
      png_voidp ret = png_malloc_base(png_ptr, size);

      if (ret != NULL)
         return ret;

      png_warning(png_ptr, "Out of memory");
   }

   return NULL;
}




void PNGAPI
png_free(png_const_structrp png_ptr, png_voidp ptr)
{
   if (png_ptr == NULL || ptr == NULL)
      return;

#ifdef PNG_USER_MEM_SUPPORTED
   if (png_ptr->free_fn != NULL)
      png_ptr->free_fn(png_constcast(png_structrp,png_ptr), ptr);

   else
      png_free_default(png_ptr, ptr);
}

PNG_FUNCTION(void,PNGAPI
png_free_default,(png_const_structrp png_ptr, png_voidp ptr),PNG_DEPRECATED)
{
   if (png_ptr == NULL || ptr == NULL)
      return;
#endif

   free(ptr);
}

#ifdef PNG_USER_MEM_SUPPORTED



void PNGAPI
png_set_mem_fn(png_structrp png_ptr, png_voidp mem_ptr, png_malloc_ptr
  malloc_fn, png_free_ptr free_fn)
{
   if (png_ptr != NULL)
   {
      png_ptr->mem_ptr = mem_ptr;
      png_ptr->malloc_fn = malloc_fn;
      png_ptr->free_fn = free_fn;
   }
}





png_voidp PNGAPI
png_get_mem_ptr(png_const_structrp png_ptr)
{
   if (png_ptr == NULL)
      return NULL;

   return png_ptr->mem_ptr;
}
#endif 
#endif 
