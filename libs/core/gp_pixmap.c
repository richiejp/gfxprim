// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2009-2011 Jiri "BlueBear" Dluhos
 *                         <jiri.bluebear.dluhos@gmail.com>
 *
 * Copyright (C) 2009-2013 Cyril Hrubis <metan@ucw.cz>
 */

#include <errno.h>
#include <string.h>

#include <core/gp_debug.h>
#include <core/gp_transform.h>
#include <core/gp_pixel.h>
#include <core/gp_get_put_pixel.h>
#include <core/gp_gamma.h>
#include "core/gp_pixmap.h"
#include <core/gp_blit.h>

static uint32_t get_bpr(uint32_t bpp, uint32_t w)
{
	uint64_t bits_per_row = (uint64_t)bpp * w;
	uint8_t padd = !!(bits_per_row % 8);

	if (bits_per_row / 8 + padd > UINT32_MAX) {
		GP_WARN("Pixmap too wide %u (overflow detected)", w);
		return 0;
	}

	return bits_per_row / 8 + padd;
}

gp_pixmap *gp_pixmap_alloc(gp_size w, gp_size h, gp_pixel_type type)
{
	gp_pixmap *pixmap;
	uint32_t bpp;
	size_t bpr;
	void *pixels;

	if (!GP_VALID_PIXELTYPE(type)) {
		GP_WARN("Invalid pixel type %u", type);
		errno = EINVAL;
		return NULL;
	}

	if (w <= 0 || h <= 0) {
		GP_WARN("Trying to allocate pixmap with zero width and/or height");
		errno = EINVAL;
		return NULL;
	}

	GP_DEBUG(1, "Allocating pixmap %u x %u - %s",
	         w, h, gp_pixel_type_name(type));

	bpp = gp_pixel_size(type);

	if (!(bpr = get_bpr(bpp, w)))
		return NULL;

	size_t size = bpr * h;

	if (size / h != bpr) {
		GP_WARN("Pixmap too big %u x %u (owerflow detected)", w, h);
		return NULL;
	}

	pixels = malloc(size);
	pixmap = malloc(sizeof(gp_pixmap));

	if (pixels == NULL || pixmap == NULL) {
		free(pixels);
		free(pixmap);
		GP_WARN("Malloc failed :(");
		errno = ENOMEM;
		return NULL;
	}

	pixmap->pixels        = pixels;
	pixmap->bpp           = bpp;
	pixmap->bytes_per_row = bpr;
	pixmap->offset        = 0;

	pixmap->w = w;
	pixmap->h = h;

	pixmap->gamma = NULL;

	pixmap->pixel_type = type;
	#warning Hmm, bit endianity... Why is not this settled by different pixel types?
	pixmap->bit_endian = gp_pixel_types[type].bit_endian;

	/* rotation and mirroring */
	gp_pixmap_set_rotation(pixmap, 0, 0, 0);

	pixmap->free_pixels = 1;

	return pixmap;
}

int gp_pixmap_set_gamma(gp_pixmap *self, float gamma)
{
	gp_gamma_release(self->gamma);

	self->gamma = gp_gamma_acquire(self->pixel_type, gamma);

	return !self->gamma;
}

void gp_pixmap_free(gp_pixmap *pixmap)
{
	GP_DEBUG(1, "Freeing pixmap (%p)", pixmap);

	if (pixmap == NULL)
		return;

	if (pixmap->free_pixels)
		free(pixmap->pixels);

	if (pixmap->gamma)
		gp_gamma_release(pixmap->gamma);

	free(pixmap);
}

gp_pixmap *gp_pixmap_init(gp_pixmap *pixmap, gp_size w, gp_size h,
                          gp_pixel_type type, void *pixels)
{
	uint32_t bpp = gp_pixel_size(type);
	uint32_t bpr = get_bpr(bpp, w);

	pixmap->pixels        = pixels;
	pixmap->bpp           = bpp;
	pixmap->bytes_per_row = bpr;
	pixmap->offset        = 0;

	pixmap->w = w;
	pixmap->h = h;

	pixmap->pixel_type = type;
	pixmap->bit_endian = 0;

	pixmap->gamma = NULL;

	/* rotation and mirroring */
	gp_pixmap_set_rotation(pixmap, 0, 0, 0);

	pixmap->free_pixels = 0;

	return pixmap;
}

