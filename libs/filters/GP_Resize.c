/*****************************************************************************
 * This file is part of gfxprim library.                                     *
 *                                                                           *
 * Gfxprim is free software; you can redistribute it and/or                  *
 * modify it under the terms of the GNU Lesser General Public                *
 * License as published by the Free Software Foundation; either              *
 * version 2.1 of the License, or (at your option) any later version.        *
 *                                                                           *
 * Gfxprim is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Lesser General Public License for more details.                           *
 *                                                                           *
 * You should have received a copy of the GNU Lesser General Public          *
 * License along with gfxprim; if not, write to the Free Software            *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,                        *
 * Boston, MA  02110-1301  USA                                               *
 *                                                                           *
 * Copyright (C) 2009-2013 Cyril Hrubis <metan@ucw.cz>                       *
 *                                                                           *
 *****************************************************************************/

#include <math.h>

#include "core/GP_Context.h"
#include "core/GP_GetPutPixel.h"
#include "core/GP_Gamma.h"

#include "core/GP_Debug.h"

#include "GP_Resize.h"

/* See GP_ResizeNN.gen.c */
int GP_FilterResizeNN_Raw(const GP_Context *src, GP_Context *dst,
                          GP_ProgressCallback *callback);

int GP_FilterResizeNN(const GP_Context *src, GP_Context *dst,
                           GP_ProgressCallback *callback)
{
	GP_ASSERT(src->pixel_type == dst->pixel_type);

	return GP_FilterResizeNN_Raw(src, dst, callback);
}

GP_Context *GP_FilterResizeNNAlloc(const GP_Context *src,
                                   GP_Size w, GP_Size h,
                                   GP_ProgressCallback *callback)
{
	GP_Context *res = GP_ContextAlloc(w, h, src->pixel_type);

	if (res == NULL)
		return NULL;

	if (GP_FilterResizeNN_Raw(src, res, callback)) {
		GP_ContextFree(res);
		return NULL;
	}

	return res;
}

/* See GP_ResizeCubic.gen.c */
int GP_FilterResizeCubicInt_Raw(const GP_Context *src, GP_Context *dst,
                                GP_ProgressCallback *callback);

int GP_FilterResizeCubicInt(const GP_Context *src, GP_Context *dst,
                            GP_ProgressCallback *callback)
{
	GP_ASSERT(src->pixel_type == dst->pixel_type);

	return GP_FilterResizeCubicInt_Raw(src, dst, callback);
}

GP_Context *GP_FilterResizeCubicIntAlloc(const GP_Context *src,
                                         GP_Size w, GP_Size h,
                                         GP_ProgressCallback *callback)
{
	GP_Context *res = GP_ContextAlloc(w, h, src->pixel_type);

	if (res == NULL)
		return NULL;

	if (GP_FilterResizeCubicInt_Raw(src, res, callback)) {
		GP_ContextFree(res);
		return NULL;
	}

	return res;
}

/* See GP_ResizeLinear.gen.c */
int GP_FilterResizeLinearInt_Raw(const GP_Context *src, GP_Context *dst,
                                 GP_ProgressCallback *callback);

int GP_FilterResizeLinearLFInt_Raw(const GP_Context *src, GP_Context *dst,
                                   GP_ProgressCallback *callback);

int GP_FilterResizeLinearInt(const GP_Context *src, GP_Context *dst,
                             GP_ProgressCallback *callback)
{
	GP_ASSERT(src->pixel_type == dst->pixel_type);

	return GP_FilterResizeLinearInt_Raw(src, dst, callback);
}

int GP_FilterResizeLinearLFInt(const GP_Context *src, GP_Context *dst,
                               GP_ProgressCallback *callback)
{
	GP_ASSERT(src->pixel_type == dst->pixel_type);

	return GP_FilterResizeLinearInt_Raw(src, dst, callback);
}

GP_Context *GP_FilterResizeLinearIntAlloc(const GP_Context *src,
                                          GP_Size w, GP_Size h,
                                          GP_ProgressCallback *callback)
{
	GP_Context *res = GP_ContextAlloc(w, h, src->pixel_type);

	if (res == NULL)
		return NULL;

	if (GP_FilterResizeCubicInt_Raw(src, res, callback)) {
		GP_ContextFree(res);
		return NULL;
	}

	return res;
}

GP_Context *GP_FilterResizeLinearLFIntAlloc(const GP_Context *src,
                                            GP_Size w, GP_Size h,
                                            GP_ProgressCallback *callback)
{
	GP_Context *res = GP_ContextAlloc(w, h, src->pixel_type);

	if (res == NULL)
		return NULL;

	if (GP_FilterResizeLinearLFInt_Raw(src, res, callback)) {
		GP_ContextFree(res);
		return NULL;
	}

	return res;
}

