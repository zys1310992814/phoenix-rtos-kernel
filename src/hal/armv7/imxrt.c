/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * iMXRT basic peripherals control functions
 *
 * Copyright 2017 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "imxrt.h"
#include "interrupts.h"
#include "pmap.h"
#include "../../../include/errno.h"


struct {
	volatile u32 *gpio[5];
	volatile u32 *ccm;
	volatile u32 *ccm_analog;
	volatile u32 *pmu;
	volatile u32 *xtalosc;
	volatile u32 *iomuxsw;
	volatile u32 *nvic;
	volatile u32 *stk;
	volatile u32 *scb;
	volatile u16 *wdog1;
	volatile u16 *wdog2;
	volatile u32 *rtwdog;
	volatile u32 *lcd;

	u32 xtaloscFreq;
	u32 cpuclk;
} imxrt_common;


enum { gpio_dr = 0, gpio_gdir, gpio_psr, gpio_icr1, gpio_icr2, gpio_imr, gpio_isr, gpio_edge_sel };


enum { lcd_ctrl = 0, lcd_ctrl_set, lcd_ctrl_clr, lcd_ctrl_tog, lcd_ctrl1, lcd_ctrl1_set, lcd_ctrl1_clr, lcd_ctrl1_tog,
	lcd_ctrl2, lcd_ctrl2_set, lcd_ctrl2_clr, lcd_ctrl2_tog, lcd_transfer_count, /* 3 reserved */ lcd_cur_buf = 16,
	/* 3 reserved */ lcd_next_buf = 20, /* 7 reserved */ lcd_vdctrl0 = 28, lcd_vdctrl0_set, lcd_vdctrl0_clr,
	lcd_vdctrl0_tog, lcd_vdctrl1, /* 3 reserved */ lcd_vdctrl2 = 36, /* 3 reserved */ lcd_vdctrl3 = 40, /* 3 reserved */
	lcd_vdctrl4 = 44, /* 55 reserved */ lcd_bm_error_stat = 100, /*  3 reserved */ lcd_crc_stat = 104, /* 3 reserved */
	lcd_stat = 108, /* 19 reserved */ lcd_thres = 128, /* 3 reserved */ lcd_as_ctrl = 132, /* 3 reserved */
	lcd_as_buf = 136, /* 3 reserved */ lcd_as_next_buf = 140, /* 3 reserved */ lcd_as_clrkeylow = 144, /* 3 reserved */
	lcd_as_clrkeyhigh = 148, /* 75 reserved */ lcd_pigeonctrl0 = 224, lcd_pigeonctrl0_set, lcd_pigeonctrl0_clr,
	lcd_pigeonctrl0_tog, lcd_pigeonctrl1, lcd_pigeonctrl1_set, lcd_pigeonctrl1_clr, lcd_pigeonctrl1_tog,
	lcd_pigeonctrl2, lcd_pigeonctrl2_set, lcd_pigeonctrl2_clr, lcd_pigeonctrl2_tog, /* 276 reserved */
	lcd_pigeon00 = 512, /* 3 reserved */ lcd_pigeon01 = 516, /* 3 reserved */ lcd_pigeon02 = 520, /* 7 reserved */
	lcd_pigeon10 = 528, /* 3 reserved */ lcd_pigeon11 = 532, /* 3 reserved */ lcd_pigeon12 = 536, /* 7 reserved */
	lcd_pigeon20 = 544, /* 3 reserved */ lcd_pigeon21 = 548, /* 3 reserved */ lcd_pigeon22 = 552, /* 7 reserved */
	lcd_pigeon30 = 560, /* 3 reserved */ lcd_pigeon31 = 564, /* 3 reserved */ lcd_pigeon32 = 568, /* 7 reserved */
	lcd_pigeon40 = 576, /* 3 reserved */ lcd_pigeon41 = 580, /* 3 reserved */ lcd_pigeon42 = 584, /* 7 reserved */
	lcd_pigeon50 = 592, /* 3 reserved */ lcd_pigeon51 = 596, /* 3 reserved */ lcd_pigeon52 = 600, /* 7 reserved */
	lcd_pigeon60 = 608, /* 3 reserved */ lcd_pigeon61 = 612, /* 3 reserved */ lcd_pigeon62 = 616, /* 7 reserved */
	lcd_pigeon70 = 624, /* 3 reserved */ lcd_pigeon71 = 628, /* 3 reserved */ lcd_pigeon72 = 632, /* 7 reserved */
	lcd_pigeon80 = 640, /* 3 reserved */ lcd_pigeon81 = 644, /* 3 reserved */ lcd_pigeon82 = 648, /* 7 reserved */
	lcd_pigeon90 = 652, /* 3 reserved */ lcd_pigeon91 = 656, /* 3 reserved */ lcd_pigeon92 = 660, /* 7 reserved */
	lcd_pigeon100 = 668, /* 3 reserved */ lcd_pigeon101 = 672, /* 3 reserved */ lcd_pigeon102 = 676, /* 7 reserved */
	lcd_pigeon110 = 684, /* 3 reserved */ lcd_pigeon111 = 688, /* 3 reserved */ lcd_pigeon112 = 672 };


enum { ccm_ccr = 0, /* reserved */ ccm_csr = 2, ccm_ccsr, ccm_cacrr, ccm_cbcdr, ccm_cbcmr, ccm_cscmr1, ccm_cscmr2,
	ccm_cscdr1, ccm_cs1cdr, ccm_cs2cdr, ccm_cdcdr, /* reserved */ ccm_cscdr2 = 14, ccm_cscdr3, /* 2 reserved */
	ccm_cdhipr = 18, /* 2 reserved */ ccm_clpcr = 21, ccm_cisr, ccm_cimr, ccm_ccosr, ccm_cgpr, ccm_ccgr0, ccm_ccgr1,
	ccm_ccgr2, ccm_ccgr3, ccm_ccgr4, ccm_ccgr5, ccm_ccgr6, /* reserved */ ccm_cmeor = 34 };


enum { ccm_analog_pll_arm, ccm_analog_pll_arm_set, ccm_analog_pll_arm_clr, ccm_analog_pll_arm_tog,
	ccm_analog_pll_usb1, ccm_analog_pll_usb1_set, ccm_analog_pll_usb1_clr, ccm_analog_pll_usb1_tog,
	ccm_analog_pll_usb2, ccm_analog_pll_usb2_set, ccm_analog_pll_usb2_clr, ccm_analog_pll_usb2_tog,
	ccm_analog_pll_sys, ccm_analog_pll_sys_set, ccm_analog_pll_sys_clr, ccm_analog_pll_sys_tog,
	ccm_analog_pll_sys_ss, /* 3 reserved */ ccm_analog_pll_sys_num = 20, /* 3 reserved */
	ccm_analog_pll_sys_denom = 24, /* 3 reserved */ ccm_analog_pll_audio = 28, ccm_analog_pll_audio_set,
	ccm_analog_pll_audio_clr, ccm_analog_pll_audio_tog, ccm_analog_pll_audio_num, /* 3 reserved */
	ccm_analog_pll_audio_denom = 36, /* 3 reserved */ ccm_analog_pll_video = 40, ccm_analog_pll_video_set,
	ccm_analog_pll_video_clr, ccm_analog_pll_video_tog, ccm_analog_pll_video_num, /* 3 reserved */
	ccm_analog_pll_video_denom = 48, /* 3 reserved */ ccm_analog_pll_enet = 52, ccm_analog_pll_enet_set,
	ccm_analog_pll_enet_clr, ccm_analog_pll_enet_tog, ccm_analog_pfd_480, ccm_analog_pfd_480_set,
	ccm_analog_pfd_480_clr, ccm_analog_pfd_480_tog, ccm_analog_pfd_528, ccm_analog_pfd_528_set,
	ccm_analog_pfd_528_clr, ccm_analog_pfd_528_tog, ccm_analog_misc0 = 84, ccm_analog_misc0_set,
	ccm_analog_misc0_clr, ccm_analog_misc0_tog, ccm_analog_misc1, ccm_analog_misc1_set, ccm_analog_misc1_clr,
	ccm_analog_misc1_tog, ccm_analog_misc2, ccm_analog_misc2_set, ccm_analog_misc2_clr, ccm_analog_misc2_tog };


enum { pmu_reg_1p1 = 0, /* 3 reserved */ pmu_reg_3p0 = 4, /* 3 reserved */ pmu_reg_2p5 = 8, /* 3 reserved */
	pmu_reg_core = 12, /* 3 reserved */ pmu_misc0 = 16, /* 3 reserved */ pmu_misc1 = 20, pmu_misc1_set,
	pmu_misc1_clr, pmu_misc1_tog, pmu_misc2, pmu_misc2_set, pmu_misc2_clr, pmu_misc2_tog };


enum { xtalosc_misc0 = 84, xtalosc_lowpwr_ctrl = 156, xtalosc_lowpwr_ctrl_set, xtalosc_lowpwr_ctrl_clr,
	xtalosc_lowpwr_ctrl_tog, xtalosc_osc_config0 = 168, xtalosc_osc_config0_set, xtalosc_osc_config0_clr,
	xtalosc_osc_config0_tog, xtalosc_osc_config1, xtalosc_osc_config1_set, xtalosc_osc_config1_clr,
	xtalosc_osc_config1_tog, xtalosc_osc_config2, xtalosc_osc_config2_set, xtalosc_osc_config2_clr, xtalosc_osc_config2_tog };


