/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
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
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/power_supply.h>


#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/setup.h>
#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>
#endif

#include <asm/mach/mmc.h>
#include <mach/vreg.h>
#include <mach/mpp.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_rpcrouter.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#include <mach/msm_serial_hs.h>
#include <mach/memory.h>
#include <mach/msm_battery.h>
#include <mach/rpc_server_handset.h>
#include <mach/msm_tsif.h>

#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/i2c.h>
#include <linux/android_pmem.h>
#include <mach/camera.h>
// {init dmesg history proc file while board init, xuxian 20110217
#include "dmesg_history.h"
// }
#include "devices.h"
#include "socinfo.h"
#include "clock.h"
#include "msm-keypad-devices.h"
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android.h>
#endif
#include "pm.h"
#ifdef CONFIG_ARCH_MSM7X27
#include <linux/msm_kgsl.h>
#endif

#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android.h>
#endif
#include <linux/atmel_qt602240.h>
#ifdef CONFIG_ARCH_MSM7X25
#define MSM_PMEM_MDP_SIZE	0xb21000
#define MSM_PMEM_ADSP_SIZE	0x97b000
#define MSM_PMEM_AUDIO_SIZE	0x121000
#define MSM_FB_SIZE		0x200000
#define PMEM_KERNEL_EBI1_SIZE	0x64000
#endif

//JRD add for battery capacity
#ifdef FEATUE_JRD_BATTERY_CAPACITY
extern const u32 jrd_batt_level[];
#endif

//add for lcd type
#define TDT_LCD				0x10
#define TRULY_LCD				0x01
#define FOXCONN_LCD			0x00
#define TRULY_LCD_HX8357		0x11
//

#ifdef CONFIG_ARCH_MSM7X27
#define MSM_PMEM_MDP_SIZE	0x1B76000
#define MSM_PMEM_ADSP_SIZE	0xB71000
#define MSM_PMEM_AUDIO_SIZE	0x5B000
#define MSM_PMEM_GPU1_SIZE      0x1600000
#define MSM_FB_SIZE		0x177000
#define MSM_GPU_PHYS_SIZE	SZ_2M
#define PMEM_KERNEL_EBI1_SIZE	0x1C000
/* Using lower 1MB of OEMSBL memory for GPU_PHYS */
#define MSM_GPU_PHYS_START_ADDR	 0xD600000ul
#endif

/* Using upper 1/2MB of Apps Bootloader memory*/
#define MSM_PMEM_AUDIO_START_ADDR	0x80000ul
static struct resource smc91x_resources[] = {
	[0] = {
		.start	= 0x9C004300,
		.end	= 0x9C0043ff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= MSM_GPIO_TO_INT(132),
		.end	= MSM_GPIO_TO_INT(132),
		.flags	= IORESOURCE_IRQ,
	},
};


struct vreg {
        const char *name;
        unsigned id;
        int status;
        unsigned refcnt;
};


#define VREG(_name, _id, _status, _refcnt) \
        { .name = _name, .id = _id, .status = _status, .refcnt = _refcnt }

//static struct vreg vreg_gp2 =VREG("gp2",    21, 0, 0);
//static struct vreg vreg_gp3 =VREG("gp3",    5, 0, 0);


#ifdef CONFIG_USB_FUNCTION
static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.nluns          = 0x02,
	.buf_size       = 16384,
	.vendor         = "GOOGLE",
	.product        = "Mass storage",
	.release        = 0xffff,
};

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &usb_mass_storage_pdata,
	},
};
#endif
#ifdef CONFIG_USB_ANDROID
/* dynamic composition */
static struct usb_composition usb_func_composition[] = {
	{
		/* MSC */
		.product_id         = 0xF000,
		.functions	    = 0x02,
		.adb_product_id     = 0x9015,
		.adb_functions	    = 0x12
	},
#ifdef CONFIG_USB_F_SERIAL
	{
		/* MODEM */
		.product_id         = 0xF00B,
		.functions	    = 0x06,
		.adb_product_id     = 0x901E,
		.adb_functions	    = 0x16,
	},
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	{
		/* DIAG */
		.product_id         = 0x900E,
		.functions	    = 0x04,
		.adb_product_id     = 0x901D,
		.adb_functions	    = 0x14,
	},
#endif
#if defined(CONFIG_USB_ANDROID_DIAG) && defined(CONFIG_USB_F_SERIAL)
	{
		/* DIAG + MODEM */
		.product_id         = 0x9004,
		.functions	    = 0x64,
		.adb_product_id     = 0x901F,
		.adb_functions	    = 0x0614,
	},
	{
		/* DIAG + MODEM + NMEA*/
		.product_id         = 0x9016,
		.functions	    = 0x764,
		.adb_product_id     = 0x9020,
		.adb_functions	    = 0x7614,
	},
#ifdef CONFIG_USB_ANDROID_ACM /* ajayet : added for kgdb */
	{
		/* DIAG + MODEM + NMEA + MSC + ACM */
		.product_id         = 0x9017,
		.functions	    = 0x52764,
		.adb_product_id     = 0x9018,
		.adb_functions	    = 0x527614, /* ajayet added ANDROID_ACM_NMEA ( bound to ttyGS1) to use with kgdb */
	},
#else
	{
		/* DIAG + MODEM + NMEA + MSC */
		.product_id         = 0x9017,
		.functions	    = 0x2764,
		.adb_product_id     = 0x9018,
		.adb_functions	    = 0x27614,
	},
#endif
#endif
#ifdef CONFIG_USB_ANDROID_CDC_ECM
	{
		/* MSC + CDC-ECM */
		.product_id         = 0x9014,
		.functions	    = 0x82,
		.adb_product_id     = 0x9023,
		.adb_functions	    = 0x812,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	{
		/* DIAG + RMNET */
		.product_id         = 0x9021,
		.functions	    = 0x94,
		.adb_product_id     = 0x9022,
		.adb_functions	    = 0x914,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	{
		/* RNDIS */
		.product_id         = 0xF00E,
		.functions	    = 0xA,
		.adb_product_id     = 0x9024,
		.adb_functions	    = 0x1A,
	},
#endif
};
static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "GOOGLE",
	.product	= "Mass Storage",
	.release	= 0xFFFF,
};
static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &mass_storage_pdata,
	},
};
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x05C6,
	.version	= 0x0100,
	.compositions   = usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.product_name	= "Qualcomm HSUSB Device",
	.manufacturer_name = "Qualcomm Incorporated",
};
static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};
#endif
static struct platform_device smc91x_device = {
	.name		= "smc91x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smc91x_resources),
	.resource	= smc91x_resources,
};

#ifdef CONFIG_USB_FUNCTION
static struct usb_function_map usb_functions_map[] = {
	{"diag", 0},
	{"adb", 1},
	{"modem", 2},
	{"nmea", 3},
	{"mass_storage", 4},
	{"ethernet", 5},
	{"rmnet", 6},
};

/* dynamic composition */
static struct usb_composition usb_func_composition[] = {
	{
		.product_id         = 0x9012,
		.functions	    = 0x5, /* 0101 */
	},

	{
		.product_id         = 0x9013,
		.functions	    = 0x15, /* 10101 */
	},

	{
		.product_id         = 0x9014,
		.functions	    = 0x30, /* 110000 */
	},

	{
		.product_id         = 0x9016,
		.functions	    = 0xD, /* 01101 */
	},

	{
		.product_id         = 0x9017,
		.functions	    = 0x1D, /* 11101 */
	},

	{
		.product_id         = 0xF000,
		.functions	    = 0x10, /* 10000 */
	},

	{
		.product_id         = 0xF009,
		.functions	    = 0x20, /* 100000 */
	},

	{
		.product_id         = 0x9018,
		.functions	    = 0x1F, /* 011111 */
	},

#ifdef CONFIG_USB_FUNCTION_RMNET
	{
		.product_id         = 0x9021,
		/* DIAG + RMNET */
		.functions	    = 0x41,
	},
	{
		.product_id         = 0x9022,
		/* DIAG + ADB + RMNET */
		.functions	    = 0x43,
	},
#endif


};

static struct msm_hsusb_platform_data msm_hsusb_pdata = {
	.version	= 0x0100,
	.phy_info	= (USB_PHY_INTEGRATED | USB_PHY_MODEL_65NM),
	.vendor_id          = 0x5c6,
	.product_name       = "Qualcomm HSUSB Device",
	.serial_number      = "1234567890ABCDEF",
	.manufacturer_name  = "Qualcomm Incorporated",
	.compositions	= usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.function_map   = usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.config_gpio    = NULL,
};
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static struct msm_otg_platform_data msm_otg_pdata = {
	.rpc_connect	= hsusb_rpc_connect,
	.pmic_notif_init         = msm_pm_app_rpc_init,
	.pmic_notif_deinit       = msm_pm_app_rpc_deinit,
	.pmic_register_vbus_sn   = msm_pm_app_register_vbus_sn,
	.pmic_unregister_vbus_sn = msm_pm_app_unregister_vbus_sn,
	.pmic_enable_ldo         = msm_pm_app_enable_usb_ldo,
	.pclk_required_during_lpm = 1,
};

#ifdef CONFIG_USB_GADGET
static struct msm_hsusb_gadget_platform_data msm_gadget_pdata;
#endif
#endif

#define SND(desc, num) { .name = #desc, .id = num }
static struct snd_endpoint snd_endpoints_list[] = {
	SND(HANDSET, 0),
	SND(MONO_HEADSET, 2),
	SND(HEADSET, 3),
	SND(SPEAKER, 6),
	SND(TTY_HEADSET, 8),
	SND(TTY_VCO, 9),
	SND(TTY_HCO, 10),
	SND(BT, 12),
	SND(IN_S_SADC_OUT_HANDSET, 16),
	SND(IN_S_SADC_OUT_SPEAKER_PHONE, 25),
	SND(FM_HEADSET, 26),               /*Liao add for FM*/
	SND(FM_SPEAKER, 27),
	SND(HANDSET_HAC, 28),
	SND(IN_S_SADC_OUT_HANDSET_HAC, 29),
	SND(CURRENT, 31),
};
#undef SND

static struct msm_snd_endpoints msm_device_snd_endpoints = {
	.endpoints = snd_endpoints_list,
	.num = sizeof(snd_endpoints_list) / sizeof(struct snd_endpoint)
};

static struct platform_device msm_device_snd = {
	.name = "msm_snd",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_snd_endpoints
	},
};

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_WAV)|(1<<MSM_ADSP_CODEC_ADPCM)| \
	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_WAV)|(1<<MSM_ADSP_CODEC_ADPCM)| \
	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP))
#ifdef CONFIG_ARCH_MSM7X25
#define DEC3_FORMAT 0
#define DEC4_FORMAT 0
#else
#define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_WAV)|(1<<MSM_ADSP_CODEC_ADPCM)| \
	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)
