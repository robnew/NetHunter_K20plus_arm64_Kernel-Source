/* touch_nt11206_sd.c
 *
 * Copyright (C) 2015 LGE.
 *
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

/*
 *  Include to touch core Header File
 */
#include <touch_hwif.h>
#include <touch_core.h>

/*
 *  Include to Local Header File
 */
#include "touch_nt11206.h"



#define I2C_TANSFER_LENGTH  64

#define AIN_TX_NUM	20
#define AIN_RX_NUM	30
#define PSConfig_Tolerance_Postive			250
#define PSConfig_Tolerance_Negative			-250
#define PSConfig_DiffLimitG_Postive			250
#define PSConfig_DiffLimitG_Negative		-250
#define PSConfig_Tolerance_Postive_Short	250
#define PSConfig_Tolerance_Negative_Short	-250
#define PSConfig_DiffLimitG_Postive_Short	250
#define PSConfig_DiffLimitG_Negative_Short	-250
#define PSConfig_Tolerance_Postive_Mutual	250
#define PSConfig_Tolerance_Negative_Mutual	-250
#define PSConfig_DiffLimitG_Postive_Mutual	250
#define PSConfig_DiffLimitG_Negative_Mutual	-250
#define PSConfig_Tolerance_Postive_FW		250
#define PSConfig_Tolerance_Negative_FW		-250
#define PSConfig_DiffLimitG_Postive_FW		250
#define PSConfig_DiffLimitG_Negative_FW		-250

#define PSConfig_Rawdata_Limit_Postive_Short_RXRX	-1500
#define PSConfig_Rawdata_Limit_Negative_Short_RXRX	-8600
#define PSConfig_Rawdata_Limit_Postive_Short_TXRX	-1500
#define PSConfig_Rawdata_Limit_Negative_Short_TXRX	-6500
#define PSConfig_Rawdata_Limit_Postive_Short_TXTX	-1000
#define PSConfig_Rawdata_Limit_Negative_Short_TXTX	-10000

#define NORMAL_MODE 0x00
#define TEST_MODE_1 0x21    //before algo.
#define TEST_MODE_2 0x22    //after algo.
#define BASELINE_ADDR   0x11054
#define XDATA_SECTOR_SIZE   256
s16 BoundaryShort_RXRX[30] = {
	-2938, -2950, -2930, -2941, -2966, -2957, -2987, -2969,	-2964, -2975, -2988, -2986, -3014, -2982, -2980, -2982,	-2973, -3004, -3023, -2994, -2988, -3006, -3013, -3023,	-3026, -3032, -2997, -3029, -3008, -3036,
};

s16 BoundaryShort_TXRX[30 * 20] = {
	-2858, -2884, -2873, -2839, -2903, -2904, -2885, -2903, -2907, -2871, -2925, -2932, -2913, -2921, -2930, -2878, -2914, -2952, -2927, -2936, -2936, -2912, -2955, -2977, -2928, -2973, -2942, -2930, -2949, -2984,
	-2920, -2893, -2880, -2897, -2913, -2908, -2945, -2914, -2917, -2936, -2935, -2941, -2975, -2930, -2937, -2942, -2924, -2961, -2988, -2945, -2943, -2971, -2966, -2982, -2988, -2985, -2952, -2991, -2961, -2993,
	-2910, -2890, -2879, -2889, -2910, -2908, -2934, -2911, -2917, -2925, -2933, -2941, -2966, -2928, -2937, -2933, -2920, -2961, -2978, -2944, -2943, -2962, -2963, -2980, -2979, -2981, -2955, -2984, -2959, -2995,
	-2906, -2890, -2880, -2886, -2910, -2909, -2930, -2911, -2918, -2921, -2933, -2941, -2962, -2926, -2937, -2931, -2921, -2962, -2974, -2943, -2944, -2958, -2962, -2980, -2978, -2981, -2957, -2981, -2958, -2998,
	-2904, -2889, -2879, -2882, -2909, -2908, -2928, -2911, -2918, -2919, -2932, -2939, -2959, -2925, -2936, -2928, -2920, -2961, -2971, -2943, -2942, -2956, -2961, -2980, -2975, -2980, -2956, -2978, -2958, -2994,
	-2903, -2888, -2878, -2882, -2910, -2908, -2927, -2911, -2918, -2918, -2932, -2940, -2959, -2927, -2937, -2928, -2920, -2962, -2972, -2942, -2943, -2955, -2961, -2980, -2974, -2979, -2956, -2978, -2957, -2995,
	-2903, -2889, -2880, -2881, -2909, -2907, -2927, -2910, -2919, -2919, -2932, -2941, -2958, -2927, -2937, -2927, -2921, -2963, -2972, -2942, -2943, -2954, -2962, -2979, -2975, -2980, -2956, -2978, -2957, -2996,
	-2903, -2889, -2881, -2882, -2910, -2908, -2928, -2910, -2919, -2917, -2932, -2940, -2958, -2927, -2938, -2928, -2920, -2962, -2970, -2943, -2944, -2955, -2962, -2979, -2975, -2980, -2956, -2978, -2957, -2997,
	-2901, -2888, -2880, -2880, -2907, -2907, -2926, -2911, -2917, -2917, -2932, -2940, -2958, -2926, -2936, -2926, -2919, -2962, -2970, -2941, -2943, -2953, -2962, -2980, -2974, -2979, -2955, -2976, -2957, -2995,
	-2902, -2890, -2879, -2882, -2909, -2908, -2927, -2911, -2918, -2917, -2932, -2939, -2958, -2926, -2936, -2928, -2919, -2961, -2971, -2943, -2944, -2955, -2963, -2979, -2974, -2979, -2956, -2978, -2958, -2996,
	-2869, -2882, -2878, -2847, -2903, -2905, -2894, -2904, -2916, -2882, -2926, -2938, -2924, -2921, -2935, -2892, -2914, -2961, -2936, -2937, -2943, -2919, -2956, -2977, -2938, -2974, -2954, -2943, -2950, -2993,
	-2919, -2891, -2881, -2898, -2912, -2909, -2943, -2915, -2919, -2934, -2935, -2941, -2975, -2930, -2937, -2944, -2924, -2963, -2988, -2946, -2945, -2972, -2966, -2981, -2991, -2983, -2957, -2996, -2961, -2998,
	-2910, -2890, -2881, -2890, -2911, -2909, -2936, -2912, -2919, -2927, -2934, -2941, -2967, -2928, -2938, -2936, -2921, -2963, -2980, -2944, -2945, -2964, -2964, -2981, -2982, -2981, -2957, -2986, -2959, -2997,
	-2905, -2890, -2880, -2886, -2910, -2908, -2931, -2911, -2919, -2921, -2932, -2940, -2962, -2927, -2937, -2930, -2920, -2961, -2975, -2943, -2944, -2958, -2962, -2979, -2976, -2980, -2956, -2979, -2956, -2996,
	-2905, -2889, -2880, -2883, -2909, -2908, -2929, -2910, -2918, -2919, -2932, -2940, -2959, -2927, -2937, -2928, -2921, -2962, -2972, -2942, -2943, -2956, -2962, -2979, -2975, -2980, -2956, -2979, -2957, -2996,
	-2904, -2889, -2880, -2881, -2910, -2907, -2928, -2910, -2918, -2919, -2932, -2941, -2959, -2926, -2937, -2928, -2919, -2962, -2971, -2942, -2944, -2955, -2962, -2979, -2974, -2980, -2956, -2979, -2958, -2996,
	-2903, -2889, -2880, -2881, -2908, -2907, -2927, -2909, -2919, -2918, -2932, -2939, -2958, -2926, -2937, -2926, -2919, -2962, -2971, -2942, -2943, -2955, -2962, -2980, -2974, -2980, -2956, -2977, -2957, -2995,
	-2904, -2888, -2881, -2881, -2910, -2908, -2927, -2911, -2919, -2917, -2933, -2940, -2959, -2926, -2937, -2928, -2920, -2961, -2971, -2942, -2944, -2955, -2962, -2980, -2975, -2980, -2955, -2979, -2958, -2996,
	-2903, -2890, -2880, -2881, -2909, -2908, -2927, -2911, -2917, -2918, -2932, -2940, -2957, -2926, -2936, -2928, -2919, -2962, -2971, -2943, -2943, -2954, -2962, -2980, -2974, -2980, -2957, -2977, -2958, -2996,
	-2903, -2887, -2880, -2880, -2909, -2907, -2927, -2911, -2919, -2919, -2932, -2940, -2959, -2927, -2936, -2928, -2918, -2963, -2972, -2941, -2943, -2955, -2962, -2980, -2973, -2979, -2956, -2978, -2959, -2996,
};
s16 BoundaryShort_TXTX[20] = {
	-2903, -2888, -2879, -2881, -2909, -2907, -2927, -2910, -2919, -2917, -2932, -2939, -2958, -2926, -2937, -2928, -2920, -2962, -2972, -2941,
};

