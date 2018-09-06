/*
 * max98925.c -- ALSA SoC MAX98925 driver
 *
 * Copyright 2013-2014 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/max98925.h>

#ifdef CONFIG_LGE_EXTERNAL_SPEAKER
#include <linux/input/epack_audio.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#define max98925_connected audio_get_epack_state()
#else
#define max98925_connected true
#endif
#include "max98925.h"

#define DEBUG_MAX98925
#ifdef DEBUG_MAX98925
#define msg_maxim(format, args...)	\
pr_info("[MAX98925_DEBUG] %s: " format "\n", __func__, ## args)
#else
#define msg_maxim(format, args...)
#endif /* DEBUG_MAX98925 */

#ifdef CONFIG_LGE_EXTERNAL_SPEAKER
static int max98925_init_status = 0;
static int max98925_amp_status = 1;

int max98925_get_init_status(void)
{
	return max98925_init_status;
}
EXPORT_SYMBOL_GPL(max98925_get_init_status);
int max98925_get_spk_amp_status(void)
{
	return max98925_amp_status;
}
EXPORT_SYMBOL_GPL(max98925_get_spk_amp_status);
#endif

static void max98925_set_slave(struct max98925_priv *max98925);
static void max98925_handle_pdata(struct snd_soc_codec *codec);
static int max98925_set_clock(struct max98925_priv *max98925, unsigned int rate);

static int max98925_regmap_write(struct max98925_priv *max98925,
	unsigned int reg,
			       unsigned int val)
{
	int ret = 0;

	if(max98925_connected) {
		ret = regmap_write(max98925->regmap, reg, val);

		if (max98925->sub_regmap)
			ret = regmap_write(max98925->sub_regmap, reg, val);
	}
	return ret;
}

static int max98925_regmap_update_bits(struct max98925_priv *max98925,
	unsigned int reg,
		       unsigned int mask, unsigned int val)
{
	int ret = 0;

	if(max98925_connected) {
		ret = regmap_update_bits(max98925->regmap, reg, mask, val);

		if (max98925->sub_regmap)
			ret = regmap_update_bits(max98925->sub_regmap, reg, mask, val);
	}

	return ret;
}

static struct reg_default max98925_reg[] = {
	{ 0x00, 0x00 }, /* Battery Voltage Data */
	{ 0x01, 0x00 }, /* Boost Voltage Data */
	{ 0x02, 0x00 }, /* Live Status0 */
	{ 0x03, 0x00 }, /* Live Status1 */
	{ 0x04, 0x00 }, /* Live Status2 */
	{ 0x05, 0x00 }, /* State0 */
	{ 0x06, 0x00 }, /* State1 */
	{ 0x07, 0x00 }, /* State2 */
	{ 0x08, 0x00 }, /* Flag0 */
	{ 0x09, 0x00 }, /* Flag1 */
	{ 0x0A, 0x00 }, /* Flag2 */
	{ 0x0B, 0x00 }, /* IRQ Enable0 */
	{ 0x0C, 0x00 }, /* IRQ Enable1 */
	{ 0x0D, 0x00 }, /* IRQ Enable2 */
	{ 0x0E, 0x00 }, /* IRQ Clear0 */
	{ 0x0F, 0x00 }, /* IRQ Clear1 */
	{ 0x10, 0x00 }, /* IRQ Clear2 */
	{ 0x11, 0xC0 }, /* Map0 */
	{ 0x12, 0x00 }, /* Map1 */
	{ 0x13, 0x00 }, /* Map2 */
	{ 0x14, 0xF0 }, /* Map3 */
	{ 0x15, 0x00 }, /* Map4 */
	{ 0x16, 0xAB }, /* Map5 */
	{ 0x17, 0x89 }, /* Map6 */
	{ 0x18, 0x00 }, /* Map7 */
	{ 0x19, 0x00 }, /* Map8 */
	{ 0x1A, 0x06 }, /* DAI Clock Mode 1 */
	{ 0x1B, 0xC0 }, /* DAI Clock Mode 2 */
	{ 0x1C, 0x00 }, /* DAI Clock Divider Denominator MSBs */
	{ 0x1D, 0x00 }, /* DAI Clock Divider Denominator LSBs */
	{ 0x1E, 0xF0 }, /* DAI Clock Divider Numerator MSBs */
	{ 0x1F, 0x00 }, /* DAI Clock Divider Numerator LSBs */
	{ 0x20, 0x50 }, /* Format */
	{ 0x21, 0x00 }, /* TDM Slot Select */
	{ 0x22, 0x00 }, /* DOUT Configuration VMON */
	{ 0x23, 0x00 }, /* DOUT Configuration IMON */
	{ 0x24, 0x00 }, /* DOUT Configuration VBAT */
	{ 0x25, 0x00 }, /* DOUT Configuration VBST */
	{ 0x26, 0x00 }, /* DOUT Configuration FLAG */
	{ 0x27, 0xFF }, /* DOUT HiZ Configuration 1 */
	{ 0x28, 0xFF }, /* DOUT HiZ Configuration 2 */
	{ 0x29, 0xFF }, /* DOUT HiZ Configuration 3 */
	{ 0x2A, 0xFF }, /* DOUT HiZ Configuration 4 */
	{ 0x2B, 0x02 }, /* DOUT Drive Strength */
	{ 0x2C, 0x90 }, /* Filters */
	{ 0x2D, 0x00 }, /* Gain */
	{ 0x2E, 0x02 }, /* Gain Ramping */
	{ 0x2F, 0x00 }, /* Speaker Amplifier */
	{ 0x30, 0x0A }, /* Threshold */
	{ 0x31, 0x00 }, /* ALC Attack */
	{ 0x32, 0x80 }, /* ALC Atten and Release */
	{ 0x33, 0x00 }, /* ALC Infinite Hold Release */
	{ 0x34, 0x92 }, /* ALC Configuration */
	{ 0x35, 0x01 }, /* Boost Converter */
	{ 0x36, 0x00 }, /* Block Enable */
	{ 0x37, 0x00 }, /* Configuration */
	{ 0x38, 0x00 }, /* Global Enable */
	{ 0x3A, 0x00 }, /* Boost Limiter */
	{ 0xFF, 0x00 }, /* Revision ID */
};

static bool max98925_volatile_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98925_R000_VBAT_DATA:
	case MAX98925_R001_VBST_DATA:
	case MAX98925_R002_LIVE_STATUS0:
	case MAX98925_R003_LIVE_STATUS1:
	case MAX98925_R004_LIVE_STATUS2:
	case MAX98925_R005_STATE0:
	case MAX98925_R006_STATE1:
	case MAX98925_R007_STATE2:
	case MAX98925_R008_FLAG0:
	case MAX98925_R009_FLAG1:
	case MAX98925_R00A_FLAG2:
	case MAX98925_R0FF_VERSION:
		return true;
	default:
		return false;
	}
}

static bool max98925_readable_register(struct device *dev, unsigned int reg)
{
	if (MAX98925_R03A_BOOST_LIMITER < reg &&
			MAX98925_R0FF_VERSION > reg)
		return false;

	switch (reg) {
	case MAX98925_R00E_IRQ_CLEAR0:
	case MAX98925_R00F_IRQ_CLEAR1:
	case MAX98925_R010_IRQ_CLEAR2:
	case MAX98925_R033_ALC_HOLD_RLS:
		return false;
	default:
		return true;
	}
};

#ifdef USE_REG_DUMP
static void reg_dump(struct max98925_priv *max98925)
{
	int val_l;
	int i, j;
	int addr;

	static const struct {
		int start;
		int count;
	} reg_table[] = {
		{ 0x02, 0x03 },
		{ 0x1A, 0x1F },
		{ 0x3A, 0x01 },
		{ 0x00, 0x00 }
	};

	i = 0;
	if(max98925_connected) {
		while (reg_table[i].count != 0) {
			for (j = 0; j < reg_table[i].count; j++) {
				addr = j + reg_table[i].start;
				regmap_read(max98925->regmap, addr, &val_l);
				msg_maxim("reg 0x%02X, val_l 0x%02X",
						addr, val_l);
			}
			i++;
		}
	}
}
#endif /* USE_REG_DUMP */

#ifdef CONFIG_SND_SOC_MAXIM_DSM
#ifdef USE_DSM_LOG
static int max98925_get_dump_status(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = maxdsm_get_dump_status();
	return 0;
}
static int max98925_set_dump_status(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);

	int val;

	if(max98925_connected) {
		regmap_read(max98925->regmap,
			MAX98925_R038_GLOBAL_ENABLE, &val);
		msg_maxim("val:%d", val);

		if (val != 0)
			maxdsm_update_param();
	} else {
		msg_maxim("EpackSPK connection : %d", max98925_connected);
	}
	return 0;
}
static ssize_t max98925_log_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return maxdsm_log_prepare(buf);
}

