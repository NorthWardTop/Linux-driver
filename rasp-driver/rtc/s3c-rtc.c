
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <asm/irq.h>


#ifndef __ASM_ARCH_REGS_RTC_H
#define __ASM_ARCH_REGS_RTC_H __FILE__

#define S3C2410_RTCREG(x) (x)
#define S3C2410_INTP		S3C2410_RTCREG(0x30)
#define S3C2410_INTP_ALM	(1 << 1)
#define S3C2410_INTP_TIC	(1 << 0)

#define S3C2410_RTCCON		S3C2410_RTCREG(0x40)
#define S3C2410_RTCCON_RTCEN	(1 << 0)
#define S3C2410_RTCCON_CNTSEL	(1 << 2)
#define S3C2410_RTCCON_CLKRST	(1 << 3)
#define S3C2443_RTCCON_TICSEL	(1 << 4)
#define S3C64XX_RTCCON_TICEN	(1 << 8)

#define S3C2410_TICNT		S3C2410_RTCREG(0x44)
#define S3C2410_TICNT_ENABLE	(1 << 7)

/* S3C2443: tick count is 15 bit wide
 * TICNT[6:0] contains upper 7 bits
 * TICNT1[7:0] contains lower 8 bits
 */
#define S3C2443_TICNT_PART(x)	((x & 0x7f00) >> 8)
#define S3C2443_TICNT1		S3C2410_RTCREG(0x4C)
#define S3C2443_TICNT1_PART(x)	(x & 0xff)

/* S3C2416: tick count is 32 bit wide
 * TICNT[6:0] contains bits [14:8]
 * TICNT1[7:0] contains lower 8 bits
 * TICNT2[16:0] contains upper 17 bits
 */
#define S3C2416_TICNT2		S3C2410_RTCREG(0x48)
#define S3C2416_TICNT2_PART(x)	((x & 0xffff8000) >> 15)

#define S3C2410_RTCALM		S3C2410_RTCREG(0x50)
#define S3C2410_RTCALM_ALMEN	(1 << 6)
#define S3C2410_RTCALM_YEAREN	(1 << 5)
#define S3C2410_RTCALM_MONEN	(1 << 4)
#define S3C2410_RTCALM_DAYEN	(1 << 3)
#define S3C2410_RTCALM_HOUREN	(1 << 2)
#define S3C2410_RTCALM_MINEN	(1 << 1)
#define S3C2410_RTCALM_SECEN	(1 << 0)

#define S3C2410_ALMSEC		S3C2410_RTCREG(0x54)
#define S3C2410_ALMMIN		S3C2410_RTCREG(0x58)
#define S3C2410_ALMHOUR		S3C2410_RTCREG(0x5c)

#define S3C2410_ALMDATE		S3C2410_RTCREG(0x60)
#define S3C2410_ALMMON		S3C2410_RTCREG(0x64)
#define S3C2410_ALMYEAR		S3C2410_RTCREG(0x68)

#define S3C2410_RTCSEC		S3C2410_RTCREG(0x70)
#define S3C2410_RTCMIN		S3C2410_RTCREG(0x74)
#define S3C2410_RTCHOUR		S3C2410_RTCREG(0x78)
#define S3C2410_RTCDATE		S3C2410_RTCREG(0x7c)
#define S3C2410_RTCMON		S3C2410_RTCREG(0x84)
#define S3C2410_RTCYEAR		S3C2410_RTCREG(0x88)

#endif /* __ASM_ARCH_REGS_RTC_H */



/**
 * S3C6410RTC驱动的rtc_class_ops实例与RTC注册
 */
static const struct rtc_class_ops s3c_rtcops = {
	.read_time	= s3c_rtc_gettime,
	.set_time	= s3c_rtc_settime,
	.read_alarm	= s3c_rtc_getalarm,
	.set_alarm	= s3c_rtc_setalarm,
	.proc		= s3c_rtc_proc,
	.alarm_irq_enable = s3c_rtc_setaie,
};

static int s3c_rtc_probe(struct platform_device *pdev)
{
	// TODO
	struct rtc_device *rtc = devm_rtc_device_register(&pdev->dev, "s3c", &s3c_rtcops, 
		THIS_MODULE);
	// TODO
}



/**
 * 
 * RTC核心层“转发”到底层RTC驱动callback
 */
static int rtc_dev_open(struct inode *inode, struct file *file)
{
	pass;
	err = ops->open ? ops->open(rtc->dev.parent) : 0;
	pass;
}

static int __rtc_read_time(struct rtc_device *rtc, struct rtc_time *tm)
{
	int err;
	if (!rtc->ops)
		err = -ENODEV;
	else if (!rtc->ops->read_time)
		err = -ENODEV;
	pass;
	return err;
}


int rtc_read_time(struct rtc_device *rtc, struct rtc_time *tm)
{
	int err = mutex_lock_interruptible(&rtc->ops_lock);
	if (err) 
		return err;
	
	err = __rtc_read_time(rtc, tm);
	mutex_unlock(&rtc->ops_lock);
	return err;
}


int rtc_set_time(struct rtc_device *rtc, struct rtc_time *tm)
{
	pass;
	if (!rtc->ops)
		err = -ENODEV;
	else if (rtc->ops->set_time)
		err = rtc->ops->set_time(rtc->dev.parent, tm);
	pass;

	return err;
}


static long rtc_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	pass;

	switch (cmd) {
	case RTC_RD_TIME:
		mutex_unlock(&rtc->ops_lock);
		
		err = rtc_read_time(rtc, &tm);
		if (err < 0)
			return err;
		
		if (copy_to_user(uarg, &tm, sizeof(tm)))
			err = -EFAULT;
		return err;
	case RTC_SET_TIME:
		mutex_unlock(&rtc->ops_lock);

		if (copy_from_user(&tm, uarg, sizeof(tm)))
			return -EFAULT;

		return rtc_set_time(rtc, &tm);
	}
}


