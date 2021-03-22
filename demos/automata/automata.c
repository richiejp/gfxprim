// SPDX-License-Identifier: GPL-2.1-or-later
/*
 * Copyright (C) 2021 Richard Palethorpe (richiejp.com)
 */

 /*

   Automata Demo.

  */

#include <gfxprim.h>

static void *uids;

static void fill_pixmap(gp_pixmap *p)
{
	gp_pixel col = gp_rgb_to_pixmap_pixel(0xff, 0xff, 0xff, p);

	gp_fill(p, col);
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

	gp_widgets_main_loop(layout, "Pixmap example", NULL, argc, argv);

	return 0;
}

// Create window

// Draw squares

// Draw 1-d automata

// Draw all 1-d automata

// Draw reversible 1-d automata