static const char *interp_types[] = {
	"Nearest Neighbour",
	"Linear (Int)",
	"Linear with Low Pass (Int)",
	"Cubic (Float)",
	"Cubic (Int)",
};

const char *GP_InterpolationTypeName(enum GP_InterpolationType interp_type)
{
	if (interp_type > GP_INTERP_MAX)
		return "Unknown";

	return interp_types[interp_type]; 
}

#define A 0.5

static float cubic(float x)
{
	if (x < 0)
		x = -x;

	if (x < 1)
		return (2 - A)*x*x*x + (A - 3)*x*x + 1;

	if (x < 2)
		return -A*x*x*x + 5*A*x*x - 8*A*x + 4*A;

	return 0;
}

typedef float v4sf __attribute__ ((vector_size (sizeof(float) * 4)));

typedef union v4f {
	v4sf v;
	float f[4];
} v4f;

#define GP_USE_GCC_VECTOR

#ifdef GP_USE_GCC_VECTOR
#define MUL_V4SF(a, b) ({v4f ret; ret.v = (a).v * (b).v; ret;})
#else
#define MUL_V4SF(a, b) ({v4f ret; \
                         ret.f[0] = (a).f[0] * (b).f[0]; \
                         ret.f[1] = (a).f[1] * (b).f[1]; \
                         ret.f[2] = (a).f[2] * (b).f[2]; \
                         ret.f[3] = (a).f[3] * (b).f[3]; \
						 ret;})
#endif /* GP_USE_GCC_VECTOR */

#define SUM_V4SF(a)    ((a).f[0] + (a).f[1] + (a).f[2] + (a).f[3])

#define CLAMP(val) do {    \
	if (val < 0)       \
		val = 0;   \
	if (val > 255)     \
		val = 255; \
} while (0)

