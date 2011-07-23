#include <string.h>
#include <assert.h>

#include <leptonica/allheaders.h>


#include "imginterface.h"


Image* img_new(size_t w, size_t h) {
	Image* result;
	result = malloc(sizeof(Image) + (w * h * 4));
	result->data = (char*) result + sizeof(Image);
	result->h = h;
	result->w = w;
	return result;
}

Image* getWorldImage(void) {
	struct Pix* pix32;
	struct Pix* pngfile = pixRead("world3.png");
	
	int w, h;

	Image* result;
	pixGetDimensions(pngfile, &w, &h, NULL);
	pix32 = pixConvertTo32(pngfile);
	result = img_new(w, h);
	memcpy(result->data, pix32->data, w * h * 4);
	pixDestroy(&pngfile);
	pixDestroy(&pix32);
	return result;
}

void img_fillcolor(Image* img, rgb_t color) {
	int* out = (int*) img->data;
	size_t h, w;
	for(h = 0; h < img->h; h++) {
		for(w = 0; w < img->w; w++) {
			*(out++) = color.asInt;
		}
	}
}

Image* img_scale(Image* img, size_t zoomFactor_w, size_t zoomFactor_h) {
	Image* result = img_new(img->w * zoomFactor_w, img->h * zoomFactor_h);
	int* in = (int*) img->data;
	int* out = (int*) result->data;
	size_t h, w, zh, zw;
	for(h = 0; h < img->h; h++) {
		for(w = 0; w < img->w; w++) {
			for(zh = 0; zh < zoomFactor_h; zh++) {
				for(zw = 0; zw < zoomFactor_w; zw++) {
					*(out + (((h * zoomFactor_h + zh) * result->w) + (w * zoomFactor_w) + zw)) = *in;
				}
			}
			in++;
		}
	}
	return result;
}

void img_embed(Image* dest, Image* source, size_t x, size_t y) {
	assert(source && dest && x + source->w < dest->w && y + source->h < dest->h);
	int* out;
	int* in = (int*) source->data;
	size_t w, h;
	for(h = 0; h < source->h; h++) {
		out = ((int*) dest->data) + ((h + y) * dest->w) + x;
		for(w = 0; w < source->w; w++) {
			*(out++) = *(in++);
		}
	}
}