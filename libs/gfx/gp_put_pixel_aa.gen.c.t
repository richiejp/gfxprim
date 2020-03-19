// SPDX-License-Identifier: GPL-2.1-or-later
/*
 * Copyright (C) 2009-2013 Cyril Hrubis <metan@ucw.cz>
 */

/* Anti Aliased Put Pixel */

#include "core/gp_pixmap.h"
#include <core/gp_mix_pixels.h>
#include <core/gp_fixed_point.h>
#include <core/gp_gamma_correction.h>

#include <gfx/gp_hline.h>
#include <gfx/gp_vline.h>

#define FP_TO_PERC(a) (GP_FP_ROUND_TO_INT((a) * 255))

void gp_putpixel_aa_raw(gp_pixmap *pixmap, gp_coord x, gp_coord y,
                        gp_pixel pixel)
{
	gp_coord int_x = GP_FP_TO_INT(x);
	gp_coord int_y = GP_FP_TO_INT(y);
	gp_coord frac_x = GP_FP_FRAC(x);
	gp_coord frac_y = GP_FP_FRAC(y);
	uint8_t perc;

	perc = FP_TO_PERC(GP_FP_MUL(GP_FP_1 - frac_x, GP_FP_1 - frac_y));
	gp_mix_pixel_raw_clipped(pixmap, int_x, int_y, pixel, perc);

	perc = FP_TO_PERC(GP_FP_MUL(frac_x, GP_FP_1 - frac_y));
	gp_mix_pixel_raw_clipped(pixmap, int_x + 1, int_y, pixel, perc);

	perc = FP_TO_PERC(GP_FP_MUL(GP_FP_1 - frac_x, frac_y));
	gp_mix_pixel_raw_clipped(pixmap, int_x, int_y + 1, pixel, perc);

	perc = FP_TO_PERC(GP_FP_MUL(frac_x, frac_y));
	gp_mix_pixel_raw_clipped(pixmap, int_x + 1, int_y + 1, pixel, perc);
}

void gp_putpixel_aa_raw_clipped(gp_pixmap *pixmap, gp_coord x, gp_coord y,
                                gp_pixel pixel)
{
	gp_putpixel_aa_raw(pixmap, x, y, pixel);
}

void gp_putpixel_aa(gp_pixmap *pixmap, gp_coord x, gp_coord y, gp_pixel pixel)
{
	GP_TRANSFORM_POINT_FP(pixmap, x, y);

	gp_putpixel_aa_raw_clipped(pixmap, x, y, pixel);
}