int gp_pixmap_resize(gp_pixmap *pixmap, gp_size w, gp_size h)
{
	uint32_t bpr = get_bpr(pixmap->bpp, w);
	void *pixels;

	pixels = realloc(pixmap->pixels, bpr * h);

	if (pixels == NULL)
		return 1;

	pixmap->w = w;
	pixmap->h = h;
	pixmap->bytes_per_row = bpr;
	pixmap->pixels = pixels;

	return 0;
}

gp_pixmap *gp_pixmap_copy(const gp_pixmap *src, int flags)
{
	gp_pixmap *new;
	uint8_t *pixels;

	if (src == NULL)
		return NULL;

	new = malloc(sizeof(gp_pixmap));
	pixels = malloc(src->bytes_per_row * src->h);

	if (pixels == NULL || new == NULL) {
		free(pixels);
		free(new);
		GP_WARN("Malloc failed :(");
		errno = ENOMEM;
		return NULL;
	}

	new->pixels = pixels;

	if (flags & GP_COPY_WITH_PIXELS)
		memcpy(pixels, src->pixels, src->bytes_per_row * src->h);

	new->bpp           = src->bpp;
	new->bytes_per_row = src->bytes_per_row;
	new->offset        = 0;

	new->w = src->w;
	new->h = src->h;

	new->pixel_type = src->pixel_type;
	new->bit_endian = src->bit_endian;

	if (flags & GP_COPY_WITH_ROTATION)
		gp_pixmap_copy_rotation(src, new);
	else
		gp_pixmap_set_rotation(new, 0, 0, 0);

	//TODO: Copy the gamma too
	new->gamma = NULL;

	new->free_pixels = 1;

	return new;
}


gp_pixmap *gp_pixmap_convert_alloc(const gp_pixmap *src,
                                   gp_pixel_type dst_pixel_type)
{
	int w = gp_pixmap_w(src);
	int h = gp_pixmap_h(src);

	gp_pixmap *ret = gp_pixmap_alloc(w, h, dst_pixel_type);

	if (ret == NULL)
		return NULL;

	/*
	 * Fill the buffer with zeroes, otherwise it will
	 * contain random data which will generate mess
	 * when converting image with alpha channel.
	 */
	memset(ret->pixels, 0, ret->bytes_per_row * ret->h);

	gp_blit(src, 0, 0, w, h, ret, 0, 0);

	return ret;
}

gp_pixmap *gp_pixmap_convert(const gp_pixmap *src, gp_pixmap *dst)
{
	//TODO: Asserts
	int w = gp_pixmap_w(src);
	int h = gp_pixmap_h(src);

	/*
	 * Fill the buffer with zeroes, otherwise it will
	 * contain random data which will generate mess
	 * when converting image with alpha channel.
	 */
	memset(dst->pixels, 0, dst->bytes_per_row * dst->h);

	gp_blit(src, 0, 0, w, h, dst, 0, 0);

	return dst;
}

gp_pixmap *gp_sub_pixmap_alloc(const gp_pixmap *pixmap,
                               gp_coord x, gp_coord y, gp_size w, gp_size h)
{
	gp_pixmap *res = malloc(sizeof(gp_pixmap));

	if (res == NULL) {
		GP_WARN("Malloc failed :(");
		errno = ENOMEM;
		return NULL;
	}

	return gp_sub_pixmap(pixmap, res, x, y, w, h);
}