enum { osc_rc = 0, osc_xtal };


enum {stk_ctrl = 0, stk_load, stk_val, stk_calib };


enum { scb_cpuid = 0, scb_icsr, scb_vtor, scb_aircr, scb_scr, scb_ccr, scb_shp0, scb_shp1,
	scb_shp2, scb_shcsr, scb_cfsr, scb_hfsr, scb_dfsr, scb_mmfar, scb_bfar, scb_afsr, scb_pfr0,
	scb_pfr1, scb_dfr, scb_afr, scb_mmfr0, scb_mmfr1, scb_mmfr2, scb_mmf3, scb_isar0, scb_isar1,
	scb_isar2, scb_isar3, scb_isar4, /* reserved */ scb_clidr = 30, scb_ctr, scb_ccsidr, scb_csselr,
	scb_cpacr, /* 93 reserved */ scb_stir = 128, /* 15 reserved */ scb_mvfr0 = 144, scb_mvfr1,
	scb_mvfr2, /* reserved */ scb_iciallu = 148, /* reserved */ scb_icimvau = 150, scb_scimvac,
	scb_dcisw, scb_dccmvau, scb_dccmvac, scb_dccsw, scb_dccimvac, scb_dccisw, /* 6 reserved */
	scb_itcmcr = 164, scb_dtcmcr, scb_ahbpcr, scb_cacr, scb_ahbscr, /* reserved */ scb_abfsr = 170 };


enum { nvic_iser = 0, nvic_icer = 32, nvic_ispr = 64, nvic_icpr = 96, nvic_iabr = 128,
	nvic_ip = 256, nvic_stir = 896 };


enum { wdog_wcr = 0, wdog_wsr, wdog_wrsr, wdog_wicr, wdog_wmcr };


enum { rtwdog_cs = 0, rtwdog_cnt, rtwdog_total, rtwdog_win };


/* platformctl syscall */


int hal_platformctl(void *ptr)
{
	return -EINVAL;
}


/* CCM (Clock Controller Module) */


static u32 _imxrt_ccmGetPeriphClkFreq(void)
{
	u32 freq;

	/* Periph_clk2_clk ---> Periph_clk */
	if (*(imxrt_common.ccm + ccm_cbcdr) & (1 << 25)) {
		switch ((*(imxrt_common.ccm + ccm_cbcmr) >> 12) & 0x3) {
			/* Pll3_sw_clk ---> Periph_clk2_clk ---> Periph_clk */
			case 0x0:
				freq = _imxrt_ccmGetPllFreq(clk_pll_usb1);
				break;

			/* Osc_clk ---> Periph_clk2_clk ---> Periph_clk */
			case 0x1:
				freq = imxrt_common.xtaloscFreq;
				break;

			default:
				freq = 0;
				break;
		}

		freq /= ((*(imxrt_common.ccm + ccm_cbcdr) >> 27) & 0x7) + 1;
	}
	else { /* Pre_Periph_clk ---> Periph_clk */
		switch ((*(imxrt_common.ccm + ccm_cbcmr) >> 18) * 0x3) {
			/* PLL2 ---> Pre_Periph_clk ---> Periph_clk */
			case 0x0:
				freq = _imxrt_ccmGetPllFreq(clk_pll_sys);
				break;

			/* PLL2 PFD2 ---> Pre_Periph_clk ---> Periph_clk */
			case 0x1:
				freq = _imxrt_ccmGetSysPfdFreq(clk_pfd2);
				break;

			/* PLL2 PFD0 ---> Pre_Periph_clk ---> Periph_clk */
			case 0x2:
				freq = _imxrt_ccmGetSysPfdFreq(clk_pfd0);
				break;

			/* PLL1 divided(/2) ---> Pre_Periph_clk ---> Periph_clk */
			case 0x3:
				freq = _imxrt_ccmGetPllFreq(clk_pll_arm) / ((*(imxrt_common.ccm + ccm_cacrr) & 0x7) + 1);
				break;

			default:
				freq = 0;
				break;
		}
	}

	return freq;
}


void _imxrt_ccmInitExterlnalClk(void)
{
	/* Power up */
	*(imxrt_common.ccm_analog + ccm_analog_misc0_clr) = 1 << 30;
	while (!(*(imxrt_common.xtalosc + xtalosc_lowpwr_ctrl) & (1 << 16)));

	/* Detect frequency */
	*(imxrt_common.ccm_analog + ccm_analog_misc0_set) = 1 << 16;
	while (!(*(imxrt_common.ccm_analog + ccm_analog_misc0) & (1 << 15)));

	*(imxrt_common.ccm_analog + ccm_analog_misc0_clr) = 1 << 16;
}


void _imxrt_ccmDeinitExternalClk(void)
{
	/* Power down */
	*(imxrt_common.ccm_analog + ccm_analog_misc0_set) = 1 << 30;
}


void _imxrt_ccmSwitchOsc(int osc)
{
	if (osc == osc_rc)
		*(imxrt_common.xtalosc + xtalosc_lowpwr_ctrl_set) = 1 << 4;
	else
		*(imxrt_common.xtalosc + xtalosc_lowpwr_ctrl_clr) = 1 << 4;
}


void _imxrt_ccmInitRcOsc24M(void)
{
	*(imxrt_common.xtalosc + xtalosc_lowpwr_ctrl_set) = 1;
}


void _imxrt_ccmDeinitRcOsc24M(void)
{
	*(imxrt_common.xtalosc + xtalosc_lowpwr_ctrl_clr) = 1;
}


u32 _imxrt_ccmGetFreq(int name)
{
	u32 freq;

	switch (name) {
		/* Periph_clk ---> AHB Clock */
		case clk_cpu:
		case clk_ahb:
			freq = _imxrt_ccmGetPeriphClkFreq() / (((*(imxrt_common.ccm + ccm_cbcdr) > 10) & 0x7) + 1);
			break;

		case clk_semc:
			/* SEMC alternative clock ---> SEMC Clock */
			if (*(imxrt_common.ccm + ccm_cbcdr) & (1 << 6)) {
				/* PLL3 PFD1 ---> SEMC alternative clock ---> SEMC Clock */
				if (*(imxrt_common.ccm + ccm_cbcdr) & 0x7)
					freq = _imxrt_ccmGetUsb1PfdFreq(clk_pfd1);
				/* PLL2 PFD2 ---> SEMC alternative clock ---> SEMC Clock */
				else
					freq = _imxrt_ccmGetSysPfdFreq(clk_pfd2);
			}
			/* Periph_clk ---> SEMC Clock */
			else {
				freq = _imxrt_ccmGetPeriphClkFreq();
			}

			freq /= ((*(imxrt_common.ccm + ccm_cbcdr) >> 16) & 0x7) + 1;
			break;

		case clk_ipg:
			/* Periph_clk ---> AHB Clock ---> IPG Clock */
			freq = _imxrt_ccmGetPeriphClkFreq() / (((*(imxrt_common.ccm + ccm_cbcdr) >> 10) & 0x7) + 1);
			freq /= ((*(imxrt_common.ccm + ccm_cbcdr) >> 8) & 0x3) + 1;
			break;

		case clk_osc:
			freq = _imxrt_ccmGetOscFreq();
			break;
		case clk_rtc:
			freq = 32768;
			break;
		case clk_armpll:
			freq = _imxrt_ccmGetPllFreq(clk_pll_arm);
			break;
		case clk_usb1pll:
			freq = _imxrt_ccmGetPllFreq(clk_pll_usb1);
			break;
		case clk_usb1pfd0:
			freq = _imxrt_ccmGetUsb1PfdFreq(clk_pfd0);
			break;
		case clk_usb1pfd1:
			freq = _imxrt_ccmGetUsb1PfdFreq(clk_pfd1);
			break;
		case clk_usb1pfd2:
			freq = _imxrt_ccmGetUsb1PfdFreq(clk_pfd2);
			break;
		case clk_usb1pfd3:
			freq = _imxrt_ccmGetUsb1PfdFreq(clk_pfd3);
			break;
		case clk_usb2pll:
			freq = _imxrt_ccmGetPllFreq(clk_pll_usb2);
			break;
		case clk_syspll:
			freq = _imxrt_ccmGetPllFreq(clk_pll_sys);
			break;
		case clk_syspdf0:
			freq = _imxrt_ccmGetSysPfdFreq(clk_pfd0);
			break;
		case clk_syspdf1:
			freq = _imxrt_ccmGetSysPfdFreq(clk_pfd1);
			break;
		case clk_syspdf2:
			freq = _imxrt_ccmGetSysPfdFreq(clk_pfd2);
			break;
		case clk_syspdf3:
			freq = _imxrt_ccmGetSysPfdFreq(clk_pfd3);
			break;
		case clk_enetpll0:
			freq = _imxrt_ccmGetPllFreq(clk_pll_enet0);
			break;
		case clk_enetpll1:
			freq = _imxrt_ccmGetPllFreq(clk_pll_enet1);
			break;
		case clk_enetpll2:
			freq = _imxrt_ccmGetPllFreq(clk_pll_enet2);
			break;
		case clk_audiopll:
			freq = _imxrt_ccmGetPllFreq(clk_pll_audio);
			break;
		case clk_videopll:
			freq = _imxrt_ccmGetPllFreq(clk_pll_video);
			break;
		default:
			freq = 0U;
			break;
	}

	return freq;
}


