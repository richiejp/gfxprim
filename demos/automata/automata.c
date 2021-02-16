// SPDX-License-Identifier: GPL-2.1-or-later
/*
 * Copyright (C) 2021 Richard Palethorpe (richiejp.com)
 */

 /*

   Automata Demo.

  */

#include <gfxprim.h>

static void *uids;

int pixmap_on_event

int main(int argc, char *argv[])
{
	gp_widget *layout = gp_widget_layout_json("automata.json", &uids);

	if (!layout)
		return 0;

	gp_widget *pixmap = gp_widget_by_uid(uids, "pixmap", GP_WIDGET_PIXMAP);

	gp_widget_event_unmask(pixmap, GP_WIDGET_EVENT_RESIZE);
	gp_widget_event_unmask(pixmap, GP_WIDGET_EVENT_INPUT);

	gp_widgets_main_loop(layout, "Pixmap example", NULL, argc, argv);

	return 0;
}

// Create window

// Draw squares

// Draw 1-d automata

// Draw all 1-d automata

// Draw reversible 1-d automata