int GP_FilterInterpolate_Cubic(const GP_Context *src, GP_Context *dst,
                               GP_ProgressCallback *callback)
{
	float col_r[src->h], col_g[src->h], col_b[src->h];
	uint32_t i, j;

	if (src->pixel_type != GP_PIXEL_RGB888)
		return 1;

	GP_DEBUG(1, "Scaling image %ux%u -> %ux%u %2.2f %2.2f",
	            src->w, src->h, dst->w, dst->h,
		    1.00 * dst->w / src->w, 1.00 * dst->h / src->h);

	for (i = 0; i < dst->w; i++) {
		float x = (1.00 * i / (dst->w - 1)) * (src->w - 1);
		v4f cvx;
		int xi[4];
		
		xi[0] = floor(x - 1);
		xi[1] = x;
		xi[2] = x + 1;
		xi[3] = x + 2;
		
		cvx.f[0] = cubic(xi[0] - x);
		cvx.f[1] = cubic(xi[1] - x);
		cvx.f[2] = cubic(xi[2] - x);
		cvx.f[3] = cubic(xi[3] - x);
		
		if (xi[0] < 0)
			xi[0] = 0;

		if (xi[2] >= (int)src->w)
			xi[2] = src->w - 1;
		
		if (xi[3] >= (int)src->w)
			xi[3] = src->w - 1;
		
		/* Generate interpolated column */
		for (j = 0; j < src->h; j++) {
			v4f rv, gv, bv;
			GP_Pixel pix[4];

			pix[0] = GP_GetPixel_Raw_24BPP(src, xi[0], j);
			pix[1] = GP_GetPixel_Raw_24BPP(src, xi[1], j);
			pix[2] = GP_GetPixel_Raw_24BPP(src, xi[2], j);
			pix[3] = GP_GetPixel_Raw_24BPP(src, xi[3], j);
				
			rv.f[0] = GP_Pixel_GET_R_RGB888(pix[0]);
			rv.f[1] = GP_Pixel_GET_R_RGB888(pix[1]);
			rv.f[2] = GP_Pixel_GET_R_RGB888(pix[2]);
			rv.f[3] = GP_Pixel_GET_R_RGB888(pix[3]);
			
			gv.f[0] = GP_Pixel_GET_G_RGB888(pix[0]);
			gv.f[1] = GP_Pixel_GET_G_RGB888(pix[1]);
			gv.f[2] = GP_Pixel_GET_G_RGB888(pix[2]);
			gv.f[3] = GP_Pixel_GET_G_RGB888(pix[3]);
			
			bv.f[0] = GP_Pixel_GET_B_RGB888(pix[0]);
			bv.f[1] = GP_Pixel_GET_B_RGB888(pix[1]);
			bv.f[2] = GP_Pixel_GET_B_RGB888(pix[2]);
			bv.f[3] = GP_Pixel_GET_B_RGB888(pix[3]);

			rv = MUL_V4SF(rv, cvx);
			gv = MUL_V4SF(gv, cvx);
			bv = MUL_V4SF(bv, cvx);
			
			col_r[j] = SUM_V4SF(rv);
			col_g[j] = SUM_V4SF(gv);
			col_b[j] = SUM_V4SF(bv);
		}

		/* now interpolate column for new image */
		for (j = 0; j < dst->h; j++) {
			float y = (1.00 * j / (dst->h - 1)) * (src->h - 1);
			v4f cvy, rv, gv, bv;
			float r, g, b;
			int yi[4];
		
			yi[0] = floor(y - 1);
			yi[1] = y;
			yi[2] = y + 1;
			yi[3] = y + 2;
			
			cvy.f[0] = cubic(yi[0] - y);
			cvy.f[1] = cubic(yi[1] - y);
			cvy.f[2] = cubic(yi[2] - y);
			cvy.f[3] = cubic(yi[3] - y);
		
			if (yi[0] < 0)
				yi[0] = 0;
		
			if (yi[2] >= (int)src->h)
				yi[2] = src->h - 1;
		
			if (yi[3] >= (int)src->h)
				yi[3] = src->h - 1;
			
			rv.f[0] = col_r[yi[0]];
			rv.f[1] = col_r[yi[1]];
			rv.f[2] = col_r[yi[2]];
			rv.f[3] = col_r[yi[3]];
			
			gv.f[0] = col_g[yi[0]];
			gv.f[1] = col_g[yi[1]];
			gv.f[2] = col_g[yi[2]];
			gv.f[3] = col_g[yi[3]];
			
			bv.f[0] = col_b[yi[0]];
			bv.f[1] = col_b[yi[1]];
			bv.f[2] = col_b[yi[2]];
			bv.f[3] = col_b[yi[3]];
			
			rv = MUL_V4SF(rv, cvy);
			gv = MUL_V4SF(gv, cvy);
			bv = MUL_V4SF(bv, cvy);

			r = SUM_V4SF(rv);
			g = SUM_V4SF(gv);
			b = SUM_V4SF(bv);

			CLAMP(r);
			CLAMP(g);
			CLAMP(b);

			GP_Pixel pix = GP_Pixel_CREATE_RGB888((uint8_t)r, (uint8_t)g, (uint8_t)b);
			GP_PutPixel_Raw_24BPP(dst, i, j, pix);
		}
		
		if (GP_ProgressCallbackReport(callback, i, dst->w, dst->h))
			return 1;
	}

	GP_ProgressCallbackDone(callback);
	return 0;
}

int GP_FilterResize_Raw(const GP_Context *src, GP_Context *dst,
                        GP_InterpolationType type,
                        GP_ProgressCallback *callback)
{
	switch (type) {
	case GP_INTERP_NN:
		return GP_FilterResizeNN_Raw(src, dst, callback);
	case GP_INTERP_LINEAR_INT:
		return GP_FilterResizeLinearInt_Raw(src, dst, callback);
	case GP_INTERP_LINEAR_LF_INT:
		return GP_FilterResizeLinearLFInt_Raw(src, dst, callback);
	case GP_INTERP_CUBIC:
		return GP_FilterInterpolate_Cubic(src, dst, callback);
	case GP_INTERP_CUBIC_INT:
		return GP_FilterResizeCubicInt_Raw(src, dst, callback);
	}

	return 1;
}

GP_Context *GP_FilterResize(const GP_Context *src, GP_Context *dst,
                            GP_InterpolationType type,
                            GP_Size w, GP_Size h,
                            GP_ProgressCallback *callback)
{
	GP_Context sub, *res;

	if (dst == NULL) {
		res = GP_ContextAlloc(w, h, src->pixel_type);

		if (res == NULL)
			return NULL;
	} else {
		GP_ASSERT(src->pixel_type == dst->pixel_type,
		          "The src and dst pixel types must match");
		/*
		 * The size of w and h is asserted in subcontext initalization
		 */	
		res = GP_SubContext(dst, &sub, 0, 0, w, h);
	}

	/*
	 * Operation was aborted by progress callback.
	 * 
	 * Free any alloacted data and exit.
	 */
	if (GP_FilterResize_Raw(src, res, type, callback)) {
		GP_DEBUG(1, "Operation aborted");
		
		if (dst == NULL)
			GP_ContextFree(dst);

		return NULL;
	}

	return dst == NULL ? res : dst;
}