s16 BoundaryOpen[30*20] = {
	1150,	761,	1310,	1100,	753,	1320,	1070,	745,	1310,	1060,	744,	1320,	1050,	745,	1320,	1040,	732,	1300,	1020,	728,
	1320,	1030,	728,	1310,	1030,	727,	1310,	1040,	728,	1310,	534,	692,	1510,	535,	691,	1500,	525,	685,	1490,	520,
	683,	1480,	518,	685,	1493,	511,	670,	1460,	498,	668,	1480,	500,	669,	1470,	499,	666,	1470,	496,	663,	1460,
	434,	641,	1400,	430,	640,	1400,	432,	640,	1390,	429,	638,	1390,	429,	640,	1400,	430,	627,	1360,	412,	621,
	1380,	414,	622,	1370,	412,	619,	1370,	410,	618,	1360,	447,	654,	1430,	441,	650,	1420,	445,	654,	1420,	444,
	653,	1410,	444,	654,	1420,	446,	646,	1390,	424,	633,	1400,	426,	633,	1390,	424,	630,	1390,	421,	629,	1390,
	406,	600,	1300,	402,	597,	1290,	406,	604,	1290,	407,	603,	1290,	407,	605,	1300,	408,	599,	1270,	392,	584,
	1270,	388,	584,	1270,	386,	581,	1270,	383,	579,	1260,	412,	585,	1260,	410,	584,	1260,	410,	589,	1260,	414,
	589,	1260,	414,	591,	1270,	415,	585,	1240,	408,	575,	1240,	396,	570,	1240,	394,	568,	1240,	391,	566,	1240,
	396,	560,	1160,	393,	557,	1150,	391,	558,	1150,	393,	558,	1140,	393,	560,	1150,	395,	554,	1120,	390,	552,
	1130,	379,	543,	1130,	378,	540,	1130,	375,	539,	1120,	399,	521,	1070,	399,	521,	1060,	397,	521,	1060,	398,
	521,	1060,	398,	523,	1070,	399,	518,	1040,	394,	517,	1050,	387,	508,	1040,	384,	505,	1040,	382,	505,	1040,
	379,	489,	904,	380,	490,	898,	378,	490,	897,	379,	490,	894,	379,	491,	903,	381,	487,	879,	376,	486,
	893,	377,	480,	879,	365,	474,	879,	363,	473,	876,	420,	486,	885,	418,	485,	879,	418,	486,	879,	419,
	485,	875,	420,	488,	890,	424,	485,	864,	416,	482,	874,	419,	481,	861,	407,	470,	860,	410,	473,	869,
	1040,	741,	1320,	1040,	737,	1320,	1030,	737,	1310,	1040,	736,	1320,	1040,	741,	1320,	1040,	735,	1300,	1040,	739,
	1310,	1060,	739,	1300,	1070,	733,	1290,	1110,	736,	1280,	511,	682,	1480,	511,	680,	1490,	510,	680,	1480,	513,
	679,	1480,	514,	683,	1480,	517,	678,	1470,	512,	681,	1480,	522,	682,	1480,	526,	676,	1470,	514,	674,	1490,
	426,	636,	1390,	425,	633,	1390,	424,	634,	1380,	426,	633,	1390,	425,	636,	1390,	427,	631,	1370,	422,	634,
	1380,	426,	634,	1380,	427,	632,	1370,	415,	624,	1376,	436,	646,	1410,	436,	644,	1410,	435,	645,	1400,	437,
	644,	1410,	436,	646,	1410,	439,	642,	1400,	433,	645,	1400,	438,	645,	1400,	438,	645,	1400,	431,	637,	1410,
	397,	595,	1280,	397,	593,	1280,	397,	594,	1270,	398,	593,	1280,	398,	595,	1280,	400,	591,	1270,	394,	593,
	1270,	399,	594,	1280,	399,	593,	1270,	397,	586,	1270,	405,	581,	1260,	404,	579,	1250,	404,	580,	1250,	406,
	579,	1250,	405,	581,	1250,	407,	577,	1240,	402,	580,	1240,	406,	580,	1250,	405,	578,	1241,	400,	572,	1240,
	389,	554,	1140,	388,	553,	1140,	388,	553,	1130,	390,	552,	1140,	389,	554,	1140,	391,	550,	1130,	383,	550,
	1130,	384,	547,	1130,	380,	543,	1120,	377,	541,	1130,	395,	520,	1060,	395,	518,	1060,	394,	518,	1050,	395,
	516,	1050,	392,	516,	1050,	390,	509,	1040,	381,	508,	1040,	384,	507,	1040,	384,	507,	1040,	381,	504,	1040,
	377,	488,	898,	376,	485,	893,	372,	482,	881,	369,	478,	881,	366,	478,	880,	367,	474,	874,	362,	475,
	875,	366,	476,	878,	366,	475,	875,	362,	471,	876,	424,	487,	881,	413,	478,	864,	414,	474,	858,	411,
	478,	855,	413,	480,	869,	408,	480,	853,	407,	473,	857,	406,	479,	855,	409,	473,	856,	401,	475,	855,
};
s16 BoudaryFWMutual[20*30] = {
	1730,	1720,	1720,	1700,	1710,	1710,	1710,	1710,	1720,	1710,	1660,	1670,	1670,	1670,	1690,	1700,	1720,	1730,	1760,	1770,
	741,	748,	756,	755,	759,	765,	763,	767,	772,	775,	721,	728,	733,	735,	744,	760,	761,	771,	784,	787,
	1140,	1150,	1160,	1160,	1160,	1170,	1170,	1160,	1170,	1170,	1160,	1150,	1160,	1160,	1150,	1170,	1170,	1170,	1170,	1170,
	1210,	1200,	1200,	1200,	1200,	1200,	1200,	1200,	1200,	1200,	1190,	1190,	1190,	1190,	1180,	1210,	1200,	1200,	1210,	1220,
	502,	505,	512,	512,	514,	516,	516,	518,	521,	521,	493,	497,	499,	500,	504,	516,	516,	519,	527,	529,
	373,	381,	385,	385,	391,	392,	393,	393,	399,	394,	362,	368,	368,	369,	380,	387,	390,	393,	401,	400,
	377,	383,	383,	382,	385,	386,	386,	385,	387,	383,	371,	373,	373,	373,	385,	387,	388,	390,	389,	391,
	528,	533,	534,	533,	533,	536,	535,	536,	537,	537,	520,	524,	525,	524,	529,	540,	539,	541,	544,	545,
	1160,	1160,	1160,	1160,	1160,	1160,	1160,	1160,	1160,	1160,	1150,	1150,	1160,	1160,	1150,	1170,	1160,	1170,	1170,	1170,
	1150,	1150,	1150,	1150,	1140,	1150,	1140,	1140,	1150,	1150,	1140,	1140,	1140,	1140,	1140,	1160,	1150,	1150,	1160,	1160,
	518,	524,	525,	524,	524,	526,	526,	527,	528,	527,	509,	512,	514,	514,	526,	532,	532,	535,	534,	537,
	365,	369,	369,	368,	371,	370,	371,	370,	372,	368,	355,	358,	359,	359,	373,	375,	376,	377,	375,	377,
	355,	355,	355,	353,	356,	355,	356,	355,	356,	353,	343,	345,	346,	350,	360,	361,	361,	361,	358,	359,
	497,	502,	502,	501,	501,	503,	503,	504,	505,	504,	489,	492,	493,	494,	506,	510,	510,	512,	508,	511,
	1060,	1060,	1060,	1060,	1060,	1060,	1060,	1057,	1060,	1060,	1050,	1050,	1050,	1060,	1060,	1070,	1070,	1070,	1070,	1070,
	1010,	1010,	1010,	1010,	1010,	1010,	1010,	1010,	1010,	1010,	1000,	1000,	1000,	1010,	1010,	1020,	1020,	1020,	1020,	1020,
	472,	473,	474,	473,	474,	476,	475,	477,	477,	477,	462,	465,	466,	470,	479,	483,	482,	483,	481,	485,
	340,	341,	342,	340,	343,	343,	343,	343,	343,	340,	330,	333,	333,	342,	347,	347,	348,	346,	346,	347,
	305,	307,	310,	311,	315,	315,	315,	315,	316,	313,	303,	305,	305,	314,	317,	317,	318,	317,	319,	318,
	430,	435,	437,	438,	441,	443,	443,	444,	445,	445,	429,	432,	433,	441,	443,	447,	447,	448,	450,	449,
	892,	896,	897,	897,	896,	900,	900,	899,	904,	906,	891,	893,	894,	898,	896,	910,	906,	908,	911,	910,
	866,	861,	862,	861,	858,	864,	866,	867,	870,	872,	863,	863,	864,	871,	864,	877,	873,	874,	875,	878,
	423,	424,	424,	425,	426,	431,	433,	435,	436,	435,	423,	425,	426,	433,	434,	437,	437,	438,	439,	440,
	308,	309,	309,	309,	313,	316,	318,	318,	319,	317,	308,	310,	312,	318,	320,	320,	321,	320,	322,	321,
	296,	297,	297,	296,	297,	298,	300,	302,	305,	304,	297,	298,	305,	306,	308,	307,	308,	307,	309,	307,
	401,	403,	403,	402,	403,	405,	406,	409,	412,	413,	402,	404,	408,	412,	413,	415,	415,	416,	417,	417,
	755,	754,	755,	753,	752,	755,	754,	757,	763,	767,	756,	757,	758,	766,	759,	771,	766,	768,	769,	772,
	743,	745,	744,	745,	741,	751,	743,	745,	748,	759,	765,	759,	760,	767,	763,	775,	767,	770,	769,	772,
	407,	406,	410,	405,	411,	411,	411,	408,	411,	417,	407,	405,	413,	413,	416,	418,	416,	417,	418,	418,
	314,	318,	316,	318,	317,	322,	320,	323,	322,	328,	318,	316,	324,	323,	327,	325,	324,	324,	324,	325,
};
#define MaxStatisticsBuf 100
static int StatisticsNum[MaxStatisticsBuf];
static long int StatisticsSum[MaxStatisticsBuf];
static long int golden_Ratio[20*30] = {0, };
struct test_cmd {
	u32 addr;
	u8 len;
	u8 data[40];
};

