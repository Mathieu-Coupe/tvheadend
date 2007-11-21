/*
 *  Output functions for fixed multicast streaming
 *  Copyright (C) 2007 Andreas �man
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "tvhead.h"
#include "iptv_output.h"
#include "dispatch.h"
#include "channels.h"
#include "subscriptions.h"
#include "tsmux.h"

#define MULTICAST_PKT_SIZ (188 * 7)

typedef struct output_multicast {
  void *om_muxer;
  int om_fd;
  struct sockaddr_in om_dst;
} output_multicast_t;

/**
 *  Output MPEG TS
 */
void
iptv_output_ts(void *opaque, th_subscription_t *s, 
              uint8_t *pkt, int blocks, int64_t pcr)
{
  output_multicast_t *om = opaque;

  sendto(om->om_fd, pkt, blocks * 188, 0, 
        (struct sockaddr *)&om->om_dst, sizeof(struct sockaddr_in));
}


/**
 * Called when a subscription gets/loses access to a transport
 */
static void
iptv_subscription_callback(struct th_subscription *s,
			   subscription_event_t event, void *opaque)
{
  output_multicast_t *om = opaque;

  switch(event) {
  case TRANSPORT_AVAILABLE:
    assert(om->om_muxer == NULL);
    om->om_muxer = ts_muxer_init(s, iptv_output_ts, om, 0);
    ts_muxer_play(om->om_muxer, 0);
    break;

  case TRANSPORT_UNAVAILABLE:
    ts_muxer_deinit(om->om_muxer);
    om->om_muxer = NULL;
    break;
  }
}



/**
 * Setup IPTV (TS over UDP) output
 */
static void
output_multicast_load(struct config_head *head)
{
  const char *name, *s, *b;
  th_channel_t *ch;
  output_multicast_t *om;
  int ttl = 32;
  struct sockaddr_in sin;
  char title[100];
  char title2[100];

  if((name = config_get_str_sub(head, "channel", NULL)) == NULL)
    return;

  ch = channel_find(name, 1);

  om = calloc(1, sizeof(output_multicast_t));
  
  if((b = config_get_str_sub(head, "interface-address", NULL)) == NULL) {
    fprintf(stderr, "no interface address configured\n");
    goto err;
  }
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = 0;
  sin.sin_addr.s_addr = inet_addr(b);

  if((s = config_get_str_sub(head, "group-address", NULL)) == NULL) {
    fprintf(stderr, "no group address configured\n");
    goto err;
  }
  om->om_dst.sin_addr.s_addr = inet_addr(s);

  if((s = config_get_str_sub(head, "port", NULL)) == NULL) {
    fprintf(stderr, "no port configured\n");
    goto err;
  }
  om->om_dst.sin_port = htons(atoi(s));

  om->om_fd = socket(AF_INET, SOCK_DGRAM, 0);

  if(bind(om->om_fd, (struct sockaddr *)&sin, sizeof(sin))==-1) {
    fprintf(stderr, "cannot bind to %s\n", b);
    close(om->om_fd);
    goto err;
  }

  if((s = config_get_str_sub(head, "ttl", NULL)) != NULL)
    ttl = atoi(s);

  setsockopt(om->om_fd, SOL_IP, IP_MULTICAST_TTL, &ttl, sizeof(int));

  snprintf(title, sizeof(title), "%s:%d", inet_ntoa(om->om_dst.sin_addr),
	   ntohs(om->om_dst.sin_port));

  syslog(LOG_INFO, "Static multicast output: \"%s\" to %s, source %s ",
	 ch->ch_name, title, inet_ntoa(sin.sin_addr));

  snprintf(title2, sizeof(title2), "IPTV-OUT: %s", title);

  subscription_create(ch, 900, title2, iptv_subscription_callback, om);
  return;

 err:
  free(om);
}



void 
output_multicast_setup(void)
{
  config_entry_t *ce;
  
  TAILQ_FOREACH(ce, &config_list, ce_link) {
    if(ce->ce_type == CFG_SUB && !strcasecmp("multicast-output", ce->ce_key)) {
      output_multicast_load(&ce->ce_sub);
    }
  }
}
