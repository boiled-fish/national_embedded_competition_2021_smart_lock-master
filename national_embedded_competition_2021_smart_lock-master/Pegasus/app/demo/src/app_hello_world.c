#include <app_hello_world.h>

extern hi_void app_i2c_oled_demo(hi_void);
extern hi_u32 app_demo_led_control_task(hi_void);
extern hi_u32 hisignaling_msg_task(hi_void);
extern hi_void uart_demo(hi_void);


/*hello world entry*/
hi_void hispark_pegasus_hello_world(hi_void)
{
	/*oled display task*/
    app_i2c_oled_demo();
	/*led control task*/
    app_demo_led_control_task();
    
    /*hand control task*/
    hisignaling_msg_task();

    /*uart task*/
    uart_demo();
}