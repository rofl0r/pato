#include <stddef.h>

#include "../concol/rgb.h"

typedef struct {
	char* data;
	size_t w;
	size_t h;
} Image;

Image* getWorldImage(void);
void img_fillcolor(Image* img, rgb_t color);
Image* img_new(size_t w, size_t h);
Image* img_scale(Image* img, size_t zoomFactor_w, size_t zoomFactor_h);
void img_embed(Image* dest, Image* source, size_t x, size_t y);