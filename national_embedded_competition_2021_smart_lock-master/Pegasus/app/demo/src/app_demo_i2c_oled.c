/*
 * Copyright (c) 2020 HiHope Community.
 * Description: oled demo
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */
#include "hi_i2c.h"
#include <hi_early_debug.h>
#include <string.h>
#include <hi_time.h>
#include <ssd1306_oled.h>
#include <code_tab.h>
#include <los_hwi.h>
#include <hi_task.h>
#include <hi_mux.h>
#include <hi_watchdog.h>

#include "hisignalling_protocol.h"

extern hi_u8 g_current_mode;

#define I2C_REG_ARRAY_LEN           (64)
#define OLED_SEND_BUFF_LEN          (28)
#define OLED_SEND_BUFF_LEN2         (25)
#define OLED_SEND_BUFF_LEN3         (27)
#define OLED_SEND_BUFF_LEN4         (29)
#define Max_Column                  (128)
#define OLED_DEMO_TASK_STAK_SIZE    (1024*2)
#define OLED_DEMO_TASK_PRIORITY     (29)
#define OLED_DISPLAY_INTERVAL_TIME  (1)
#define SEND_CMD_LEN                (2)

hi_u32  g_oled_demo_task_id =0;

/**
    @bref   Write command to ssd1306 screen register
	@param: hi_ u8 reg_ Addr: the write register type is passed in. If it is 0x00, the write command is sent. If it is 0x40, the data is written
    @status 0: indicates that the writing is successful, otherwise it fails
*/
static hi_u32 i2c_write_byte(hi_u8 reg_addr, hi_u8 cmd)
{
    hi_u32 status =0;
    hi_i2c_idx id = 0;//I2C 0
    hi_u8 send_len =2;
    hi_u8 user_data = cmd;
    hi_i2c_data oled_i2c_cmd = { 0 };
    hi_i2c_data oled_i2c_write_cmd = { 0 };

    hi_u8 send_user_cmd [SEND_CMD_LEN] = {OLED_ADDRESS_WRITE_CMD,user_data};
    hi_u8 send_user_data [SEND_CMD_LEN] = {OLED_ADDRESS_WRITE_DATA,user_data};

    /*If it is a write command, send the write command address 0x00*/
    if (reg_addr == OLED_ADDRESS_WRITE_CMD) {
        oled_i2c_write_cmd.send_buf = send_user_cmd;
        oled_i2c_write_cmd.send_len = send_len;
        status = hi_i2c_write(id, OLED_ADDRESS, &oled_i2c_write_cmd);
        if (status != HI_ERR_SUCCESS) {
            return status;
        }
    }
    /*If it is writing data, send and write data address 0x40*/
    else if (reg_addr == OLED_ADDRESS_WRITE_DATA) {
        oled_i2c_cmd.send_buf = send_user_data;
        oled_i2c_cmd.send_len = send_len;
        status = hi_i2c_write(id, OLED_ADDRESS, &oled_i2c_cmd);
        if (status != HI_ERR_SUCCESS) {
            return status;
        }
    }

    return HI_ERR_SUCCESS;
}