#endif

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DMA)), 0,
	0, 0, 0,

	/* Concurrency 1 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	 /* Concurrency 2 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 3 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 4 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 5 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 6 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 4),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 4),  /* AudPlay2BitStreamCtrlQueue */
#ifdef CONFIG_ARCH_MSM7X25
	DEC_INFO("AUDPLAY3TASK", 16, 3, 0),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY4TASK", 17, 4, 0),  /* AudPlay4BitStreamCtrlQueue */
#else
	DEC_INFO("AUDPLAY3TASK", 16, 3, 4),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
#endif
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
	.name = PMEM_KERNEL_EBI1_DATA_NAME,
	/* if no allocator_type, defaults to PMEM_ALLOCATORTYPE_BITMAP,
	 * the only valid choice at this time. The board structure is
	 * set to all zeros by the C runtime initialization and that is now
	 * the enum value of PMEM_ALLOCATORTYPE_BITMAP, now forced to 0 in
	 * include/linux/android_pmem.h.
	 */
	.cached = 0,
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

static struct platform_device android_pmem_kernel_ebi1_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

/* TSIF begin */
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)

#define TSIF_B_SYNC      GPIO_CFG(87, 5, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)
#define TSIF_B_DATA      GPIO_CFG(86, 3, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)
#define TSIF_B_EN        GPIO_CFG(85, 3, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)
#define TSIF_B_CLK       GPIO_CFG(84, 4, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)

static const struct msm_gpio tsif_gpios[] = {
	{ .gpio_cfg = TSIF_B_CLK,  .label =  "tsif_clk", },
	{ .gpio_cfg = TSIF_B_EN,   .label =  "tsif_en", },
	{ .gpio_cfg = TSIF_B_DATA, .label =  "tsif_data", },
	{ .gpio_cfg = TSIF_B_SYNC, .label =  "tsif_sync", },
};

static struct msm_tsif_platform_data tsif_platform_data = {
	.num_gpios = ARRAY_SIZE(tsif_gpios),
	.gpios = tsif_gpios,
	.tsif_clk = "tsif_clk",
	.tsif_pclk = "tsif_pclk",
	.tsif_ref_clk = "tsif_ref_clk",
};
#endif /* defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE) */
/* TSIF end   */

#define LCDC_CONFIG_PROC          21
#define LCDC_UN_CONFIG_PROC       22
#define LCDC_API_PROG             0x30000066
#define LCDC_API_VERS             0x00010001

#define GPIO_OUT_132    132
#define GPIO_OUT_131    131
#define GPIO_OUT_103    103
#define GPIO_OUT_102    102
#define GPIO_OUT_88     88

static struct msm_rpc_endpoint *lcdc_ep;

static int msm_fb_lcdc_config(int on)
{
	int rc = 0;
	struct rpc_request_hdr hdr;

	if (on)
		pr_info("lcdc config\n");
	else
		pr_info("lcdc un-config\n");

	lcdc_ep = msm_rpc_connect_compatible(LCDC_API_PROG, LCDC_API_VERS, 0);
	if (IS_ERR(lcdc_ep)) {
		printk(KERN_ERR "%s: msm_rpc_connect failed! rc = %ld\n",
			__func__, PTR_ERR(lcdc_ep));
		return -EINVAL;
	}

	rc = msm_rpc_call(lcdc_ep,
				(on) ? LCDC_CONFIG_PROC : LCDC_UN_CONFIG_PROC,
				&hdr, sizeof(hdr),
				5 * HZ);
	if (rc)
		printk(KERN_ERR
			"%s: msm_rpc_call failed! rc = %d\n", __func__, rc);

	msm_rpc_close(lcdc_ep);
	return rc;
}

static int gpio_array_num[] = {
				GPIO_OUT_132, /* spi_clk */
				GPIO_OUT_131, /* spi_cs  */
				GPIO_OUT_103, /* spi_sdi */
				GPIO_OUT_102, /* spi_sdoi */
				GPIO_OUT_88
				};

static void lcdc_gordon_gpio_init(void)
{
	if (gpio_request(GPIO_OUT_132, "spi_clk"))
		pr_err("failed to request gpio spi_clk\n");
	if (gpio_request(GPIO_OUT_131, "spi_cs"))
		pr_err("failed to request gpio spi_cs\n");
	if (gpio_request(GPIO_OUT_103, "spi_sdi"))
		pr_err("failed to request gpio spi_sdi\n");
	if (gpio_request(GPIO_OUT_102, "spi_sdoi"))
		pr_err("failed to request gpio spi_sdoi\n");
	if (gpio_request(GPIO_OUT_88, "gpio_dac"))
		pr_err("failed to request gpio_dac\n");
}

static void lcdc_gpio_init(void)
{
	if (gpio_request(GPIO_OUT_132, "spi_clk"))
		pr_err("failed to request gpio spi_clk\n");
	if (gpio_request(GPIO_OUT_131, "spi_cs"))
		pr_err("failed to request gpio spi_cs\n");
	if (gpio_request(GPIO_OUT_103, "spi_sdi"))
		pr_err("failed to request gpio spi_sdi\n");
	if (gpio_request(GPIO_OUT_102, "spi_sdoi"))
		pr_err("failed to request gpio spi_sdoi\n");
	if (gpio_request(GPIO_OUT_88, "gpio_dac"))
		pr_err("failed to request gpio_dac\n");
}

static uint32_t lcdc_gpio_table[] = {
	GPIO_CFG(GPIO_OUT_132, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_OUT_131, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_OUT_103, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	//GPIO_CFG(GPIO_OUT_102, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_OUT_88,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
};

//static            deleted by liuyu
void config_lcdc_gpio_table(uint32_t *table, int len, unsigned enable)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n],
			enable ? GPIO_ENABLE : GPIO_DISABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static void lcdc_gordon_config_gpios(int enable)
{
	config_lcdc_gpio_table(lcdc_gpio_table,
		ARRAY_SIZE(lcdc_gpio_table), enable);
}

static void lcdc_config_gpios(int enable)
{
	config_lcdc_gpio_table(lcdc_gpio_table,
		ARRAY_SIZE(lcdc_gpio_table), enable);
}

static char *msm_fb_lcdc_vreg[] = {
	"gp5"
};

static int msm_fb_lcdc_power_save(int on)
{
	struct vreg *vreg[ARRAY_SIZE(msm_fb_lcdc_vreg)];
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(msm_fb_lcdc_vreg); i++) {
		if (on) {
			vreg[i] = vreg_get(0, msm_fb_lcdc_vreg[i]);
			rc = vreg_enable(vreg[i]);
			if (rc) {
				printk(KERN_ERR "vreg_enable: %s vreg"
						"operation failed \n",
						msm_fb_lcdc_vreg[i]);
				goto bail;
			}
		} else {
			int tmp;
			vreg[i] = vreg_get(0, msm_fb_lcdc_vreg[i]);
			tmp = vreg_disable(vreg[i]);
			if (tmp) {
				printk(KERN_ERR "vreg_disable: %s vreg "
						"operation failed \n",
						msm_fb_lcdc_vreg[i]);
				if (!rc)
					rc = tmp;
			}
			tmp = gpio_tlmm_config(GPIO_CFG(GPIO_OUT_88, 0,
						GPIO_OUTPUT, GPIO_NO_PULL,
						GPIO_2MA), GPIO_ENABLE);
			if (tmp) {
				printk(KERN_ERR "gpio_tlmm_config failed\n");
				if (!rc)
					rc = tmp;
			}
			gpio_set_value(88, 0);
			mdelay(15);
			gpio_set_value(88, 1);
			mdelay(15);
		}
	}

	return rc;

bail:
	if (on) {
		for (; i > 0; i--)
			vreg_disable(vreg[i - 1]);
	}

	return rc;
}
static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_gpio_config = msm_fb_lcdc_config,
	.lcdc_power_save   = msm_fb_lcdc_power_save,
};

static struct msm_panel_common_pdata lcdc_gordon_panel_data = {
	.panel_config_gpio = lcdc_gordon_config_gpios,
	.gpio_num          = gpio_array_num,
};

static struct msm_panel_common_pdata lcdc_panel_pdata = {
	.panel_config_gpio = lcdc_config_gpios,
	.gpio_num          = gpio_array_num,
};

static struct platform_device lcdc_gordon_panel_device = {
	.name   = "lcdc_gordon_vga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_gordon_panel_data,
	}
};

/*
static struct platform_device lcdc_panel_device = {
	.name   = "lcdc_ili9481_rgb",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_pdata,
	}
};
*/

static struct platform_device lcdc_panel_device = {
	//.name   = "lcdc_r61529_rgb",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_pdata,
	}
};

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	int ret = -EPERM;

	if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) {
		//if (!strcmp(name, "lcdc_r61529_rgb"))
		if(!strcmp(name,lcdc_panel_device.name))
			ret = 0;
		else
			ret = -ENODEV;
	}

	return ret;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.mddi_prescan = 1,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

#ifdef CONFIG_BT
static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
};

enum {
	BT_WAKE,
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
	BT_PCM_DOUT,
	BT_PCM_DIN,
	BT_PCM_SYNC,
	BT_PCM_CLK,
	BT_HOST_WAKE,
};