u32 _imxrt_ccmGetOscFreq(void)
{
	return imxrt_common.xtaloscFreq;
}


void _imxrt_ccmSetOscFreq(u32 freq)
{
	imxrt_common.xtaloscFreq = freq;
}


void _imxrt_ccmInitArmPll(u32 div)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_arm) = (1 << 13) | (div & 0x7f);

	while (!(*(imxrt_common.ccm_analog + ccm_analog_pll_arm) & (1 << 31)));
}


void _imxrt_ccmDeinitArmPll(void)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_arm) = 1 << 12;
}


void _imxrt_ccmInitSysPll(u8 div)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_sys) =  (1 << 13) | (div & 1);

	while (!(*(imxrt_common.ccm_analog + ccm_analog_pll_sys) & (1 << 31)));
}


void _imxrt_ccmDeinitSysPll(void)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_sys) = 1 << 12;
}


void _imxrt_ccmInitUsb1Pll(u8 div)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_usb1) = (1 << 13) | (1 << 12) | (1 << 6) | (div & 0x3);

	while (!(*(imxrt_common.ccm_analog + ccm_analog_pll_usb1) & (1 << 31)));
}


void _imxrt_ccmDeinitUsb1Pll(void)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_usb1) = 0;
}


void _imxrt_ccmInitUsb2Pll(u8 div)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_usb2) = (1 << 13) | (1 << 12) | (1 << 6) | (div & 0x3);

	while (!(*(imxrt_common.ccm_analog + ccm_analog_pll_usb2) & (1 << 31)));
}


void _imxrt_ccmDeinitUsb2Pll(void)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_usb2) = 0;
}


void _imxrt_ccmInitAudioPll(u8 loopdiv, u8 postdiv, u32 num, u32 denom)
{
	u32 pllAudio;

	*(imxrt_common.ccm_analog + ccm_analog_pll_audio_num) = num & 0x3fffffff;
	*(imxrt_common.ccm_analog + ccm_analog_pll_audio_denom) = denom & 0x3fffffff;

	pllAudio = (1 << 13) | (loopdiv & 0x7f);

	switch (postdiv) {
		case 16:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_set) = (1 << 23) | (1 << 15);
			break;

		case 8:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_set) = (1 << 23) | (1 << 15);
			pllAudio |= 1 << 19;
			break;

		case 4:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_set) = (1 << 23) | (1 << 15);
			pllAudio |= 1 << 20;
			break;

		case 2:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_clr) = (1 << 23) | (1 << 15);
			pllAudio |= 1 << 19;
			break;

		default:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_clr) = (1 << 23) | (1 << 15);
			pllAudio |= 1 << 20;
			break;
	}

	*(imxrt_common.ccm_analog + ccm_analog_pll_audio) = pllAudio;

	while (!(*(imxrt_common.ccm_analog + ccm_analog_pll_audio) & (1 << 31)));
}


void _imxrt_ccmDeinitAudioPll(void)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_audio) = 1 << 12;
}


void _imxrt_ccmInitVideoPll(u8 loopdiv, u8 postdiv, u32 num, u32 denom)
{
	u32 pllVideo;

	*(imxrt_common.ccm_analog + ccm_analog_pll_video_num) = num & 0x3fffffff;
	*(imxrt_common.ccm_analog + ccm_analog_pll_video_denom) = denom & 0x3fffffff;

	pllVideo = (1 << 13) | (loopdiv & 0x7f);

	switch (postdiv) {
		case 16:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_set) = (3 << 30);
			break;

		case 8:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_set) = (3 << 30);
			pllVideo |= 1 << 19;
			break;

		case 4:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_set) = (3 << 30);
			pllVideo |= 1 << 20;
			break;

		case 2:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_clr) = (3 << 30);
			pllVideo |= 1 << 19;
			break;

		default:
			*(imxrt_common.ccm_analog + ccm_analog_misc2_clr) = (3 << 30);
			pllVideo |= 1 << 20;
			break;
	}

	*(imxrt_common.ccm_analog + ccm_analog_pll_video) = pllVideo;

	while (!(*(imxrt_common.ccm_analog + ccm_analog_pll_video) & (1 << 31)));
}


void _imxrt_ccmDeinitVideoPll(void)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_video) = 1 << 12;
}


void _imxrt_ccmInitEnetPll(u8 enclk0, u8 enclk1, u8 enclk2, u8 div0, u8 div1)
{
	u32 enet_pll = ((div1 & 0x3) << 2) | (div0 & 0x3);

	if (enclk0)
		enet_pll |= 1 << 12;

	if (enclk1)
		enet_pll |= 1 << 20;

	if (enclk2)
		enet_pll |= 1 << 21;

	*(imxrt_common.ccm_analog + ccm_analog_pll_enet) = enet_pll;

	while (!(*(imxrt_common.ccm_analog + ccm_analog_pll_enet) & (1 << 31)));
}


void _imxrt_ccmDeinitEnetPll(void)
{
	*(imxrt_common.ccm_analog + ccm_analog_pll_enet) = 1 << 12;
}


u32 _imxrt_ccmGetPllFreq(int pll)
{
	u32 freq, divSel;
	u64 tmp;

	switch (pll) {
		case clk_pll_arm:
			freq = ((_imxrt_ccmGetOscFreq() * (*(imxrt_common.ccm_analog + ccm_analog_pll_arm) & 0x7f)) >> 1);
			break;

		case clk_pll_sys:
			freq = _imxrt_ccmGetOscFreq();

			/* PLL output frequency = Fref * (DIV_SELECT + NUM/DENOM). */
			tmp = ((u64)freq * (u64)*(imxrt_common.ccm_analog + ccm_analog_pll_sys_num)) / (u64)*(imxrt_common.ccm_analog + ccm_analog_pll_sys_denom);

			if (*(imxrt_common.ccm_analog + ccm_analog_pll_sys) & 1)
				freq *= 22;
			else
				freq *= 20;

			freq += (u32)tmp;
			break;

		case clk_pll_usb1:
			freq = _imxrt_ccmGetOscFreq() * ((*(imxrt_common.ccm_analog + ccm_analog_pll_usb1) & 0x3) ? 22 : 20);
			break;

		case clk_pll_audio:
			freq = _imxrt_ccmGetOscFreq();

			divSel = *(imxrt_common.ccm_analog + ccm_analog_pll_audio) & 0x7f;
			tmp = ((u64)freq * (u64)*(imxrt_common.ccm_analog + ccm_analog_pll_audio_num)) / (u64)*(imxrt_common.ccm_analog + ccm_analog_pll_audio_denom);
			freq = freq * divSel + (u32)tmp;

			switch ((*(imxrt_common.ccm_analog + ccm_analog_pll_audio) >> 19) & 0x3) {
				case 0:
					freq >>= 2;
					break;

				case 1:
					freq >>= 1;

				default:
					break;
			}

			if (*(imxrt_common.ccm_analog + ccm_analog_misc2) & (1 << 15)) {
				if (*(imxrt_common.ccm_analog + ccm_analog_misc2) & (1 << 31))
					freq >>= 2;
				else
					freq >>= 1;
			}
			break;

		case clk_pll_video:
			freq = _imxrt_ccmGetOscFreq();

			divSel = *(imxrt_common.ccm_analog + ccm_analog_pll_video) & 0x7F;

			tmp = ((u64)freq * (u64)*(imxrt_common.ccm_analog + ccm_analog_pll_video_num)) / (u64)*(imxrt_common.ccm_analog + ccm_analog_pll_video_denom);

			freq = freq * divSel + (u32)tmp;

			switch ((*(imxrt_common.ccm_analog + ccm_analog_pll_video) >> 19) & 0x3) {
				case 0:
					freq >>= 2;
					break;

				case 1:
					freq >>= 1;
					break;

				default:
					break;
			}

			if (*(imxrt_common.ccm_analog + ccm_analog_misc2) & (1 << 30)) {
				if (*(imxrt_common.ccm_analog + ccm_analog_misc2) & (1 << 31))
					freq >>= 2;
				else
					freq >>= 1;
			}
			break;

		case clk_pll_enet0:
			divSel = *(imxrt_common.ccm_analog + ccm_analog_pll_enet) & 0x3;

			switch (divSel) {
				case 0:
					freq = 25000000;
					break;

				case 1:
					freq = 50000000;
					break;

				case 2:
					freq = 100000000;
					break;

				default:
					freq = 125000000;
					break;
			}
			break;

		case clk_pll_enet1:
			divSel = *(imxrt_common.ccm_analog + ccm_analog_pll_enet) & (0x3 << 2);

			switch (divSel) {
				case 0:
					freq = 25000000;
					break;

				case 1:
					freq = 50000000;
					break;

				case 2:
					freq = 100000000;
					break;

				default:
					freq = 125000000;
					break;
			}
			break;

		case clk_pll_enet2:
			/* ref_enetpll2 if fixed at 25MHz. */
			freq = 25000000;
			break;

		case clk_pll_usb2:
			freq = _imxrt_ccmGetOscFreq() * ((*(imxrt_common.ccm_analog + ccm_analog_pll_usb2) & 0x3) ? 22 : 20);
			break;

		default:
			freq = 0;
			break;
	}

	return freq;
}