/**
	@berf Write command operation
	@param hi_u8 cmd: Incoming command to be written
	@return HI_ERR_FAILURE/HI_ERR_SUCCESS ,Failure or success
*/
static hi_u32 write_cmd(hi_u8 cmd)//Write command
{
    hi_u8 status =0;
    /*Write device address*/
	status = i2c_write_byte(OLED_ADDRESS_WRITE_CMD, cmd);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
}
/*
	@berf Write data operation
	@param hi_u8 i2c_data:Incoming data to be written
	@return HI_ERR_FAILURE/HI_ERR_SUCCESS ,Failure or success
*/
static hi_void write_data(hi_u8 i2c_data)//Write data
{
    hi_u8 status =0;
    /*Write device address*/
	status = i2c_write_byte(OLED_ADDRESS_WRITE_DATA, i2c_data);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
}
/* ssd1306 oled initialization*/
hi_u32 oled_init(hi_void)
{
    hi_u32 status;
    hi_udelay(DELAY_100_MS);//100ms  The delay here is very important

    status = write_cmd(DISPLAY_OFF);//--display off
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_LOW_COLUMN_ADDRESS);//---set low column address
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_HIGH_COLUMN_ADDRESS);//---set high column address
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_START_LINE_ADDRESS);//--set start line address
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_PAGE_ADDRESS);//--set page address
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(CONTRACT_CONTROL);// contract control
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(FULL_SCREEN);//--128
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }  
    status= write_cmd(SET_SEGMENT_REMAP);//set segment remap
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    } 
    status = write_cmd(NORMAL);//--normal / reverse
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status =write_cmd(SET_MULTIPLEX);//--set multiplex ratio(1 to 64)
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DUTY);//--1/32 duty
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SCAN_DIRECTION);//Com scan direction
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DISPLAY_OFFSET);//-set display offset
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DISPLAY_TYPE);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(OSC_DIVISION);//set osc division
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DIVISION);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(COLOR_MODE_OFF);//set area color mode off
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status= write_cmd(COLOR);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PRE_CHARGE_PERIOD);//Set Pre-Charge Period
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PERIOD);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PIN_CONFIGUARTION);//set com pin configuartion
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(CONFIGUARTION);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_VCOMH);//set Vcomh
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(VCOMH);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_CHARGE_PUMP_ENABLE);//set charge pump enable
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PUMP_ENABLE);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(TURN_ON_OLED_PANEL);//--turn on oled panel
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    return HI_ERR_SUCCESS;
}
/*
    @bref set start position Set starting point coordinates
    @param hi_u8 x:write start from x axis
           hi_u8 y:write start from y axis
*/
hi_void oled_set_position(hi_u8 x, hi_u8 y) 
{ 
    write_cmd(0xb0+y);
    write_cmd(((x&0xf0)>>4)|0x10);
    write_cmd(x&0x0f);
}
/*Full screen fill*/
hi_void oled_fill_screen(hi_u8 fii_data)
{
    hi_u8 m =0;
    hi_u8 n =0;

    for (m=0;m<8;m++) {
        write_cmd(0xb0+m);
        write_cmd(0x00);
        write_cmd(0x10);

        for (n=0;n<128;n++) {
            write_data(fii_data);
        }
    }
}
/*
    @bref Clear from a location
    @param hi_u8 fill_data: write data to screen register 
           hi_u8 line:write positon start from Y axis 
           hi_u8 pos :write positon start from x axis 
           hi_u8 len:write data len
*/
hi_void oled_position_clean_screen(hi_u8 fill_data, hi_u8 line, hi_u8 pos, hi_u8 len)
{
    hi_u8 m =line;
    hi_u8 n =0;

    write_cmd(0xb0+m);
    write_cmd(0x00);
    write_cmd(0x10);

    for (n=pos;n<len;n++) {
        write_data(fill_data);
    }   
}
/*
    @bref 8*16 typeface
    @param hi_u8 x:write positon start from x axis 
           hi_u8 y:write positon start from y axis
           hi_u8 chr:write data
           hi_u8 char_size:select typeface
 */
hi_void oled_show_char(hi_u8 x, hi_u8 y, hi_u8 chr,hi_u8 char_size)
{      	
	hi_u8 c=0;
    hi_u8 i=0;

    c = chr-' '; //Get the offset value	

    if (x>Max_Column-1) {
        x=0;
        y=y+2;
    }

    if (char_size ==16) {
        oled_set_position(x,y);	
        for(i=0;i<8;i++){
            write_data(F8X16[c*16+i]);
        }
        
        oled_set_position(x,y+1);
        for (i=0;i<8;i++) {
            write_data(F8X16[c*16+i+8]);
        }
        
    } else {	
        oled_set_position(x,y);
        for (i=0;i<6;i++) {
            write_data(F6x8[c][i]);
        }            
    }
}