static DEVICE_ATTR(dsm_log, S_IRUGO, max98925_log_show, NULL);
#endif /* USE_DSM_LOG */

#ifdef USE_DSM_UPDATE_CAL
static int max98925_get_dsm_param(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = maxdsm_cal_avail();
	return 0;
}

static int max98925_set_dsm_param(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	maxdsm_update_caldata(ucontrol->value.integer.value[0]);
	return 0;
}
static ssize_t max98925_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return maxdsm_cal_prepare(buf);
}
static DEVICE_ATTR(dsm_cal, S_IRUGO, max98925_cal_show, NULL);
#endif /* USE_DSM_UPDATE_CAL */

#if defined(USE_DSM_LOG) || defined(USE_DSM_UPDATE_CAL)
#define DEFAULT_LOG_CLASS_NAME "dsm"
static const char *class_name_log = DEFAULT_LOG_CLASS_NAME;

static struct attribute *max98925_attributes[] = {
#ifdef USE_DSM_LOG
	&dev_attr_dsm_log.attr,
#endif /* USE_DSM_LOG */
#ifdef USE_DSM_UPDATE_CAL
	&dev_attr_dsm_cal.attr,
#endif /* USE_DSM_UPDATE_CAL */
	NULL
};

static struct attribute_group max98925_attribute_group = {
	.attrs = max98925_attributes
};
#endif /* USE_DSM_LOG || USE_DSM_UPDATE_CAL */
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

static const unsigned int max98925_spk_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	1, 31, TLV_DB_SCALE_ITEM(-600, 100, 0),
};

static int max98925_spk_vol_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = max98925->volume;

	return 0;
}

static int max98925_spk_vol_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	unsigned int sel = ucontrol->value.integer.value[0];

	if(max98925_connected) {
		regmap_update_bits(max98925->regmap, MAX98925_R02D_GAIN,
				MAX98925_SPK_GAIN_MASK, sel << MAX98925_SPK_GAIN_SHIFT);

		max98925->volume = sel;
	} else {
		msg_maxim("EpackSPK connection : %d", max98925_connected);
	}

	return 0;
}

static int max98925_reg_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol, unsigned int reg,
		unsigned int mask, unsigned int shift)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	int data;

	if(max98925_connected) {
		regmap_read(max98925->regmap, reg, &data);

		ucontrol->value.integer.value[0] =
			(data & mask) >> shift;
	} else {
		msg_maxim("EpackSPK connection : %d", max98925_connected);
	}
	return 0;
}

static int max98925_reg_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol, unsigned int reg,
		unsigned int mask, unsigned int shift)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	unsigned int sel = ucontrol->value.integer.value[0];

	max98925_regmap_update_bits(max98925, reg, mask, sel << shift);

	return 0;
}

static int max98925_spk_ramp_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98925_reg_get(kcontrol, ucontrol, MAX98925_R02E_GAIN_RAMPING,
			MAX98925_SPK_RMP_EN_MASK, MAX98925_SPK_RMP_EN_SHIFT);
}

static int max98925_spk_ramp_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98925_reg_put(kcontrol, ucontrol, MAX98925_R02E_GAIN_RAMPING,
			MAX98925_SPK_RMP_EN_MASK, MAX98925_SPK_RMP_EN_SHIFT);
}

static int max98925_spk_zcd_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98925_reg_get(kcontrol, ucontrol, MAX98925_R02E_GAIN_RAMPING,
			MAX98925_SPK_ZCD_EN_MASK, MAX98925_SPK_ZCD_EN_SHIFT);
}

static int max98925_spk_zcd_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98925_reg_put(kcontrol, ucontrol, MAX98925_R02E_GAIN_RAMPING,
			MAX98925_SPK_ZCD_EN_MASK, MAX98925_SPK_ZCD_EN_SHIFT);
}

static int max98925_alc_en_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98925_reg_get(kcontrol, ucontrol, MAX98925_R030_THRESHOLD,
			MAX98925_ALC_EN_MASK, MAX98925_ALC_EN_SHIFT);
}

static int max98925_alc_en_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98925_reg_put(kcontrol, ucontrol, MAX98925_R030_THRESHOLD,
			MAX98925_ALC_EN_MASK, MAX98925_ALC_EN_SHIFT);
}

static int max98925_alc_threshold_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98925_reg_get(kcontrol, ucontrol, MAX98925_R030_THRESHOLD,
			MAX98925_ALC_TH_MASK, MAX98925_ALC_TH_SHIFT);
}

static int max98925_alc_threshold_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98925_reg_put(kcontrol, ucontrol, MAX98925_R030_THRESHOLD,
			MAX98925_ALC_TH_MASK, MAX98925_ALC_TH_SHIFT);
}

static const char * const max98925_bout_volt_text[] = {"8.5V", "8.25V",
	"8.0V", "7.75V", "7.5V", "7.25V", "7.0V", "6.75V", "6.5V"};

static const struct soc_enum max98925_boost_voltage_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(max98925_bout_volt_text),
		max98925_bout_volt_text),
};

static int max98925_bout_voltage_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol) {
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	unsigned int val;

	if(max98925_connected) {
		regmap_read(max98925->regmap,
			MAX98925_R037_CONFIGURATION, &val);
			ucontrol->value.integer.value[0] =
			(val & MAX98925_BST_VOUT_MASK)>>MAX98925_BST_VOUT_SHIFT;
	} else {
		msg_maxim("EpackSPK connection : %d", max98925_connected);
	}
	return 0;
}

static int max98925_bout_voltage_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol) {
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);

	max98925_regmap_update_bits(max98925,
		MAX98925_R037_CONFIGURATION,
		MAX98925_BST_VOUT_MASK,
		ucontrol->value.integer.value[0]<<MAX98925_BST_VOUT_SHIFT);

	return 0;
}

static const char * const spk_state_text[] = {"Disable", "Enable"};

static const struct soc_enum spk_state_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spk_state_text), spk_state_text),
};

static int max98925_spk_out_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol) {
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	unsigned int val;
	if(max98925_connected) {
		regmap_read(max98925->regmap,
				MAX98925_R038_GLOBAL_ENABLE, &val);
		ucontrol->value.integer.value[0] = !!(val & MAX98925_EN_MASK);

		msg_maxim("EpackSPK state '%s'",
				spk_state_text[ucontrol->value.integer.value[0]]);
	} else {
		msg_maxim("EpackSPK connection :%d", max98925_connected);
	}
	return 0;
}

static int max98925_spk_out_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol) {
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);

	if (ucontrol->value.integer.value[0])
		max98925_regmap_write(max98925,
				MAX98925_R038_GLOBAL_ENABLE,
				MAX98925_EN_MASK);
	else
		max98925_regmap_update_bits(max98925,
				MAX98925_R038_GLOBAL_ENABLE,
				MAX98925_EN_MASK, 0x0);

	msg_maxim("Speaker was set by '%s'",
			spk_state_text[!!(ucontrol->value.integer.value[0])]);

	return 0;
}

static int max98925_adc_en_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	int data;
	if(max98925_connected) {
		regmap_read(max98925->regmap, MAX98925_R036_BLOCK_ENABLE, &data);

		if (data & MAX98925_ADC_VIMON_EN_MASK)
			ucontrol->value.integer.value[0] = 1;
		else
			ucontrol->value.integer.value[0] = 0;
	} else {
		msg_maxim("EpackSPK connection :%d", max98925_connected);
	}
	return 0;
}

static int max98925_adc_en_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	struct max98925_pdata *pdata = max98925->pdata;
	int sel = (int)ucontrol->value.integer.value[0];

	if (sel)
		max98925_regmap_update_bits(max98925,
		MAX98925_R036_BLOCK_ENABLE,
		MAX98925_ADC_VIMON_EN_MASK,
		MAX98925_ADC_VIMON_EN_MASK);
	else
		max98925_regmap_update_bits(max98925,
		MAX98925_R036_BLOCK_ENABLE,
		MAX98925_ADC_VIMON_EN_MASK, 0);

	pdata->vstep.adc_status = !!sel;

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	maxdsm_update_feature_en_adc(!!sel);
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

	return 0;
}

static int max98925_adc_thres_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	struct max98925_pdata *pdata = max98925->pdata;

	ucontrol->value.integer.value[0] = pdata->vstep.adc_thres;

	return 0;
}

static int max98925_adc_thres_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	struct max98925_pdata *pdata = max98925->pdata;
	int ret = 0;

	if (ucontrol->value.integer.value[0] >= MAX98925_VSTEP_0 &&
			ucontrol->value.integer.value[0] <= MAX98925_VSTEP_15)
		pdata->vstep.adc_thres = (int)ucontrol->value.integer.value[0];
	else
		ret = -EINVAL;

	return ret;
}

