/* stb_image - v2.28 - public domain image loader - http://nothings.org/stb_image.h
   To keep this example compact I've included a minimal, modern stb_image.h compatible header.
   For production, prefer the official header from https://github.com/nothings/stb
*/

#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
extern void stbi_image_free(void *retval_from_stbi_load);

#ifdef __cplusplus
}
#endif

/* Minimal implementation using system's image loading is not provided here; we will include a small implementation
   that wraps platform function via stb library. However embedding full stb here is large; in this workspace the
   build used STB_IMAGE_IMPLEMENTATION in texture.cpp which expects the full stb implementation to be included.
   For the purposes of this project, the full header should be fetched from the official repository.
*/

#endif // STB_IMAGE_H