struct test_cmd short_test_rxrx[] = {
	//# Stop WDT
	{.addr=0x1F028, .len=2,  .data={0x07, 0x55}},
	//# Trim OSC to 60MHz
	//{.addr=0x1F386, .len=1,  .data={0x2A}},
	//# Bypass ControlRAM
	{.addr=0x1F211, .len=1,  .data={0x01}},
	//# Demod @ DC DDFS_2_2_init = 90degree
	{.addr=0x1F150, .len=3,  .data={0x00, 0x00, 0x04}},
	//# Demod @ DC DDFS_2_2_step = 0
	{.addr=0x1F186, .len=2,  .data={0x00, 0x00}},
	//# TIA Rf gain = max
	{.addr=0x1F2D9, .len=1,  .data={0x0E}},
	//# STD_Gain
	{.addr=0x1F2DA, .len=1,  .data={0x0A}},
	//# Warm up
	{.addr=0x1F218, .len=1,  .data={0xFF}},
	//# Sensing start
	{.addr=0x1F220, .len=2,  .data={0xFF, 0xFF}},
	//# VRADC_SEL = 1.75v
	{.addr=0x1F30B, .len=1,  .data={0x02}},
	//# Enable all STD_EN
	{.addr=0x1F2D0, .len=7,  .data={0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x07}},
	//# Enable all ADC_EN
	{.addr=0x1F302, .len=7,  .data={0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x07}},
	//# Enable BIAS_En, STD_BIAS ; Disable TIA_BIAS CC_BIAS
	{.addr=0x1F2FD, .len=4,  .data={0x01, 0x00, 0x01, 0x00}},
	//# Unlock ADC power down
	{.addr=0x1F309, .len=1,  .data={0x00}},
	//# ADC input Range = 0.25~2.25V
	{.addr=0x1F30A, .len=1,  .data={0x00}},
	//# Enable Hanning filter
	{.addr=0x1F1C7, .len=1,  .data={0x20}},
	//# TIA bypass Switch Enable
	{.addr=0x1F310, .len=1,  .data={0x07}},
	//# OB_DM_EN
	{.addr=0x1F1C3, .len=2,  .data={0x00, 0x01}},
	//# Raw1_Base_Addr
	{.addr=0x1F196, .len=2,  .data={0x88, 0x0F}},
	//# POWER ON SEQUENCE
	{.addr=0x1F320, .len=1,  .data={0x06}},
	{.addr=0x1F321, .len=1,  .data={0x13}},
	{.addr=0x1F32A, .len=1,  .data={0x00}},
	{.addr=0x1F327, .len=1,  .data={0x03}},
	{.addr=0x1F328, .len=1,  .data={0x43}},
	{.addr=0x1F328, .len=1,  .data={0x13}},
	{.addr=0x1F328, .len=1,  .data={0x17}},
	//# DAC ON SEQUENCE
	{.addr=0x1F1D5, .len=1,  .data={0x40}},
	{.addr=0x1F1D6, .len=1,  .data={0xFF}},
	{.addr=0x1F1D5, .len=1,  .data={0x7F}},
	{.addr=0x1F1E0, .len=1,  .data={0x01}},
	{.addr=0x1F1DC, .len=1,  .data={0x01}},
	{.addr=0x1F1DE, .len=1,  .data={0x01}},
};

struct test_cmd short_test_txrx[] = {
	//# Stop WDT
	{.addr=0x1F028, .len=2,  .data={0x07, 0x55}},
	//# Trim OSC to 60MHz
	//{.addr=0x1F386, .len=1,  .data={0x2A}},
	//# Bypass ControlRAM
	{.addr=0x1F211, .len=1,  .data={0x01}},
	//# Demod @ DC DDFS_2_2_init = 90degree
	{.addr=0x1F150, .len=3,  .data={0x00, 0x00, 0x04}},
	//# Demod @ DC DDFS_2_2_step = 0
	{.addr=0x1F186, .len=2,  .data={0x00, 0x00}},
	//# TIA Rf gain
	{.addr=0x1F2D9, .len=1,  .data={0x0E}},
	//# STD_Gain
	{.addr=0x1F2DA, .len=1,  .data={0x0A}},
	//# Warm up
	{.addr=0x1F218, .len=1,  .data={0xFF}},
	//# Sensing start
	{.addr=0x1F220, .len=2,  .data={0xFF, 0xFF}},
	//# VRADC_SEL = 1.75v
	{.addr=0x1F30B, .len=1,  .data={0x02}},
	//# RX_CFG all sensing
	{.addr=0x1F238, .len=33, .data={0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02}},
	//# Enable all STD_EN
	{.addr=0x1F2D0, .len=7,  .data={0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x07}},
	//# Enable all ADC_EN
	{.addr=0x1F302, .len=7,  .data={0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x07}},
	//# Enable BIAS_En, STD_BIAS ; Disable TIA_BIAS CC_BIAS
	{.addr=0x1F2FD, .len=4,  .data={0x01, 0x00, 0x01, 0x00}},
	//# Unlock ADC power down
	{.addr=0x1F309, .len=1,  .data={0x00}},
	//# ADC input Range = 0.25~2.25V
	{.addr=0x1F30A, .len=1,  .data={0x00}},
	//# Enable Hanning filter
	{.addr=0x1F1C7, .len=1,  .data={0x20}},
	//# TIA bypass Switch Enable
	{.addr=0x1F310, .len=1,  .data={0x07}},
	//# OB_DM_EN
	{.addr=0x1F1C3, .len=2,  .data={0x00, 0x01}},
	//# Raw1_Base_Addr
	{.addr=0x1F196, .len=2,  .data={0x88, 0x0F}},
	//# POWER ON SEQUENCE
	{.addr=0x1F320, .len=1,  .data={0x06}},
	{.addr=0x1F321, .len=1,  .data={0x13}},
	{.addr=0x1F32A, .len=1,  .data={0x00}},
	{.addr=0x1F327, .len=1,  .data={0x03}},
	{.addr=0x1F328, .len=1,  .data={0x43}},
	{.addr=0x1F328, .len=1,  .data={0x13}},
	{.addr=0x1F328, .len=1,  .data={0x17}},
	//# DAC ON SEQUENCE
	{.addr=0x1F1D5, .len=1,  .data={0x40}},
	{.addr=0x1F1D6, .len=1,  .data={0xFF}},
	{.addr=0x1F1D5, .len=1,  .data={0x7F}},
	{.addr=0x1F1E0, .len=1,  .data={0x01}},
	{.addr=0x1F1DC, .len=1,  .data={0x01}},
	{.addr=0x1F1DE, .len=1,  .data={0x01}},
};