static int max98925_volume_step_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	struct max98925_pdata *pdata = max98925->pdata;

	ucontrol->value.integer.value[0] = pdata->vstep.vol_step;

	return 0;
}

static int max98925_volume_step_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	struct max98925_pdata *pdata = max98925->pdata;

	int sel = (int)ucontrol->value.integer.value[0];
	unsigned int mask = 0;
	bool adc_status = pdata->vstep.adc_status;

	/*
	 * ADC status will be updated according to the volume.
	 * Under step 7 : Disable
	 * Over step 7  : Enable
	 */
	if (sel <= pdata->vstep.adc_thres
			&& pdata->vstep.adc_status) {
		max98925_regmap_update_bits(max98925,
				MAX98925_R036_BLOCK_ENABLE,
				MAX98925_ADC_VIMON_EN_MASK,
				0);
		adc_status = !pdata->vstep.adc_status;
	} else if (sel > pdata->vstep.adc_thres
			&& !pdata->vstep.adc_status) {
		max98925_regmap_update_bits(max98925,
				MAX98925_R036_BLOCK_ENABLE,
				MAX98925_ADC_VIMON_EN_MASK,
				MAX98925_ADC_VIMON_EN_MASK);
		adc_status = !pdata->vstep.adc_status;
	} else if (sel > MAX98925_VSTEP_MAX) {
		msg_maxim("Unknown value %d", sel);
		return -EINVAL;
	}

	if (adc_status != pdata->vstep.adc_status) {
		pdata->vstep.adc_status = adc_status;
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		maxdsm_update_feature_en_adc((int)adc_status);
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
	}

	/*
	 * Boost voltage will be updated according to the volume.
	 * Step 0 ~ Step 13 : 6.5V
	 * Step 14			: 8.0V
	 * Over step 15		: 8.5V
	 */
	mask |= pdata->vstep.boost_step[sel];
	mask <<= MAX98925_BST_VOUT_SHIFT;
	max98925_regmap_update_bits(max98925,
			MAX98925_R037_CONFIGURATION,
			MAX98925_BST_VOUT_MASK,
			mask);

	/* Set volume step to ... */
	pdata->vstep.vol_step = sel;

	return 0;
}

static int max98925_check_revID(struct regmap *regmap)
{
	int ret = 0;
	int loop;
	uint32_t reg = 0x00;
	uint32_t version_table[] = {
		MAX98925_VERSION,
		MAX98925_VERSION1,
		MAX98925_VERSION2,
		MAX98925_VERSION3,
	};

	if (max98925_connected) {
		regmap_read(regmap, MAX98925_R0FF_VERSION, &reg);
		for (loop = 0; loop < ARRAY_SIZE(version_table); loop++) {
			if (reg == version_table[loop])
				ret = reg;
		}
	}
	return ret;
}

static int max98925_check_version(struct max98925_priv *max98925)
{
	int rev_id = 0;

	rev_id = max98925_check_revID(max98925->regmap);
	pr_err("%s REV ID:0x%x\n", __func__, rev_id);

	return rev_id;
}

static void __max98925_dai_digital_mute(
		struct max98925_priv *max98925, int mute)
{
	int val = 0;
	if (mute) {
		if (max98925_connected) {
			regmap_read(max98925->regmap,
			MAX98925_R038_GLOBAL_ENABLE, &val);
			msg_maxim("EpackSPK enable register(0x38) val : 0x%x", val);
		}
		if(val) {
			msg_maxim("mute %d, epack_state %d", mute, max98925_connected);
		max98925_regmap_update_bits(max98925,
				MAX98925_R02D_GAIN,
				MAX98925_SPK_GAIN_MASK, 0x00);
		usleep_range(4999, 5000);
		max98925_regmap_update_bits(max98925,
				MAX98925_R038_GLOBAL_ENABLE,
				MAX98925_EN_MASK, 0x0);
		}
	} else  {
		if (!delayed_work_pending(&max98925->work)){
			msg_maxim("mute %d, epack_state %d", mute, max98925_connected);
			max98925_regmap_update_bits(max98925,
					MAX98925_R02D_GAIN,
					MAX98925_SPK_GAIN_MASK,
					max98925->volume);
			max98925_regmap_update_bits(max98925,
					MAX98925_R036_BLOCK_ENABLE,
					MAX98925_BST_EN_MASK |
					MAX98925_SPK_EN_MASK,
					MAX98925_BST_EN_MASK |
					MAX98925_SPK_EN_MASK);
			max98925_regmap_write(max98925,
					MAX98925_R038_GLOBAL_ENABLE,
					MAX98925_EN_MASK);
		}
	}
}

static int max98925_dai_digital_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct max98925_priv *max98925
		= snd_soc_codec_get_drvdata(codec_dai->codec);
	struct max98925_pdata *pdata = max98925->pdata;
	bool action = 1;

	if (pdata->capture_active != codec_dai->capture_active) {
		pdata->capture_active = codec_dai->capture_active;
		action = 0;
	}

	if (pdata->playback_active != codec_dai->playback_active) {
		pdata->playback_active = codec_dai->playback_active;
		action = 1;
	}

	msg_maxim("mute=%d playback_active=%d capture_active=%d action=%d",
			mute, pdata->playback_active,
			pdata->capture_active, action);

	if (action)
		__max98925_dai_digital_mute(max98925, mute);

#ifdef CONFIG_LGE_EXTERNAL_SPEAKER
	if (action && !mute && (max98925_check_version(max98925) == 0))
		max98925_amp_status = 0;
	else
		max98925_amp_status = 1;
#endif
#ifdef USE_REG_DUMP
	if (action)
		reg_dump(max98925);
#endif /* USE_REG_DUMP */
	return 0;
}
#ifdef CONFIG_LGE_EXTERNAL_SPEAKER
static void max98925_amp_re_enable(struct max98925_priv *max98925)
{
	unsigned int val = 0;

	if(max98925_connected) {
		regmap_read(max98925->regmap,
			MAX98925_R038_GLOBAL_ENABLE, &val);
		msg_maxim("EpackSPK enable register(0x38) val : 0x%x", val);

		if (!val) {
			max98925_regmap_update_bits(max98925,
					MAX98925_R02D_GAIN,
					MAX98925_SPK_GAIN_MASK,
					max98925->volume);
			max98925_regmap_update_bits(max98925,
					MAX98925_R036_BLOCK_ENABLE,
					MAX98925_BST_EN_MASK |
					MAX98925_SPK_EN_MASK,
					MAX98925_BST_EN_MASK |
					MAX98925_SPK_EN_MASK);
			max98925_regmap_write(max98925,
					MAX98925_R038_GLOBAL_ENABLE,
					MAX98925_EN_MASK);
		}
	} else {
		msg_maxim("EpackSPK connection :%d", max98925_connected);
	}
}

static void max98925_amp_work(struct work_struct *work)
{
	struct max98925_priv *max98925;
	max98925 = container_of(work, struct max98925_priv, work.work);
	mutex_lock(&max98925->mutex);
	max98925_amp_re_enable(max98925);
	mutex_unlock(&max98925->mutex);
}

static int maxd98925_amp_enable_check(struct max98925_priv *max98925, int action)
{
	int ret = 0;
	if (delayed_work_pending(&max98925->work))
		cancel_delayed_work(&max98925->work);
	if (action) {
		queue_delayed_work(max98925->wq,
				&max98925->work,
				msecs_to_jiffies(0));
	}
	return ret;
}
#endif
static int max98925_amp_reinit_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;
	int i;
	unsigned int ch_size, dai_bclk;
	int sel = (int)ucontrol->value.integer.value[0];
#ifndef CONFIG_LGE_EXTERNAL_SPEAKER
	int reg = 0;
#endif

	msg_maxim("Reinit in, sel : %d\n", sel);
#ifdef CONFIG_LGE_EXTERNAL_SPEAKER
	max98925_init_status = sel;
	if (!sel) {
		msg_maxim("EPackSPK Off, epack_state : %d", max98925_connected);
		__max98925_dai_digital_mute(max98925, 1);
		return ret;
	}
#else
	reg = max98925_check_version(max98925);
	if (!reg) {
		dev_err(codec->dev,
			"device re-initialization error (0x%02X)\n",
			reg);
		return ret;
	}
