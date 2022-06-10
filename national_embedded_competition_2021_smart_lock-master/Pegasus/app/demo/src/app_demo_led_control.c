#include <hi_gpio.h>
#include <hi_early_debug.h>
#include <hi_io.h>
#include <hi_time.h>
#include <hi_watchdog.h>
#include <hi_task.h>

#include <app_demo_adc.h>
#include "hisignalling_protocol.h"


#define LED_LIGHT_DELAY_1S (1000 * 1000)
#define hi_unuse_param(p) ((p) = (p))
#define LED_CONTROL_TASK_SLEEP_20MS (20)
#define power_sleep_100MS (100)
#define LED_CONTROL_TASK_SIZE (1024)
#define LED_CONTROL_TASK_PRIO (28)

hi_u32 g_led_control_id;
extern face_type face_sort;

/*
	@berf led control
	@param hi_void
	@return null
*/

extern hi_void oled_display_close(hi_void);
extern hi_void oled_display_open(hi_void);
extern hi_void hi_switch_init(hi_io_name id, hi_u8 val,hi_gpio_idx idx, hi_gpio_dir dir, hi_io_pull pval);

hi_bool hi_lock_status;

/*Method 2*/
hi_void *led_control_demo(hi_void* param)
{
    hi_u32 ret;
    hi_gpio_value unlock;

    hi_unuse_param(param);
    hi_gpio_init();

    hi_gpio_get_input_val(HI_GPIO_IDX_12, &unlock);

    /**
        book the times of one face showing consistantly
        记录某一分类的人脸连续出现的次数，超过 3 则视为成功识别并解锁
    */
    face_type last_face = UNKNOWN;
    static int book[4] = {0, 0, 0, 0};

    for (;;)
    {
        // 通过 GPIO12 的电平高低，判断锁的开关状态 unlock
        hi_gpio_get_input_val(HI_GPIO_IDX_12, &unlock);

        if(unlock)
        {
            // 锁开状态下，不执行开锁操作
            last_face = UNKNOWN;
        }
        else
        {
            // 只有在锁定状态下，进行相关判断

            // 判断识别结果是否与上次一致，不一致则清空
            if (last_face != face_sort)
                book[last_face] = 0;
            book[face_sort]++;
            last_face = face_sort;

            if (face_sort == ZZY && book[ZZY] > 3) // 设置 ZZY 开锁
            {
                // 将 GPIO9 电平拉高、开锁后，迅速拉低，防止锁烧坏
                hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE1);
                hi_sleep(power_sleep_100MS);
                hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE0);
                // 重置人脸分类结果和计数
                face_sort = UNKNOWN;
                book[ZZY] = 0;
            }
            else
            {
                hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE0);
            }
        }

        /*Release CPU resources for 20ms*/
        hi_sleep(10 * LED_CONTROL_TASK_SLEEP_20MS);
    }
    /*Delete task*/
    ret = hi_task_delete(g_led_control_id);
    if (ret != HI_ERR_SUCCESS)
    {
        printf("Failed to delete led control demo task\r\n");
    }
}
/*
	@berf Create task锛歛pp_demo_led_control_task 
	@param hi_void 
	@return HI_ERR_FAILURE/HI_ERR_SUCCESS ,Task creation failed or succeeded 
*/
hi_u32 app_demo_led_control_task(hi_void)
{
    hi_u32 ret;
    hi_task_attr led_control_attr = {0};
    /*led_ control_ Parameter configuration of attr structure*/
    led_control_attr.stack_size = LED_CONTROL_TASK_SIZE;            //Task stack memory
    led_control_attr.task_prio = LED_CONTROL_TASK_PRIO;             //The task priority ranges from 0 to 31. Tasks 0 to 10 should not be used. The SDK has been used. The higher the value, the lower the priority
    led_control_attr.task_name = (hi_char *)"app demo led control"; //Task name
                                                                    //Create task
    ret = hi_task_create(&g_led_control_id, &led_control_attr, led_control_demo, HI_NULL);
    if (ret != HI_ERR_SUCCESS)
    {
        printf("Failed to create led control demo\r\n");
        return HI_ERR_FAILURE;
    }
    return HI_ERR_SUCCESS;
}