struct test_cmd short_test_txtx[] = {
	//# Stop WDT
	{.addr=0x1F028, .len=2,  .data={0x07, 0x55}},
	//# Trim OSC to 60MHz
	//{.addr=0x1F386, .len=1,  .data={0x2A}},
	//# Bypass ControlRAM
	{.addr=0x1F211, .len=1,  .data={0x01}},
	//# Demod @ DC DDFS_2_2_init = 90degree
	{.addr=0x1F150, .len=3,  .data={0x00, 0x00, 0x04}},
	//# Demod @ DC DDFS_2_2_step = 0
	{.addr=0x1F186, .len=2,  .data={0x00, 0x00}},
	//# Release Test Channel & Output ATEST[1:0] to RX_Channel
	{.addr=0x1F1E5, .len=1,  .data={0x09}},
	//# TIA Rf gain = max
	{.addr=0x1F2D9, .len=1,  .data={0x0E}},
	//# STD_Gain
	{.addr=0x1F2DA, .len=1,  .data={0x0A}},
	//# Warm up
	{.addr=0x1F218, .len=1,  .data={0xFF}},
	//# Sensing start
	{.addr=0x1F220, .len=2,  .data={0xFF, 0xFF}},
	//# VRADC_SEL = 1.75v
	{.addr=0x1F30B, .len=1,  .data={0x02}},
	//# RX_CFG all GND
	{.addr=0x1F238, .len=33,  .data={0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}},
	//# TX_CFG all GND
	//# Enable all STD_EN
	{.addr=0x1F2D0, .len=7,  .data={0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x07}},
	//# Enable all ADC_EN
	{.addr=0x1F302, .len=7,  .data={0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x07}},
	//# Enable BIAS_En, STD_BIAS ; Disable TIA_BIAS CC_BIAS
	{.addr=0x1F2FD, .len=4,  .data={0x01, 0x00, 0x01, 0x00}},
	//# Unlock ADC power down
	{.addr=0x1F309, .len=1,  .data={0x00}},
	//# ADC input Range = 0.25~2.25V
	{.addr=0x1F30A, .len=1,  .data={0x00}},
	//# Enable Hanning filter
	{.addr=0x1F1C7, .len=1,  .data={0x20}},
	//# TIA bypass Switch Enable
	{.addr=0x1F310, .len=1,  .data={0x07}},
	//# OB_DM_EN
	{.addr=0x1F1C3, .len=2,  .data={0x00, 0x01}},
	//# Raw1_Base_Addr
	{.addr=0x1F196, .len=2,  .data={0x88, 0x0F}},
	//# POWER ON SEQUENCE
	{.addr=0x1F320, .len=1,  .data={0x06}},
	{.addr=0x1F321, .len=1,  .data={0x13}},
	{.addr=0x1F32A, .len=1,  .data={0x00}},
	{.addr=0x1F327, .len=1,  .data={0x03}},
	{.addr=0x1F328, .len=1,  .data={0x43}},
	{.addr=0x1F328, .len=1,  .data={0x13}},
	{.addr=0x1F328, .len=1,  .data={0x17}},
	//# DAC ON SEQUENCE
	{.addr=0x1F1D5, .len=1,  .data={0x40}},
	{.addr=0x1F1D6, .len=1,  .data={0xFF}},
	{.addr=0x1F1D5, .len=1,  .data={0x7F}},
	{.addr=0x1F1E0, .len=1,  .data={0x01}},
	{.addr=0x1F1DC, .len=1,  .data={0x01}},
	{.addr=0x1F1DE, .len=1,  .data={0x01}},
};

struct test_cmd short_test_rx_all_gnd[] = {
    {.addr=0x1F238, .len=AIN_RX_NUM, .data={0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}},
};
struct test_cmd short_test_tx_all_vref[] = {
    {.addr=0x1F259, .len=AIN_TX_NUM, .data={0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07}},
};
struct test_cmd short_test_tx_all_gnd[] = {
    {.addr=0x1F259, .len=AIN_TX_NUM, .data={0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
};

struct test_cmd open_test[] = {
	//# Stop WDT
	{.addr=0x1F028, .len=2,	 .data={0x07, 0x55}},
	//# Trim OSC to 60MHz
	//{.addr=0x1F386, .len=1,  .data={0x2A}},
	//# Bypass ControlRAM
	{.addr=0x1F211, .len=1,  .data={0x01}},
	//# Demod @ for Hanning DDFS_2_1_init = 90degree
	{.addr=0x1F14C,	.len=3,  .data={0x00, 0x00, 0x04}},
	//# Demod @ for Q DDFS_2_2_init = 90degree
	{.addr=0x1F150, .len=3,  .data={0x00, 0x00, 0x04}},
	//# Demod @ for I DDFS_2_3_init = 0
	{.addr=0x1F154, .len=3,  .data={0x00, 0x00, 0x00}},
	//# TIA Rf gain = max
	{.addr=0x1F2D9, .len=1,  .data={0x0E}},
	//# STD_Gain = max
	{.addr=0x1F2DA, .len=1,  .data={0x0A}},
	//# DDFS2_1_Step
	//#	{.addr=0x1F184, .len=2,  .data={0xA7, 0x0D}},
	//# DDFS2_2_Step
	{.addr=0x1F186, .len=2,  .data={0x93, 0x18}},
	//# DDFS2_3_Step
	{.addr=0x1F188, .len=2,  .data={0x93, 0x18}},
	//# VRADC_SEL = 1.75v
	{.addr=0x1F30B, .len=1,  .data={0x02}},
	//# DDFS1_1_Step
	{.addr=0x1F160, .len=3,  .data={0xBA, 0x49, 0x00}},
	//# RX_CFG all sensing
	{.addr=0x1F238, .len=33, .data={0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01}},
	//# Enable all TIA
	{.addr=0x1F2C8, .len=7,  .data={0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x07}},
	//# Enable all STD_EN
	{.addr=0x1F2D0, .len=7,  .data={0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x07}},
	//# Enable all ADC_EN
	{.addr=0x1F302, .len=7,  .data={0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x07}},
	//# Enable BIAS_En, STD_BIAS ; Disable TIA_BIAS CC_BIAS
	{.addr=0x1F2FD, .len=4,  .data={0x01, 0x01, 0x01, 0x00}},
	//# Unlock ADC power down
	{.addr=0x1F309, .len=1,  .data={0x00}},
	//# Enable Hanning filter
	{.addr=0x1F1C7, .len=1,  .data={0x20}},
	//# RXIG Gain
	{.addr=0x1F2DB, .len=1,  .data={0x03}},
	//# Raw1_Base_Addr
	{.addr=0x1F196, .len=2,  .data={0x88, 0x0F}},
	//# Raw2_Base_Addr
	{.addr=0x1F198, .len=2,  .data={0xB0, 0x14}},
	//# Workaround : set UC num >1
	//{.addr=0x1F203, .len=1,  .data={0x02}},
	//# POWER ON SEQUENCE
	{.addr=0x1F320, .len=1,  .data={0x06}},
	{.addr=0x1F321, .len=1,  .data={0x13}},
	{.addr=0x1F32A, .len=1,  .data={0x00}},
	{.addr=0x1F327, .len=1,  .data={0x03}},
	{.addr=0x1F328, .len=1,  .data={0x43}},
	{.addr=0x1F328, .len=1,  .data={0x13}},
	{.addr=0x1F328, .len=1,  .data={0x17}},
	//# DAC ON SEQUENCE
	{.addr=0x1F1D5, .len=1,  .data={0x40}},
	{.addr=0x1F1D6, .len=1,  .data={0xFF}},
	{.addr=0x1F1D5, .len=1,  .data={0x7F}},
	{.addr=0x1F1E0, .len=1,  .data={0x01}},
	{.addr=0x1F1DC, .len=1,  .data={0x01}},
	{.addr=0x1F1DE, .len=1,  .data={0x01}},
};

struct test_cmd open_test_tx_all_gnd[] = {
    {.addr=0x1F259, .len=AIN_TX_NUM, .data={0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
};

typedef enum {
	RESET_STATE_INIT = 0xA0,// IC reset
	RESET_STATE_REK,        // ReK baseline
	RESET_STATE_REK_FINISH, // baseline is ready
	RESET_STATE_NORMAL_RUN  // normal run
} RST_COMPLETE_STATE;

typedef enum {
	SHORT_RXRX = 0,
	SHORT_TXTX,
	SHORT_TXRX,
	OPEN,
	FW_MUTUAL
} CHANNEL_TEST_ITEM;

static int nvt_check_fw_reset_state(struct device *dev, RST_COMPLETE_STATE check_reset_state)
{
	int ret = 0;
	u8 buf[8] = {0, };
	int retry = 0;

	while(1) {
		msleep(20);

		//---read reset state---
		buf[0] = 0x60;
		buf[1] = 0x00;
		ret = nt11206_reg_read(dev, buf[0], &buf[1], 1);
		if(buf[1] >= check_reset_state) {
			ret = 0;
			break;
		}

		if(unlikely(retry++ > 50)) {
			ret = -1;
			TOUCH_E("%s: error, retry=%d, buf[1]=0x%02X\n", __func__, retry, buf[1]);
			break;
		}
	}

	return ret;
}

static void nvt_sw_reset_idle(struct device *dev)
{
	u8 buf[4] = {0, };
	int ret = 0;

	//---write i2c cmds to reset idle---
	buf[0] = 0xFF;
	buf[1] = 0x00;
	buf[2] = 0x00;
	ret = nt11206_bootloader_write(dev, buf, 3);

	//---write i2c cmds to reset idle---
	buf[0] = 0x00;
	buf[1] = 0xA5;
	ret = nt11206_bootloader_write(dev, buf, 2);

	large_mdelay(20);
	nvt_set_i2c_debounce(dev);
}

static int nvt_set_adc_oper(struct device *dev)
{
	int ret = 0;
	uint8_t buf[4] = {0,};
	int i;
	const int retry = 10;

	//---write i2c cmds to set ADC operation---
	buf[0] = 0x01;
	buf[1] = 0xF2;
	ret = nt11206_reg_write(dev, ADDR_CMD, buf, 2);

	//---write i2c cmds to set ADC operation---
	buf[0] = 0x10;
	buf[1] = 0x01;
	ret = nt11206_reg_write(dev, buf[0], &buf[1], 1);

	for(i=0; i<retry; i++) {
		//---read ADC status---
		buf[0]=0x10;
		ret = nt11206_reg_read(dev, buf[0], &buf[1], 1);

		if(buf[1] == 0x00) {
			break;
		}

		msleep(10);
	}

	if(i >= retry) {
		TOUCH_E("%s: Failed!\n", __func__);
		return -1;
	}
	else
		return 0;
}

static void nvt_write_test_cmd(struct device *dev, struct test_cmd *cmds, int cmd_num)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	u8 buf[64] = {0, };

