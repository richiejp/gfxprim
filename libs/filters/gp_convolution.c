// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2009-2012 Cyril Hrubis <metan@ucw.cz>
 */

#include <core/gp_debug.h>
#include <filters/gp_linear.h>
#include <filters/gp_convolution.h>

int gp_filter_convolution_ex(const gp_pixmap *src,
                             gp_coord x_src, gp_coord y_src,
                             gp_size w_src, gp_coord h_src,
                             gp_pixmap *dst,
                             gp_coord x_dst, gp_coord y_dst,
                             const gp_filter_kernel_2d *kernel,
                             gp_progress_cb *callback)
{
	GP_CHECK(src->pixel_type == dst->pixel_type);

	/* Check that destination is large enough */
	GP_CHECK(x_dst + (gp_coord)w_src <= (gp_coord)dst->w);
	GP_CHECK(y_dst + (gp_coord)h_src <= (gp_coord)dst->h);

	/* The source pixel coordinates are clamped inside of the filter */

	GP_DEBUG(1, "Linear convolution kernel size %ux%u",
	         kernel->w, kernel->h);

	return gp_filter_linear_convolution_raw(src, x_src, y_src, w_src, h_src,
	                                        dst, x_dst, y_dst, kernel->kernel,
	                                        kernel->w, kernel->h, kernel->div,
					        callback);
}

gp_pixmap *gp_filter_convolution_ex_alloc(const gp_pixmap *src,
                                          gp_coord x_src, gp_coord y_src,
                                          gp_size w_src, gp_size h_src,
                                          const gp_filter_kernel_2d *kernel,
                                          gp_progress_cb *callback)
{
	gp_pixmap *ret = gp_pixmap_alloc(w_src, h_src, src->pixel_type);

	GP_DEBUG(1, "Linear convolution kernel size %ux%u",
	         kernel->w, kernel->h);

	if (ret == NULL)
		return NULL;

	if (gp_filter_linear_convolution_raw(src, x_src, y_src, w_src, h_src,
	                                     ret, 0, 0, kernel->kernel, kernel->w,
					     kernel->h, kernel->div, callback)) {
		gp_pixmap_free(ret);
		return NULL;
	}

	return ret;
}