#endif
	max98925_regmap_write(max98925, MAX98925_R038_GLOBAL_ENABLE, 0x00);

	for (i = 0; i < ARRAY_SIZE(max98925_reg); i++)
		max98925_regmap_write(max98925,
				max98925_reg[i].reg,
				max98925_reg[i].def);

	/* It's not the default but we need to set DAI_DLY */
	max98925_regmap_write(max98925, MAX98925_R020_FORMAT,
			MAX98925_DAI_DLY_MASK);

	max98925_regmap_write(max98925, MAX98925_R021_TDM_SLOT_SELECT, 0xC8);

	max98925_regmap_write(max98925, MAX98925_R027_DOUT_HIZ_CFG1, 0xFF);
	max98925_regmap_write(max98925, MAX98925_R028_DOUT_HIZ_CFG2, 0xFF);
	max98925_regmap_write(max98925, MAX98925_R029_DOUT_HIZ_CFG3, 0xFF);
	max98925_regmap_write(max98925, MAX98925_R02A_DOUT_HIZ_CFG4, 0xF0);

	max98925_regmap_write(max98925, MAX98925_R02C_FILTERS, 0xD9);
	max98925_regmap_write(max98925, MAX98925_R034_ALC_CONFIGURATION, 0x12);

	/* Set boost output to maximum */
	max98925_regmap_write(max98925, MAX98925_R037_CONFIGURATION, 0x00);

	/* Disable ALC muting */
	max98925_regmap_write(max98925, MAX98925_R03A_BOOST_LIMITER, 0xF8);

	if (!max98925->sub_regmap) {
		regmap_update_bits(max98925->regmap,
				MAX98925_R02D_GAIN, MAX98925_DAC_IN_SEL_MASK,
				MAX98925_DAC_IN_SEL_DIV2_SUMMED_DAI);
	} else {
		regmap_update_bits(max98925->regmap,
				MAX98925_R02D_GAIN, MAX98925_DAC_IN_SEL_MASK,
				MAX98925_DAC_IN_SEL_LEFT_DAI);
		regmap_update_bits(max98925->sub_regmap,
				MAX98925_R02D_GAIN, MAX98925_DAC_IN_SEL_MASK,
				MAX98925_DAC_IN_SEL_RIGHT_DAI);
	}
	/* Enable ADC */
	if (!max98925->nodsm)
		max98925_regmap_update_bits(max98925,
				MAX98925_R036_BLOCK_ENABLE,
				MAX98925_ADC_VIMON_EN_MASK,
				MAX98925_ADC_VIMON_EN_MASK);

	/* Enable Clock Monitoring*/
	max98925_regmap_update_bits(max98925,
				MAX98925_R036_BLOCK_ENABLE,
				MAX98925_CLKMON_EN_MASK,
				MAX98925_CLKMON_EN_MASK);

	max98925_set_slave(max98925);
	max98925_handle_pdata(codec);
#ifndef CONFIG_LGE_EXTERNAL_SPEAKER
	max98925_regmap_update_bits(max98925,
			MAX98925_R02D_GAIN,
			MAX98925_SPK_GAIN_MASK,
			max98925->volume);
	max98925_regmap_update_bits(max98925,
			MAX98925_R036_BLOCK_ENABLE,
			MAX98925_BST_EN_MASK |
			MAX98925_SPK_EN_MASK,
			MAX98925_BST_EN_MASK |
			MAX98925_SPK_EN_MASK);
#endif
/* DAI format configuration */
	if (max98925->ch_size ==32){
		dai_bclk = MAX98925_DAI_BSEL_64;
		ch_size = MAX98925_DAI_CHANSZ_32;
	} else if (max98925->ch_size ==24){
		dai_bclk = MAX98925_DAI_BSEL_64;
		ch_size = MAX98925_DAI_CHANSZ_32;
	} else {
		dai_bclk = MAX98925_DAI_BSEL_32;
		ch_size = MAX98925_DAI_CHANSZ_16;
	}

	msg_maxim("max98925->ch_size : %d,  ch_size : %d, max98925->sample_rate : %d",max98925->ch_size,ch_size,max98925->sample_rate);
	max98925_regmap_update_bits(max98925,
		MAX98925_R01B_DAI_CLK_MODE2,
		MAX98925_DAI_BSEL_MASK,
		dai_bclk);
	max98925_regmap_update_bits(max98925,
		MAX98925_R020_FORMAT,
		MAX98925_DAI_CHANSZ_MASK,
		ch_size);

	if (!max98925->sample_rate)
		max98925->sample_rate = 48000;
	max98925_set_clock(max98925, max98925->sample_rate);
#ifdef CONFIG_LGE_EXTERNAL_SPEAKER
	maxd98925_amp_enable_check(max98925, sel);
#else
	max98925_regmap_write(max98925, MAX98925_R038_GLOBAL_ENABLE, 0x80);
#endif
	msg_maxim("Reinit out\n");

    return ret;
}

static int max98925_amp_reinit_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98925_reg_get(kcontrol, ucontrol,
		MAX98925_R038_GLOBAL_ENABLE,
		MAX98925_EN_MASK,
		0);
}
static void max98925_handle_pdata(struct snd_soc_codec *codec)
{
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	struct max98925_reg_default *regInfo;
	int cfg_size = 0;
	int x;

	if (max98925->regcfg != NULL)
		cfg_size = max98925->regcfg_sz / sizeof(uint32_t);

	if (cfg_size <= 0) {
		dev_dbg(codec->dev,
			"Register configuration is not required.\n");
		return;
	}

	/*
	* This is for direct register configuration from device tree.
	* It is needed for special configuration such as IV slot allocation
	*
	* (Example) maxim,regcfg =
	*	<0x0 0x001e 0x10>,	<channel_info register value>,
	*	channel info :
	*         0 is primary amplifier, 1 is secondary amplifier
	*	register : register number
	*	value : register value
	*/
	for (x = 0; x < cfg_size; x += 3) {
		regInfo = (struct max98925_reg_default *)&max98925->regcfg[x];
		dev_info(codec->dev, "CH:%d, reg:0x%02x, value:0x%02x\n",
			be32_to_cpu(regInfo->ch),
			be32_to_cpu(regInfo->reg),
			be32_to_cpu(regInfo->def));
		if (be32_to_cpu(regInfo->ch) == PRI_MAX98925
			&& max98925->regmap && max98925_connected)
			regmap_write(max98925->regmap,
				be32_to_cpu(regInfo->reg),
				be32_to_cpu(regInfo->def));
		else if (be32_to_cpu(regInfo->ch) == SEC_MAX98925
			&& max98925->sub_regmap && max98925_connected)
			regmap_write(max98925->sub_regmap,
				be32_to_cpu(regInfo->reg),
				be32_to_cpu(regInfo->def));
	}

}

static const struct snd_kcontrol_new max98925_snd_controls[] = {

	SOC_SINGLE_EXT_TLV("Speaker Volume", MAX98925_R02D_GAIN,
		MAX98925_SPK_GAIN_SHIFT,
		(1<<MAX98925_SPK_GAIN_WIDTH)-1,
		0,
		max98925_spk_vol_get,
		max98925_spk_vol_put,
		max98925_spk_tlv),

	SOC_SINGLE_EXT("Speaker Ramp", 0, 0, 1, 0,
		max98925_spk_ramp_get, max98925_spk_ramp_put),

	SOC_SINGLE_EXT("Speaker ZCD", 0, 0, 1, 0,
		max98925_spk_zcd_get, max98925_spk_zcd_put),

	SOC_SINGLE_EXT("ALC Enable", 0, 0, 1, 0,
		max98925_alc_en_get, max98925_alc_en_put),

	SOC_SINGLE_EXT("ALC Threshold", 0, 0, (1<<MAX98925_ALC_TH_WIDTH)-1, 0,
		max98925_alc_threshold_get, max98925_alc_threshold_put),

	SOC_ENUM_EXT("Boost Output Voltage", max98925_boost_voltage_enum,
		max98925_bout_voltage_get, max98925_bout_voltage_put),

	SOC_ENUM_EXT("SPK out", spk_state_enum,
		max98925_spk_out_get, max98925_spk_out_put),

	SOC_SINGLE_EXT("ADC Enable", 0, 0, 1, 0,
		max98925_adc_en_get, max98925_adc_en_put),

	SOC_SINGLE_EXT("ADC Threshold", SND_SOC_NOPM, 0, 15, 0,
		max98925_adc_thres_get, max98925_adc_thres_put),

	SOC_SINGLE_EXT("Volume Step", SND_SOC_NOPM, 0, 15, 0,
		max98925_volume_step_get, max98925_volume_step_put),

    SOC_SINGLE_EXT("Amp Reinit", SND_SOC_NOPM,
        0, 1, 0,
        max98925_amp_reinit_get, max98925_amp_reinit_put),
#ifdef CONFIG_SND_SOC_MAXIM_DSM
#ifdef USE_DSM_LOG
	SOC_SINGLE_EXT("DSM LOG", SND_SOC_NOPM, 0, 3, 0,
		max98925_get_dump_status, max98925_set_dump_status),
#endif /* USE_DSM_LOG */
#ifdef USE_DSM_UPDATE_CAL
	SOC_SINGLE_EXT("DSM SetParam", SND_SOC_NOPM, 0, 1, 0,
		max98925_get_dsm_param, max98925_set_dsm_param),
#endif /* USE_DSM_UPDATE_CAL */
#endif
};