void _imxrt_ccmInitSysPfd(int pfd, u8 pfdFrac)
{
	u32 pfd528 = *(imxrt_common.ccm_analog + ccm_analog_pfd_528) & ~(0xbf << (pfd << 3));

	*(imxrt_common.ccm_analog + ccm_analog_pfd_528) = pfd528 | ((1 << 7) << (pfd << 3));
	*(imxrt_common.ccm_analog + ccm_analog_pfd_528) = pfd528 | (((u32)pfdFrac & 0x3f) << (pfd << 3));
}


void _imxrt_ccmDeinitSysPfd(int pfd)
{
	*(imxrt_common.ccm_analog + ccm_analog_pfd_528) |= (1 << 7) << (pfd << 3);
}


void _imxrt_ccmInitUsb1Pfd(int pfd, u8 pfdFrac)
{
	u32 pfd480 = *(imxrt_common.ccm_analog + ccm_analog_pfd_480) & ~(0xbf << (pfd << 3));

	*(imxrt_common.ccm_analog + ccm_analog_pfd_480) = pfd480 | ((1 << 7) << (pfd << 3));
	*(imxrt_common.ccm_analog + ccm_analog_pfd_480) = pfd480 | (((u32)pfdFrac & 0x3f) << (pfd << 3));
}


void _imxrt_ccmDeinitUsb1Pfd(int pfd)
{
	*(imxrt_common.ccm_analog + ccm_analog_pfd_480) |= (1 << 7) << (pfd << 3);
}


u32 _imxrt_ccmGetSysPfdFreq(int pfd)
{
	u32 freq = _imxrt_ccmGetPllFreq(clk_pll_sys);

	switch (pfd) {
		case clk_pfd0:
			freq /= *(imxrt_common.ccm_analog + ccm_analog_pfd_528) & 0x3f;
			break;

		case clk_pfd1:
			freq /= (*(imxrt_common.ccm_analog + ccm_analog_pfd_528) >> 8) & 0x3f;
			break;

		case clk_pfd2:
			freq /= (*(imxrt_common.ccm_analog + ccm_analog_pfd_528) >> 16) & 0x3f;
			break;

		case clk_pfd3:
			freq /= (*(imxrt_common.ccm_analog + ccm_analog_pfd_528) >> 24) & 0x3f;
			break;

		default:
			freq = 0;
			break;
	}

	return freq * 18;
}


u32 _imxrt_ccmGetUsb1PfdFreq(int pfd)
{
	u32 freq = _imxrt_ccmGetPllFreq(clk_pll_usb1);

	switch (pfd)
	{
		case clk_pfd0:
			freq /= *(imxrt_common.ccm_analog + ccm_analog_pfd_480) & 0x3f;
			break;

		case clk_pfd1:
			freq /= (*(imxrt_common.ccm_analog + ccm_analog_pfd_480) >> 8) & 0x3f;
			break;

		case clk_pfd2:
			freq /= (*(imxrt_common.ccm_analog + ccm_analog_pfd_480) >> 16) & 0x3f;
			break;

		case clk_pfd3:
			freq /= (*(imxrt_common.ccm_analog + ccm_analog_pfd_480) >> 24) & 0x3f;
			break;

		default:
			freq = 0U;
			break;
	}

	return freq * 18;
}


void _imxrt_ccmSetMux(int mux, u32 val)
{
	switch (mux) {
		case clk_mux_pll3:
			*(imxrt_common.ccm + ccm_ccsr) = (*(imxrt_common.ccm + ccm_ccsr) & ~1) | (val & 1);
			break;

		case clk_mux_periph:
			*(imxrt_common.ccm + ccm_cbcdr) = (*(imxrt_common.ccm + ccm_cbcdr) & ~(1 << 25)) | ((val & 1) << 25);
			while (*(imxrt_common.ccm + ccm_cdhipr) & (1 << 5));
			break;

		case clk_mux_semcAlt:
			*(imxrt_common.ccm + ccm_cbcdr) = (*(imxrt_common.ccm + ccm_cbcdr) & ~(1 << 7)) | ((val & 1) << 7);
			break;

		case clk_mux_semc:
			*(imxrt_common.ccm + ccm_cbcdr) = (*(imxrt_common.ccm + ccm_cbcdr) & ~(1 << 6)) | ((val & 1) << 6);
			break;

		case clk_mux_prePeriph:
			*(imxrt_common.ccm + ccm_cbcmr) = (*(imxrt_common.ccm + ccm_cbcmr) & ~(0x3 << 18)) | ((val & 0x3) << 18);
			break;

		case clk_mux_trace:
			*(imxrt_common.ccm + ccm_cbcmr) = (*(imxrt_common.ccm + ccm_cbcmr) & ~(0x3 << 14)) | ((val & 0x3) << 14);
			break;

		case clk_mux_periphclk2:
			*(imxrt_common.ccm + ccm_cbcmr) = (*(imxrt_common.ccm + ccm_cbcmr) & ~(0x3 << 12)) | ((val & 0x3) << 12);
			break;

		case clk_mux_lpspi:
			*(imxrt_common.ccm + ccm_cbcmr) = (*(imxrt_common.ccm + ccm_cbcmr) & ~(0x3 << 4)) | ((val & 0x3) << 4);
			break;

		case clk_mux_flexspi:
			*(imxrt_common.ccm + ccm_cscmr1) = (*(imxrt_common.ccm + ccm_cscmr1) & ~(0x3 << 29)) | ((val & 0x3) << 29);
			break;

		case clk_mux_usdhc2:
			*(imxrt_common.ccm + ccm_cscmr1) = (*(imxrt_common.ccm + ccm_cscmr1) & ~(1 << 17)) | ((val & 1) << 17);
			break;

		case clk_mux_usdhc1:
			*(imxrt_common.ccm + ccm_cscmr1) = (*(imxrt_common.ccm + ccm_cscmr1) & ~(1 << 16)) | ((val & 1) << 16);
			break;

		case clk_mux_sai3:
			*(imxrt_common.ccm + ccm_cscmr1) = (*(imxrt_common.ccm + ccm_cscmr1) & ~(0x3 << 14)) | ((val & 0x3) << 14);
			break;

		case clk_mux_sai2:
			*(imxrt_common.ccm + ccm_cscmr1) = (*(imxrt_common.ccm + ccm_cscmr1) & ~(0x3 << 12)) | ((val & 0x3) << 12);
			break;

		case clk_mux_sai1:
			*(imxrt_common.ccm + ccm_cscmr1) = (*(imxrt_common.ccm + ccm_cscmr1) & ~(0x3 << 10)) | ((val & 0x3) << 10);
			break;

		case clk_mux_perclk:
			*(imxrt_common.ccm + ccm_cscmr1) = (*(imxrt_common.ccm + ccm_cscmr1) & ~(1 << 6)) | ((val & 1) << 6);
			break;

		case clk_mux_flexio2:
			*(imxrt_common.ccm + ccm_cscmr2) = (*(imxrt_common.ccm + ccm_cscmr2) & ~(0x3 << 19)) | ((val & 0x3) << 19);
			break;

		case clk_mux_can:
			*(imxrt_common.ccm + ccm_cscmr2) = (*(imxrt_common.ccm + ccm_cscmr2) & ~(0x3 << 8)) | ((val & 0x3) << 8);
			break;

		case clk_mux_uart:
			*(imxrt_common.ccm + ccm_cscdr1) = (*(imxrt_common.ccm + ccm_cscdr1) & ~(1 << 6)) | ((val & 1) << 6);
			break;

		case clk_mux_enc:
			*(imxrt_common.ccm + ccm_cs2cdr) = (*(imxrt_common.ccm + ccm_cs2cdr) & ~(0x7 << 15)) | ((val & 0x7) << 15);
			break;

		case clk_mux_ldbDi1:
			*(imxrt_common.ccm + ccm_cs2cdr) = (*(imxrt_common.ccm + ccm_cs2cdr) & ~(0x7 << 12)) | ((val & 0x7) << 12);
			break;

		case clk_mux_ldbDi0:
			*(imxrt_common.ccm + ccm_cs2cdr) = (*(imxrt_common.ccm + ccm_cs2cdr) & ~(0x7 << 9)) | ((val & 0x7) << 9);
			break;

		case clk_mux_spdif:
			*(imxrt_common.ccm + ccm_cdcdr) = (*(imxrt_common.ccm + ccm_cdcdr) & ~(0x3 << 20)) | ((val & 0x3) << 20);
			break;

		case clk_mux_flexio1:
			*(imxrt_common.ccm + ccm_cdcdr) = (*(imxrt_common.ccm + ccm_cdcdr) & ~(0x3 << 7)) | ((val & 0x3) << 7);
			break;

		case clk_mux_lpi2c:
			*(imxrt_common.ccm + ccm_cscdr2) = (*(imxrt_common.ccm + ccm_cscdr2) & ~(1 << 18)) | ((val & 1) << 18);
			break;

		case clk_mux_lcdif1pre:
			*(imxrt_common.ccm + ccm_cscdr2) = (*(imxrt_common.ccm + ccm_cscdr2) & ~(0x7 << 15)) | ((val & 0x7) << 15);
			break;

		case clk_mux_lcdif1:
			*(imxrt_common.ccm + ccm_cscdr2) = (*(imxrt_common.ccm + ccm_cscdr2) & ~(0x7 << 9)) | ((val & 0x7) << 9);
			break;

		case clk_mux_csi:
			*(imxrt_common.ccm + ccm_cscdr3) = (*(imxrt_common.ccm + ccm_cscdr3) & ~(0x3 << 9)) | ((val & 0x3) << 9);
			break;
	}
}