/*
    @bref display string
    @param hi_u8 x:write positon start from x axis 
           hi_u8 y:write positon start from y axis
           hi_u8 *chr:write data
           hi_u8 char_size:select typeface
*/ 
hi_void oled_show_str(hi_u8 x, hi_u8 y, hi_u8 *chr, hi_u8 char_size)
{
	hi_u8 j=0;

    if (chr == NULL) {
        printf("param is NULL,Please check!!!\r\n");
        return;
    }

	while (chr[j] != '\0') {
        oled_show_char(x, y, chr[j], char_size);
		x += 8;
		if (x>120) {
            x = 0;
            y += 2;
        }
		j++;
	}
}
/**
	@berf Decimal to string
	*Input: Double decimal
	*Output: converted String
*/
hi_u8  *flaot_to_string(hi_double d, hi_u8 *str )
{
	hi_u8 str1[40] = {0};
	hi_s32 j = 0;
    hi_s32 k = 0;
    hi_s32 i = 0;

    if (str == NULL) {
        return;
    }

	i = (hi_s32)d;//The integral part of a floating point number
	while (i > 0) {
		str1[j++] = i % 10 + '0';
		i = i / 10;
	}

	for (k = 0;k < j;k++) {
		str[k] = str1[j-1-k];//The extracted integer parts are stored in another array in positive order
	}
	str[j++] = '.';
 
	d = d - (hi_s32)d;//Decimals extraction
	for (i = 0;i <1;i++) {
		d = d*10;
		str[j++] = (hi_s32)d + '0';
		d = d - (hi_s32)d;
	}
	while(str[--j] == '0');
	str[++j] = '\0';
	return str;
}
/**
	@berf Combustible gas value conversion, decimal conversion to character type
	@param hi_double d: Double precision value to be converted to character type
	@param hi_u8 *str: Data after conversion to STR character type
	@return str
*/
hi_u8  *flaot_to_string_gas(hi_double d, hi_u8 *str)
{
	hi_u8 str1[40] = {0};
	hi_s32 j = 0;
    hi_s32 k = 0;
    hi_s32 i = 0;

    if (str == NULL) {
        return;
    }

	i = (hi_s32)d;//The integral part of a floating point number
	while (i > 0) {
		str1[j++] = i % 10 + '0';
		i = i / 10;
	}

	for (k = 0;k < j;k++) {
		str[k] = str1[j-1-k];//The extracted integer parts are stored in another array in positive order
	}
	str[j++] = '.';
 
	d = d - (hi_s32)d;//Decimals extraction
	for (i = 0;i <3;i++) {
		d = d*10;
		str[j++] = (hi_s32)d + '0';
		d = d - (hi_s32)d;
	}
	while(str[--j] == '0');
	str[++j] = '\0';
	return str;
}

/* display close */
hi_void oled_display_close(hi_void)
{
    // oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen 
    oled_position_clean_screen(OLED_CLEAN_SCREEN, 2, 45, 50);
    oled_show_str(45, 2, "CLOSE", 1);
}

/* display open */
hi_void oled_display_open(hi_void)
{
    // oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen 
    oled_position_clean_screen(OLED_CLEAN_SCREEN, 2, 45, 50);
    oled_show_str(45, 2, "OPEN", 1);
}


/* display init */
hi_void oled_display_init()
{
    oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen 
    oled_show_str(20, 4, "working...", 1);
}

/* display times */
hi_void oled_display_times(face_type face_sort, hi_u8 times)
{
    char buf[4] = {0};
    buf[0] = times / 100 + '0';
    buf[1] = times / 10 % 10 + '0';
    buf[2] = times % 10 + '0';

    switch (face_sort) {
        case SZY:
            oled_position_clean_screen(OLED_CLEAN_SCREEN, 6, 15, 6);
            oled_show_str(15, 6, buf, 1);
            break;
        case CZX:
            oled_position_clean_screen(OLED_CLEAN_SCREEN, 6, 51, 6);
            oled_show_str(51, 6, buf, 1);
            break;
        case ZZY:
            oled_position_clean_screen(OLED_CLEAN_SCREEN, 6, 87, 6);
            oled_show_str(87, 6, buf, 1);
            break;
        default:
            break;
    }
}


/* oled screen demo display */
hi_void app_i2c_oled_demo(hi_void)
{
    /*Initialization time screen i2c baudrate setting*/
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400kbps */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
    /*ssd1306 config init*/
    oled_init();
    /*display init*/  
    oled_display_init();  
}
