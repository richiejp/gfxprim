// SPDX-License-Identifier: GPL-2.1-or-later
/*
 * Copyright (C) 2021 Richard Palethorpe (richiejp.com)
 */

 /*

   Automata Demo.

  */

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

static uint64_t *steps;
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

static inline uint64_t ca1d_rule_apply(const struct ca1d_rule *rule, uint64_t c)
{
	int i;
	const struct ca1d_pattern *p;
	uint64_t l = (c >> 1) ^ (c << 63);
	uint64_t r = (c << 1) ^ (c >> 63);
	uint64_t cn = 0;

	for (i = 0; i < 8; i++) {
		p = rule->patterns + i;

		cn |= p->active &
			~(p->left ^ l) & ~(p->center ^ c) & ~(p->right ^ r);
	}

	return cn;
}

static void ca1d_run(const struct ca1d_rule *rule)
{
	size_t i;

	steps[0] = 1UL << 31;

	for (i = 0; i < gp_vec_len(steps) - 1; i++)
		steps[i + 1] = ca1d_rule_apply(rule, steps[i]);
}

static void fill_pixmap(gp_pixmap *p)
{
	size_t i, j;
	gp_coord cw = p->w / 64 + 1;
	gp_coord ch = p->h / gp_vec_len(steps) + 1;
	gp_pixel bg = gp_rgb_to_pixmap_pixel(0xff, 0xff, 0xff, p);
	gp_pixel fg = gp_rgb_to_pixmap_pixel(0x00, 0x00, 0x00, p);

	gp_fill(p, bg);

	ca1d_run(ca1d_rules + 30);

	for (i = 0; i < gp_vec_len(steps); i++) {
		for (j = 0; j < 64; j++) {
			gp_fill_rect_xywh(p, cw * j, ch * i, cw, ch,
					  steps[i] & (1UL << j) ? fg : bg);
		}
	}
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

int main(int argc, char *argv[])
{
	gp_widget *layout = gp_widget_layout_json("automata.json", &uids);

	if (!layout)
		return 0;

	gp_widget *pixmap = gp_widget_by_uid(uids, "pixmap", GP_WIDGET_PIXMAP);

	gp_widget_event_unmask(pixmap, GP_WIDGET_EVENT_RESIZE);

	ca1d_preprocess();
	steps = gp_vec_new(64, sizeof(uint64_t));
	gp_widgets_main_loop(layout, "Pixmap example", NULL, argc, argv);

	return 0;
}

// Create window

// Draw squares

// Draw 1-d automata

// Draw all 1-d automata

// Draw reversible 1-d automata