u32 _imxrt_ccmGetMux(int mux)
{
	u32 val = 0;

	switch (mux) {
		case clk_mux_pll3:
			val = *(imxrt_common.ccm + ccm_ccsr) & 1;
			break;

		case clk_mux_periph:
			val = (*(imxrt_common.ccm + ccm_cbcdr) >> 25) & 1;
			break;

		case clk_mux_semcAlt:
			val = (*(imxrt_common.ccm + ccm_cbcdr) >> 7) & 1;
			break;

		case clk_mux_semc:
			val = (*(imxrt_common.ccm + ccm_cbcdr) >> 6) & 1;
			break;

		case clk_mux_prePeriph:
			val = (*(imxrt_common.ccm + ccm_cbcmr) >> 18) & 0x3;
			break;

		case clk_mux_trace:
			val = (*(imxrt_common.ccm + ccm_cbcmr) >> 14) & 0x3;
			break;

		case clk_mux_periphclk2:
			val = (*(imxrt_common.ccm + ccm_cbcmr) >> 12) & 0x3;
			break;

		case clk_mux_lpspi:
			val = (*(imxrt_common.ccm + ccm_cbcmr) >> 4) & 0x3;
			break;

		case clk_mux_flexspi:
			val = (*(imxrt_common.ccm + ccm_cscmr1) >> 29) & 0x3;
			break;

		case clk_mux_usdhc2:
			val = (*(imxrt_common.ccm + ccm_cscmr1) >> 17) & 1;
			break;

		case clk_mux_usdhc1:
			val = (*(imxrt_common.ccm + ccm_cscmr1) >> 16) & 1;
			break;

		case clk_mux_sai3:
			val = (*(imxrt_common.ccm + ccm_cscmr1) >> 14) & 0x3;
			break;

		case clk_mux_sai2:
			val = (*(imxrt_common.ccm + ccm_cscmr1) >> 12) & 0x3;
			break;

		case clk_mux_sai1:
			val = (*(imxrt_common.ccm + ccm_cscmr1) >> 10) & 0x3;
			break;

		case clk_mux_perclk:
			val = (*(imxrt_common.ccm + ccm_cscmr1) >> 6) & 1;
			break;

		case clk_mux_flexio2:
			val = (*(imxrt_common.ccm + ccm_cscmr2) >> 19) & 0x3;
			break;

		case clk_mux_can:
			val = (*(imxrt_common.ccm + ccm_cscmr2) >> 8) & 0x3;
			break;

		case clk_mux_uart:
			val = (*(imxrt_common.ccm + ccm_cscdr1) >> 6) & 1;
			break;

		case clk_mux_enc:
			val = (*(imxrt_common.ccm + ccm_cs2cdr) >> 15) & 0x7;
			break;

		case clk_mux_ldbDi1:
			val = (*(imxrt_common.ccm + ccm_cs2cdr) >> 12) & 0x7;
			break;

		case clk_mux_ldbDi0:
			val = (*(imxrt_common.ccm + ccm_cs2cdr) >> 9) & 0x7;
			break;

		case clk_mux_spdif:
			val = (*(imxrt_common.ccm + ccm_cdcdr) >> 20) & 0x3;
			break;

		case clk_mux_flexio1:
			val = (*(imxrt_common.ccm + ccm_cdcdr) >> 7) & 0x3;
			break;

		case clk_mux_lpi2c:
			val = (*(imxrt_common.ccm + ccm_cscdr2) >> 18) & 1;
			break;

		case clk_mux_lcdif1pre:
			val = (*(imxrt_common.ccm + ccm_cscdr2) >> 15) & 0x7;
			break;

		case clk_mux_lcdif1:
			val = (*(imxrt_common.ccm + ccm_cscdr2) >> 9) & 0x7;
			break;

		case clk_mux_csi:
			val = (*(imxrt_common.ccm + ccm_cscdr3) >> 9) & 0x3;
			break;
	}

	return val;
}


void _imxrt_ccmSetDiv(int div, u32 val)
{
	switch (div) {
		case clk_div_arm: /* CACRR */
			*(imxrt_common.ccm + ccm_cacrr) = (*(imxrt_common.ccm + ccm_cacrr) & ~0x7) | (val & 0x7);
			while (*(imxrt_common.ccm + ccm_cdhipr) & (1 << 16));
			break;

		case clk_div_periphclk2: /* CBCDR */
			*(imxrt_common.ccm + ccm_cbcdr) = (*(imxrt_common.ccm + ccm_cbcdr) & ~(0x7 << 27)) | ((val & 0x7) << 27);
			break;

		case clk_div_semc: /* CBCDR */
			*(imxrt_common.ccm + ccm_cbcdr) = (*(imxrt_common.ccm + ccm_cbcdr) & ~(0x7 << 16)) | ((val & 0x7) << 16);
			while (*(imxrt_common.ccm + ccm_cdhipr) & 1);
			break;

		case clk_div_ahb: /* CBCDR */
			*(imxrt_common.ccm + ccm_cbcdr) = (*(imxrt_common.ccm + ccm_cbcdr) & ~(0x7 << 10)) | ((val & 0x7) << 10);
			while (*(imxrt_common.ccm + ccm_cdhipr) & (1 << 1));
			break;

		case clk_div_ipg: /* CBCDR */
			*(imxrt_common.ccm + ccm_cbcdr) = (*(imxrt_common.ccm + ccm_cbcdr) & ~(0x3 << 8)) | ((val & 0x3) << 8);
			break;

		case clk_div_lpspi: /* CBCMR */
			*(imxrt_common.ccm + ccm_cbcmr) = (*(imxrt_common.ccm + ccm_cbcmr) & ~(0x7 << 26)) | ((val & 0x7) << 26);
			break;

		case clk_div_lcdif1: /* CBCMR */
			*(imxrt_common.ccm + ccm_cbcmr) = (*(imxrt_common.ccm + ccm_cbcmr) & ~(0x7 << 23)) | ((val & 0x7) << 23);
			break;

		case clk_div_flexspi: /* CSCMR1 */
			*(imxrt_common.ccm + ccm_cscmr1) = (*(imxrt_common.ccm + ccm_cscmr1) & ~(0x7 << 23)) | ((val & 0x7) << 23);
			break;

		case clk_div_perclk: /* CSCMR1 */
			*(imxrt_common.ccm + ccm_cscmr1) = (*(imxrt_common.ccm + ccm_cscmr1) & ~0x3f) | (val & 0x3f);
			break;

		case clk_div_ldbDi1: /* CSCMR2 */
			*(imxrt_common.ccm + ccm_cscmr2) = (*(imxrt_common.ccm + ccm_cscmr2) & ~(1 << 11)) | ((val & 1) << 11);
			break;

		case clk_div_ldbDi0: /* CSCMR2 */
			*(imxrt_common.ccm + ccm_cscmr2) = (*(imxrt_common.ccm + ccm_cscmr2) & ~(1 << 10)) | ((val & 1) << 10);
			break;

		case clk_div_can: /* CSCMR2 */
			*(imxrt_common.ccm + ccm_cscmr2) = (*(imxrt_common.ccm + ccm_cscmr2) & ~(0x3f << 2)) | ((val & 0x3f) << 2);
			break;

		case clk_div_trace: /* CSCDR1 */
			*(imxrt_common.ccm + ccm_cscdr1) = (*(imxrt_common.ccm + ccm_cscdr1) & ~(0x7 << 25)) | ((val & 0x7) << 25);
			break;

		case clk_div_usdhc2: /* CSCDR1 */
			*(imxrt_common.ccm + ccm_cscdr1) = (*(imxrt_common.ccm + ccm_cscdr1) & ~(0x7 << 16)) | ((val & 0x7) << 16);
			break;

		case clk_div_usdhc1: /* CSCDR1 */
			*(imxrt_common.ccm + ccm_cscdr1) = (*(imxrt_common.ccm + ccm_cscdr1) & ~(0x7 << 11)) | ((val & 0x7) << 11);
			break;

		case clk_div_uart: /* CSCDR1 */
			*(imxrt_common.ccm + ccm_cscdr1) = (*(imxrt_common.ccm + ccm_cscdr1) & ~0x3f) | (val & 0x3f);
			break;

		case clk_div_flexio2: /* CS1CDR */
			*(imxrt_common.ccm + ccm_cs1cdr) = (*(imxrt_common.ccm + ccm_cs1cdr) & ~(0x7 << 25)) | ((val & 0x7) << 25);
			break;

		case clk_div_sai3pre: /* CS1CDR */
			*(imxrt_common.ccm + ccm_cs1cdr) = (*(imxrt_common.ccm + ccm_cs1cdr) & ~(0x7 << 22)) | ((val & 0x7) << 22);
			break;

		case clk_div_sai3: /* CS1CDR */
			*(imxrt_common.ccm + ccm_cs1cdr) = (*(imxrt_common.ccm + ccm_cs1cdr) & ~(0x3f << 16)) | ((val & 0x3f) << 16);
			break;

		case clk_div_flexio2pre: /* CS1CDR */
			*(imxrt_common.ccm + ccm_cs1cdr) = (*(imxrt_common.ccm + ccm_cs1cdr) & ~(0x7 << 9)) | ((val & 0x7) << 9);
			break;

		case clk_div_sai1pre: /* CS1CDR */
			*(imxrt_common.ccm + ccm_cs1cdr) = (*(imxrt_common.ccm + ccm_cs1cdr) & ~(0x7 << 6)) | ((val & 0x7) << 6);
			break;

		case clk_div_sai1: /* CS1CDR */
			*(imxrt_common.ccm + ccm_cs1cdr) = (*(imxrt_common.ccm + ccm_cs1cdr) & ~0x3f) | (val & 0x3f);
			break;

		case clk_div_enc: /* CS2CDR */
			*(imxrt_common.ccm + ccm_cs2cdr) = (*(imxrt_common.ccm + ccm_cs2cdr) & ~(0x3f << 21)) | ((val & 0x3f) << 21);
			break;

		case clk_div_encpre: /* CS2CDR */
			*(imxrt_common.ccm + ccm_cs2cdr) = (*(imxrt_common.ccm + ccm_cs2cdr) & ~(0x7 << 18)) | ((val & 0x7) << 18);
			break;

		case clk_div_sai2pre: /* CS2CDR */
			*(imxrt_common.ccm + ccm_cs2cdr) = (*(imxrt_common.ccm + ccm_cs2cdr) & ~(0x7 << 6)) | ((val & 0x7) << 6);
			break;

		case clk_div_sai2: /* CS2CDR */
			*(imxrt_common.ccm + ccm_cs2cdr) = (*(imxrt_common.ccm + ccm_cs2cdr) & ~0x3f) | (val & 0x3f);
			break;

		case clk_div_spdif0pre: /* CDCDR */
			*(imxrt_common.ccm + ccm_cdcdr) = (*(imxrt_common.ccm + ccm_cdcdr) & ~(0x7 << 25)) | ((val & 0x7) << 25);
			break;

		case clk_div_spdif0: /* CDCDR */
			*(imxrt_common.ccm + ccm_cdcdr) = (*(imxrt_common.ccm + ccm_cdcdr) & ~(0x7 << 22)) | ((val & 0x7) << 22);
			break;

		case clk_div_flexio1pre: /* CDCDR */
			*(imxrt_common.ccm + ccm_cdcdr) = (*(imxrt_common.ccm + ccm_cdcdr) & ~(0x7 << 12)) | ((val & 0x7) << 12);
			break;

		case clk_div_flexio1: /* CDCDR */
			*(imxrt_common.ccm + ccm_cdcdr) = (*(imxrt_common.ccm + ccm_cdcdr) & ~(0x7 << 9)) | ((val & 0x7) << 9);
			break;

		case clk_div_lpi2c: /* CSCDR2 */
			*(imxrt_common.ccm + ccm_cscdr2) = (*(imxrt_common.ccm + ccm_cscdr2) & ~(0x3f << 19)) | ((val & 0x3f) << 19);
			break;

		case clk_div_lcdif1pre: /* CSCDR2 */
			*(imxrt_common.ccm + ccm_cscdr2) = (*(imxrt_common.ccm + ccm_cscdr2) & ~(0x7 << 12)) | ((val & 0x7) << 12);
			break;

		case clk_div_csi: /* CSCDR3 */
			*(imxrt_common.ccm + ccm_cscdr3) = (*(imxrt_common.ccm + ccm_cscdr3) & ~(0x7 << 11)) | ((val & 0x7) << 11);
			break;
	}
}


