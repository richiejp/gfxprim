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
static size_t width = 20;
/* Number of steps in the simulation */
static size_t height = 1280;
/* Matrix of bitfields representing the automata's state over time */
static uint64_t *steps;
static int rule = 110;
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

static inline uint64_t ca1d_rule_apply(const struct ca1d_rule *rule,
				       uint64_t c_prev, uint64_t c, uint64_t c_next)
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

	return cn;
}

static void ca1d_rule_apply_row(const struct ca1d_rule *r, int step, size_t len)
{
	size_t i;
	const uint64_t *row = steps + gp_matrix_idx(len, step, 0);
	uint64_t *next = steps + gp_matrix_idx(len, step + 1, 0);

	next[0] = ca1d_rule_apply(r, row[len - 1], row[0], row[GP_MIN(1, len - 1)]);

	for (i = 1; i < len - 1; i++)
		next[i] = ca1d_rule_apply(r, row[i - 1], row[i], row[i + 1]);

	if (i < len)
		next[i] = ca1d_rule_apply(r, row[i - 1], row[i], row[0]);
}

static void ca1d_run(void)
{
	const struct ca1d_rule *r = ca1d_rules + rule;
	size_t i;

	steps[width / 2] = 1UL << (63 - (width * 32) % 64);

	for (i = 0; i < height - 1; i++)
		ca1d_rule_apply_row(r, i, width);

}

static void fill_pixmap(gp_pixmap *p)
{
	size_t i, j, k;
	gp_coord cw = p->w / (64 * width) + 1;
	gp_coord ch = p->h / height + 1;
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

int rule_tbox_on_event(gp_widget_event *ev)
{
	struct gp_widget_tbox *tb = ev->self->tbox;
	gp_widget *pixmap;
	char tbuf[4] = { 0 };
	char c;
	int r;

	gp_widget_event_dump(ev);

	switch(ev->type) {
	case GP_WIDGET_EVENT_WIDGET:
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
			pixmap = gp_widget_by_uid(uids, "pixmap", GP_WIDGET_PIXMAP);
			fill_pixmap(pixmap->pixmap->pixmap);
			gp_widget_redraw(pixmap);
			break;
		default:
			break;
		}
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
	steps = gp_matrix_new(width, height, sizeof(uint64_t));
	gp_widgets_main_loop(layout, "Pixmap example", NULL, argc, argv);

	return 0;
}

// Create window

// Draw squares

// Draw 1-d automata

// Draw all 1-d automata

// Draw reversible 1-d automata