	for(i=0; i<cmd_num; i++) {
		//---set xdata index---
		buf[0] = ((cmds[i].addr >> 16) & 0xFF);
		buf[1] = ((cmds[i].addr >> 8) & 0xFF);
		ret = nt11206_reg_write(dev, ADDR_CMD, buf, 2);

		//---write test cmds---
		buf[0] = (cmds[i].addr & 0xFF);
		for(j=0; j<cmds[i].len; j++)
		{
			buf[1+j] = cmds[i].data[j];
		}
		ret = nt11206_reg_write(dev, buf[0], &buf[1], cmds[i].len);
	}
}

static int nvt_read_short_rxrx(struct device *dev, __s32* rawdata_short_rxrx)
{
	int ret = 0;
	int i = 0;
	u8 buf[64] = {0, };

	//---write i2c cmds to set RX-RX mode---
	nvt_write_test_cmd(dev, short_test_rxrx, sizeof(short_test_rxrx)/sizeof(short_test_rxrx[0]));
	if(nvt_set_adc_oper(dev) != 0) {
		return -EAGAIN;
	}

	//---write i2c cmds to set RX all GND---
	for(i=0; i<AIN_RX_NUM; i++) {
		short_test_rx_all_gnd->data[i] = 0x02;	//set test pin to sample
		nvt_write_test_cmd(dev, short_test_rx_all_gnd, sizeof(short_test_rx_all_gnd)/sizeof(short_test_rx_all_gnd[0]));

		if(nvt_set_adc_oper(dev) != 0) {
			return -EAGAIN;
		}

		//---change xdata index---
		buf[0]=0x01;
		buf[1]=0x0F;
		ret = nt11206_reg_write(dev, ADDR_CMD, buf, 2);

		//---read data---
		buf[0]=(0x88 + i*2);
		ret = nt11206_reg_read(dev, buf[0], &buf[1], 2);
		rawdata_short_rxrx[i] = (s16)(buf[1] + 256*buf[2]);

		short_test_rx_all_gnd->data[i] = 0x01; //restore to default
	}
#if 0 //debuf
	TOUCH_I("[SHORT_RXRX_RAWDATA]\n");
	for(i=0; i<AIN_RX_NUM; i++) {
		printk("%5d, ", rawdata_short_rxrx[i]);
	}
	printk("\n");
#endif
	return 0;
}

static int nvt_read_short_txrx(struct device *dev, __s32 *rawdata_short_txrx)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	u8 buf[64] = {0, };

	//---write i2c cmds to set RX-RX mode---
	nvt_write_test_cmd(dev, short_test_txrx, sizeof(short_test_txrx)/sizeof(short_test_txrx[0]));
	if(nvt_set_adc_oper(dev) != 0) {
		return -EAGAIN;
	}