gp_pixmap *gp_sub_pixmap(const gp_pixmap *pixmap, gp_pixmap *subpixmap,
                         gp_coord x, gp_coord y, gp_size w, gp_size h)
{
	GP_CHECK(pixmap, "NULL pixmap");

	GP_TRANSFORM_RECT(pixmap, x, y, w, h);

	GP_CHECK(pixmap->w >= x + w, "Subpixmap w out of original pixmap.");
	GP_CHECK(pixmap->h >= y + h, "Subpixmap h out of original pixmap.");

	subpixmap->bpp           = pixmap->bpp;
	subpixmap->bytes_per_row = pixmap->bytes_per_row;
	subpixmap->offset        = (pixmap->offset +
	                            gp_pixel_addr_offset(x, pixmap->pixel_type)) % 8;

	subpixmap->w = w;
	subpixmap->h = h;

	subpixmap->pixel_type = pixmap->pixel_type;
	subpixmap->bit_endian = pixmap->bit_endian;

	/* gamma */
	subpixmap->gamma = pixmap->gamma;

	/* rotation and mirroring */
	gp_pixmap_copy_rotation(pixmap, subpixmap);

	subpixmap->pixels = GP_PIXEL_ADDR(pixmap, x, y);

	subpixmap->free_pixels = 0;

	return subpixmap;
}

void gp_pixmap_print_info(const gp_pixmap *self)
{
	printf("Pixmap info\n");
	printf("------------\n");
	printf("Size\t%ux%u\n", self->w, self->h);
	printf("BPP\t%u\n", self->bpp);
	printf("BPR\t%u\n", self->bytes_per_row);
	printf("Pixel\t%s (%u)\n", gp_pixel_type_name(self->pixel_type),
	       self->pixel_type);
	printf("Offset\t%u (only unaligned pixel types)\n", self->offset);
	printf("Flags\taxes_swap=%u x_swap=%u y_swap=%u free_pixels=%u\n",
	       self->axes_swap, self->x_swap, self->y_swap, self->free_pixels);

	if (self->gamma)
		gp_gamma_print(self->gamma);
}

/*
 * The pixmap rotations consists of two cyclic permutation groups that are
 * mirrored.
 *
 * The flags change as follows:
 *
 * One group:
 *
 * x_swap y_swap axes_swap
 *      0      0         0
 *      1      0         1
 *      1      1         0
 *      0      1         1
 *
 * And mirrored group:
 *
 * x_swap y_swap axes_swap
 *      0      0         1
 *      1      0         0
 *      1      1         1
 *      0      1         0
 *
 */
void gp_pixmap_rotate_cw(gp_pixmap *pixmap)
{
	pixmap->axes_swap = !pixmap->axes_swap;

	if (!pixmap->x_swap && !pixmap->y_swap) {
		pixmap->x_swap = 1;
		return;
	}

	if (pixmap->x_swap && !pixmap->y_swap) {
		pixmap->y_swap = 1;
		return;
	}

	if (pixmap->x_swap && pixmap->y_swap) {
		pixmap->x_swap = 0;
		return;
	}

	pixmap->y_swap  = 0;
}

void gp_pixmap_rotate_ccw(gp_pixmap *pixmap)
{
	pixmap->axes_swap = !pixmap->axes_swap;

	if (!pixmap->x_swap && !pixmap->y_swap) {
		pixmap->y_swap = 1;
		return;
	}

	if (pixmap->x_swap && !pixmap->y_swap) {
		pixmap->x_swap = 0;
		return;
	}

	if (pixmap->x_swap && pixmap->y_swap) {
		pixmap->y_swap = 0;
		return;
	}

	pixmap->x_swap  = 1;
}

int gp_pixmap_equal(const gp_pixmap *pixmap1, const gp_pixmap *pixmap2)
{
	if (pixmap1->pixel_type != pixmap2->pixel_type)
		return 0;

	if (gp_pixmap_w(pixmap1) != gp_pixmap_w(pixmap2))
		return 0;

	if (gp_pixmap_h(pixmap1) != gp_pixmap_h(pixmap2))
		return 0;

	gp_coord x, y, w = gp_pixmap_w(pixmap1), h = gp_pixmap_h(pixmap1);

	for (x = 0; x < w; x++)
		for (y = 0; y < h; y++)
			if (gp_getpixel(pixmap1, x, y) != gp_getpixel(pixmap2, x, y))
				return 0;

	return 1;
}
