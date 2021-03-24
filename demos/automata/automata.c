// SPDX-License-Identifier: GPL-2.1-or-later
/*
 * Copyright (C) 2021 Richard Palethorpe (richiejp.com)
 */

 /*

   Automata Demo.

  */

#include <stdlib.h>
#include <errno.h>
#include <gfxprim.h>

#define BITN(b, n) (b & (1UL << n) ? ~0UL : 0UL)

/* One of 8 unique patterns in a rule.
 *
 * Note that the values should be a full or empty bit mask, that is
 * ~0UL or 0.
 */
struct ca1d_pattern {
	/* Whether the rule matches on this pattern */
	uint64_t active;
	/* The state of the left neighbor to match */
	uint64_t left;
	/* The state of the current cell to match */
	uint64_t center;
	/* The state of the right cell to match */
	uint64_t right;
};

/* A 1D Cellular Automata rule. There are 8 patterns in each rule
 * which may be combined to create 256 unique rules.
 */
struct ca1d_rule {
	struct ca1d_pattern patterns[8];
} static ca1d_rules[256];

/* Number of bitfields in a row */
static size_t width = 1;
/* Number of steps in the simulation */
static size_t height = 64;
/* Matrix of bitfields representing the automata's state over time */
static uint64_t *steps;
/* Initial conditions */
static uint64_t *init;
/* Zero mask */
static uint64_t *zeroes;
/* Numeric representation of the current update rule */
static int rule = 110;
/* Whether to use the reversible version of the current rule */
static int reversible;

static void *uids;

/* Create the pattern masks in advance */
static void ca1d_preprocess(void)
{
	struct ca1d_pattern *p;
	int r, pi;

	for (r = 0; r < 256; r++) {
		for (pi = 0; pi < 8; pi++) {
			p = ca1d_rules[r].patterns + pi;

			p->active = BITN(r, pi);
			p->left   = BITN(pi, 2);
			p->center = BITN(pi, 1);
			p->right  = BITN(pi, 0);
		}
	}
}

static void ca1d_allocate(void)
{
	if (init)
		gp_vec_free(init);
	init = gp_vec_new(width, sizeof(uint64_t));
	init[width / 2] = 1UL << (63 - (width * 32) % 64);

	if (zeroes)
		gp_vec_free(zeroes);
	zeroes = gp_vec_new(width, sizeof(uint64_t));

	if (steps)
		gp_vec_free(steps);
	steps = gp_matrix_new(width, height, sizeof(uint64_t));
}

static inline uint64_t ca1d_rule_apply(const struct ca1d_rule *rule,
				       uint64_t c_prev, uint64_t c, uint64_t c_next,
				       uint64_t c_prev_step)
{
	int i;
	const struct ca1d_pattern *p;
	uint64_t l = (c >> 1) ^ (c_prev << 63);
	uint64_t r = (c << 1) ^ (c_next >> 63);
	uint64_t cn = 0;

	for (i = 0; i < 8; i++) {
		p = rule->patterns + i;

		cn |= p->active &
			~(p->left ^ l) & ~(p->center ^ c) & ~(p->right ^ r);
	}

	return cn ^ c_prev_step;
}

static inline void ca1d_rule_apply_row(const struct ca1d_rule *r,
				       const uint64_t *prev,
				       const uint64_t *cur,
				       uint64_t *next)
{
	size_t i;

	next[0] = ca1d_rule_apply(r, cur[width - 1], cur[0],
				  cur[GP_MIN(1, width - 1)], prev[0]);

	for (i = 1; i < width - 1; i++) {
		next[i] = ca1d_rule_apply(r, cur[i - 1], cur[i], cur[i + 1],
					  prev[i]);
	}

	if (i >= width)
		return;

	next[i] = ca1d_rule_apply(r, cur[i - 1], cur[i], cur[0], prev[i]);
}

static void ca1d_run(void)
{
	const struct ca1d_rule *r = ca1d_rules + rule;
	const uint64_t *prev = zeroes;
	const uint64_t *cur = steps;
	uint64_t *next = steps + gp_matrix_idx(width, 1, 0);
	size_t i = 1;

	memcpy(steps, init, width * sizeof(uint64_t));

	for (;;) {
		ca1d_rule_apply_row(r, prev, cur, next);

		if (++i >= height)
			break;

		prev = reversible ? cur : zeroes;
		cur = next;
		next = steps + gp_matrix_idx(width, i, 0);
	}

}

static void fill_pixmap(gp_pixmap *p)
{
	size_t i, j, k;
	gp_coord cw = p->w / (64 * width);
	gp_coord ch = p->h / height;
	gp_pixel fill = gp_rgb_to_pixmap_pixel(0xff, 0x00, 0x00, p);
	gp_pixel bg = gp_rgb_to_pixmap_pixel(0xff, 0xff, 0xff, p);
	gp_pixel fg = gp_rgb_to_pixmap_pixel(0x00, 0x00, 0x00, p);
	gp_pixel px;
	uint64_t s, t;

	s = gp_time_stamp();
	gp_fill(p, fill);
	t = gp_time_stamp();

	printf("Fill time %lums\n", t - s);

	s = gp_time_stamp();
	ca1d_run();
	t = gp_time_stamp();

	printf("Automata time %lums\n", t - s);

	s = gp_time_stamp();
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			for (k = 0; k < 64; k++) {
				px = steps[gp_matrix_idx(width, i, j)] &
					(1UL << (63 - k)) ? fg : bg;
				gp_fill_rect_xywh(p, cw * (j * 64 + k), ch * i,
						  cw, ch, px);
			}
		}
	}
	t = gp_time_stamp();

	printf("Fill rects time %lums\n", t - s);
}