static int max98925_add_widgets(struct snd_soc_codec *codec)
{
	int ret;
	msg_maxim("max98925_add_widgets in");
	ret = snd_soc_add_codec_controls(codec, max98925_snd_controls,
			ARRAY_SIZE(max98925_snd_controls));
	msg_maxim("max98925_add_widgets out as %d", ret);
	return 0;
}

/* codec sample rate and n/m dividers parameter table */
static const struct {
	u32 rate;
	u8  sr;
	u32 divisors[3][2];
} rate_table[] = {
	{ 8000, 0, {{  1,   375}, {5, 1764}, {  1,   384} } },
	{11025,	1, {{147, 40000}, {1,  256}, {147, 40960} } },
	{12000, 2, {{  1,   250}, {5, 1176}, {  1,   256} } },
	{16000, 3, {{  2,   375}, {5,  882}, {  1,   192} } },
	{22050, 4, {{147, 20000}, {1,  128}, {147, 20480} } },
	{24000, 5, {{  1,   125}, {5,  588}, {  1,   128} } },
	{32000, 6, {{  4,   375}, {5,  441}, {  1,    96} } },
	{44100, 7, {{147, 10000}, {1,   64}, {147, 10240} } },
	{48000, 8, {{  2,   125}, {5,  294}, {  1,    64} } },
};

static inline int max98925_rate_value(int rate,
		int clock, u8 *value, int *n, int *m)
{
	int ret = -ENODATA;
	int i;

	for (i = 0; i < ARRAY_SIZE(rate_table); i++) {
		if (rate_table[i].rate >= rate) {
			*value = rate_table[i].sr;
			*n = rate_table[i].divisors[clock][0];
			*m = rate_table[i].divisors[clock][1];
			ret = 0;
			break;
		}
	}

	msg_maxim("sample rate is %d, returning %d",
			rate_table[i < ARRAY_SIZE(rate_table) ? i : (ARRAY_SIZE(rate_table)-1)].rate, *value);

	return ret;
}

static int max98925_set_tdm_slot(struct snd_soc_dai *codec_dai,
		unsigned int tx_mask, unsigned int rx_mask,
		int slots, int slot_width)
{
	msg_maxim("tx_mask 0x%X, rx_mask 0x%X, slots %d, slot width %d",
			tx_mask, rx_mask, slots, slot_width);
	return 0;
}

static void max98925_set_slave(struct max98925_priv *max98925)
{
	struct max98925_pdata *pdata = max98925->pdata;

	msg_maxim("ENTER");

	/*
	 * 1. use BCLK instead of MCLK
	 */
	max98925_regmap_update_bits(max98925,
			MAX98925_R01A_DAI_CLK_MODE1,
			MAX98925_DAI_CLK_SOURCE_MASK,
			MAX98925_DAI_CLK_SOURCE_MASK);
	/*
	 * 2. set DAI to slave mode
	 */
	max98925_regmap_update_bits(max98925,
			MAX98925_R01B_DAI_CLK_MODE2,
			MAX98925_DAI_MAS_MASK, 0);
	/*
	 * 3. set BLCKs to LRCLKs to 64
	 */
	max98925_regmap_update_bits(max98925,
			MAX98925_R01B_DAI_CLK_MODE2,
			MAX98925_DAI_BSEL_MASK,
			MAX98925_DAI_BSEL_32);
	/*
	 * 4. set VMON and IMON slots
	 */
	if (!pdata->vmon_slot)	{
		max98925_regmap_update_bits(max98925,
				MAX98925_R022_DOUT_CFG_VMON,
				MAX98925_DAI_VMON_EN_MASK,
				MAX98925_DAI_VMON_EN_MASK);
		max98925_regmap_update_bits(max98925,
				MAX98925_R022_DOUT_CFG_VMON,
				MAX98925_DAI_VMON_SLOT_MASK,
				MAX98925_DAI_VMON_SLOT_02_03);
		max98925_regmap_update_bits(max98925,
				MAX98925_R023_DOUT_CFG_IMON,
				MAX98925_DAI_IMON_EN_MASK,
				MAX98925_DAI_IMON_EN_MASK);
		max98925_regmap_update_bits(max98925,
				MAX98925_R023_DOUT_CFG_IMON,
				MAX98925_DAI_IMON_SLOT_MASK,
				MAX98925_DAI_IMON_SLOT_00_01);
	} else {
		max98925_regmap_update_bits(max98925,
				MAX98925_R022_DOUT_CFG_VMON,
				MAX98925_DAI_VMON_EN_MASK,
				MAX98925_DAI_VMON_EN_MASK);
		max98925_regmap_update_bits(max98925,
				MAX98925_R022_DOUT_CFG_VMON,
				MAX98925_DAI_VMON_SLOT_MASK,
				MAX98925_DAI_VMON_SLOT_00_01);
		max98925_regmap_update_bits(max98925,
				MAX98925_R023_DOUT_CFG_IMON,
				MAX98925_DAI_IMON_EN_MASK,
				MAX98925_DAI_IMON_EN_MASK);
		max98925_regmap_update_bits(max98925,
				MAX98925_R023_DOUT_CFG_IMON,
				MAX98925_DAI_IMON_SLOT_MASK,
				MAX98925_DAI_IMON_SLOT_02_03);
	}
}

static void max98925_set_master(struct max98925_priv *max98925)
{
	msg_maxim("ENTER");

	/*
	 * 1. use MCLK for Left channel, right channel always BCLK
	 */
	max98925_regmap_update_bits(max98925, MAX98925_R01A_DAI_CLK_MODE1,
			MAX98925_DAI_CLK_SOURCE_MASK, 0);
	/*
	 * 2. set left channel DAI to master mode, right channel always slave
	 */
	max98925_regmap_update_bits(max98925, MAX98925_R01B_DAI_CLK_MODE2,
			MAX98925_DAI_MAS_MASK, MAX98925_DAI_MAS_MASK);
}

static int max98925_dai_set_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	struct max98925_cdata *cdata;
	unsigned int invert = 0;

	msg_maxim("fmt 0x%08X", fmt);

	cdata = &max98925->dai[0];

	cdata->fmt = fmt;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		max98925_set_slave(max98925);
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		max98925_set_master(max98925);
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
	case SND_SOC_DAIFMT_CBM_CFS:
	default:
		dev_err(codec->dev, "DAI clock mode unsupported");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		msg_maxim("set SND_SOC_DAIFMT_I2S");
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		msg_maxim("set SND_SOC_DAIFMT_LEFT_J");
		break;
	case SND_SOC_DAIFMT_DSP_A:
		msg_maxim("set SND_SOC_DAIFMT_DSP_A");
	default:
		dev_warn(codec->dev, "DAI format unsupported, fmt:0x%x", fmt);
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		invert = MAX98925_DAI_WCI_MASK;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		invert = MAX98925_DAI_BCI_MASK;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		invert = MAX98925_DAI_BCI_MASK | MAX98925_DAI_WCI_MASK;
		break;
	default:
		dev_err(codec->dev, "DAI invert mode unsupported");
		return -EINVAL;
	}

	max98925_regmap_update_bits(max98925, MAX98925_R020_FORMAT,
			MAX98925_DAI_BCI_MASK | MAX98925_DAI_BCI_MASK, invert);

	return 0;
}

static int max98925_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	codec->dapm.bias_level = level;
	return 0;
}

static int max98925_set_clock(struct max98925_priv *max98925, unsigned int rate)
{
	unsigned int clock;
	unsigned int mdll;
	unsigned int n;
	unsigned int m;
	u8 dai_sr = 0;

	switch (max98925->sysclk) {
	case 6000000:
		clock = 0;
		mdll  = MAX98925_MDLL_MULT_MCLKx16;
		break;
	case 11289600:
		clock = 1;
		mdll  = MAX98925_MDLL_MULT_MCLKx8;
		break;
	case 12000000:
		clock = 0;
		mdll  = MAX98925_MDLL_MULT_MCLKx8;
		break;
	case 12288000:
		clock = 2;
		mdll  = MAX98925_MDLL_MULT_MCLKx8;
		break;
	default:
		dev_info(max98925->codec->dev, "unsupported sysclk %d\n",
				max98925->sysclk);
		return -EINVAL;
	}

	if (max98925_rate_value(rate, clock, &dai_sr, &n, &m))
		return -EINVAL;

	/*
	 * 1. set DAI_SR to correct LRCLK frequency
	 */
	max98925_regmap_update_bits(max98925, MAX98925_R01B_DAI_CLK_MODE2,
			MAX98925_DAI_SR_MASK, dai_sr << MAX98925_DAI_SR_SHIFT);
	/*
	 * 2. set DAI m divider
	 */
	max98925_regmap_write(max98925, MAX98925_R01C_DAI_CLK_DIV_M_MSBS,
			m >> 8);
	max98925_regmap_write(max98925, MAX98925_R01D_DAI_CLK_DIV_M_LSBS,
			m & 0xFF);
	/*
	 * 3. set DAI n divider
	 */
	max98925_regmap_write(max98925, MAX98925_R01E_DAI_CLK_DIV_N_MSBS,
			n >> 8);
	max98925_regmap_write(max98925, MAX98925_R01F_DAI_CLK_DIV_N_LSBS,
			n & 0xFF);
	/*
	 * 4. set MDLL
	 */
	max98925_regmap_update_bits(max98925, MAX98925_R01A_DAI_CLK_MODE1,
			MAX98925_MDLL_MULT_MASK,
			mdll << MAX98925_MDLL_MULT_SHIFT);

	return 0;
}