u32 _imxrt_ccmGetDiv(int div)
{
	u32 val = 0;

	switch (div) {
		case clk_div_arm: /* CACRR */
			val = *(imxrt_common.ccm + ccm_cacrr) & 0x7;
			break;

		case clk_div_periphclk2: /* CBCDR */
			val = (*(imxrt_common.ccm + ccm_cbcdr) >> 27) & 0x7;
			break;

		case clk_div_semc: /* CBCDR */
			val = (*(imxrt_common.ccm + ccm_cbcdr) >> 16) & 0x7;
			break;

		case clk_div_ahb: /* CBCDR */
			val = (*(imxrt_common.ccm + ccm_cbcdr) >> 10) & 0x7;
			break;

		case clk_div_ipg: /* CBCDR */
			val = (*(imxrt_common.ccm + ccm_cbcdr) >> 8) & 0x3;
			break;

		case clk_div_lpspi: /* CBCMR */
			val = (*(imxrt_common.ccm + ccm_cbcmr) >> 26) & 0x7;
			break;

		case clk_div_lcdif1: /* CBCMR */
			val = (*(imxrt_common.ccm + ccm_cbcmr) >> 23) & 0x7;
			break;

		case clk_div_flexspi: /* CSCMR1 */
			val = (*(imxrt_common.ccm + ccm_cscmr1) >> 23) & 0x7;
			break;

		case clk_div_perclk: /* CSCMR1 */
			val = *(imxrt_common.ccm + ccm_cscmr1) & 0x3f;
			break;

		case clk_div_ldbDi1: /* CSCMR2 */
			val = (*(imxrt_common.ccm + ccm_cscmr2) >> 11) & 1;
			break;

		case clk_div_ldbDi0: /* CSCMR2 */
			val = (*(imxrt_common.ccm + ccm_cscmr2) >> 10) & 1;
			break;

		case clk_div_can: /* CSCMR2 */
			val = (*(imxrt_common.ccm + ccm_cscmr2) >> 2) & 0x3f;
			break;

		case clk_div_trace: /* CSCDR1 */
			val = (*(imxrt_common.ccm + ccm_cscdr1) >> 25) & 0x7;
			break;

		case clk_div_usdhc2: /* CSCDR1 */
			val = (*(imxrt_common.ccm + ccm_cscdr1) >> 16) & 0x7;
			break;

		case clk_div_usdhc1: /* CSCDR1 */
			val = (*(imxrt_common.ccm + ccm_cscdr1) >> 11) & 0x7;
			break;

		case clk_div_uart: /* CSCDR1 */
			val = *(imxrt_common.ccm + ccm_cscdr1) & 0x3f;
			break;

		case clk_div_flexio2: /* CS1CDR */
			val = (*(imxrt_common.ccm + ccm_cs1cdr) >> 25) & 0x7;
			break;

		case clk_div_sai3pre: /* CS1CDR */
			val = (*(imxrt_common.ccm + ccm_cs1cdr) >> 22) & 0x7;
			break;

		case clk_div_sai3: /* CS1CDR */
			val = (*(imxrt_common.ccm + ccm_cs1cdr) >> 16) & 0x3f;
			break;

		case clk_div_flexio2pre: /* CS1CDR */
			val = (*(imxrt_common.ccm + ccm_cs1cdr) >> 9) & 0x7;
			break;

		case clk_div_sai1pre: /* CS1CDR */
			val = (*(imxrt_common.ccm + ccm_cs1cdr) >> 6) & 0x7;
			break;

		case clk_div_sai1: /* CS1CDR */
			val = *(imxrt_common.ccm + ccm_cs1cdr) & 0x3f;
			break;

		case clk_div_enc: /* CS2CDR */
			val = (*(imxrt_common.ccm + ccm_cs2cdr) >> 21) & 0x3f;
			break;

		case clk_div_encpre: /* CS2CDR */
			val = (*(imxrt_common.ccm + ccm_cs2cdr) >> 18) & 0x7;
			break;

		case clk_div_sai2pre: /* CS2CDR */
			val = (*(imxrt_common.ccm + ccm_cs2cdr) >> 6) & 0x7;
			break;

		case clk_div_sai2: /* CS2CDR */
			val = *(imxrt_common.ccm + ccm_cs2cdr) & 0x3f;
			break;

		case clk_div_spdif0pre: /* CDCDR */
			val = (*(imxrt_common.ccm + ccm_cdcdr) >> 25) & 0x7;
			break;

		case clk_div_spdif0: /* CDCDR */
			val = (*(imxrt_common.ccm + ccm_cdcdr) >> 22) & 0x7;
			break;

		case clk_div_flexio1pre: /* CDCDR */
			val = (*(imxrt_common.ccm + ccm_cdcdr) >> 12) & 0x7;
			break;

		case clk_div_flexio1: /* CDCDR */
			val = (*(imxrt_common.ccm + ccm_cdcdr) >> 9) & 0x7;
			break;

		case clk_div_lpi2c: /* CSCDR2 */
			val = (*(imxrt_common.ccm + ccm_cscdr2) >> 19) & 0x3f;
			break;

		case clk_div_lcdif1pre: /* CSCDR2 */
			val = (*(imxrt_common.ccm + ccm_cscdr2) >> 12) & 0x7;
			break;

		case clk_div_csi: /* CSCDR3 */
			val = (*(imxrt_common.ccm + ccm_cscdr3) >> 11) & 0x7;
			break;
	}

	return val;
}


void _imxrt_ccmControlGate(int dev, int state)
{
	int index = dev >> 4, shift = (dev & 0xf) << 1;

	if (index > 6)
		return;

	*(imxrt_common.ccm + ccm_ccgr0 + index) = (*(imxrt_common.ccm + ccm_ccgr0 + index) & ~(0x3 << shift)) | ((state & 0x3) << shift);
}


void _imxrt_ccmSetMode(int mode)
{
	*(imxrt_common.ccm + ccm_clpcr) = (*(imxrt_common.ccm + ccm_clpcr) & ~0x3) | (mode & 0x3);
}