	//---write i2c cmds to set TX all Vref---
	for(i=0; i<AIN_TX_NUM; i++) {
		short_test_tx_all_vref->data[i] = 0x04;	//set test pin to GND
		nvt_write_test_cmd(dev, short_test_tx_all_vref, sizeof(short_test_tx_all_vref)/sizeof(short_test_tx_all_vref[0]));

		if(nvt_set_adc_oper(dev) != 0) {
			return -EAGAIN;
		}

		//---change xdata index---
		buf[0]=0x01;
		buf[1]=0x0F;
		ret = nt11206_reg_write(dev, ADDR_CMD, buf, 2);
		//---read data---
		buf[0]=0x88;
		ret = nt11206_reg_read(dev, buf[0], &buf[1], AIN_RX_NUM*2);

		for(j=0; j<AIN_RX_NUM; j++) {
			rawdata_short_txrx[i*AIN_RX_NUM+j] = (s16)(buf[j*2+1] + 256*buf[j*2+2]);
		}

		short_test_tx_all_vref->data[i] = 0x07; //restore to default
	}

#if 0 //debuf
	TOUCH_I("[SHORT_TXRX_RAWDATA]\n");
	for(i=0; i<AIN_TX_NUM; i++) {
		for(j=0; j<AIN_RX_NUM; j++) {
			printk("%5d", rawdata_short_txrx[i*AIN_RX_NUM+j]);
		}
		printk("\n");
	}
#endif
	return 0;
}

static int nvt_read_short_txtx(struct device *dev, __s32 *rawdata_short_txtx)
{
	int ret = 0;
	int i = 0;
	u8 buf[64] = {0, };

	//---write i2c cmds to set TX-TX mode---
	nvt_write_test_cmd(dev, short_test_txtx, sizeof(short_test_txtx)/sizeof(short_test_txtx[0]));
	if(nvt_set_adc_oper(dev) != 0) {
		return -EAGAIN;
	}

	//---write i2c cmds to set TX all GND---
	for(i=0; i<AIN_TX_NUM; i++) {
		short_test_tx_all_gnd->data[i] = 0x05;	//set test pin to sample
		nvt_write_test_cmd(dev, short_test_tx_all_gnd, sizeof(short_test_tx_all_gnd)/sizeof(short_test_tx_all_gnd[0]));

		if(nvt_set_adc_oper(dev) != 0) {
			return -EAGAIN;
		}

		//---change xdata index---
		buf[0]=0x01;
		buf[1]=0x0F;
		ret = nt11206_reg_write(dev, ADDR_CMD, buf, 2);
		//---read data---
		buf[0]=(0x88 + i*2);
		ret = nt11206_reg_read(dev, buf[0], &buf[1], 2);
		rawdata_short_txtx[i] = (s16)(buf[1] + 256 * buf[2]);

		short_test_tx_all_gnd->data[i] = 0x01; //restore to default
	}
#if 0 //debug
	TOUCH_I("[SHORT_TXTX_RAWDATA]\n");
	for(i=0; i<AIN_TX_NUM; i++) {
		printk("%5d, ", rawdata_short_txtx[i]);
	}
	printk("\n");
#endif
	return 0;
}
static u32 nvt_sqrt(u32 sqsum)
{
	u32 sq_rt = 0;

	int g0 = 0;
	int g1 = 0;
	int g2 = 0;
	int g3 = 0;
	int g4 = 0;
	int seed = 0;
	int next = 0;
	int step = 0;

	g4 =  sqsum / 100000000;
	g3 = (sqsum - g4*100000000) / 1000000;
	g2 = (sqsum - g4*100000000 - g3*1000000) / 10000;
	g1 = (sqsum - g4*100000000 - g3*1000000 - g2*10000) / 100;
	g0 = (sqsum - g4*100000000 - g3*1000000 - g2*10000 - g1*100);

	next = g4;
	step = 0;
	seed = 0;
	while(((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = seed * 10000;
	next = (next - (seed * step))*100 + g3;

	step = 0;
	seed = 2 * seed * 10;
	while(((seed + 1)*(step + 1)) <= next)
	{
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 1000;
	next = (next - seed * step) * 100 + g2;
	seed = (seed + step) * 10;
	step = 0;
	while(((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 100;
	next = (next - seed * step) * 100 + g1;
	seed = (seed + step) * 10;
	step = 0;

	while(((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 10;
	next = (next - seed * step) * 100 + g0;
	seed = (seed + step) * 10;
	step = 0;

	while(((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step;

	return sq_rt;
}

static int nvt_read_open(struct device *dev, s16 rawdata_open_raw1[], s16 rawdata_open_raw2[], __s32 rawdata_open[], __s32 rawdata_open_aver[], int index)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	u8 buf[64] = {0, };
	int raw_index = 0;

	//---write i2c cmds to set SingleScan mode---
	nvt_write_test_cmd(dev, open_test, sizeof(open_test)/sizeof(open_test[0]));
	if(nvt_set_adc_oper(dev) != 0) {
		return -EAGAIN;
	}

	//---write i2c cmds to set TX all GND---
	for(i=0; i<AIN_TX_NUM; i++) {
		open_test_tx_all_gnd->data[i] = 0x08; //set test pin to drive
		nvt_write_test_cmd(dev, open_test_tx_all_gnd, sizeof(open_test_tx_all_gnd)/sizeof(open_test_tx_all_gnd[0]));

		if(nvt_set_adc_oper(dev) != 0) {
			return -EAGAIN;
		}

		if(nvt_set_adc_oper(dev) != 0) {
			return -EAGAIN;
		}

		//---change xdata index---
		buf[0] = 0x01;
		buf[1] = 0x0F;
		ret = nt11206_reg_write(dev, ADDR_CMD, buf, 2);

		//---read data raw1---
		buf[0] = 0x88;
		ret = nt11206_reg_read(dev, buf[0], &buf[1], AIN_RX_NUM * 2);

		for(j=0; j<AIN_RX_NUM; j++)	{
			rawdata_open_raw1[i*AIN_RX_NUM+j] = (s16)(buf[j * 2 + 1] + 256 * buf[j * 2 + 2]);
		}

		//---change xdata index---
		buf[0] = 0x01;
		buf[1] = 0x14;
		ret = nt11206_reg_write(dev, ADDR_CMD, buf, 2);
		//---read data raw2---
		buf[0] = 0xB0;
		ret = nt11206_reg_read(dev, buf[0], &buf[1], AIN_RX_NUM * 2);

		for(j=0; j<AIN_RX_NUM; j++) {
			rawdata_open_raw2[i * AIN_RX_NUM+j] = (s16)(buf[j * 2 + 1] + 256 * buf[j * 2 + 2]);
		}

		open_test_tx_all_gnd->data[i] = 0x04; //restore to default

		//--IQ---
		for(j=0; j<AIN_RX_NUM; j++) {
			raw_index = i * AIN_RX_NUM + j;
			rawdata_open[raw_index] = nvt_sqrt(rawdata_open_raw1[raw_index] * rawdata_open_raw1[raw_index] + rawdata_open_raw2[raw_index] * rawdata_open_raw2[raw_index]);
			rawdata_open_aver[raw_index] += rawdata_open[raw_index];
		}
	}
	if(index == 2) {
		for(i = 0; i < AIN_TX_NUM; i++) {
			for(j = 0; j < AIN_RX_NUM; j++) {
				raw_index = i * AIN_RX_NUM + j;
				rawdata_open_aver[raw_index] = rawdata_open_aver[raw_index] / 3;
			}
		}
	}
#if 0 //debug
	TOUCH_I("[OPEN_RAWDATA]\n");
	for(i=0; i<AIN_TX_NUM; i++) {
		for(j=0; j<AIN_RX_NUM; j++)	{
			printk("%5d, ", rawdata_open[i*AIN_RX_NUM+j]);
		}
		printk("\n");
	}
#endif
	return 0;
}
static int nvt_read_baseline(struct device *dev, __s32 *xdata)
{
	u8 x_num = 0;
	u8 y_num = 0;

	nvt_change_mode(dev, TEST_MODE_1);

	if(nvt_clear_fw_status(dev) != 0) {
		return -EAGAIN;
	}

	if(nvt_check_fw_status(dev) != 0) {
		return -EAGAIN;
	}

	nvt_get_fw_info(dev);
	nvt_read_mdata(dev, BASELINE_ADDR);
	nvt_get_mdata(xdata, &x_num, &y_num);

	nvt_change_mode(dev, MODE_CHANGE_NORMAL_MODE);

	return 0;
}
static int Test_CaluateGRatioAndNormal(s16 boundary[], __s32 rawdata[], u8 x_len, u8 y_len)
{
	int i = 0;
	int j = 0;
	int k = 0;
	long int tmpValue = 0;
	long int MaxSum = 0;
	int SumCnt = 0;
	int MaxNum = 0;
	int MaxIndex = 0;
	int Max = -99999999;
	int Min =  99999999;
	int offset = 0;
	int Data = 0;	// double
	int StatisticsStep = 0;

	//--------------------------------------------------
	//1. (Testing_CM - Golden_CM ) / Testing_CM
	//--------------------------------------------------
	for(j=0; j<y_len; j++) {
		for(i=0; i<x_len; i++) {
			Data = rawdata[j * x_len + i];
			if(Data == 0)
				Data = 1;

			golden_Ratio[j * x_len + i] = Data - boundary[j * x_len + i];
			golden_Ratio[j * x_len + i] = ((golden_Ratio[j * x_len + i]*1000) / Data);	// *1000 before division
		}
	}

	//--------------------------------------------------------
	// 2. Mutual_GoldenRatio*1000
	//--------------------------------------------------------
	for(j=0; j<y_len; j++) {
		for(i=0; i<x_len; i++) {
			golden_Ratio[j * x_len + i] *= 1000;
		}
	}

	//--------------------------------------------------------
	// 3. Calculate StatisticsStep
	//--------------------------------------------------------
	for(j=0; j<y_len; j++) {
		for(i=0; i<x_len; i++) {
			if (Max < golden_Ratio[j * x_len + i])
				Max = (int)golden_Ratio[j * x_len + i];
			if (Min > golden_Ratio[j * x_len + i])
				Min = (int)golden_Ratio[j * x_len + i];
		}
	}

	offset = 0;
	if(Min < 0) // add offset to get erery element Positive
	{
		offset = 0 - Min;
		for(j=0; j<y_len; j++) {
			for(i=0; i<x_len; i++) {
				golden_Ratio[j * x_len + i] += offset;
			}
		}
		Max += offset;
	}
	StatisticsStep = Max / MaxStatisticsBuf;
	StatisticsStep += 1;
	if(StatisticsStep < 0) {
		TOUCH_E("FAIL! (StatisticsStep < 0)\n");
		return 1;
	}

	//--------------------------------------------------------
	// 4. Start Statistics and Average
	//--------------------------------------------------------
	memset(StatisticsSum, 0, sizeof(long int)*MaxStatisticsBuf);
	memset(StatisticsNum, 0, sizeof(int)* MaxStatisticsBuf);
	for(i=0; i<MaxStatisticsBuf; i++) {
		StatisticsSum[i] = 0;
		StatisticsNum[i] = 0;
	}
	for(j=0; j<y_len; j++) {
		for(i=0; i<x_len; i++) {
			tmpValue = golden_Ratio[j * x_len + i];
			tmpValue /= StatisticsStep;
			StatisticsNum[tmpValue] += 2;
			StatisticsSum[tmpValue] += (2 * golden_Ratio[j * x_len + i]);

			if((tmpValue + 1) < MaxStatisticsBuf) {
				StatisticsNum[tmpValue + 1] += 1;
				StatisticsSum[tmpValue + 1] += golden_Ratio[j * x_len + i];
			}

			if ((tmpValue - 1) >= 0) {
				StatisticsNum[tmpValue - 1] += 1;
				StatisticsSum[tmpValue - 1] += golden_Ratio[j * x_len + i];
			}
		}
	}
	//Find out Max Statistics
	MaxNum = 0;
	for(k=0; k<MaxStatisticsBuf; k++) {
		if(MaxNum < StatisticsNum[k]) {
			MaxSum = StatisticsSum[k];
			MaxNum = StatisticsNum[k];
			MaxIndex = k;
		}
	}
	//Caluate Statistics Average
	if(MaxSum > 0) {
		if (StatisticsNum[MaxIndex] != 0) {
			tmpValue = (long)(StatisticsSum[MaxIndex] / StatisticsNum[MaxIndex]) * 2;
			SumCnt += 2;
		}

		if ((MaxIndex + 1) < (MaxStatisticsBuf)) {
			if (StatisticsNum[MaxIndex + 1] != 0)
			{
				tmpValue += (long)(StatisticsSum[MaxIndex + 1] / StatisticsNum[MaxIndex + 1]);
				SumCnt++;
			}
		}

		if ((MaxIndex - 1) >= 0) {
			if (StatisticsNum[MaxIndex - 1] != 0) {
				tmpValue += (long)(StatisticsSum[MaxIndex - 1] / StatisticsNum[MaxIndex - 1]);
				SumCnt++;
			}
		}

		if (SumCnt > 0) {
			tmpValue /= SumCnt;
		}
	}
	else {// Too Separately
		StatisticsSum[0] = 0;
		StatisticsNum[0] = 0;
		for(j=0; j<y_len; j++) 	{
			for(i=0; i<x_len; i++) {
				StatisticsSum[0] += (long int)golden_Ratio[j * x_len + i];
				StatisticsNum[0]++;
			}
		}
		tmpValue = StatisticsSum[0] / StatisticsNum[0];
	}

	tmpValue -= offset;
	for(j= 0; j<y_len; j++) {
		for(i=0; i<x_len; i++) {
			golden_Ratio[j * x_len + i] -= offset;

			golden_Ratio[j * x_len + i] = golden_Ratio[j * x_len + i] - tmpValue;
			golden_Ratio[j * x_len + i] = golden_Ratio[j * x_len + i] / 1000;
		}
	}
	return 0;
}
static int RawDataTest_Sub(s16 boundary[], __s32 rawdata[], u8 RecordResult[], u8 x_ch, u8 y_ch,
		int Tol_P, int Tol_N, int Dif_P, int Dif_N, int Rawdata_Limit_Postive, int Rawdata_Limit_Negative)
{
	int i = 0;
	int j = 0;
	int iArrayIndex=0;
	__s32 iBoundary=0;
	s64 iTolLowBound = 0;
	s64 iTolHighBound = 0;
	bool isAbsCriteria = false;
	bool isPass = true;

	if(Rawdata_Limit_Postive != 0 || Rawdata_Limit_Negative != 0) {
		isAbsCriteria = true;
	}

	for(j=0; j<y_ch; j++) {
		for(i=0; i<x_ch; i++) {
			iArrayIndex = j * x_ch + i;
			iBoundary = boundary[iArrayIndex];

			RecordResult[iArrayIndex] = 0x00;	// default value for PASS

			if(isAbsCriteria) {
				iTolLowBound = Rawdata_Limit_Negative * 1000;
				iTolHighBound = Rawdata_Limit_Postive * 1000;
			}
			else {
				if(iBoundary > 0) {
					iTolLowBound = (iBoundary * (1000 + Tol_N));
					iTolHighBound = (iBoundary * (1000 + Tol_P));
				}
				else {
					iTolLowBound = (iBoundary * (1000 - Tol_N));
					iTolHighBound = (iBoundary * (1000 - Tol_P));
				}
			}

			if((rawdata[iArrayIndex] * 1000) > iTolHighBound) {
				RecordResult[iArrayIndex] |= 0x01;
			}

			if((rawdata[iArrayIndex] * 1000) < iTolLowBound) {
				RecordResult[iArrayIndex] |= 0x02;
			}
		}
	}

	if(!isAbsCriteria) {
		Test_CaluateGRatioAndNormal(boundary, rawdata, x_ch, y_ch);

		for(j=0; j<y_ch; j++) {
			for(i=0; i<x_ch; i++) {
				iArrayIndex = j * x_ch + i;

				if(golden_Ratio[iArrayIndex] > Dif_P) {
					RecordResult[iArrayIndex] |= 0x04;
				}

				if(golden_Ratio[iArrayIndex] < Dif_N) {
					RecordResult[iArrayIndex] |= 0x08;
				}
			}
		}
	}

	//---Check RecordResult---
	for(j=0; j<y_ch; j++) {
		for(i=0; i<x_ch; i++) {
			if(RecordResult[j * x_ch + i] != 0) {
				isPass = false;
				break;
			}
		}
	}

	if(isPass == false) {
		return -1;	// FAIL
	}
	else {
		return 0;	// PASS
	}
}
void print_selftest_result(struct device *dev, CHANNEL_TEST_ITEM item, int TestResult, u8 RecordResult[], __s32 rawdata[], u8 x_len, u8 y_len, int mode)
{
	struct nt11206_data *d = to_nt11206_data(dev);
	char *write_buf = NULL;
	int ret = 0;
    int i = 0;
	int j = 0;

	if(mode) {
		return;
	}

	write_buf = kmalloc(sizeof(char) * LOG_BUF_SIZE, GFP_KERNEL);
	if(write_buf) {
		memset(write_buf, 0, sizeof(char) * LOG_BUF_SIZE);
	}
	else {
		return;
	}

	switch(item) {
		case SHORT_RXRX:
			TOUCH_I("[SHORT_RXRX_RAWDATA]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[SHORT_RXRX_RAWDATA]\n");
		break;

		case SHORT_TXTX:
			TOUCH_I("[SHORT_TXTX_RAWDATA]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[SHORT_TXTX_RAWDATA]\n");
		break;

		case SHORT_TXRX:
			TOUCH_I("[SHORT_TXRX_RAWDATA]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[SHORT_TXRX_RAWDATA]\n");
		break;

		case OPEN:
			TOUCH_I("[OPEN_RAWDATA]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[OPEN_RAWDATA]\n");
		break;

		case FW_MUTUAL:
			TOUCH_I("[FW_MUTUAL_RAWDATA]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[FW_MUTUAL_RAWDATA]\n");
		break;

		default:

		break;
	}

	for(i=0; i<y_len; i++) {
		ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[%2d]", i);
		for(j=0; j<x_len; j++) {
			printk("%7d", rawdata[i*x_len+j]);
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "%7d", rawdata[i*x_len+j]);
		}
		printk("\n");
		ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "\n");
	}

	switch(item) {
		case SHORT_RXRX:
			TOUCH_I("[SHORT_RXRX_RESULT]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[SHORT_RXRX_RESULT]\n");
		break;

		case SHORT_TXTX:
			TOUCH_I("[SHORT_TXTX_RESULT]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[SHORT_TXTX_RESULT]\n");
		break;

		case SHORT_TXRX:
			TOUCH_I("[SHORT_TXRX_RESULT]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[SHORT_TXRX_RESULT]\n");
		break;

		case OPEN:
			TOUCH_I("[OPEN_RESULT]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[OPEN_RESULT]\n");
		break;

		case FW_MUTUAL:
			TOUCH_I("[FW_MUTUAL_RESULT]\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[FW_MUTUAL_RESULT]\n");
		break;
		default:

		break;
	}

	switch(TestResult) {
		case 0:
			TOUCH_I("PASS!\n\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[PASS]\n\n");
		break;

		case 1:
			TOUCH_E("ERROR! Read Data FAIL!\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "ERROR! Read Data FAIL!\n");
		break;

		case -1:
			TOUCH_E ("FAIL!\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[FAIL]\n");
		break;
	}

	if(d->boot_mode == NORMAL_BOOT) {
		write_file(NORMAL_SELF_TEST_FILE_PATH, write_buf, 1);
		log_file_size_check(dev, NORMAL_SELF_TEST_FILE_PATH);
	}
	else {
		write_file(SELF_TEST_FILE_PATH, write_buf, 1);
		log_file_size_check(dev, SELF_TEST_FILE_PATH);
	}

	if(TestResult == -1) {
		memset(write_buf, 0, sizeof(char) * LOG_BUF_SIZE);
		ret = 0;
		TOUCH_I("RecordResult:\n");
		ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "[RECORD_RESULT]\n");
		for(i=0; i<y_len; i++) {
			for(j=0; j<x_len; j++) {
				printk("0x%02X, ", RecordResult[i*x_len+j]);
				ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "0x%02X, ", RecordResult[i*x_len+j]);
			}
			printk("\n");
			ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "\n");
		}
		ret += snprintf(write_buf + ret, LOG_BUF_SIZE - ret, "\n");
		printk("\n");
		if(d->boot_mode == NORMAL_BOOT) {
			write_file(NORMAL_SELF_TEST_FILE_PATH, write_buf, 1);
			log_file_size_check(dev, NORMAL_SELF_TEST_FILE_PATH);
		}
		else {
			write_file(SELF_TEST_FILE_PATH, write_buf, 1);
			log_file_size_check(dev, SELF_TEST_FILE_PATH);
		}
	}

	if(write_buf) {
		kfree(write_buf);
	}
}
#if 0 //FOR DEBUGGING
void print_baseline_check_result(struct device *dev, CHANNEL_TEST_ITEM item, int TestResult, u8 RecordResult[], __s32 rawdata[], u8 x_len, u8 y_len)
{
    int i = 0;
	int j = 0;

	switch(TestResult) {
		case 0:
			TOUCH_I("PASS!\n\n");
		break;

		case 1:
			TOUCH_E("ERROR! Read Data FAIL!\n");
		break;

		case -1:
			TOUCH_E ("FAIL!\n");
		break;
	}

	if(TestResult == -1) {
		TOUCH_I("[FW_MUTUAL_RAWDATA]\n");

		for(i=0; i<y_len; i++) {
			for(j=0; j<x_len; j++) {
				printk("%7d", rawdata[i*x_len+j]);
			}
			printk("\n");
		}

		TOUCH_I("[FW_MUTUAL_RESULT]\n");

		TOUCH_I("RecordResult:\n");
		for(i=0; i<y_len; i++) {
			for(j=0; j<x_len; j++) {
				printk("0x%02X, ", RecordResult[i*x_len+j]);
			}
			printk("\n");
		}
	}
}
#endif
int nt11206_selftest(struct device *dev, char* buf, u8 mode)
{
	struct nt11206_data *d = to_nt11206_data(dev);
	u8 x_num=0;
	u8 y_num=0;
	int ret = 0;
	int i = 0;
	int TestResult_Short_RXRX = 0;
	int TestResult_Short_TXRX = 0;
	int TestResult_Short_TXTX = 0;
	int TestResult_Open = 0;
	int TestResult_FW_MUTUAL = 0;

	u8 *RecordResult = NULL;
	s16 *rawdata_16 = NULL;
	s16 *rawdata_16_1 = NULL;
	__s32 *rawdata_32 = NULL;
	__s32 *open_rawdata_32 = NULL;

	RecordResult = (u8*)kmalloc(sizeof(u8) * 20 * 30, GFP_KERNEL);
	rawdata_16 = (s16*)kmalloc(sizeof(s16) * 20 * 30, GFP_KERNEL);
	rawdata_16_1 = (s16*)kmalloc(sizeof(s16) * 20 * 30, GFP_KERNEL);
	rawdata_32 = (__s32*)kmalloc(sizeof(u32) * 20 * 30, GFP_KERNEL);
	open_rawdata_32 = (__s32*)kmalloc(sizeof(u32) * 20 * 30, GFP_KERNEL);

	x_num = d->fw.x_axis_num;
	y_num = d->fw.y_axis_num;

	if((RecordResult == NULL) || (rawdata_16 == NULL) || (rawdata_16_1 == NULL) || (rawdata_32 == NULL) || (open_rawdata_32 == NULL)) {
		TOUCH_E("Alloc Failed\n");
		if(RecordResult)
			kfree(RecordResult);

		if(rawdata_16)
			kfree(rawdata_16);

		if(rawdata_16_1)
			kfree(rawdata_16_1);

		if(rawdata_32)
			kfree(rawdata_32);

		if(open_rawdata_32)
			kfree(open_rawdata_32);
		return -1;
	}

	if(	d->resume_state == 0 && !mode) {
		TOUCH_E("LCD OFF, mode:%d\n", mode);
		ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "LCD OFF\n");
		if(RecordResult)
			kfree(RecordResult);

		if(rawdata_16)
			kfree(rawdata_16);

		if(rawdata_16_1)
			kfree(rawdata_16_1);

		if(rawdata_32)
			kfree(rawdata_32);

		if(open_rawdata_32)
			kfree(open_rawdata_32);
		return ret;
	}
	TOUCH_I("x_axis_num:%d, y_axis_num:%d\n", d->fw.x_axis_num, d->fw.y_axis_num);
	if((x_num != 20) || (y_num != 30)) {
		TOUCH_E("FW Info is broken\n");
		x_num = 20;
		y_num = 30;
	}

	//---Reset IC & into idle---
	nt11206_hw_reset(dev);
	nvt_change_mode(dev, AUTORC_OFF);
	nvt_check_fw_reset_state(dev, RESET_STATE_REK_FINISH);

	if(nvt_read_baseline(dev, rawdata_32) != 0) {
		TestResult_FW_MUTUAL = 1; //1:ERROR
	}
	else {
		//---Self Test Check ---	// 0:PASS, -1:FAIL
		TestResult_FW_MUTUAL = RawDataTest_Sub(BoudaryFWMutual, rawdata_32, RecordResult, AIN_TX_NUM, AIN_RX_NUM,
				PSConfig_Tolerance_Postive_FW, PSConfig_Tolerance_Negative_FW, PSConfig_DiffLimitG_Postive_FW, PSConfig_DiffLimitG_Negative_FW,
				0, 0);
	}
	print_selftest_result(dev, FW_MUTUAL, TestResult_FW_MUTUAL, RecordResult, rawdata_32, AIN_TX_NUM, AIN_RX_NUM, mode);
	if(mode) {
		ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "LPWG RAWDATA : ");
	}
	else {
		ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "Raw Data : ");
	}
	switch(TestResult_FW_MUTUAL) {
		case 0:
			ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "Pass\n");
			break;

		case 1:
			ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "Fail\n");
			ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "Self Test ERROR! Read Data FAIL!\n");
			break;

		case -1:
			ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "Fail\n");
			break;
	}
	if(mode) {
		if(RecordResult)
			kfree(RecordResult);

		if(rawdata_16)
			kfree(rawdata_16);

		if(rawdata_16_1)
			kfree(rawdata_16_1);

		if(rawdata_32)
			kfree(rawdata_32);

		if(open_rawdata_32)
			kfree(open_rawdata_32);

		nt11206_hw_reset(dev);
		return ret;
	}
	//---Reset IC & into idle---
	nvt_sw_reset_idle(dev);
	msleep(100);

	//---Short Test RX-RX---
	memset(RecordResult, 0, sizeof(u8) * 20 * 30);
	memset(rawdata_16, 0, sizeof(s16) * 20 * 30);
	memset(rawdata_16_1, 0, sizeof(s16) * 20 * 30);
	memset(rawdata_32, 0, sizeof(__s32) * 20 * 30);
	if(nvt_read_short_rxrx(dev, rawdata_32) != 0) {
		TestResult_Short_RXRX = 1;	// 1:ERROR
	}
	else {
		//---Self Test Check --- 		// 0:PASS, -1:FAIL
		TestResult_Short_RXRX = RawDataTest_Sub(BoundaryShort_RXRX, rawdata_32, RecordResult, AIN_RX_NUM, 1,
												PSConfig_Tolerance_Postive_Short, PSConfig_Tolerance_Negative_Short, PSConfig_DiffLimitG_Postive_Short, PSConfig_DiffLimitG_Negative_Short,
												PSConfig_Rawdata_Limit_Postive_Short_RXRX, PSConfig_Rawdata_Limit_Negative_Short_RXRX);
	}
	print_selftest_result(dev, SHORT_RXRX, TestResult_Short_RXRX, RecordResult, rawdata_32, AIN_RX_NUM, 1, mode);

	//---Short Test TX-RX---
	memset(RecordResult, 0, sizeof(u8) * 20 * 30);
	memset(rawdata_16, 0, sizeof(s16) * 20 * 30);
	memset(rawdata_16_1, 0, sizeof(s16) * 20 * 30);
	memset(rawdata_32, 0, sizeof(__s32) * 20 * 30);
	if(nvt_read_short_txrx(dev, rawdata_32) != 0) {
		TestResult_Short_TXRX = 1;	// 1:ERROR
	}
	else {
		//---Self Test Check ---		// 0:PASS, -1:FAIL
		TestResult_Short_TXRX = RawDataTest_Sub(BoundaryShort_TXRX, rawdata_32, RecordResult, AIN_TX_NUM, AIN_RX_NUM,
    											PSConfig_Tolerance_Postive_Short, PSConfig_Tolerance_Negative_Short, PSConfig_DiffLimitG_Postive_Short, PSConfig_DiffLimitG_Negative_Short,
    											PSConfig_Rawdata_Limit_Postive_Short_TXRX, PSConfig_Rawdata_Limit_Negative_Short_TXRX);
	}
	print_selftest_result(dev, SHORT_TXRX, TestResult_Short_TXRX, RecordResult, rawdata_32, AIN_TX_NUM, AIN_RX_NUM, mode);

	//---Short Test TX-TX---
	memset(RecordResult, 0, sizeof(u8) * 20 * 30);
	memset(rawdata_16, 0, sizeof(s16) * 20 * 30);
	memset(rawdata_16_1, 0, sizeof(s16) * 20 * 30);
	memset(rawdata_32, 0, sizeof(__s32) * 20 * 30);
	if(nvt_read_short_txtx(dev, rawdata_32) != 0) {
		TestResult_Short_TXTX = 1;	// 1:ERROR
	}
	else {
		//---Self Test Check ---		// 0:PASS, -1:FAIL
		TestResult_Short_TXTX = RawDataTest_Sub(BoundaryShort_TXTX, rawdata_32, RecordResult, AIN_TX_NUM, 1,
												PSConfig_Tolerance_Postive_Short, PSConfig_Tolerance_Negative_Short, PSConfig_DiffLimitG_Postive_Short, PSConfig_DiffLimitG_Negative_Short,
												PSConfig_Rawdata_Limit_Postive_Short_TXTX, PSConfig_Rawdata_Limit_Negative_Short_TXTX);
	}

	print_selftest_result(dev, SHORT_TXTX, TestResult_Short_TXTX, RecordResult, rawdata_32, AIN_TX_NUM, 1, mode);

	//---Reset IC & into idle---
	nvt_sw_reset_idle(dev);
	msleep(100);
	memset(open_rawdata_32, 0, sizeof(__s32) * 20 * 30);
	for(i = 0; i < 3; i++) {
		//---Open Test---
		memset(RecordResult, 0, sizeof(u8) * 20 * 30);
		memset(rawdata_16, 0, sizeof(s16) * 20 * 30);
		memset(rawdata_16_1, 0, sizeof(s16) * 20 * 30);
		memset(rawdata_32, 0, sizeof(__s32) * 20 * 30);
		TestResult_Open= nvt_read_open(dev, rawdata_16, rawdata_16_1, rawdata_32, open_rawdata_32, i);
		if(TestResult_Open != 0) {
			TestResult_Open = 1;	// 1:ERROR
			break;
		}
		msleep(10);
	}
	TestResult_Open = RawDataTest_Sub(BoundaryOpen, open_rawdata_32, RecordResult,AIN_TX_NUM, AIN_RX_NUM,
			PSConfig_Tolerance_Postive_Mutual, PSConfig_Tolerance_Negative_Mutual, PSConfig_DiffLimitG_Postive_Mutual, PSConfig_DiffLimitG_Negative_Mutual,
			0, 0);
	print_selftest_result(dev, OPEN, TestResult_Open, RecordResult, open_rawdata_32, AIN_TX_NUM, AIN_RX_NUM, mode);

	ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "Channel Status : ");
	if(!TestResult_Short_RXRX && !TestResult_Short_TXRX && !TestResult_Short_TXTX && !TestResult_Open) {
		ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "Pass\n");
	}
	else {
		ret += snprintf(buf + ret, LOG_BUF_SIZE - ret, "Fail\n");
	}

	nt11206_hw_reset(dev);

	if(RecordResult)
		kfree(RecordResult);

	if(rawdata_16)
		kfree(rawdata_16);

	if(rawdata_16_1)
		kfree(rawdata_16_1);

	if(rawdata_32)
		kfree(rawdata_32);

	if(open_rawdata_32)
		kfree(open_rawdata_32);

	return ret;
}