static int max98925_dai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	struct max98925_cdata *cdata;
	unsigned int rate;

	msg_maxim("enter");

	cdata = &max98925->dai[0];

	rate = params_rate(params);
	max98925->sample_rate = rate;

	msg_maxim("width %d rate %d, params_format(params) : %d, max98925->ch_size : %d",
		snd_pcm_format_width(params_format(params)),
		rate,params_format(params), max98925->ch_size);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		msg_maxim("set SNDRV_PCM_FORMAT_S16_LE");
		max98925_regmap_update_bits(max98925,
				MAX98925_R01B_DAI_CLK_MODE2,
				MAX98925_DAI_BSEL_MASK,
				MAX98925_DAI_BSEL_32);

		max98925_regmap_update_bits(max98925,
				MAX98925_R020_FORMAT,
				MAX98925_DAI_CHANSZ_MASK,
				MAX98925_DAI_CHANSZ_16);
		max98925->ch_size = 16;
		break;
    case SNDRV_PCM_FORMAT_S24_3LE:
		msg_maxim("set  SNDRV_PCM_FORMAT_S24_3LE");
	case SNDRV_PCM_FORMAT_S24_LE:
		msg_maxim("set SNDRV_PCM_FORMAT_S24_LE");
		max98925_regmap_update_bits(max98925,
				MAX98925_R01B_DAI_CLK_MODE2,
				MAX98925_DAI_BSEL_MASK,
				MAX98925_DAI_BSEL_64);

		max98925_regmap_update_bits(max98925,
				MAX98925_R020_FORMAT,
				MAX98925_DAI_CHANSZ_MASK,
				MAX98925_DAI_CHANSZ_32);
		max98925->ch_size = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		msg_maxim("set SNDRV_PCM_FORMAT_S32_LE");
		max98925_regmap_update_bits(max98925,
				MAX98925_R01B_DAI_CLK_MODE2,
				MAX98925_DAI_BSEL_MASK,
				MAX98925_DAI_BSEL_64);

		max98925_regmap_update_bits(max98925,
				MAX98925_R020_FORMAT,
				MAX98925_DAI_CHANSZ_MASK,
				MAX98925_DAI_CHANSZ_32);
		max98925->ch_size = 32;
		break;
	default:
		msg_maxim("format unsupported %d",
				params_format(params));
		return -EINVAL;
	}

	return max98925_set_clock(max98925, rate);
}

static int max98925_dai_set_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);

	msg_maxim("clk_id %d, freq %d, dir %d",
			clk_id, freq, dir);

	max98925->sysclk = freq;

	return 0;
}
#ifdef CONFIG_LGE_EXTERNAL_SPEAKER
static struct max98925_priv *g_max98925;
void max98925_spk_enable(int enable)
{
	msg_maxim("EPackSPK Enable : %d", enable);
	if (g_max98925->nodsm) {
		__max98925_dai_digital_mute(g_max98925, !enable);
	} else {
		__max98925_dai_digital_mute(g_max98925, !enable);
	}
}
EXPORT_SYMBOL_GPL(max98925_spk_enable);
#endif
#define MAX98925_RATES SNDRV_PCM_RATE_8000_48000
#define MAX98925_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops max98925_dai_ops = {
	.set_sysclk = max98925_dai_set_sysclk,
	.set_fmt = max98925_dai_set_fmt,
	.set_tdm_slot = max98925_set_tdm_slot,
	.hw_params = max98925_dai_hw_params,
	.digital_mute = max98925_dai_digital_mute,
};

static struct snd_soc_dai_driver max98925_dai[] = {
	{
		.name = "max98925-aif1",
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98925_RATES,
			.formats = MAX98925_FORMATS,
		},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98925_RATES,
			.formats = MAX98925_FORMATS,
		},
		.ops = &max98925_dai_ops,
	}
};

#ifdef CONFIG_PM
static int max98925_suspend(struct snd_soc_codec *codec)
{
	msg_maxim("enter");

	return 0;
}

static int max98925_resume(struct snd_soc_codec *codec)
{
	msg_maxim("enter");

	return 0;
}
#else
#define max98925_suspend NULL
#define max98925_resume NULL
#endif /* CONFIG_PM */

#ifdef USE_MAX98925_IRQ
static irqreturn_t max98925_interrupt(int irq, void *data)
{
	struct max98925_priv *max98925 = (struct max98925_priv *) data;

	unsigned int mask0;
	unsigned int mask1;
	unsigned int mask2;
	unsigned int flag0;
	unsigned int flag1;
	unsigned int flag2;

	if (max98925_connected) {
		regmap_read(max98925->regmap, MAX98925_R00B_IRQ_ENABLE0, &mask0);
		regmap_read(max98925->regmap, MAX98925_R008_FLAG0, &flag0);

		regmap_read(max98925->regmap, MAX98925_R00C_IRQ_ENABLE1, &mask1);
		regmap_read(max98925->regmap, MAX98925_R009_FLAG1, &flag1);

		regmap_read(max98925->regmap, MAX98925_R00D_IRQ_ENABLE2, &mask2);
		regmap_read(max98925->regmap, MAX98925_R00A_FLAG2, &flag2);
	}
	flag0 &= mask0;
	flag1 &= mask1;
	flag2 &= mask2;

	if (!flag0 && !flag1 && !flag2)
		return IRQ_NONE;

	/* Send work to be scheduled */
	if (flag0 & MAX98925_THERMWARN_END_STATE_MASK)
		msg_maxim("MAX98925_THERMWARN_STATE_MASK active!");

	if (flag0 & MAX98925_THERMWARN_BGN_STATE_MASK)
		msg_maxim("MAX98925_THERMWARN_BGN_STATE_MASK active!");

	if (flag0 & MAX98925_THERMSHDN_END_STATE_MASK)
		msg_maxim("MAX98925_THERMSHDN_END_STATE_MASK active!");

	if (flag0 & MAX98925_THERMSHDN_BGN_STATE_MASK)
		msg_maxim("MAX98925_THERMSHDN_BGN_STATE_MASK active!");

	if (flag1 & MAX98925_SPRCURNT_STATE_MASK)
		msg_maxim("MAX98925_SPRCURNT_STATE_MASK active!");

	if (flag1 & MAX98925_WATCHFAIL_STATE_MASK)
		msg_maxim("MAX98925_WATCHFAIL_STATE_MASK active!");

	if (flag1 & MAX98925_ALCINFH_STATE_MASK)
		msg_maxim("MAX98925_ALCINFH_STATE_MASK active!");

	if (flag1 & MAX98925_ALCACT_STATE_MASK)
		msg_maxim("MAX98925_ALCACT_STATE_MASK active!");

	if (flag1 & MAX98925_ALCMUT_STATE_MASK)
		msg_maxim("MAX98925_ALCMUT_STATE_MASK active!");

	if (flag1 & MAX98925_ALCP_STATE_MASK)
		msg_maxim("MAX98925_ALCP_STATE_MASK active!");

	if (flag2 & MAX98925_SLOTOVRN_STATE_MASK)
		msg_maxim("MAX98925_SLOTOVRN_STATE_MASK active!");

	if (flag2 & MAX98925_INVALSLOT_STATE_MASK)
		msg_maxim("MAX98925_INVALSLOT_STATE_MASK active!");

	if (flag2 & MAX98925_SLOTCNFLT_STATE_MASK)
		msg_maxim("MAX98925_SLOTCNFLT_STATE_MASK active!");

	if (flag2 & MAX98925_VBSTOVFL_STATE_MASK)
		msg_maxim("MAX98925_VBSTOVFL_STATE_MASK active!");

	if (flag2 & MAX98925_VBATOVFL_STATE_MASK)
		msg_maxim("MAX98925_VBATOVFL_STATE_MASK active!");

	if (flag2 & MAX98925_IMONOVFL_STATE_MASK)
		msg_maxim("MAX98925_IMONOVFL_STATE_MASK active!");

	if (flag2 & MAX98925_VMONOVFL_STATE_MASK)
		msg_maxim("MAX98925_VMONOVFL_STATE_MASK active!");

	max98925_regmap_write(max98925, MAX98925_R00E_IRQ_CLEAR0,
			flag0&0xff);
	max98925_regmap_write(max98925, MAX98925_R00F_IRQ_CLEAR1,
			flag1&0xff);
	max98925_regmap_write(max98925, MAX98925_R010_IRQ_CLEAR2,
			flag2&0xff);

	return IRQ_HANDLED;
}
#endif /* USE_MAX98925_IRQ */