/* SCB */


void _imxrt_scbSetPriorityGrouping(u32 group)
{
	u32 t;

	/* Get register value and clear bits to set */
	t = *(imxrt_common.scb + scb_aircr) & ~0xffff0700;

	/* Store new value */
	*(imxrt_common.scb + scb_aircr) = t | 0x5fa0000 | ((group & 7) << 8);
}


u32 _imxrt_scbGetPriorityGrouping(void)
{
	return (*(imxrt_common.scb + scb_aircr) & 0x700) >> 8;
}


void _imxrt_scbSetPriority(s8 excpn, u32 priority)
{
	volatile u8 *ptr;

	ptr = &((u8*)(imxrt_common.scb + scb_shp0))[excpn - 4];

	*ptr = (priority << 4) & 0x0ff;
}


u32 _imxrt_scbGetPriority(s8 excpn)
{
	volatile u8 *ptr;

	ptr = &((u8*)(imxrt_common.scb + scb_shp0))[excpn - 4];

	return *ptr >> 4;
}


/* NVIC (Nested Vectored Interrupt Controller */


void _imxrt_nvicSetIRQ(s8 irqn, u8 state)
{
	volatile u32 *ptr = imxrt_common.nvic + ((u8)irqn >> 5) + (state ? nvic_iser: nvic_icer);
	*ptr |= 1 << (irqn & 0x1F);
}


u32 _imxrt_nvicGetPendingIRQ(s8 irqn)
{
	volatile u32 *ptr = imxrt_common.nvic + ((u8)irqn >> 5) + nvic_ispr;
	return !!(*ptr & (1 << (irqn & 0x1F)));
}


void _imxrt_nvicSetPendingIRQ(s8 irqn, u8 state)
{
	volatile u32 *ptr = imxrt_common.nvic + ((u8)irqn >> 5) + (state ? nvic_ispr: nvic_icpr);
	*ptr |= 1 << (irqn & 0x1F);
}


u32 _imxrt_nvicGetActive(s8 irqn)
{
	volatile u32 *ptr = imxrt_common.nvic + ((u8)irqn >> 5) + nvic_iabr;
	return !!(*ptr & (1 << (irqn & 0x1F)));
}


void _imxrt_nvicSetPriority(s8 irqn, u32 priority)
{
	volatile u8 *ptr;

	ptr = (u8*)(imxrt_common.nvic + irqn + nvic_ip);

	*ptr = (priority << 4) & 0x0ff;
}


u8 _imxrt_nvicGetPriority(s8 irqn)
{
	volatile u8 *ptr;

	ptr = (u8*)(imxrt_common.nvic + irqn + nvic_ip);

	return *ptr >> 4;
}


void _imxrt_nvicSystemReset(void)
{
	*(imxrt_common.scb + scb_aircr) = ((0x5fa << 16) | (*(imxrt_common.scb + scb_aircr) & (0x700)) | (1 << 0x02));

	__asm__ volatile ("dsb");

	for(;;);
}


/* SysTick */


int _imxrt_systickInit(u32 interval)
{
	u64 load = ((u64) interval * imxrt_common.cpuclk) / 1000000;
	if (load > 0x00ffffff)
		return -EINVAL;

	*(imxrt_common.stk + stk_load) = (u32)load;
	*(imxrt_common.stk + stk_ctrl) = 0x7;

	return EOK;
}


void _imxrt_systickSet(u8 state)
{
	*(imxrt_common.stk + stk_ctrl) &= ~(!state);
	*(imxrt_common.stk + stk_ctrl) |= !!state;
}


u32 _imxrt_systickGet(void)
{
	u32 cb;

	cb = ((*(imxrt_common.stk + stk_load) - *(imxrt_common.stk + stk_val)) * 1000) / *(imxrt_common.stk + stk_load);

	/* Add 1000 us if there's systick pending */
	if (*(imxrt_common.scb + scb_icsr) & (1 << 26))
		cb += 1000;

	return cb;
}


/* GPIO */


static volatile u32 *_imxrt_gpioGetReg(unsigned int d)
{
	switch (d) {
		case gpio1:
			return imxrt_common.gpio[0];
		case gpio2:
			return imxrt_common.gpio[1];
		case gpio3:
			return imxrt_common.gpio[2];
		case gpio4:
			return imxrt_common.gpio[3];
		case gpio5:
			return imxrt_common.gpio[4];
	}

	return NULL;
}


int _imxrt_gpioConfig(unsigned int d, u8 pin, u8 dir)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	_imxrt_ccmControlGate(d, 1);

	if (!reg || pin > 31)
		return -EINVAL;

	*(reg + gpio_gdir) &= ~(!dir << pin);
	*(reg + gpio_gdir) |= !!dir << pin;

	return EOK;
}


int _imxrt_gpioSet(unsigned int d, u8 pin, u8 val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if (!reg || pin > 31)
		return -EINVAL;

	*(reg + gpio_dr) &= ~(!val << pin);
	*(reg + gpio_dr) |= !!val << pin;

	return EOK;
}


int _imxrt_gpioSetPort(unsigned int d, u32 val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if (!reg)
		return -EINVAL;

	*(reg + gpio_dr) = val;

	return EOK;
}


int _imxrt_gpioGet(unsigned int d, u8 pin, u8 *val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if (!reg || pin > 31)
		return -EINVAL;

	*val = !!(*(reg + gpio_psr) & (1 << pin));

	return EOK;
}


int _imxrt_gpioGetPort(unsigned int d, u32 *val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if (!reg)
		return -EINVAL;

	*val = *(reg + gpio_psr);

	return EOK;
}


/* IOMUX */


void _imxrt_iomuxSetPinMux(int pin, u32 mode, u8 sion)
{
	volatile u32 *reg;

	if (pin <= gpio_stby)
		reg = (void *)(0x400a8000 + ((pin - gpio_wakeup) << 2));
	else if (pin <= gpio_onoff)
		return;
	else
		reg = (void *)(0x401f8014 + ((pin - gpio_emc_00) << 2));

	*reg = (mode & 0x7) | (!!sion << 4);
}


void _imxrt_iomuxSetPinConfig(int pin, u8 hys, u8 pus, u8 pue, u8 pke, u8 ode, u8 speed, u8 dse, u8 sre)
{
	volatile u32 *reg;

	if (pin <= gpio_stby)
		reg = (void *)(0x400a8018 + ((pin - gpio_wakeup) << 2));
	else if (pin <= gpio_onoff)
		reg = (void *)(0x400a800c + ((pin - gpio_test) << 2));
	else
		reg = (void *)(0x401f8204 + ((pin - gpio_emc_00) << 2));

	*reg = (sre & 1) | ((dse & 0x7) << 3) | ((speed & 0x7) << 6) | ((ode & 1) << 11) | ((pke & 1) << 12) |
			((pue & 1) << 13) | ((pus & 0x3) << 14) | ((hys & 1) << 16);
}


/* LCD interface */


void _imxrt_lcdInit(void)
{
	volatile u32 i;

	/* Init pixel clock */
	_imxrt_ccmInitVideoPll(31, 8, 0, 0);
	_imxrt_ccmSetMux(clk_mux_lcdif1pre, 2);
	_imxrt_ccmSetDiv(clk_div_lcdif1pre, 4);
	_imxrt_ccmSetDiv(clk_div_lcdif1, 1);
	_imxrt_ccmSetMux(clk_mux_lcdif1, 0);
	_imxrt_ccmControlGate(lcd, 1);
	_imxrt_ccmControlGate(lcdpixel, 1);

	/* Reset the LCD */
	_imxrt_gpioConfig(gpio1, 2, 1);
	_imxrt_gpioSet(gpio1, 2, 0);
	for (i = 0; i < 256; ++i)
		__asm__ volatile ("nop");
	_imxrt_gpioSet(gpio1, 2, 1);

	/* Turn on backlight */
	_imxrt_gpioConfig(gpio2, 31, 1);
	_imxrt_gpioSet(gpio2, 31, 1);

	/* Reset LCD driver */
	*(imxrt_common.lcd + lcd_ctrl_clr) = 1 << 30;
	while (*(imxrt_common.lcd + lcd_ctrl) & (1 << 30));
	*(imxrt_common.lcd + lcd_ctrl_set) = 1 << 31;
	while (!(*(imxrt_common.lcd + lcd_ctrl) & (1 << 31)));
	for (i = 0; i < 256; ++i)
		__asm__ volatile ("nop");
	*(imxrt_common.lcd + lcd_ctrl_clr) = 0x3 << 30;
}


void _imxrt_lcdSetTiming(u16 width, u16 height, u32 flags, u8 hsw, u8 hfp, u8 hbp, u8 vsw, u8 vfp, u8 vbp)
{
	*(imxrt_common.lcd + lcd_transfer_count) = ((u32)height << 16) | (u32)width;
	*(imxrt_common.lcd + lcd_vdctrl0) = (1 << 28) | (1 << 21) | (1 << 20) | flags | (u32)vsw;
	*(imxrt_common.lcd + lcd_vdctrl1) = (u32)vsw + (u32)vbp + (u32)vfp + (u32)height;
	*(imxrt_common.lcd + lcd_vdctrl2) = ((u32)hsw << 18) | ((u32)hfp + (u32)hbp + (u32)hsw + (u32)width);
	*(imxrt_common.lcd + lcd_vdctrl3) = (((u32)hbp + (u32)hsw) << 16) | ((u32)vbp + (u32)vsw);
	*(imxrt_common.lcd + lcd_vdctrl4) = (1 << 18) | (u32)width;
}


