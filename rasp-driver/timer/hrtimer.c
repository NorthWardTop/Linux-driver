#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <sound/pcm.h>

struct imx_pcm_runtime_data {
	// unsigned int period;
	// int periods;
	// unsigned long offset;
	struct hrtimer hrt;
	int poll_time_ns;
	// struct snd_pcm_substream *substream;
	// atomic_t playing;
	// atomic_t capturing;
};


// 定时器到时间执行, 在中断上下文执行, 
static enum hrtimer_restart snd_hrtimer_callback(struct hrtimer *hrt)
{
	struct imx_pcm_runtime_data *iprtd =
		container_of(hrt, struct imx_pcm_runtime_data, hrt);

	//执行完成内容后, timer向前移动poll_time_ns
	hrtimer_forward_now(hrt, ns_to_ktime(iprtd->poll_time_ns));

	return HRTIMER_RESTART;
}



static int snd_imx_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct imx_pcm_runtime_data *iprtd = runtime->private_data;

	switch (cmd) {
	//启动播放
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		//启动timer, 经过poll_time_ns后, snd_hrtimer_callback在中断上下文被执行
		hrtimer_start(&iprtd->hrt, ns_to_ktime(iprtd->poll_time_ns),
		      HRTIMER_MODE_REL);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}


static int snd_imx_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct imx_pcm_runtime_data *iprtd;

	iprtd = kzalloc(sizeof(*iprtd), GFP_KERNEL);
	if (iprtd == NULL)
		return -ENOMEM;
	runtime->private_data = iprtd;
	// 打开时候初始化
	hrtimer_init(&iprtd->hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	//并指定回调函数
	iprtd->hrt.function = snd_hrtimer_callback;

	return 0;
}


static int snd_imx_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct imx_pcm_runtime_data *iprtd = runtime->private_data;
	//声卡关闭时取消timer
	hrtimer_cancel(&iprtd->hrt);

	kfree(iprtd);

	return 0;
}


static struct snd_pcm_ops imx_pcm_ops = {
	.open		= snd_imx_open,
	.close		= snd_imx_close,
	.trigger	= snd_imx_pcm_trigger,
};