static int max98925_probe(struct snd_soc_codec *codec)
{
	struct max98925_priv *max98925 = snd_soc_codec_get_drvdata(codec);
	struct max98925_pdata *pdata = max98925->pdata;
	struct max98925_cdata *cdata;
	int ret = 0;
	int reg = 0;

	dev_info(codec->dev, "build number %s\n", MAX98925_REVISION);

	max98925->codec = codec;

	max98925->sysclk = pdata->sysclk;
	max98925->volume = pdata->spk_vol;

	cdata = &max98925->dai[0];
	cdata->rate = (unsigned)-1;
	cdata->fmt  = (unsigned)-1;

	reg = max98925_check_version(max98925);
#ifndef CONFIG_LGE_EXTERNAL_SPEAKER
	if (!reg) {
		dev_err(codec->dev,
			"device initialization error (0x%02X)\n",
			reg);
		goto err_version;
	}
#endif
	msg_maxim("device version 0x%02x", reg);

	max98925_regmap_write(max98925, MAX98925_R038_GLOBAL_ENABLE, 0x00);

	/* It's not the default but we need to set DAI_DLY */
	max98925_regmap_write(max98925, MAX98925_R020_FORMAT,
			MAX98925_DAI_DLY_MASK);

	max98925_regmap_write(max98925, MAX98925_R021_TDM_SLOT_SELECT, 0xC8);

	max98925_regmap_write(max98925, MAX98925_R027_DOUT_HIZ_CFG1, 0xFF);
	max98925_regmap_write(max98925, MAX98925_R028_DOUT_HIZ_CFG2, 0xFF);
	max98925_regmap_write(max98925, MAX98925_R029_DOUT_HIZ_CFG3, 0xFF);
	max98925_regmap_write(max98925, MAX98925_R02A_DOUT_HIZ_CFG4, 0xF0);

	max98925_regmap_write(max98925, MAX98925_R02C_FILTERS, 0xD9);
	max98925_regmap_write(max98925, MAX98925_R034_ALC_CONFIGURATION, 0x12);

	/* Set boost output to maximum */
	max98925_regmap_write(max98925, MAX98925_R037_CONFIGURATION, 0x00);

	/* Disable ALC muting */
	max98925_regmap_write(max98925, MAX98925_R03A_BOOST_LIMITER, 0xF8);

	if (max98925_connected) {
		if (!max98925->sub_regmap) {
			regmap_update_bits(max98925->regmap,
					MAX98925_R02D_GAIN, MAX98925_DAC_IN_SEL_MASK,
					MAX98925_DAC_IN_SEL_DIV2_SUMMED_DAI);
		} else {
			regmap_update_bits(max98925->regmap,
					MAX98925_R02D_GAIN, MAX98925_DAC_IN_SEL_MASK,
					MAX98925_DAC_IN_SEL_LEFT_DAI);
			regmap_update_bits(max98925->sub_regmap,
					MAX98925_R02D_GAIN, MAX98925_DAC_IN_SEL_MASK,
					MAX98925_DAC_IN_SEL_RIGHT_DAI);
		}
	}
	/* Enable ADC */
	if (!max98925->nodsm)
		max98925_regmap_update_bits(max98925,
				MAX98925_R036_BLOCK_ENABLE,
				MAX98925_ADC_VIMON_EN_MASK,
				MAX98925_ADC_VIMON_EN_MASK);

	pdata->vstep.adc_status = 1;

	/* Enable Clock Monitoring*/
	max98925_regmap_update_bits(max98925,
				MAX98925_R036_BLOCK_ENABLE,
				MAX98925_CLKMON_EN_MASK,
				MAX98925_CLKMON_EN_MASK);

	max98925_set_slave(max98925);
	max98925_handle_pdata(codec);
	max98925_add_widgets(codec);

#if defined(USE_DSM_LOG) || defined(USE_DSM_UPDATE_CAL)
	if (!g_class)
		g_class = class_create(THIS_MODULE, class_name_log);
	max98925->dev_log_class = g_class;
	if (max98925->dev_log_class) {
		max98925->dev_log =
			device_create(max98925->dev_log_class,
					NULL, 1, NULL, "max98925");
		if (IS_ERR(max98925->dev_log)) {
			ret = sysfs_create_group(&codec->dev->kobj,
				&max98925_attribute_group);
			if (ret)
				msg_maxim(
				"failed to create sysfs group [%d]", ret);
		} else {
			ret = sysfs_create_group(&max98925->dev_log->kobj,
				&max98925_attribute_group);
			if (ret)
				msg_maxim(
				"failed to create sysfs group [%d]", ret);
		}
	}
	msg_maxim("g_class=%p %p", g_class, max98925->dev_log_class);
	max98925->ch_size = 0;
	max98925->sample_rate = 0;
#endif /* USE_DSM_LOG || USE_DSM_UPDATE_CAL */
#ifdef CONFIG_LGE_EXTERNAL_SPEAKER
	max98925->wq = create_singlethread_workqueue(MA98925_WQ_NAME);
	if (max98925->wq == NULL) {
		return -ENOMEM;
	}
	INIT_DELAYED_WORK(&max98925->work, max98925_amp_work);
	mutex_init(&max98925->mutex);
#else
err_version:
	msg_maxim("exit %d", ret);
#endif
	return ret;
}

