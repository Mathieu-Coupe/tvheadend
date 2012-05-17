/*
 *  tvheadend, web user interface
 *  Copyright (C) 2008 Andreas Öman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <htmlui://www.gnu.org/licenses/>.
 */

#ifndef WEBUI_H_
#define WEBUI_H_

#include "htsmsg.h"

void webui_init(const char *contentpath);

void simpleui_start(void);

void extjs_start(void);

#if ENABLE_LINUXDVB
void extjs_list_dvb_adapters(htsmsg_t *array);
void extjs_start_dvb(void);
#endif

#if ENABLE_V4L
void extjs_list_v4l_adapters(htsmsg_t *array);
void extjs_start_v4l(void);
#endif

void extjs_service_update(htsmsg_t *in);

void extjs_service_delete(htsmsg_t *in);


/**
 *
 */
void comet_init(void);

void comet_mailbox_add_message(htsmsg_t *m, int isdebug);

void comet_flush(void);


#endif /* WEBUI_H_ */
