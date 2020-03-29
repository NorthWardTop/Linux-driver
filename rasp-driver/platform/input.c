
#include <linux/input.h>




// 分配/释放一个输入设备
struct input_dev *input_allocate_device(void);
void input_free_device(struct input_dev *dev);


/* 报告指定type、code的输入事件 */
void input_event(struct input_dev *dev, unsigned int type, unsigned int code, int value);
/* 报告键值 */
void input_report_key(struct input_dev *dev, unsigned int code, int value);
/* 报告相对坐标 */
void input_report_rel(struct input_dev *dev, unsigned int code, int value);
/* 报告绝对坐标 */
void input_report_abs(struct input_dev *dev, unsigned int code, int value);
/* 报告同步事件 */
void input_sync(struct input_dev *dev);


/**
 * 所有的输入事件, 使用input_event结构
 */
struct input_event {
	struct timeval time;
	__u16 type;
	__u16 code;
	__s32 value;
};