static int max98925_remove(struct snd_soc_codec *codec)
{
	msg_maxim("enter");

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_max98925 = {
	.probe            = max98925_probe,
	.remove           = max98925_remove,
	.set_bias_level   = max98925_set_bias_level,
	.suspend          = max98925_suspend,
	.resume           = max98925_resume,
};

static const struct regmap_config max98925_regmap = {
	.reg_bits         = 8,
	.val_bits         = 8,
	.max_register     = MAX98925_R0FF_VERSION,
	.reg_defaults     = max98925_reg,
	.num_reg_defaults = ARRAY_SIZE(max98925_reg),
	.volatile_reg     = max98925_volatile_register,
	.readable_reg     = max98925_readable_register,
	.cache_type       = REGCACHE_RBTREE,
};

static struct i2c_board_info max98925_i2c_sub_board[] = {
	{
		I2C_BOARD_INFO("max98925_sub", 0x34),
	}
};

static struct i2c_driver max98925_i2c_sub_driver = {
	.driver = {
		.name = "max98925_sub",
		.owner = THIS_MODULE,
	},
};

struct i2c_client *max98925_add_sub_device(int bus_id, int slave_addr)
{
	struct i2c_client *i2c = NULL;
	struct i2c_adapter *adapter;

	msg_maxim("bus_id:%d, slave_addr:0x%x", bus_id, slave_addr);
	max98925_i2c_sub_board[0].addr = slave_addr;

	adapter = i2c_get_adapter(bus_id);
	if (adapter) {
		i2c = i2c_new_device(adapter, max98925_i2c_sub_board);
		if (i2c)
			i2c->dev.driver = &max98925_i2c_sub_driver.driver;
	}

	return i2c;
}

static int max98925_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
{
	struct max98925_priv *max98925;
	struct max98925_pdata *pdata;
	int ret;
	int pinfo_status = 0;

	msg_maxim("enter, device '%s'", id->name);

	max98925 = devm_kzalloc(&i2c->dev,
			sizeof(struct max98925_priv), GFP_KERNEL);
	if (!max98925) {
		ret = -ENOMEM;
		goto err_allocate_priv;
	}

	max98925->pdata = devm_kzalloc(&i2c->dev,
			sizeof(struct max98925_pdata), GFP_KERNEL);
	if (!max98925->pdata) {
		ret = -ENOMEM;
		goto err_allocate_pdata;
	}

	i2c_set_clientdata(i2c, max98925);
	pdata = max98925->pdata;

	if (i2c->dev.of_node) {
		/* Read system clock */
		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,sysclk", &pdata->sysclk);
		if (ret) {
			dev_err(&i2c->dev, "There is no sysclk property.");
			pdata->sysclk = 12288000;
		}

		/* Read speaker volume */
		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,spk_vol", &pdata->spk_vol);
		if (ret) {
			dev_err(&i2c->dev, "There is no spk_vol property.");
			pdata->spk_vol = 0x14;
		}

		/* Read VMON slot info.*/
		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,vmon_slot", &pdata->vmon_slot);
		if (ret) {
			dev_err(&i2c->dev, "There is no vmon_slot property.");
			pdata->vmon_slot = 0;
		}

#ifdef USE_MAX98925_IRQ
		pdata->irq = of_get_named_gpio_flags(
				i2c->dev.of_node, "maxim,irq-gpio", 0, NULL);
#endif /* USE_MAX98925_IRQ */

		/* Read information related to DSM */
		ret = of_property_read_u32_array(i2c->dev.of_node,
			"maxim,platform_info", (u32 *) &pdata->pinfo,
			sizeof(pdata->pinfo)/sizeof(uint32_t));
		if (ret)
			dev_err(&i2c->dev, "There is no platform info. property.\n");
		else
			pinfo_status = 1;

		ret = of_property_read_u32_array(i2c->dev.of_node,
			"maxim,boost_step",
			(uint32_t *) &pdata->vstep.boost_step,
			sizeof(pdata->vstep.boost_step)/sizeof(uint32_t));
		if (ret) {
			dev_err(&i2c->dev, "There is no boost_step property.\n");
			for (ret = 0; ret < MAX98925_VSTEP_14; ret++)
				pdata->vstep.boost_step[ret] = 0x0F;
			pdata->vstep.boost_step[MAX98925_VSTEP_14] = 0x02;
			pdata->vstep.boost_step[MAX98925_VSTEP_15] = 0x00;
		}

		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,adc_threshold", &pdata->vstep.adc_thres);
		if (ret) {
			dev_err(&i2c->dev, "There is no adc_threshold property.");
			pdata->vstep.adc_thres = MAX98925_VSTEP_7;
		}

		pdata->reg_arr = of_get_property(i2c->dev.of_node,
				"maxim,registers-of-amp", &pdata->reg_arr_len);
		if (pdata->reg_arr == NULL)
			dev_err(&i2c->dev, "There is no registers-diff property.");

#ifdef USE_DSM_LOG
		ret = of_property_read_string(i2c->dev.of_node,
			"maxim,log_class", &class_name_log);
		if (ret) {
			dev_err(&i2c->dev, "There is no log_class property.\n");
			class_name_log = DEFAULT_LOG_CLASS_NAME;
		}
#endif /* USE_DSM_LOG */

		/* update direct configuration info */
		pdata->regcfg = of_get_property(i2c->dev.of_node,
				"maxim,regcfg", &pdata->regcfg_sz);

		/* check for secondary MAX98925 */
		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,sub_reg", &pdata->sub_reg);
		if (ret) {
			dev_err(&i2c->dev, "Sub-device slave address was not found.\n");
			max98925->sub_reg = -1;
		}
		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,sub_bus", &pdata->sub_bus);
		if (ret) {
			dev_err(&i2c->dev, "Sub-device bus information was not found.\n");
			max98925->sub_bus = i2c->adapter->nr;
		}

		pdata->nodsm = of_property_read_bool(
				i2c->dev.of_node, "maxim,nodsm");
		if (pdata->nodsm)
			max98925->nodsm = pdata->nodsm;
		msg_maxim("use DSM(%d)", pdata->nodsm);

	} else {
		pdata->sysclk = 12288000;
		pdata->spk_vol = 0x14;
		pdata->vmon_slot = 0;
		pdata->nodsm = 0;
	}

#ifdef USE_MAX98925_IRQ
	if (pdata != NULL && gpio_is_valid(pdata->irq)) {
		ret = gpio_request(pdata->irq, "max98925_irq_gpio");
		if (ret) {
			dev_err(&i2c->dev, "unable to request gpio [%d]",
					pdata->irq);
			goto err_irq_gpio_req;
		}
		ret = gpio_direction_input(pdata->irq);
		if (ret) {
			dev_err(&i2c->dev,
					"unable to set direction for gpio [%d]",
					pdata->irq);
			goto err_irq_gpio_req;
		}
		i2c->irq = gpio_to_irq(pdata->irq);

		ret = request_threaded_irq(i2c->irq, NULL, max98925_interrupt,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"max98925_interrupt", max98925);
		if (ret)
			dev_err(&i2c->dev, "Failed to register interrupt");
	} else {
		dev_err(&i2c->dev, "irq gpio not provided\n");
	}
	dev_dbg(&i2c->dev, "requested irq for max98925");
	goto go_ahead_next_step;

err_irq_gpio_req:
	if (gpio_is_valid(pdata->irq))
		gpio_free(pdata->irq);

go_ahead_next_step:
#endif /* USE_MAX98925_IRQ */

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	maxdsm_init();
	if (pinfo_status)
		maxdsm_update_info(pdata->pinfo);
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_max98925,
			max98925_dai, ARRAY_SIZE(max98925_dai));
	if (ret) {
		dev_err(&i2c->dev, "Failed to register codec");
		goto err_register_codec;
	}

	max98925->regmap = regmap_init_i2c(i2c, &max98925_regmap);
	if (IS_ERR(max98925->regmap)) {
		ret = PTR_ERR(max98925->regmap);
		dev_err(&i2c->dev, "Failed to initialize regmap: %d", ret);
		goto err_regmap;
	}

	if (pdata->sub_reg != 0)	{
		max98925->sub_i2c =
			max98925_add_sub_device(i2c->adapter->nr,
					pdata->sub_reg);
		if (IS_ERR(max98925->sub_i2c)) {
			if (max98925->sub_i2c)
				dev_err(&max98925->sub_i2c->dev,
						"Second MAX98925 was not found\n");
			else
				msg_maxim("Second MAX98925 was not found & max98925->sub_i2c is null");
			ret = -ENODEV;
			goto err_regmap;
		} else {
			if(max98925->sub_i2c) {
				max98925->sub_regmap = regmap_init_i2c(
						max98925->sub_i2c, &max98925_regmap);
				if (IS_ERR(max98925->sub_regmap)) {
					ret = PTR_ERR(max98925->sub_regmap);
					dev_err(&max98925->sub_i2c->dev,
						"Failed to allocate sub_regmap: %d\n",
						ret);
					goto err_regmap;
				}
			} else {
				msg_maxim("Second MAX98925 was not found & max98925->sub_i2c is null");
				ret = -ENODEV;
				goto err_regmap;
			}
		}
	}

	if (pdata->sub_reg) {
		if (max98925_check_revID(max98925->regmap)
				|| max98925_check_revID(max98925->sub_regmap))
			max98925_init_status = 1;
	} else if (max98925_check_revID(max98925->regmap))
		max98925_init_status = 1;

	g_max98925 = max98925;

	msg_maxim("exit, device '%s'", id->name);

	return 0;

err_regmap:
	snd_soc_unregister_codec(&i2c->dev);
	if (max98925->regmap)
		regmap_exit(max98925->regmap);

err_register_codec:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
	maxdsm_deinit();
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
	devm_kfree(&i2c->dev, max98925->pdata);

err_allocate_pdata:
	devm_kfree(&i2c->dev, max98925);

err_allocate_priv:
	msg_maxim("exit with errors. ret=%d", ret);

	return ret;
}

static int max98925_i2c_remove(struct i2c_client *client)
{
	struct max98925_priv *max98925 = i2c_get_clientdata(client);
	struct max98925_pdata *pdata = max98925->pdata;

	snd_soc_unregister_codec(&client->dev);
	if (max98925->regmap)
		regmap_exit(max98925->regmap);
	if (max98925->sub_regmap)
		regmap_exit(max98925->sub_regmap);
	if (pdata->sub_reg != 0)
		i2c_unregister_device(max98925->sub_i2c);
	devm_kfree(&client->dev, pdata);
	devm_kfree(&client->dev, max98925);

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	maxdsm_deinit();
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

	msg_maxim("exit");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id max98925_dt_ids[] = {
	{ .compatible = "maxim,max98925" },
	{ }
};
#else
#define max98925_dt_ids NULL
#endif /* CONFIG_OF */

static const struct i2c_device_id max98925_i2c_id[] = {
	{ "max98925", MAX98925 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max98925_i2c_id);

static struct i2c_driver max98925_i2c_driver = {
	.driver = {
		.name = "max98925",
		.owner = THIS_MODULE,
		.of_match_table = max98925_dt_ids,
	},
	.probe  = max98925_i2c_probe,
	.remove = max98925_i2c_remove,
	.id_table = max98925_i2c_id,
};

module_i2c_driver(max98925_i2c_driver);

MODULE_DESCRIPTION("ALSA SoC MAX98925 driver");
MODULE_AUTHOR("Ralph Birt <rdbirt@gmail.com>");
MODULE_LICENSE("GPL");
