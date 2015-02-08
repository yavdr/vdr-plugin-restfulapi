/*
 * Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __FEMONSERVICE_H
#define __FEMONSERVICE_H

#include <linux/dvb/frontend.h>

struct FemonService_v1_0 {
  cString fe_name;
  cString fe_status;
  uint16_t fe_snr;
  uint16_t fe_signal;
  uint32_t fe_ber;
  uint32_t fe_unc;
  double video_bitrate;
  double audio_bitrate;
  double dolby_bitrate;
  };

#endif //__FEMONSERVICE_H