static void allocate_backing_pixmap(gp_widget_event *ev)
{
	gp_widget *w = ev->self;

	gp_pixmap_free(w->pixmap->pixmap);

	w->pixmap->pixmap = gp_pixmap_alloc(w->w, w->h, ev->ctx->pixel_type);

	fill_pixmap(w->pixmap->pixmap);
}

int pixmap_on_event(gp_widget_event *ev)
{
	gp_widget_event_dump(ev);

	switch (ev->type) {
	case GP_WIDGET_EVENT_RESIZE:
		allocate_backing_pixmap(ev);
	break;
	default:
	break;
	}

	return 0;
}

static void pixmap_do_redraw(void)
{
	gp_widget *pixmap = gp_widget_by_uid(uids, "pixmap", GP_WIDGET_PIXMAP);

	fill_pixmap(pixmap->pixmap->pixmap);
	gp_widget_redraw(pixmap);
}

int rule_widget_on_event(gp_widget_event *ev)
{
	struct gp_widget_tbox *tb = ev->self->tbox;
	char tbuf[4] = { 0 };
	char c;
	int r;

	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	switch(ev->self->type) {
	case GP_WIDGET_TBOX:
		switch(ev->sub_type) {
		case GP_WIDGET_TBOX_FILTER:
			c = (char)ev->val;

			if (c < '0' || c > '9')
				return 1;

			if (!gp_vec_strlen(tb->buf))
				return 0;

			strcpy(tbuf, tb->buf);
			tbuf[tb->cur_pos] = c;

			r = strtol(tbuf, NULL, 10);

			return r > 255;
			break;
		case GP_WIDGET_TBOX_EDIT:
			rule = strtol(tb->buf, NULL, 10);
			break;
		default:
			break;
		}
		break;
	case GP_WIDGET_CHECKBOX:
		reversible = ev->self->checkbox->val;
		break;
	default:
		return 0;
	}

	pixmap_do_redraw();

	return 0;
}

static void init_from_text(void)
{
	gp_widget *self = gp_widget_by_uid(uids, "init", GP_WIDGET_TBOX);
	const char *text = gp_widget_tbox_text(self);
	size_t len = gp_vec_strlen(text);

	memset(init, 0, width * sizeof(uint64_t));

	if (!len)
		init[width / 2] = 1UL << (63 - (width * 32) % 64);
	else
		memcpy(init, text, GP_MIN(width * sizeof(uint64_t), len));
}

int width_widget_on_event(gp_widget_event *ev)
{
	struct gp_widget_tbox *tb = ev->self->tbox;
	char tbuf[3] = { 0 };
	char c;
	int r;

	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	switch(ev->sub_type) {
	case GP_WIDGET_TBOX_FILTER:
		c = (char)ev->val;

		if (c < '0' || c > '9')
			return 1;

		strcpy(tbuf, tb->buf);
		tbuf[tb->cur_pos] = c;

		r = strtol(tbuf, NULL, 10);

		return r < 1;
		break;
	case GP_WIDGET_TBOX_EDIT:
		if (!gp_vec_strlen(tb->buf))
			return 0;

		width = strtol(tb->buf, NULL, 10);
		ca1d_allocate();
		init_from_text();
		pixmap_do_redraw();
		break;
	default:
		break;
	}

	return 0;
}

int height_widget_on_event(gp_widget_event *ev)
{
	struct gp_widget_tbox *tb = ev->self->tbox;
	char c;

	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	switch(ev->sub_type) {
	case GP_WIDGET_TBOX_FILTER:
		c = (char)ev->val;

		return c < '0' || c > '9';

		break;
	case GP_WIDGET_TBOX_EDIT:
		if (!gp_vec_strlen(tb->buf))
			return 0;

		height = GP_MAX(2, strtol(tb->buf, NULL, 10));
		ca1d_allocate();
		init_from_text();
		pixmap_do_redraw();
		break;
	default:
		break;
	}

	return 0;
}

int init_widget_on_event(gp_widget_event *ev)
{

	if (ev->type != GP_WIDGET_EVENT_WIDGET)
		return 0;

	switch(ev->sub_type) {
	case GP_WIDGET_TBOX_EDIT:
		init_from_text();
		pixmap_do_redraw();
		break;
	default:
		break;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	gp_widget *layout = gp_widget_layout_json("automata.json", &uids);

	if (!layout)
		return 0;

	gp_widget *pixmap = gp_widget_by_uid(uids, "pixmap", GP_WIDGET_PIXMAP);

	gp_widget_event_unmask(pixmap, GP_WIDGET_EVENT_RESIZE);

	ca1d_preprocess();
	ca1d_allocate();
	gp_widgets_main_loop(layout, "Pixmap example", NULL, argc, argv);

	return 0;
}