int _imxrt_lcdSetConfig(int format, int bus)
{
	u32 ctrl, ctrl1;

	switch (format) {
		case lcd_RAW8:
			ctrl = 1 << 8;
			ctrl1 = 0xf << 16;
			break;

		case lcd_RGB565:
			ctrl = 0;
			ctrl1 = 0xf << 16;
			break;

		case lcd_RGB666:
			ctrl = (0x3 << 8) | 0x2;
			ctrl1 = 0x7 << 16;
			break;

		case lcd_ARGB888:
			ctrl = 0x3 << 8;
			ctrl1 = 0x7 << 16;
			break;

		case lcd_RGB888:
			ctrl = 0x3 << 8;
			ctrl1 = 0xf << 16;
			break;

		default:
			return -EINVAL;
	}

	switch (bus) {
		case lcd_bus8:
			ctrl |= 1 << 10;
			break;

		case lcd_bus16:
			break;

		case lcd_bus18:
			ctrl |= 1 << 11;
			break;

		case lcd_bus24:
			ctrl |= 0x3 << 10;
			break;

		default:
			return -EINVAL;
	}

	ctrl |= (1 << 17) | (1 << 19) | (1 << 5);

	*(imxrt_common.lcd + lcd_ctrl) = ctrl;
	*(imxrt_common.lcd + lcd_ctrl1) = ctrl1;

	return EOK;
}


void _imxrt_lcdSetBuffer(void * buffer)
{
	*(imxrt_common.lcd + lcd_next_buf) = (u32)buffer;
}


void _imxrt_lcdStart(void * buffer)
{
	*(imxrt_common.lcd + lcd_cur_buf) = (u32)buffer;
	*(imxrt_common.lcd + lcd_next_buf) = (u32)buffer;
	*(imxrt_common.lcd + lcd_ctrl_set) = (1 << 17) | 1;
}


/* PendSV and Systick */


void _imxrt_invokePendSV(void)
{
	*(imxrt_common.scb + scb_icsr) |= (1 << 28);
}


void _imxrt_invokeSysTick(void)
{
	*(imxrt_common.scb + scb_icsr) |= (1 << 26);
}


unsigned int _imxrt_cpuid(void)
{
	return *(imxrt_common.scb + scb_cpuid);
}


void _imxrt_wdgReload(void)
{
}


static void _imxrt_initPins(void)
{
	_imxrt_ccmControlGate(iomuxc, clk_state_run_wait);

	/* GPIO as I2C1 */
	_imxrt_iomuxSetPinMux(gpio_ad_b1_00, 5, 1);
	_imxrt_iomuxSetPinMux(gpio_ad_b1_01, 5, 1);

	/* LPUART1 */
	_imxrt_iomuxSetPinMux(gpio_ad_b0_12, 2, 0);
	_imxrt_iomuxSetPinMux(gpio_ad_b0_13, 2, 0);

	_imxrt_iomuxSetPinMux(gpio_ad_b0_02, 5, 0);
	_imxrt_iomuxSetPinMux(gpio_ad_b1_15, 5, 0);

	/* LCD */
	_imxrt_iomuxSetPinMux(gpio_b0_00, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_01, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_02, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_03, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_04, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_05, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_06, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_07, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_08, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_09, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_10, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_11, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_12, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_13, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_14, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b0_15, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b1_00, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b1_01, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b1_02, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_b1_03, 0, 0);
	_imxrt_iomuxSetPinMux(gpio_sd_b0_04, 6, 0);

	_imxrt_iomuxSetPinConfig(gpio_ad_b1_00, 0, 2, 1, 1, 1, 2, 7, 0);
	_imxrt_iomuxSetPinConfig(gpio_ad_b1_01, 0, 2, 1, 1, 1, 2, 7, 0);
	_imxrt_iomuxSetPinConfig(gpio_ad_b0_12, 0, 0, 0, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_ad_b0_13, 0, 0, 0, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_ad_b0_02, 0, 0, 0, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_ad_b1_15, 0, 0, 0, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_00, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_01, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_02, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_03, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_04, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_05, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_06, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_07, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_08, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_09, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_10, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_11, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_12, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_13, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_14, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b0_15, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b1_00, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b1_01, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b1_02, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_b1_03, 1, 2, 1, 1, 0, 2, 6, 0);
	_imxrt_iomuxSetPinConfig(gpio_sd_b0_04, 1, 0, 0, 0, 0, 3, 7, 1);
}


void _imxrt_init(void)
{
	u32 ccsidr, sets, ways;

	imxrt_common.gpio[0] = (void *)0x401b8000;
	imxrt_common.gpio[1] = (void *)0x401bc000;
	imxrt_common.gpio[2] = (void *)0x401c0000;
	imxrt_common.gpio[3] = (void *)0x401c4000;
	imxrt_common.gpio[4] = (void *)0x400c0000;
	imxrt_common.ccm = (void *)0x400fc000;
	imxrt_common.ccm_analog = (void *)0x400d8000;
	imxrt_common.pmu = (void *)0x400d8110;
	imxrt_common.xtalosc = (void *)0x400d8000;
	imxrt_common.iomuxsw = (void *)0x401f8000;
	imxrt_common.nvic = (void *)0xe000e100;
	imxrt_common.scb = (void *)0xe000ed00;
	imxrt_common.stk = (void *)0xe000e010;
	imxrt_common.wdog1 = (void *)0x400b8000;
	imxrt_common.wdog2 = (void *)0x400d0000;
	imxrt_common.rtwdog = (void *)0x400bc000;
	imxrt_common.lcd = (void *)0x402b8000;

	imxrt_common.xtaloscFreq = 24000000;
	imxrt_common.cpuclk = 528000000; /* Default system clock */

	/* Disable watchdogs */
	if (*(imxrt_common.wdog1 + wdog_wcr) & (1 << 2))
		*(imxrt_common.wdog1 + wdog_wcr) &= ~(1 << 2);
	if (*(imxrt_common.wdog2 + wdog_wcr) & (1 << 2))
		*(imxrt_common.wdog2 + wdog_wcr) &= ~(1 << 2);

	*(imxrt_common.rtwdog + rtwdog_cnt) = 0xd928c520; /* Update key */
	*(imxrt_common.rtwdog + rtwdog_total) = 0xffff;
	*(imxrt_common.rtwdog + rtwdog_cs) = (*(imxrt_common.rtwdog + rtwdog_cs) & ~(1 << 7)) | (1 << 5);

	/* Disable Systick which might be enabled by bootrom */
	if (*(imxrt_common.stk + stk_ctrl) & 1)
		*(imxrt_common.stk + stk_ctrl) &= ~1;

	/* Enable I$ */
	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
	*(imxrt_common.scb + scb_iciallu) = 0; /* Invalidate I$ */
	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
	*(imxrt_common.scb + scb_ccr) |= 1 << 17;
	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();

	/* Enable D$ */
	*(imxrt_common.scb + scb_csselr) = 0;
	hal_cpuDataSyncBarrier();

	ccsidr = *(imxrt_common.scb + scb_ccsidr);

	/* Invalidate D$ */
	for (sets = (ccsidr >> 13) & 0x7fff; sets-- != 0; )
		for (ways = (ccsidr >> 3) & 0x3ff; ways-- != 0; )
			*(imxrt_common.scb + scb_dcisw) = ((sets & 0x1ff) << 5) | ((ways & 0x3) << 30);
	hal_cpuDataSyncBarrier();

	*(imxrt_common.scb + scb_ccr) |= 1 << 16;

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();

	_imxrt_initPins();

	_imxrt_ccmSetMux(clk_mux_periphclk2, 0x1);
	_imxrt_ccmSetMux(clk_mux_periph, 0x1);

	/* Configure ARM PLL to 1056M */
	_imxrt_ccmInitArmPll(88);
	_imxrt_ccmInitSysPll(1);
	_imxrt_ccmInitUsb1Pll(0);

	_imxrt_ccmSetDiv(clk_div_arm, 0x1);
	_imxrt_ccmSetDiv(clk_div_ahb, 0x0);
	_imxrt_ccmSetDiv(clk_div_ipg, 0x3);

	/* Now CPU runs again on ARM PLL at 600M (with divider 2) */
	_imxrt_ccmSetMux(clk_mux_prePeriph, 0x3);
	_imxrt_ccmSetMux(clk_mux_periph, 0x0);

	/* Disable unused clocks */
	*(imxrt_common.ccm + ccm_ccgr0) = 0x00c0000f;
	*(imxrt_common.ccm + ccm_ccgr1) = 0x30000000;
	*(imxrt_common.ccm + ccm_ccgr2) = 0xff3f003f;
	*(imxrt_common.ccm + ccm_ccgr3) = 0xf00c0fff;
	*(imxrt_common.ccm + ccm_ccgr4) = 0x0000ff3c;
	*(imxrt_common.ccm + ccm_ccgr5) = 0xf00f330f;
	*(imxrt_common.ccm + ccm_ccgr6) = 0x00fc0f00;

	/* Power down all unused PLL */
	_imxrt_ccmDeinitAudioPll();
	_imxrt_ccmDeinitEnetPll();
	_imxrt_ccmDeinitUsb2Pll();
}
