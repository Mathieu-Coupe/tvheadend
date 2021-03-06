/*
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


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "tvheadend.h"
#include "streaming.h"
#include "dvr.h"
#include "mkts.h"
#include "psi.h"

/**
 *
 */
struct mk_ts {
  int fd;
  char *filename;
  int error;
  char *title;
};

/**
 *
 */
mk_ts_t *
mk_ts_create(const char *filename,
		  const struct streaming_start *ss,
	      const struct dvr_entry *de)
{
  mk_ts_t *mkr;
  int fd;
  dvr_config_t *cfg = dvr_config_find_by_name_default(de->de_config_name);

  fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if(fd == -1)
    return NULL;

  mkr = calloc(1, sizeof(struct mk_ts));
  mkr->filename = strdup(filename);
  mkr->fd = fd;
  mkr->title = strdup(de->de_title);

  // XXX Code from webui.c, it doesn't work with HD stream (H264+EAC3) and XBMC
  if (strcmp(cfg->dvr_format, "mpegts") == 0) {
        streaming_tsbuf_t tsbuf;
        struct {
          uint8_t pat_ts[188];
          uint8_t pmt_ts[188];
        } s;
	int pcrpid = ss->ss_pcr_pid;
	int pmtpid = 0x0fff;

	tsbuf.ts_cnt = 2;
	tsbuf.ts_data = (uint8_t *)&s;

	//Build PAT
	memset(s.pat_ts, 0xff, 188);
	psi_build_pat(NULL, s.pat_ts+5, 183, pmtpid);
	s.pat_ts[0] = 0x47;
	s.pat_ts[1] = 0x40;
	s.pat_ts[2] = 0x00;
	s.pat_ts[3] = 0x10;
	s.pat_ts[4] = 0x00;

	//Build PMT
	memset(s.pmt_ts, 0xff, 188);
	psi_build_pmt(ss, s.pmt_ts+5, 183, pcrpid);
	s.pmt_ts[0] = 0x47;
	s.pmt_ts[1] = 0x40 | (pmtpid >> 8);
	s.pmt_ts[2] = pmtpid;
	s.pmt_ts[3] = 0x10;
	s.pmt_ts[4] = 0x00;

	//Write PAT+PMT
	mk_ts_write(mkr, &tsbuf);
	if(mkr->error)
	  return NULL;
  }

  return mkr;
}

/**
 *
 */
void
mk_ts_write(mk_ts_t *mkr, const streaming_tsbuf_t *tsbuf)
{
  if(!mkr->error) {
    if(!write(mkr->fd, tsbuf->ts_data, tsbuf->ts_cnt * 188)) {
      mkr->error = errno;
      tvhlog(LOG_ERR, "RAWTS", "%s: Unable to write -- %s",
        mkr->filename, strerror(errno));
	  return;
    }
  }
}

/**
 *
 */
void
mk_ts_close(mk_ts_t *mkr)
{
  close(mkr->fd);
  free(mkr->filename);
  free(mkr->title);
  free(mkr);
}