#define REG_ON_GPIO 81 //liuyu added
static unsigned bt_config_power_on[] = {
	GPIO_CFG(42, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* WAKE */
	GPIO_CFG(43, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* RFR */
	GPIO_CFG(44, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* CTS */
	GPIO_CFG(45, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* Rx */
	GPIO_CFG(46, 3, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* Tx */
	GPIO_CFG(68, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* HOST_WAKE */
	GPIO_CFG(81, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* REG_ON*/
};
static unsigned bt_config_power_off[] = {
	GPIO_CFG(42, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* WAKE */
	GPIO_CFG(43, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* RFR */
	GPIO_CFG(44, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* CTS */
	GPIO_CFG(45, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Rx */
	GPIO_CFG(46, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Tx */
	GPIO_CFG(68, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* HOST_WAKE */
    GPIO_CFG(81, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* REG_ON*/
};


//liuyu added
struct semaphore BT_Wifi_Power_Lock; //liuyu added
short BT_Request = 0;
short WIFI_Request = 0;
//BTorWIFI who call this function if BT 1; if wifi 0.
//on request to power on or off if on 1; if off 0.
#define BT_REQUEST 1
#define WIFI_REQUEST 0
#define POWER_ON 1
#define POWER_OFF 0
void BCM4325_power(int BTorWIFI, int on)
{
	static short bPowered=0;
	//static short BT_Request=0;
	//static short WIFI_Request=0;
	struct vreg *vreg_bt;
	int rc;
	down(&BT_Wifi_Power_Lock);
	rc = gpio_tlmm_config(GPIO_CFG(81, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	vreg_bt = vreg_get(NULL, "gp4"); //liuyu changed
	if (IS_ERR(vreg_bt)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       		__func__, PTR_ERR(vreg_bt));
		return PTR_ERR(vreg_bt);
	}
	if(on)
	{
		if(BTorWIFI)
			BT_Request=1; //BT request power on
		else
			WIFI_Request=1; //wifi request power on

		if(!bPowered) //if not powered on yet
		{
			rc = vreg_set_level(vreg_bt, 1800);
			if (rc) {
				printk(KERN_ERR "%s: vreg set level failed (%d)\n",
			       			__func__, rc);
				return -EIO;
			}
			rc = vreg_enable(vreg_bt);
			if (rc) {
				printk(KERN_ERR "%s: vreg enable failed (%d)\n",
			       			__func__, rc);
				return -EIO;
			}
			gpio_request(REG_ON_GPIO, "btwifi_regon");
			gpio_direction_output(REG_ON_GPIO, 1);
			msleep(200);

			gpio_set_value(REG_ON_GPIO, 1); //turn on REG_ON
			bPowered=1;
		}
	}
	else
	{
		if(BTorWIFI)
			BT_Request=0; //BT request power off
		else
			WIFI_Request=0; //wifi request power off

		if((!BT_Request) && (!WIFI_Request)) //only when BT and wifi all requested power off
		{
			if(bPowered) //if powered on now
			{
			    vreg_set_level(vreg_bt, 0);
				gpio_set_value(REG_ON_GPIO, 0); // turn down REG_ON
				gpio_free(REG_ON_GPIO);
				rc = vreg_disable(vreg_bt);
				if (rc) {
					printk(KERN_ERR "%s: vreg disable failed (%d)\n",
			       					__func__, rc);
					return -EIO;
				}
				bPowered=0;
			}
		}
	}

	up(&BT_Wifi_Power_Lock);
}
int BTWakesMSMIRQ; //liuyu added

//liuyu added
static irqreturn_t BtWakesMSMIrqProc(int irq, void *dev_id)
{
	//printk("//////////////////////BtWakesMSMIrqProc\n");

	//printk("///////////////GPIO 83's value is%d \n", gpio_get_value(83));
	return IRQ_HANDLED;
}

int bluetooth_power(int on)
{
//	struct vreg *vreg_bt;
	int pin, rc;

	printk(KERN_DEBUG "%s\n", __func__);
	if(on)
		printk("Turn on BT, GP4 got\n");
	else
		printk("Turn off BT, GP4 got\n");

	if (on) {
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
			rc = gpio_tlmm_config(bt_config_power_on[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_on[pin], rc);
				return -EIO;
			}
		}

		gpio_set_value(42, 1); //liuyu added
/*		BTWakesMSMIRQ=MSM_GPIO_TO_INT(83);
		int res=request_irq(BTWakesMSMIRQ, BtWakesMSMIrqProc,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				 "BT Wakes MSM IRQ", NULL);
		if(res!=0)
			printk("IRQ for GPIO83 failed\n");
		set_irq_wake(BTWakesMSMIRQ, 1);*/
		//when reg_on up if wifi don't open, wifi reset pin need output low
		if(!WIFI_Request)
			mpp_config_digital_out(17,
					     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					     MPP_DLOGIC_OUT_CTRL_LOW));
		mpp_config_digital_out(9,
					     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					     MPP_DLOGIC_OUT_CTRL_LOW));
		BCM4325_power(BT_REQUEST, POWER_ON);
		//gpio_set_value(REG_ON_GPIO, 1); //turn on REG_ON liuyu added

		msleep(100);
		//Turn up the BT_RST_N
		printk("Turn up the BT_RST_N\n");
		mpp_config_digital_out(9,
					     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					     MPP_DLOGIC_OUT_CTRL_HIGH));
	}
	else
	{

//		free_irq(BTWakesMSMIRQ, NULL);
		printk("Turn down BT32_K, BT_RST_N, REG_ON\n");
		mpp_config_digital_out(9,
					     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					     MPP_DLOGIC_OUT_CTRL_LOW));

		BCM4325_power(BT_REQUEST, POWER_OFF);
		//when reg_on off, reset pin output high to avoid current leak.
		if(!WIFI_Request)
		{
			mpp_config_digital_out(9,
					     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					     MPP_DLOGIC_OUT_CTRL_HIGH));
			mpp_config_digital_out(17,
					     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					     MPP_DLOGIC_OUT_CTRL_HIGH));
		}
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++) {
			rc = gpio_tlmm_config(bt_config_power_off[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_off[pin], rc);
				return -EIO;
			}
		}

	}
	return 0;
}

static void __init bt_power_init(void)
{
	mpp_config_digital_out(9,
			     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
			     MPP_DLOGIC_OUT_CTRL_HIGH));//for current leak[down 0.2~0.3]
	msm_bt_power_device.dev.platform_data = &bluetooth_power;
}
#else
#define bt_power_init(x) do {} while (0)
#endif

#ifdef CONFIG_ARCH_MSM7X27
static struct resource kgsl_resources[] = {
	{
		.name = "kgsl_reg_memory",
		.start = 0xA0000000,
		.end = 0xA001ffff,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "kgsl_phys_memory",
		.start = 0,
		.end = 0,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "kgsl_yamato_irq",
		.start = INT_GRAPHICS,
		.end = INT_GRAPHICS,
		.flags = IORESOURCE_IRQ,
	},
};

static struct kgsl_platform_data kgsl_pdata;

static struct platform_device msm_device_kgsl = {
	.name = "kgsl",
	.id = -1,
	.num_resources = ARRAY_SIZE(kgsl_resources),
	.resource = kgsl_resources,
	.dev = {
		.platform_data = &kgsl_pdata,
	},
};
#endif

static struct platform_device msm_device_pmic_leds = {
	.name   = "pmic-leds",
	.id = -1,
};

static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 83,
		.end	= 83,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 42,
		.end	= 42,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(83),
		.end	= MSM_GPIO_TO_INT(83),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};

static struct i2c_board_info i2c_devices[] = {
	#if !TARGET_BUILD_PIO// 1
	{
		I2C_BOARD_INFO("ov7690", 0x42 >> 1),
	},
	{
		I2C_BOARD_INFO("ov5647", 0x6c >> 1),
	},
	#else
	{
		I2C_BOARD_INFO("ft5x02-ts", 0x70>>1),
	},
	#endif
};

#if TARGET_BUILD_PIO
struct atmel_i2c_platform_data mxt224_ts_data[] ;
static struct i2c_board_info i2c_devices_1[] = {
	{
		I2C_BOARD_INFO(ATMEL_QT602240_NAME, 0x94>> 1),
		.platform_data = &mxt224_ts_data,
		.irq = MSM_GPIO_TO_INT(19),
	},
};
#endif

#if defined(CONFIG_I2C_GPIO) ||defined(CONFIG_I2C_GPIO_MODULE)

#if defined(CONFIG_BMA222_I2C) || defined(CONFIG_BMA222_I2C_MODULE)
#define GPIO_BMA222_I2C_INT  27
static struct i2c_board_info i2c_devices_2[] = {
	{
		I2C_BOARD_INFO("bma222", 0x08),
		.platform_data = NULL,
		.irq = MSM_GPIO_TO_INT(GPIO_BMA222_I2C_INT)
	},
};
#endif

//>>>>>>>>flin add to support eCompass using GPIO-I2C>>>>>>
#if defined(CONFIG_SENSORS_AKM8975) || defined(CONFIG_SENSORS_AKM8975_MODULE)
#define CAD0 (0)
#define CAD1 (0)
#define AK8975_BASE_ADDR (0x03)
#define AK8975_ADDR  ((AK8975_BASE_ADDR) << 2 |(CAD0) << 1|(CAD1))
#define GPIO_AK8975C_INT 20
static struct i2c_board_info i2c_devices_3[] = {
	{
		I2C_BOARD_INFO("akm8975c", AK8975_ADDR),
		.platform_data = NULL,
		.irq = MSM_GPIO_TO_INT(GPIO_AK8975C_INT)
	},
};
#endif
//<<<<<<<<<<<<<<end<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#if defined(CONFIG_SENSORS_TAOS)  || defined(CONFIG_SENSORS_TAOS_MODULE)
#define TAOS_TMD2771x_ADDR (0x39)
#define GPIO_TAOS_TMD2771x_INT 94
static struct i2c_board_info i2c_devices_4[] = {
	{
		I2C_BOARD_INFO("tritonFN", TAOS_TMD2771x_ADDR),
		.platform_data = NULL,
		.irq = MSM_GPIO_TO_INT(GPIO_TAOS_TMD2771x_INT),
	},
};
#endif

#if defined(CONFIG_KEYBOARD_FT5X02) || defined(CONFIG_KEYBOARD_MXT224)

struct atmel_i2c_platform_data mxt224_ts_data[] = {
{
	.version = 0x16,
	.abs_x_min = 0,
	.abs_x_max = 320,
	.abs_y_min = 0,
	.abs_y_max = 480,
	.abs_pressure_min = 0,
	.abs_pressure_max = 255,
	.abs_width_min = 0,
	.abs_width_max = 20,
	.gpio_irq = 19,
	.power = NULL,
	.config_T6 = {0, 0, 0, 0, 0, 0},
	.config_T7 = {50, 13, 50},
	.config_T8 = {6, 0, 10, 10, 0, 0, 5, 20},
	.config_T9 = {131, 0, 0, 18, 12, 0, 33, 50, 3, 3, 10, 6, 3, 0x30, 2, 10, 30, 20,7,2,63,1,2, 2, 10, 10, 0x40,0,0,0,20}, // setting [14] to 5 enables 5 pt multitouch, needs to be done for each controller version - netarchy
	.config_T18 = {0, 0},
	.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T19 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T20 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T22 = {13, 0, 0, 25, 0, 231, 255, 16, 25, 0, 1, 5, 10, 15, 20, 25, 16},
	.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T24 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T25 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T27 = {0, 0, 0, 0, 0, 0, 0},
	.config_T28 = {0, 0, 2, 16, 16, 30},
	.config_T38 = {0, 0, 0, 0, 0, 0, 0, 0},
	.object_crc = {0x89, 0xCC, 0xD5},
//	.cable_config = {30, 30, 8, 16},
	.GCAF_level = {20, 24, 28, 40, 63},
	.filter_level = {2, 4, 316, 318},
//},
}
/*
{
	.version = 0x15,
	.abs_x_min = 0,
	.abs_x_max = 320,
	.abs_y_min = 0,
	.abs_y_max = 480,
	.abs_pressure_min = 0,
	.abs_pressure_max = 255,
	.abs_width_min = 0,
	.abs_width_max = 20,
	.gpio_irq = 19,
	.power = NULL,
	.config_T6 = {0, 0, 0, 0, 0, 0},
	.config_T7 = {32, 16, 50},
	.config_T8 = {8, 0, 50, 50, 0, 0, 50, 0},
	.config_T9 = {3, 0, 0, 18, 12, 0, 32, 40, 2, 5, 0, 0, 0, 0, 5, 10, 10, 10, 63,1, 7, 2, 0, 0, 0, 0, 143, 47, 145, 81},
	.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T19 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T20 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T22 = {7, 0, 0, 25, 0, -25, 255, 4, 50, 0, 1, 10, 15, 20, 25, 30, 4},
	.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T24 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T25 = {3, 0, 200, 50, 64, 31, 0, 0, 0, 0, 0, 0, 0, 0},
	.config_T27 = {0, 0, 0, 0, 0, 0, 0},
	.config_T28 = {0, 0, 2, 4, 8, 60},
	.object_crc = {0x89, 0xCC, 0xD5},
}
*/
};
static struct i2c_board_info i2c_devices_5[]={
#if !TARGET_BUILD_PIO

	{
		I2C_BOARD_INFO("ft5x02-ts", 0x70>>1),

	},
	{
		I2C_BOARD_INFO(ATMEL_QT602240_NAME, 0x94>> 1),
                .platform_data = &mxt224_ts_data,
                .irq = MSM_GPIO_TO_INT(19),

	},
#else
	{
		I2C_BOARD_INFO("ov7690", 0x42 >> 1),
	},
	{
		I2C_BOARD_INFO("ov5647", 0x6c >> 1),
	},
#endif
};
#endif

//#if defined(CONFIG_KEYBOARD_MXT224)
//320X480 report XY resolution ,so as to match LCD
//rang{(0,480),(320,480)}->range{(0,480+40),(320,480+40)} is touch key


//static struct i2c_board_info i2c_devices_6[]={
//	{
//		I2C_BOARD_INFO(ATMEL_QT602240_NAME, 0x94>> 1),
//		.platform_data = &mxt224_ts_data,
//		.irq = MSM_GPIO_TO_INT(19),
//	},
//};
//#endif

#endif

#if defined(CONFIG_I2C_GPIO) ||defined(CONFIG_I2C_GPIO_MODULE)
#if defined(CONFIG_BMA222_I2C) || defined(CONFIG_BMA222_I2C_MODULE)
#define BMA222_SDA 16
#define BMA222_SCL 17
static uint32_t bma222_i2c_gpio_table[] = {
		GPIO_CFG(BMA222_SDA,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), /* DAT0 */
		GPIO_CFG(BMA222_SCL,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), /* DAT1 */
		//GPIO_CFG(GPIO_BMA222_I2C_INT,  0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
};
#endif

#ifndef SND_SPKR_PA
#define SND_SPKR_PA 38
#endif
static uint32_t snd_spkr_pa_gpio_table[] = {
		GPIO_CFG(SND_SPKR_PA,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* PA*/
};

//>>>>>>>>flin add to support eCompass using GPIO-I2C>>>>>>>
#if defined(CONFIG_SENSORS_AKM8975) || defined(CONFIG_SENSORS_AKM8975_MODULE)
#define AKM8975_SDA 76
#define AKM8975_SCL 91
#define AKM8975_INT 20
static uint32_t akm8975_i2c_gpio_table[] = {
		/* AK8973 GPIO-I2C interfaces */
		GPIO_CFG(AKM8975_SDA,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), /* SDA */
		GPIO_CFG(AKM8975_SCL,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), /* SCL */
		GPIO_CFG(AKM8975_INT,  0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
};
#endif
//<<<<<<<<<<<<<<end<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#if defined(CONFIG_SENSORS_TAOS) || defined(CONFIG_SENSORS_TAOS_MODULE)
#define TAOS_TMD2771x_SDA 108
#define TAOS_TMD2771x_SCL 109
#define TAOS_TMD2771x_INT   94
static uint32_t taos_tmd2771x_i2c_gpio_table[] = {
		GPIO_CFG(TAOS_TMD2771x_SDA,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), /* SDA */
		GPIO_CFG(TAOS_TMD2771x_SCL,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), /* SCL */
		GPIO_CFG(TAOS_TMD2771x_INT,  0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
};
#endif
#if defined(CONFIG_KEYBOARD_FT5X02)

#define FT5X02_SDA 107
#define FT5X02_SCK 93

static uint32_t ft5x02_i2c_gpio_table[] = {

	GPIO_CFG(FT5X02_SDA,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* SDA */
	GPIO_CFG(FT5X02_SCK,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* SCL */
};

#endif
#if defined(CONFIG_KEYBOARD_MXT224)

#define MXT224_SDA 107
#define MXT224_SCK 93

static uint32_t mxt224_i2c_gpio_table[] = {

	GPIO_CFG(MXT224_SDA,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* SDA */
	GPIO_CFG(MXT224_SCK,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* SCL */
};

#endif

#endif

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
        GPIO_CFG(0,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* RESET */
        GPIO_CFG(1,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PDN */
	GPIO_CFG(2,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	GPIO_CFG(3,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	GPIO_CFG(4,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(5,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(6,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(7,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(8,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
	GPIO_CFG(9,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
	GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
	GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
	GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* MCLK */
	GPIO_CFG(23,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* sub PDN */
	GPIO_CFG(78,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* vcm */
};

static uint32_t camera_on_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(0,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* RESET */
	GPIO_CFG(1,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PDN */
	GPIO_CFG(2,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	GPIO_CFG(3,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	GPIO_CFG(4,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(5,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(6,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(7,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(8,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
	GPIO_CFG(9,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
	GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
	GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
	GPIO_CFG(12, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA), /* PCLK */
	GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_16MA), /* MCLK */
	GPIO_CFG(23,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* sub PDN */
	GPIO_CFG(78,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* vcm */
	};

void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static struct vreg *vreg_gp2;
static struct vreg *vreg_gp3;
static struct vreg *vreg_gp5;
static struct vreg *vreg_boost;

static void msm_camera_vreg_config(int vreg_en)
{
	int rc;

	/*commented by litao for correct set the camera voltage*/
	//if (vreg_gp2 == NULL) {
		vreg_gp2 = vreg_get(NULL, "gp2");
		if (IS_ERR(vreg_gp2)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp2", PTR_ERR(vreg_gp2));
			return;
		}

		//printk("%s %s %d===============set v to 1500xxyy======\n", __FILE__, __func__, __LINE__);
		printk("gp2 1500\n");
		rc = vreg_set_level(vreg_gp2, 1500);
		if (rc) {
			printk(KERN_ERR "%s: GP2 set_level failed (%d)\n",
				__func__, rc);
		}
	//}

	//if (vreg_gp3 == NULL) {
		vreg_gp3 = vreg_get(NULL, "gp3");
		if (IS_ERR(vreg_gp3)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp3", PTR_ERR(vreg_gp3));
			return;
		}

		printk("gp3 2800\n");
		rc = vreg_set_level(vreg_gp3, 2800);
		if (rc) {
			printk(KERN_ERR "%s: GP3 set level failed (%d)\n",
				__func__, rc);
		}
	//}

	//if (vreg_gp5 == NULL) {
		vreg_gp5 = vreg_get(NULL, "gp5");
		if (IS_ERR(vreg_gp5)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp5", PTR_ERR(vreg_gp5));
			return;
		}

		printk("gp5 2800\n");
		rc = vreg_set_level(vreg_gp5, 2800);
		if (rc) {
			printk(KERN_ERR "%s: GP5 set level failed (%d)\n",
				__func__, rc);
		}

		//for vreg_5v setting
		vreg_boost = vreg_get(NULL, "boost");
		if (IS_ERR(vreg_boost)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "boost", PTR_ERR(vreg_boost));
			return;
		}

		printk("vreg_5v 5000\n");
		rc = vreg_set_level(vreg_boost, 5000);
		if (rc) {
			printk(KERN_ERR "%s: boost set level failed (%d)\n",
				__func__, rc);
		}
	//}

	if (vreg_en) {
		rc = vreg_enable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 enable failed (%d)\n",
				__func__, rc);
		}

		rc = vreg_enable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 enable failed (%d)\n",
				 __func__, rc);
		}

		rc = vreg_enable(vreg_gp5);
		if (rc) {
			printk(KERN_ERR "%s: GP5 enable failed (%d)\n",
				__func__, rc);
		}

		rc = vreg_enable(vreg_boost);
		if (rc) {
			printk(KERN_ERR "%s: boost enable failed (%d)\n",
				__func__, rc);
		}
	} else {
		/*
		rc = vreg_disable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 disable failed (%d)\n",
				 __func__, rc);
		}

		rc = vreg_disable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 disable failed (%d)\n",
				__func__, rc);
		}
		*/
		rc = vreg_disable(vreg_gp5);
		if (rc) {
			printk(KERN_ERR "%s: GP5 disable failed (%d)\n",
				__func__, rc);
		}

		rc = vreg_disable(vreg_boost);
		if (rc) {
			printk(KERN_ERR "%s: boost disable failed (%d)\n",
				__func__, rc);
		}
		gpio_request(0, "ov5647");
		gpio_direction_output(0, 1);
		gpio_free(0);
		gpio_request(23, "ov7690");
		gpio_direction_output(23, 1);
		gpio_free(23);
	}
}

static void config_camera_on_gpios(void)
{
	int vreg_en = 1;

	if (machine_is_msm7x25_ffa() ||
	    machine_is_msm7x27_ffa())
		msm_camera_vreg_config(vreg_en);

	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
}

static void config_camera_off_gpios(void)
{
	int vreg_en = 0;

	if (machine_is_msm7x25_ffa() ||
	    machine_is_msm7x27_ffa())
		msm_camera_vreg_config(vreg_en);

	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

static struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.mdcphy = MSM_MDC_PHYS,
	.ioext.mdcsz  = MSM_MDC_SIZE,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
};

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PMIC,
	._fsrc.pmic_src.low_current  = 68,
	._fsrc.pmic_src.high_current = 200,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7690_data = {
	.sensor_name    = "ov7690",
	.sensor_pwd     = 23,
	.vcm_pwd        = 0,
	.pdata          = &msm_camera_device_data,
};

static struct platform_device msm_camera_sensor_ov7690 = {
	.name      = "msm_camera_ov7690",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov7690_data,
	},
};

static struct msm_camera_sensor_flash_data flash_ov5647 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5647_data = {
	.sensor_name    = "ov5647",
	.sensor_reset   = 1,
	.sensor_pwd     = 0,
	.vcm_pwd        = 78,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_ov5647
};

static struct platform_device msm_camera_sensor_ov5647 = {
	.name      = "msm_camera_ov5647",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov5647_data,
	},
};

#ifdef CONFIG_OV2655
static struct msm_camera_sensor_info msm_camera_sensor_ov2655_data = {
	.sensor_name    = "ov2655",
	.sensor_reset   = 1,
	.sensor_pwd     = 0,
	.vcm_pwd        = 0,
	.pdata          = &msm_camera_device_data,
	.flash_type     = MSM_CAMERA_FLASH_LED
};

static struct platform_device msm_camera_sensor_ov2655 = {
	.name      = "msm_camera_ov2655",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov2655_data,
	},
};
#endif

#ifdef CONFIG_MT9D112
static struct msm_camera_sensor_flash_data flash_mt9d112 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9d112_data = {
	.sensor_name    = "mt9d112",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_mt9d112
};

static struct platform_device msm_camera_sensor_mt9d112 = {
	.name      = "msm_camera_mt9d112",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9d112_data,
	},
};
#endif

#ifdef CONFIG_S5K3E2FX
static struct msm_camera_sensor_flash_data flash_s5k3e2fx = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3e2fx_data = {
	.sensor_name    = "s5k3e2fx",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_s5k3e2fx
};

static struct platform_device msm_camera_sensor_s5k3e2fx = {
	.name      = "msm_camera_s5k3e2fx",
	.dev       = {
		.platform_data = &msm_camera_sensor_s5k3e2fx_data,
	},
};
#endif

#ifdef CONFIG_MT9P012
static struct msm_camera_sensor_flash_data flash_mt9p012 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9p012_data = {
	.sensor_name    = "mt9p012",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 88,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_mt9p012
};

static struct platform_device msm_camera_sensor_mt9p012 = {
	.name      = "msm_camera_mt9p012",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p012_data,
	},
};
#endif

#ifdef CONFIG_MT9P012_KM
static struct msm_camera_sensor_flash_data flash_mt9p012_km = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9p012_km_data = {
	.sensor_name    = "mt9p012_km",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 88,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_mt9p012_km
};

static struct platform_device msm_camera_sensor_mt9p012_km = {
	.name      = "msm_camera_mt9p012_km",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p012_km_data,
	},
};
#endif

#ifdef CONFIG_MT9T013
static struct msm_camera_sensor_flash_data flash_mt9t013 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data = {
	.sensor_name    = "mt9t013",
	.sensor_reset   = 89,
	.sensor_pwd     = 85,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_mt9t013
};

static struct platform_device msm_camera_sensor_mt9t013 = {
	.name      = "msm_camera_mt9t013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9t013_data,
	},
};
#endif

#ifdef CONFIG_VB6801
static struct msm_camera_sensor_flash_data flash_vb6801 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_vb6801_data = {
	.sensor_name    = "vb6801",
	.sensor_reset   = 89,
	.sensor_pwd     = 88,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_vb6801
};

static struct platform_device msm_camera_sensor_vb6801 = {
	.name      = "msm_camera_vb6801",
	.dev       = {
		.platform_data = &msm_camera_sensor_vb6801_data,
	},
};
#endif
#endif

static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 2800,
	.voltage_max_design	= 4300,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity	= &msm_calculate_batt_capacity,
};

#ifdef FEATUE_JRD_BATTERY_CAPACITY

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
    u32 low_voltage   = 0;
    u32 high_voltage  = 0;
    u16 level_index = 0;
    msm_psy_batt_data.voltage_min_design = 3000;
    msm_psy_batt_data.voltage_max_design = 4200;
    low_voltage   = msm_psy_batt_data.voltage_min_design;
    high_voltage  = msm_psy_batt_data.voltage_max_design;

    if (current_voltage > high_voltage)
    {
        level_index = 0;
    }
    else if (current_voltage < low_voltage)
    {
        level_index = 120;
    }
    else
    {
        level_index = (high_voltage - current_voltage)/10;
    }

    //printk(KERN_INFO "%s() : YZB ==========+++++++++++++current_voltage: %d;level_index:%d ;\n\n\n",
    //						__func__,  current_voltage,level_index);

    return jrd_batt_level[level_index];

}

#else

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	u32 low_voltage   = msm_psy_batt_data.voltage_min_design;
	u32 high_voltage  = msm_psy_batt_data.voltage_max_design;

    //printk(KERN_INFO "%s() : YZB ==========+++++++++++++low_voltage: %d;high_voltage:%d ;\n\n\n",
    //						__func__,  msm_psy_batt_data.voltage_min_design,msm_psy_batt_data.voltage_max_design);

	return (current_voltage - low_voltage) * 100
		/ (high_voltage - low_voltage);
}
#endif

static struct platform_device msm_batt_device = {
	.name 		    = "msm-battery",
	.id		    = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

static struct platform_device *early_devices[] __initdata = {
#ifdef CONFIG_GPIOLIB
	&msm_gpio_devices[0],
	&msm_gpio_devices[1],
	&msm_gpio_devices[2],
	&msm_gpio_devices[3],
	&msm_gpio_devices[4],
	&msm_gpio_devices[5],
#endif
};

#if defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C_GPIO_MODULE)
#include <linux/i2c-gpio.h>

#if defined(CONFIG_BMA222_I2C) || defined(CONFIG_BMA222_I2C_MODULE)
static struct i2c_gpio_platform_data bma222_i2c_gpio_data = {
	.sda_pin		= 16,
	.scl_pin		= 17,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 2,
};

static struct platform_device bma222_i2c_gpio_device = {
	.name		= "bma222_i2c-gpio",
	.id		= 2,
	.dev		= {
		.platform_data	= &bma222_i2c_gpio_data,
	},

};
#endif

//>>>>>>>>flin add to support eCompass using GPIO-I2C>>>>>>
#if defined (CONFIG_SENSORS_AKM8975) || defined(CONFIG_SENSORS_AKM8975_MODULE)
static struct i2c_gpio_platform_data akm8975_i2c_gpio_data = {
	.sda_pin		= 76,
	.scl_pin		= 91,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 2,
};

static struct platform_device akm8975_i2c_gpio_device = {
	.name		= "akm8975_gpio-i2c",
	.id		= 3,
	.dev		= {
		.platform_data	= &akm8975_i2c_gpio_data,
	},

};
#endif
//<<<<<<<<<<<<<<<<<<<end<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#if defined(CONFIG_SENSORS_TAOS) || defined(CONFIG_SENSORS_TAOS_MODULE)
static struct i2c_gpio_platform_data taos_i2c_gpio_data = {
	.sda_pin		= 108,
	.scl_pin		= 109,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 2,
};

static struct platform_device taos_i2c_gpio_device = {
	.name		= "taos_gpio-i2c",
	.id		= 4,
	.dev		= {
		.platform_data	= &taos_i2c_gpio_data,
	},

};
#endif

#if defined(CONFIG_KEYBOARD_FT5X02)
static struct i2c_gpio_platform_data ft5x02_i2c_gpio_data = {
	.sda_pin		= 107,
	.scl_pin		=93,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 2,
};

static struct platform_device ft5x02_i2c_gpio_device = {
	.name		= "ft5x02_gpio-i2c",
	.id		= 5,
	.dev		= {
		.platform_data	= &ft5x02_i2c_gpio_data,
	},

};

#endif
#if defined(CONFIG_KEYBOARD_MXT224)
static struct i2c_gpio_platform_data mxt224_i2c_gpio_data = {
	.sda_pin		= 107,
	.scl_pin		=93,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 2,
};

static struct platform_device mxt224_i2c_gpio_device = {
	.name		= "mxt224_gpio-i2c",
	.id		= 6,
	.dev		= {
		.platform_data	= &mxt224_i2c_gpio_data,
	},

};

#endif

#endif

struct platform_device msm_slide_device =
{
	.name		= "msm_slide",
};
static uint32_t wifi_on_gpio_table[] = {
    GPIO_CFG(62, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),/* "sdc2_clk"*/
    GPIO_CFG(63, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),/* "sdc2_cmd"*/
    GPIO_CFG(64, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),/* "sdc2_dat_3"*/
    GPIO_CFG(65, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),/* "sdc2_dat_2"*/
    GPIO_CFG(66, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),/* "sdc2_dat_1"*/
    GPIO_CFG(67, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),/* "sdc2_dat_0"*/
    GPIO_CFG(21, 0, GPIO_INPUT , GPIO_NO_PULL, GPIO_2MA),/* "WLAN IRQ"*/
};

static uint32_t wifi_sleep_gpio_table[] = {
    GPIO_CFG(62, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),/* "sdc2_clk"*/
    GPIO_CFG(63, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),/* "sdc2_cmd"*/
    GPIO_CFG(64, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),/* "sdc2_dat_3"*/
    GPIO_CFG(65, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),/* "sdc2_dat_2"*/
    GPIO_CFG(66, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),/* "sdc2_dat_1"*/
    GPIO_CFG(67, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),/* "sdc2_dat_0"*/
    GPIO_CFG(21, 0, GPIO_OUTPUT,GPIO_NO_PULL, GPIO_2MA), /* "WLAN IRQ"*/
};

int msm_wifi_power(int on)
{

	int rc = 0;
	if (on)
	{
		config_gpio_table(wifi_on_gpio_table,
				ARRAY_SIZE(wifi_on_gpio_table));
		mdelay(50);
	} else {
		config_gpio_table(wifi_sleep_gpio_table,
				ARRAY_SIZE(wifi_sleep_gpio_table));
	}

	if(on)
	{
		//when reg_on up , if bt don't open ,bt reset pin need output low.
		if(!BT_Request)
		rc = mpp_config_digital_out(9,
					MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					MPP_DLOGIC_OUT_CTRL_LOW));

		rc += mpp_config_digital_out(17,
					MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					MPP_DLOGIC_OUT_CTRL_LOW));
		BCM4325_power(WIFI_REQUEST, POWER_ON);
		msleep(150);
		rc += mpp_config_digital_out(17,
					MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					MPP_DLOGIC_OUT_CTRL_HIGH));

		if (rc) {
			printk(KERN_ERR"%s: mpp_config_digital_out return val: %d \n",
				   __func__, rc);
			return rc;
		}

	}
	else
	{
		//reset low
		rc = mpp_config_digital_out(17,
					MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					MPP_DLOGIC_OUT_CTRL_LOW));
		msleep(100);
		if (rc) {
			printk(KERN_ERR"%s: mpp_config_digital_out return val %d \n",
				   __func__, rc);
			return rc;
		}
        BCM4325_power(WIFI_REQUEST, POWER_OFF);
		//when reg_on off, wifi reset pin output high to avoid current leak.
		if(!BT_Request)
		{
			 mpp_config_digital_out(17,
					MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					MPP_DLOGIC_OUT_CTRL_HIGH));
			 mpp_config_digital_out(9,
					MPP_CFG(MPP_DLOGIC_LVL_MSMP,
					MPP_DLOGIC_OUT_CTRL_HIGH));
		}
	}
	return 0;
}

int msm_wifi_reset(int on)
{
    int rc=0;
	if(on)
    {
    	// reset high
	    rc = mpp_config_digital_out(17,
    			MPP_CFG(MPP_DLOGIC_LVL_MSMP,
			    MPP_DLOGIC_OUT_CTRL_HIGH));
    }
    else
    {
	    // reset low
    	rc = mpp_config_digital_out(17,
			    MPP_CFG(MPP_DLOGIC_LVL_MSMP,
    			MPP_DLOGIC_OUT_CTRL_LOW));
    }
    if(rc<0)
    {
		printk("%s: set Wifi_RST pin error\n",__FUNCTION__);
	}
	return rc;
}

void (*wifi_status_cb)(int card_present, void *dev_id);
void *wifi_status_cb_devid;

EXPORT_SYMBOL(wifi_status_cb_devid);
int wifi_status_notify(void (*callback)(int card_present, void *dev_id), void *dev_id)
{
        if (wifi_status_cb)
                return -EAGAIN;
        wifi_status_cb = callback;
        wifi_status_cb_devid = dev_id;
        return 0;
}

int msm_wifi_set_carddetect(int val)
{
    printk(KERN_INFO"%s: %d\n", __func__, val);

	if (wifi_status_cb) {
        wifi_status_cb(val, wifi_status_cb_devid);
    } else
        printk(KERN_WARNING"%s: Nobody to notify\n", __func__);
    return 0;
}

static struct resource msm_wifi_resources[] = {
    [0] = {
      .name       = "bcm4329_wlan_irq",
      .start      = MSM_GPIO_TO_INT(21),
      .end        = MSM_GPIO_TO_INT(21),
      .flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE ,
    },
};

#include <linux/wlan_plat.h>
static struct wifi_platform_data msm_wifi_control = {
    .set_power      = msm_wifi_power,
    .set_reset      = msm_wifi_reset,
    .set_carddetect = msm_wifi_set_carddetect,
//    .mem_prealloc   = msm_wifi_mem_prealloc,
};

static struct platform_device msm_wifi_device = {
        .name           = "bcm4329_wlan",
        .id             = 1,
        .num_resources  = ARRAY_SIZE(msm_wifi_resources),
        .resource       = msm_wifi_resources,
        .dev            = {
                .platform_data = &msm_wifi_control,
        },
};

static struct platform_device *devices[] __initdata = {
#if !defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart3,
#endif
	&msm_device_smd,
	&msm_device_dmov,
	&msm_device_nand,

#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#endif

#ifdef CONFIG_USB_FUNCTION
	&msm_device_hsusb_peripheral,
	&mass_storage_device,
#endif

#ifdef CONFIG_USB_ANDROID
	&mass_storage_device,
	&android_usb_device,
#endif
	&msm_device_i2c,
	&smc91x_device,
	&msm_device_tssc,
	&android_pmem_kernel_ebi1_device,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_fb_device,
	&lcdc_panel_device,
	&msm_device_uart_dm1,
#ifdef CONFIG_BT
	&msm_bt_power_device,
#endif
	&msm_device_pmic_leds,
	&msm_device_snd,
	&msm_device_adspdec,
#if defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C_GPIO_MODULE)

#if defined(CONFIG_BMA222_I2C) || defined(CONFIG_BMA222_I2C_MODULE)
	&bma222_i2c_gpio_device,
#endif

#if defined (CONFIG_SENSORS_AKM8975) || defined(CONFIG_SENSORS_AKM8975_MODULE)
	&akm8975_i2c_gpio_device,
#endif

#if defined(CONFIG_SENSORS_TAOS) || defined(CONFIG_SENSORS_TAOS_MODULE)
	&taos_i2c_gpio_device,
#endif

#endif
#ifdef CONFIG_MT9T013
	&msm_camera_sensor_mt9t013,
#endif
#ifdef CONFIG_OV2655
	&msm_camera_sensor_ov2655,
#endif
	&msm_camera_sensor_ov7690,
	&msm_camera_sensor_ov5647,
#ifdef CONFIG_MT9D112
	&msm_camera_sensor_mt9d112,
#endif
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_MT9P012
	&msm_camera_sensor_mt9p012,
#endif
#ifdef CONFIG_MT9P012_KM
	&msm_camera_sensor_mt9p012_km,
#endif
#ifdef CONFIG_VB6801
	&msm_camera_sensor_vb6801,
#endif
	&msm_bluesleep_device,
#ifdef CONFIG_ARCH_MSM7X27
	&msm_device_kgsl,
#endif
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	&msm_device_tsif,
#endif
	&hs_device,
	&msm_batt_device,
	&msm_slide_device //liuyu added
#if defined(CONFIG_KEYBOARD_FT5X02)
	,&ft5x02_i2c_gpio_device
#endif
#if defined(CONFIG_KEYBOARD_MXT224)
	,&mxt224_i2c_gpio_device
#endif
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("pmdh", 0);
	msm_fb_register_device("lcdc", &lcdc_pdata);
}

extern struct sys_timer msm_timer;

static void __init msm7x2x_init_irq(void)
{
	msm_init_irq();
}

static struct msm_acpu_clock_platform_data msm7x2x_clock_data = {
	.acpu_switch_time_us = 50,
	.max_speed_delta_khz = 400000,
	.vdd_switch_time_us = 62,
	.max_axi_khz = 160000,
};

void msm_serial_debug_init(unsigned int base, int irq,
			   struct device *clk_device, int signal_irq);

#ifdef CONFIG_USB_EHCI_MSM
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	if (on)
		msm_hsusb_vbus_powerup();
	else
		msm_hsusb_vbus_shutdown();
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info       = (USB_PHY_INTEGRATED | USB_PHY_MODEL_65NM),
	.vbus_power = msm_hsusb_vbus_power,
};
static void __init msm7x2x_init_host(void)
{
	if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa())
		return;

	msm_add_host(0, &msm_usb_host_pdata);
}
#endif


#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

static unsigned long vreg_sts, gpio_sts;
static struct vreg *vreg_mmc;
static unsigned mpp_mmc = 2;

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
	struct msm_gpio *sleep_cfg_data;
};

static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(51, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_12MA), "sdc1_dat_3"},
	{GPIO_CFG(52, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_12MA), "sdc1_dat_2"},
	{GPIO_CFG(53, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_12MA), "sdc1_dat_1"},
	{GPIO_CFG(54, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_12MA), "sdc1_dat_0"},
	{GPIO_CFG(55, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_12MA), "sdc1_cmd"},
	{GPIO_CFG(56, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_12MA), "sdc1_clk"},
};

static struct msm_gpio sdc2_cfg_data[] = {
	{GPIO_CFG(62, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), "sdc2_clk"},
	{GPIO_CFG(63, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_cmd"},
	{GPIO_CFG(64, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_3"},
	{GPIO_CFG(65, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_2"},
	{GPIO_CFG(66, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_1"},
	{GPIO_CFG(67, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_0"},
};

static struct msm_gpio sdc2_sleep_cfg_data[] = {
	{GPIO_CFG(62, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), "sdc2_clk"},
	{GPIO_CFG(63, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), "sdc2_cmd"},
	{GPIO_CFG(64, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), "sdc2_dat_3"},
	{GPIO_CFG(65, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), "sdc2_dat_2"},
	{GPIO_CFG(66, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), "sdc2_dat_1"},
	{GPIO_CFG(67, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), "sdc2_dat_0"},
};
static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(88, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), "sdc3_clk"},
	{GPIO_CFG(89, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_cmd"},
	{GPIO_CFG(90, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_3"},
	{GPIO_CFG(91, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_2"},
	{GPIO_CFG(92, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_1"},
	{GPIO_CFG(93, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_0"},
};

static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(19, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_3"},
	{GPIO_CFG(20, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_2"},
	{GPIO_CFG(21, 4, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_1"},
	{GPIO_CFG(107, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_cmd"},
	{GPIO_CFG(108, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_0"},
	{GPIO_CFG(109, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), "sdc4_clk"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
		.sleep_cfg_data = sdc2_sleep_cfg_data,
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
		.sleep_cfg_data = NULL,
	},
};

static void msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];
	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			printk(KERN_ERR "%s: Failed to turn on GPIOs for slot %d\n",
				__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		if (curr->sleep_cfg_data) {
			msm_gpios_enable(curr->sleep_cfg_data, curr->size);
			msm_gpios_free(curr->sleep_cfg_data, curr->size);
			return;
		}
		msm_gpios_disable_free(curr->cfg_data, curr->size);
	}
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);

	if (vdd == 0) {
		if (!vreg_sts)
			return 0;

		clear_bit(pdev->id, &vreg_sts);

		if (!vreg_sts) {
			//if (machine_is_msm7x27_ffa()) {
			if (!(machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa())) { //changed by liuyu
				rc = mpp_config_digital_out(mpp_mmc,
				     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				     MPP_DLOGIC_OUT_CTRL_LOW));
			}
			else
				//rc = vreg_disable(vreg_mmc);
				printk("sdcard power not shut down!^^^^^^^^^^^^^^^^^^^^^^\n");

			if (rc)
				printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
		}
		return 0;
	}

	if (!vreg_sts) {
		//if (machine_is_msm7x27_ffa()) {
		if (!(machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa() )) { //changed by liuyu
			rc = mpp_config_digital_out(mpp_mmc,
			     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
			     MPP_DLOGIC_OUT_CTRL_HIGH));

		} else {
			rc = vreg_set_level(vreg_mmc, 2850);
			if (!rc)
				rc = vreg_enable(vreg_mmc);
		}

		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);

	}


	set_bit(pdev->id, &vreg_sts);
	return 0;
}

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
struct mmc_platform_data msm7x2x_sdc1_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct mmc_platform_data msm7x2x_sdc2_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
	.sdiowakeup_irq = MSM_GPIO_TO_INT(66),
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data msm7x2x_sdc3_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct mmc_platform_data msm7x2x_sdc4_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

// chengyingkai 20091130 add for wifi begin
static uint32_t msm_sdcc_wifi_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);

	return 0;
}
//1203 add begin
#if 1
static struct sdio_embedded_func wifi_func = {
	.f_class        = 0x07/*SDIO_CLASS_WLAN*/,
 .f_maxblksize   = 512,
};
#endif

static struct embedded_sdio_data wifi_emb_data = {
	.cis    = {
		.vendor         = 0x02d0,
  .device         = 0x4325,
  .blksize        = 512,
  /*.max_dtr      = 24000000,  Max of chip - no worky on Trout */
  .max_dtr        =   48000000,
	},
 .cccr   = {
	 .multi_block    = 1,
  .low_speed      = 1,
  .wide_bus       = 1,
  .high_power     = 1,
  .high_speed     = 1,
 },
 .funcs  = &wifi_func,
 .num_funcs = 2,
};

//1203 add end
static struct mmc_platform_data msm7x27_sdcc_wifi_data = {
	.ocr_mask	= MMC_VDD_28_29,
 .embedded_sdio		= &wifi_emb_data,
    .register_status_notify = wifi_status_notify,
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	//.msmsdcc_fmax	= 49152000,
	//FAE comment: bcm4329 Module sclk can't over 25M
	.msmsdcc_fmax	= 24576000,
	.nonremovable	= 0,
};
// chengyingkai 20091130 add for wifi end

extern struct resource resources_sdc1[3];
extern struct platform_device *msm_device_sdc1;
static void __init msm7x2x_init_mmc(void)
{

	//if (machine_is_msm7x27_ffa()) {
	if (!( machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa() )) { //changed by liuyu
		mpp_mmc = 2;
		if (!mpp_mmc) {
			printk(KERN_ERR "%s: mpp get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	} else {
		vreg_mmc = vreg_get(NULL, "mmc");
		if (IS_ERR(vreg_mmc)) {
			printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	}

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	msm_device_sdc1 = platform_device_alloc("msm_sdcc", 1);
	platform_device_add_resources(msm_device_sdc1, resources_sdc1, ARRAY_SIZE(resources_sdc1));
	platform_device_add_data(msm_device_sdc1, &msm7x2x_sdc1_data, sizeof(msm7x2x_sdc1_data));
	msm_device_sdc1->dev.coherent_dma_mask = 0xffffffff;
	platform_device_add(msm_device_sdc1);
#endif

// chengyingkai add for OPAL wifi 20091130 begin
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	//printk(KERN_ERR "\n\n\n\n\n\n\n\n\t\t%s: msm_add_sdcc(2\n\n\n\n\n", __func__);
	msm_add_sdcc(2, &msm7x27_sdcc_wifi_data);
#endif
// chengyingkai add for OPAL wifi 20091130 end

	if ( machine_is_msm7x25_surf() || machine_is_msm7x27_surf()) {
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
		msm_add_sdcc(3, &msm7x2x_sdc3_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
		msm_add_sdcc(4, &msm7x2x_sdc4_data);
#endif
	}
}
#else
#define msm7x2x_init_mmc() do {} while (0)
#endif

static void __init msm7x2x_init_wifi(void)
{
	int rc = 0;
	//msm_add_sdcc(2, &msm7x2x_wifi_data);

	//rc = gpio_tlmm_config(bcm4329_gpio_regon[0], GPIO_CFG_ENABLE);
    rc = gpio_tlmm_config(GPIO_CFG(81, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);

	if (rc)
	{
		printk("### %s \n", __FUNCTION__);
		return;
	}
	gpio_request(REG_ON_GPIO, "btwifi_regon");
	gpio_direction_output(REG_ON_GPIO, 1);

	gpio_set_value(REG_ON_GPIO, 0);
}

static struct msm_pm_platform_data msm7x25_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 16000,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 12000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 2000,
};

static struct msm_pm_platform_data msm7x27_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 16000,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 20000,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 12000,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 20000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled
		= 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 2000,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 0,
};

static void
msm_i2c_gpio_config(int iface, int config_type)
{
	int gpio_scl;
	int gpio_sda;
	if (iface) {
		gpio_scl = 95;
		gpio_sda = 96;
	} else {
		gpio_scl = 60;
		gpio_sda = 61;
	}
	if (config_type) {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	} else {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 300000,
	.rmutex  = 0,
	.pri_clk = 60,
	.pri_dat = 61,
	.aux_clk = 95,
	.aux_dat = 96,
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_init(void)
{
	if (gpio_request(60, "i2c_pri_clk"))
		pr_err("failed to request gpio i2c_pri_clk\n");
	if (gpio_request(61, "i2c_pri_dat"))
		pr_err("failed to request gpio i2c_pri_dat\n");
	if (gpio_request(95, "i2c_sec_clk"))
		pr_err("failed to request gpio i2c_sec_clk\n");
	if (gpio_request(96, "i2c_sec_dat"))
		pr_err("failed to request gpio i2c_sec_dat\n");

	if (cpu_is_msm7x27())
		msm_i2c_pdata.pm_lat =
		msm7x27_pm_data[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN]
		.latency;
	else
		msm_i2c_pdata.pm_lat =
		msm7x25_pm_data[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN]
		.latency;

	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

static void usb_mpp_init(void)
{
        unsigned rc;
        unsigned mpp_usb = 7;

        if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) {
                rc = mpp_config_digital_out(mpp_usb,
                        MPP_CFG(MPP_DLOGIC_LVL_VDD,
                                MPP_DLOGIC_OUT_CTRL_HIGH));
                if (rc)
                        pr_err("%s: configuring mpp pin"
                                "to enable 3.3V LDO failed\n", __func__);
        }
}

void config_keypad_gpio_table(uint32_t *table, int len, unsigned enable) //added by huyugui
{
        int n, rc;
        for (n = 0; n < len; n++) {
                rc = gpio_tlmm_config(table[n],
                        enable ? GPIO_ENABLE : GPIO_DISABLE);
                if (rc) {
                        printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
                                __func__, table[n], rc);
                        break;
                }
        }
}

static uint32_t keypad_gpio_table[] = { //added by huyugui
	GPIO_CFG(41,  	       0, GPIO_INPUT , GPIO_PULL_UP, GPIO_2MA),
	GPIO_CFG(40,  	       0, GPIO_INPUT , GPIO_PULL_UP, GPIO_2MA),
	GPIO_CFG(39,  	       0, GPIO_INPUT , GPIO_PULL_UP, GPIO_2MA),
//	GPIO_CFG(38,  	       0, GPIO_INPUT , GPIO_PULL_UP, GPIO_2MA),
	GPIO_CFG(37,  	       0, GPIO_INPUT , GPIO_PULL_UP, GPIO_2MA),
	GPIO_CFG(36,  	       0, GPIO_INPUT , GPIO_PULL_UP, GPIO_2MA),

	GPIO_CFG(35,  	       0, GPIO_OUTPUT , GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(34,  	       0, GPIO_OUTPUT , GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(33,  	       0, GPIO_OUTPUT , GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(32,  	       0, GPIO_OUTPUT , GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(31,  	       0, GPIO_OUTPUT , GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(29,  	       0, GPIO_OUTPUT , GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(28,  	       0, GPIO_OUTPUT , GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(30,  	       0, GPIO_OUTPUT , GPIO_NO_PULL, GPIO_2MA),

};
static void msm7x27_init_keypad(void) //added by huyugui
{
	config_keypad_gpio_table(keypad_gpio_table,
		ARRAY_SIZE(keypad_gpio_table), 1);
}

static void msm7x27_wlan_init(void)
{
	int rc = 0;
	/* TBD: if (machine_is_msm7x27_ffa_with_wcn1312()) */
	//add bcm4329 dev
	platform_device_register(&msm_wifi_device);

	if (machine_is_msm7x27_ffa()) {
		rc = mpp_config_digital_out(17, MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				MPP_DLOGIC_OUT_CTRL_HIGH));//avoid current leak from mpp18 [down 0.2~0.3]
		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
				__func__, rc);
	}
}

/*
*Msm7x27 lcd module type is detected by the voltage of GPIO_88,which
*high or low of it  figures P5562 and ILI9481relevantly.
*/
static int msm7x27_lcd_id_detect(void)
{
	int rc;

	rc = gpio_tlmm_config(GPIO_CFG(88,0,	GPIO_INPUT,
						GPIO_NO_PULL,GPIO_2MA),
						GPIO_ENABLE);
	if(rc){
		printk(KERN_ERR "gpio_tlmm_config failed\n");
		return rc;
	}

	rc = gpio_tlmm_config(GPIO_CFG(28,0,	GPIO_INPUT,
						GPIO_NO_PULL,GPIO_2MA),
						GPIO_ENABLE);
	if(rc){
		printk(KERN_ERR "gpio_tlmm_config failed\n");
		return rc;
	}

	rc =( (gpio_get_value(88) << 4)  | gpio_get_value(28));

	printk(KERN_ERR"---------------------------------------------------------rc =:0x%x\n",rc);

	return rc;
}

static void __init msm7x2x_init(void)
{
#if defined(CONFIG_KEYBOARD_MXT224) ||defined(CONFIG_KEYBOARD_FT5X02)
	int rv;
#endif
// {init dmesg proc file while board init, xuxian 20110217
        if (dmesg_history_init() < 0)
                printk(KERN_ERR "%s: dmesg_history_init() failed!\n",
                       __func__);
// }
#ifdef CONFIG_ARCH_MSM7X25
	msm_clock_init(msm_clocks_7x25, msm_num_clocks_7x25);
#elif CONFIG_ARCH_MSM7X27
	msm_clock_init(msm_clocks_7x27, msm_num_clocks_7x27);
#endif
	platform_add_devices(early_devices, ARRAY_SIZE(early_devices));

#if defined(CONFIG_MSM_SERIAL_DEBUGGER)
	msm_serial_debug_init(MSM_UART3_PHYS, INT_UART3,
			&msm_device_uart3.dev, 1);
#endif
#if defined(CONFIG_SMC91X)
	if (machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) {
	//if (machine_is_msm7x27_ffa()) {
	if (!( machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa() )) { //changed by liuyu
		smc91x_resources[0].start = 0x98000300;
		smc91x_resources[0].end = 0x980003ff;
		smc91x_resources[1].start = MSM_GPIO_TO_INT(85);
		smc91x_resources[1].end = MSM_GPIO_TO_INT(85);
		if (gpio_tlmm_config(GPIO_CFG(85, 0,
					      GPIO_INPUT,
					      GPIO_PULL_DOWN,
					      GPIO_2MA),
				     GPIO_ENABLE)) {
			printk(KERN_ERR
			       "%s: Err: Config GPIO-85 INT\n",
				__func__);
		}
	}
#endif

	if (cpu_is_msm7x27())
		msm7x2x_clock_data.max_axi_khz = 200000;

	msm_acpu_clock_init(&msm7x2x_clock_data);

#ifdef CONFIG_ARCH_MSM7X27
	/* Initialize the zero page for barriers and cache ops */
	map_zero_page_strongly_ordered();
	/* This value has been set to 160000 for power savings. */
	/* OEMs may modify the value at their discretion for performance */
	/* The appropriate maximum replacement for 160000 is: */
	/* clk_get_max_axi_khz() */
	kgsl_pdata.high_axi_3d = 160000;

	/* 7x27 doesn't allow graphics clocks to be run asynchronously to */
	/* the AXI bus */
	kgsl_pdata.max_grp2d_freq = 0;
	kgsl_pdata.min_grp2d_freq = 0;
	kgsl_pdata.set_grp2d_async = NULL;
	kgsl_pdata.max_grp3d_freq = 0;
	kgsl_pdata.min_grp3d_freq = 0;
	kgsl_pdata.set_grp3d_async = NULL;
	kgsl_pdata.imem_clk_name = "imem_clk";
	kgsl_pdata.grp3d_clk_name = "grp_clk";
	kgsl_pdata.grp2d_clk_name = NULL;
#endif
	usb_mpp_init();

#ifdef CONFIG_USB_FUNCTION
	msm_hsusb_pdata.swfi_latency =
		msm7x27_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
	if (machine_is_msm7x25_surf() || machine_is_msm7x25_ffa()) {
		msm_otg_pdata.pemp_level =
			PRE_EMPHASIS_WITH_20_PERCENT;
		msm_otg_pdata.drv_ampl = HS_DRV_AMPLITUDE_5_PERCENT;
		msm_otg_pdata.cdr_autoreset = CDR_AUTO_RESET_ENABLE;
		msm_otg_pdata.phy_reset_sig_inverted = 1;
	}
	if (machine_is_msm7x27_surf() || machine_is_msm7x27_ffa()) {
		msm_otg_pdata.pemp_level =
			PRE_EMPHASIS_WITH_10_PERCENT;
		msm_otg_pdata.drv_ampl = HS_DRV_AMPLITUDE_5_PERCENT;
		msm_otg_pdata.cdr_autoreset = CDR_AUTO_RESET_DISABLE;
		msm_otg_pdata.phy_reset_sig_inverted = 1;
	}

#ifdef CONFIG_USB_GADGET
	msm_gadget_pdata.swfi_latency =
		msm7x27_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
#endif
#endif
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	msm_device_tsif.dev.platform_data = &tsif_platform_data;
#endif

	rv = msm7x27_lcd_id_detect();

	switch (rv)
	{
		case TDT_LCD:
			lcdc_panel_device.name = "lcdc_ili9481_rgb";
			break;
		case TRULY_LCD:
			lcdc_panel_device.name = "lcdc_r61529_rgb";
			break;
		case FOXCONN_LCD:
			lcdc_panel_device.name = "lcdc_innolux_ili9481_rgb";
			break;
		case TRULY_LCD_HX8357:
			lcdc_panel_device.name = "lcdc_hx8357_rgb";
			break;
		default :
			lcdc_panel_device.name = "empty";
			break;
	}

	platform_add_devices(devices, ARRAY_SIZE(devices));
#ifdef CONFIG_MSM_CAMERA
	config_camera_off_gpios(); /* might not be necessary */
#endif
	msm_device_i2c_init();
//Brandy choose Atmel or Focaltech touch pannel according to TPYE_ID(GPIO 97)
#if defined(CONFIG_KEYBOARD_MXT224) ||defined(CONFIG_KEYBOARD_FT5X02)
	rv = gpio_tlmm_config(GPIO_CFG(97, 0, GPIO_INPUT,
				GPIO_PULL_DOWN, GPIO_4MA), GPIO_ENABLE);


        if (rv) {
               printk(KERN_ERR "%s: gpio_tlmm_config=%d\n",
                                 __func__, rv);
        }
#endif
#if TARGET_BUILD_PIO//0
//	if(!gpio_get_value(97))//yangyufeng debug
		i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
//	else
		i2c_register_board_info(0,i2c_devices_1,ARRAY_SIZE(i2c_devices_1));
#else
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
#endif

#if defined(CONFIG_I2C_GPIO) ||defined(CONFIG_I2C_GPIO_MODULE)

#if defined(CONFIG_BMA222_I2C) ||defined(CONFIG_BMA222_I2C_MODULE)
	i2c_register_board_info(2, i2c_devices_2, ARRAY_SIZE(i2c_devices_2));
#endif

//>>>>>>>>>>>>>>flin add to support GPIO_I2C AKM8973B>>>>>>>>>>
#if defined(CONFIG_SENSORS_AKM8975) || defined(CONFIG_SENSORS_AKM8975_MODULE)
	i2c_register_board_info(3, i2c_devices_3, ARRAY_SIZE(i2c_devices_3));
#endif
//<<<<<<<<<<<<<<<<<<<<<<<end<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#if defined(CONFIG_SENSORS_TAOS) || defined(CONFIG_SENSORS_TAOS_MODULE)
	i2c_register_board_info(4, i2c_devices_4, ARRAY_SIZE(i2c_devices_4));
#endif

#if defined(CONFIG_KEYBOARD_FT5X02) || defined(CONFIG_KEYBOARD_MXT224)
	i2c_register_board_info(5, i2c_devices_5, ARRAY_SIZE(i2c_devices_5));
#endif

//#if defined(CONFIG_KEYBOARD_MXT224)
//	i2c_register_board_info(6, i2c_devices_6, ARRAY_SIZE(i2c_devices_6));
//#endif

#endif

#ifdef CONFIG_SURF_FFA_GPIO_KEYPAD
	//if (machine_is_msm7x27_ffa())
	if ( machine_is_msm7x25_ffa() || machine_is_msm7x27_ffa()) //changed by liuyu
		platform_device_register(&keypad_device_7k_ffa);
	else
		platform_device_register(&keypad_device_surf);
#endif
	lcdc_gpio_init();
	msm_fb_add_devices();
#ifdef CONFIG_USB_EHCI_MSM
	msm7x2x_init_host();
#endif
	msm7x2x_init_mmc();
	msm7x2x_init_wifi();

	msm7x27_init_keypad(); //added by huyugui
	init_MUTEX( &BT_Wifi_Power_Lock); /* add a semaphore for BT&WIFI power control */
	bt_power_init();
	config_gpio_table(snd_spkr_pa_gpio_table,ARRAY_SIZE(snd_spkr_pa_gpio_table));     /*add for brandy audio PA gpio initialize*/

	if (cpu_is_msm7x27())
		msm_pm_set_platform_data(msm7x27_pm_data,
					ARRAY_SIZE(msm7x27_pm_data));
	else
		msm_pm_set_platform_data(msm7x25_pm_data,
					ARRAY_SIZE(msm7x25_pm_data));

	msm7x27_wlan_init();
	//	msm_pm_set_platform_data(msm7x25_pm_data);

#if defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C_GPIO_MODULE)
#if defined(CONFIG_BMA222_I2C) ||defined(CONFIG_BMA222_I2C_MODULE)
	config_gpio_table(bma222_i2c_gpio_table,ARRAY_SIZE(bma222_i2c_gpio_table));
#endif

//>>>>>>>>>>>>>>flin add to support GPIO_I2C AKM8973B>>>>>>>>>>
#if defined(CONFIG_SENSORS_AKM8975) || defined(CONFIG_SENSORS_AKM8975_MODULE)
	config_gpio_table(akm8975_i2c_gpio_table,ARRAY_SIZE(akm8975_i2c_gpio_table));
#endif
//<<<<<<<<<<<<<<<<<<<<<<<end<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#if defined(CONFIG_SENSORS_TAOS) || defined(CONFIG_SENSORS_TAOS_MODULE)
	config_gpio_table(taos_tmd2771x_i2c_gpio_table, ARRAY_SIZE(taos_tmd2771x_i2c_gpio_table));
#endif
#if  defined(CONFIG_KEYBOARD_FT5X02)
	config_gpio_table(ft5x02_i2c_gpio_table,ARRAY_SIZE(ft5x02_i2c_gpio_table));
#endif
#if  defined(CONFIG_KEYBOARD_MXT224)
	config_gpio_table(mxt224_i2c_gpio_table,ARRAY_SIZE(mxt224_i2c_gpio_table));
#endif
#endif
}

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static void __init pmem_kernel_ebi1_size_setup(char **p)
{
	pmem_kernel_ebi1_size = memparse(*p, p);
}
__early_param("pmem_kernel_ebi1_size=", pmem_kernel_ebi1_size_setup);

static unsigned pmem_mdp_size = MSM_PMEM_MDP_SIZE;
static void __init pmem_mdp_size_setup(char **p)
{
	pmem_mdp_size = memparse(*p, p);
}
__early_param("pmem_mdp_size=", pmem_mdp_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static void __init pmem_adsp_size_setup(char **p)
{
	pmem_adsp_size = memparse(*p, p);
}
__early_param("pmem_adsp_size=", pmem_adsp_size_setup);

#ifdef CONFIG_ARCH_MSM7X27
static unsigned pmem_gpu1_size = MSM_PMEM_GPU1_SIZE;
static void __init pmem_gpu1_size_setup(char **p)
{
	pmem_gpu1_size = memparse(*p, p);
}
__early_param("pmem_gpu1_size=", pmem_gpu1_size_setup);
#endif

static unsigned fb_size = MSM_FB_SIZE;
static void __init fb_size_setup(char **p)
{
	fb_size = memparse(*p, p);
}
__early_param("fb_size=", fb_size_setup);

static void __init msm_msm7x2x_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = pmem_mdp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for mdp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = MSM_PMEM_AUDIO_SIZE ;
	android_pmem_audio_pdata.start = MSM_PMEM_AUDIO_START_ADDR ;
	android_pmem_audio_pdata.size = size;
	pr_info("allocating %lu bytes (at %lx physical) for audio "
		"pmem arena\n", size , MSM_PMEM_AUDIO_START_ADDR);

	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}
#ifdef CONFIG_ARCH_MSM7X27
	size = MSM_GPU_PHYS_SIZE;
	kgsl_resources[1].start = MSM_GPU_PHYS_START_ADDR ;
	kgsl_resources[1].end = kgsl_resources[1].start + size - 1;
	pr_info("allocating %lu bytes (at %lx physical) for KGSL\n",
		size , MSM_GPU_PHYS_START_ADDR);

#endif

	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

	size = MSM_GPU_PHYS_SIZE;
	addr = alloc_bootmem(size);
	kgsl_resources[1].start = __pa(addr);
	kgsl_resources[1].end = kgsl_resources[1].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for KGSL\n",
		size, addr, __pa(addr));
}

static void __init msm7x2x_map_io(void)
{
	msm_map_common_io();
	msm_msm7x2x_allocate_memory_regions();

	if (socinfo_init() < 0)
		BUG();

#ifdef CONFIG_CACHE_L2X0
	if (machine_is_msm7x27_surf() || machine_is_msm7x27_ffa()) {
		/* 7x27 has 256KB L2 cache:
			64Kb/Way and 4-Way Associativity;
			evmon/parity/share disabled. */
		if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) > 1)
			|| ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 1)
			&& (SOCINFO_VERSION_MINOR(socinfo_get_version()) >= 3)))
			/* R/W latency: 4 cycles; */
			l2x0_init(MSM_L2CC_BASE, 0x0006801B, 0xfe000000);
		else
			/* R/W latency: 3 cycles; */
			l2x0_init(MSM_L2CC_BASE, 0x00068012, 0xfe000000);
	}
#endif
}

MACHINE_START(MSM7X27_SURF, "QCT MSM7x27 SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END

MACHINE_START(MSM7X27_FFA, "QCT MSM7x27 FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END

MACHINE_START(MSM7X25_SURF, "QCT MSM7x25 SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END

MACHINE_START(MSM7X25_FFA, "QCT MSM7x25 FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm7x2x_map_io,
	.init_irq	= msm7x2x_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
MACHINE_END

