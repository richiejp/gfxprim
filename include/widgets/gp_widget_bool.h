//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_BOOL_H
#define GP_WIDGET_BOOL_H

struct gp_widget_bool {
	const char *label;
	int val;
	int type;
	char payload[];
};

#endif /* GP_WIDGET_BOOL_H */
