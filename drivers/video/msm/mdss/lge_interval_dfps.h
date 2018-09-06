/*
 * Interval DFPS: Dynamic FPS based on frame interval.
 *
 * Copyright (C) 2016 LGE, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef LGE_INTERVAL_DFPS_H
#define LGE_INTERVAL_DFPS_H

#include <linux/workqueue.h>
#include "mdss_mdp.h"

#define MIN_INTERVAL (USEC_PER_SEC/60)
#define MAX_INTERVAL (USEC_PER_SEC/1)
#define DEFAULT_WSIZE (4)

struct lge_interval_dfps {
	u32 fps;
	u32 *iwin;
	u32 interval;
	u32 upthresh;
	u32 downthresh;
	u32 wavg;
	u32 wsum;
	ktime_t pre_ts;
	ktime_t cur_ts;
	int w_idx;
	int w_sz;
	int enable;
	int inited;
	struct mutex rw_lock;
	struct delayed_work idle_work;
	/* Add needed data type */
	struct fb_info **fbi_list;
	struct msm_fb_data_type *mfd;
	struct mdss_overlay_private *mdp5_data;
	struct mdss_mdp_ctl *ctl;
	struct mdss_panel_data *pdata;
};

struct lge_interval_dfps *lge_get_idfps_data(void);
int lge_dfps_interval_notify(ktime_t cur_us);
int lge_dfps_input_notify(void);
ssize_t dfps_show(struct device *dev,
		  struct device_attribute *attr, char *buf);
ssize_t dfps_store(struct device *dev,
		   struct device_attribute *attr, const char *buf,
		   size_t count);
ssize_t dfps_en_show(struct device *dev,
		     struct device_attribute *attr, char *buf);
ssize_t dfps_en_store(struct device *dev,
		      struct device_attribute *attr, const char *buf,
		      size_t count);
ssize_t dfps_interval_show(struct device *dev,
			   struct device_attribute *attr, char *buf);
ssize_t dfps_wsize_show(struct device *dev,
			struct device_attribute *attr, char *buf);
ssize_t dfps_wsize_store(struct device *dev,
			 struct device_attribute *attr, const char *buf,
			 size_t count);

#endif /* LGE_INTERVAL_DFPS_H */
