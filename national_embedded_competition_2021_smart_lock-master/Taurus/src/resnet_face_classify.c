#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "sample_comm_nnie.h"
#include "nnie_sample_plug.h"

#include "hi_ext_util.h"
#include "mpp_help.h"
#include "ai_plug.h"

#include "hisignalling.h"


#define PLUG_UUID          "\"hi.resnet_face_classify\""
#define PLUG_DESC          "\"组员识别(resnet)\""     // UTF8 encode

#define FRM_WIDTH          224
#define FRM_HEIGHT         224

#define MODEL_FILE_FACE    "./plugs/resnet_face_classify.wk" // 开源模型转换

#define RET_NUM_MAX         4 		// 返回number的最大数目trr
#define SCORE_MAX           4096 	// 最大概率对应的score
#define THRESH_MIN          30 		// 可接受的概率阈值(超过此值则返回给app)

#define TXT_BEGX           20
#define TXT_BEGY           20
#define FONT_WIDTH         32
#define FONT_HEIGHT        40
#define AUDIO_CASE_TWO     2
#define AUDIO_SCORE        90 		// 置信度可自行配置
#define AUDIO_FRAME        14 		// 每隔15帧识别一次，可自行配置

int uart_face_fd;

static OsdSet* g_osdsFace = NULL;
static HI_S32 g_osd0Face = -1;

static pthread_t g_thrdId = 0;

static const char RESNET_FACE_CLASSIFY[] = "{"
    "\"uuid\": " PLUG_UUID ","
    "\"desc\": " PLUG_DESC ","
    "\"frmWidth\": " HI_TO_STR(FRM_WIDTH) ","
    "\"frmHeight\": " HI_TO_STR(FRM_HEIGHT) ","
    "\"butt\": 0"
"}";

static const char* ResnetFaceClassifyProf(void)
{
    return RESNET_FACE_CLASSIFY;
}
/*
    加载模型
*/
static HI_S32 ResnetFaceClassifyLoad(uintptr_t* model, OsdSet* osds)
{
    SAMPLE_SVP_NNIE_CFG_S *self = NULL;
    HI_S32 ret;

    g_osdsFace = osds;
    HI_ASSERT(g_osdsFace);
    g_osd0Face = OsdsCreateRgn(g_osdsFace);
    HI_ASSERT(g_osd0Face >= 0);

    ret = CnnCreate(&self, MODEL_FILE_FACE);
    *model = ret < 0 ? 0 : (uintptr_t)self;

    uart_face_fd = uartOpenInit();
	return ret;
}
/*
    卸载模型
*/
static HI_S32 ResnetFaceClassifyUnload(uintptr_t model)
{
    CnnDestroy((SAMPLE_SVP_NNIE_CFG_S*)model);
    OsdsClear(g_osdsFace);
	
    return HI_SUCCESS;
}
/**
    将计算结果打包为resJson.
*/
HI_CHAR* ResnetFaceClassifyToJson(const RecogNumInfo items[], HI_S32 itemNum)
{
    HI_S32 jsonSize = TINY_BUF_SIZE + itemNum * TINY_BUF_SIZE; // 每个item的打包size为TINY_BUF_SIZE
    HI_CHAR *jsonBuf = (HI_CHAR*)malloc(jsonSize);
    HI_ASSERT(jsonBuf);
    HI_S32 offset = 0;

    offset += snprintf_s(jsonBuf + offset, jsonSize - offset, jsonSize - offset - 1, "[");
    for (HI_S32 i = 0; i < itemNum; i++) {
        const RecogNumInfo *item = &items[i];
        uint32_t score = item->score * HI_PER_BASE / SCORE_MAX;
        if (score < THRESH_MIN) {
            break;
        }

        offset += snprintf_s(jsonBuf + offset, jsonSize - offset, jsonSize - offset - 1,
            "%s{ \"classify num\": %u, \"score\": %u }", (i == 0 ? "\n  " : ", "), (uint)item->num, (uint)score);
        HI_ASSERT(offset < jsonSize);
    }
    offset += snprintf_s(jsonBuf + offset, jsonSize - offset, jsonSize - offset - 1, "]");
    HI_ASSERT(offset < jsonSize);
    return jsonBuf;
}

/**
    发送分类信息到Pegasus
*/
static HI_S32 Resnet_face_classify_send_msg(const RecogNumInfo items[], HI_S32 itemNum, HI_CHAR* buf, HI_S32 size)
{
    HI_S32 offset = 0;
    HI_CHAR *person_name = NULL;

    offset += snprintf_s(buf + offset, size - offset, size - offset - 1, "team member classify: {");
    for (HI_S32 i = 0; i < itemNum; i++) {
        const RecogNumInfo *item = &items[i];
        uint32_t score = item->score * HI_PER_BASE / SCORE_MAX;
        if (score < THRESH_MIN) {
            break;
        }
        switch (item->num) {
            case 0u:
                person_name = "Zzy";
                usbUartSendRead(uart_face_fd, ZZY);
                usleep(100*100);
                break;
            case 1u:
                person_name = "Czx";
                usbUartSendRead(uart_face_fd, CZX);
                usleep(100*100);
                break;
            case 2u:
                person_name = "Szy";
                usbUartSendRead(uart_face_fd, SZY);
                usleep(100*100);
                break;
            default:
                person_name = "Unknown";
                usbUartSendRead(uart_face_fd, 3);
                usleep(100*100);
        }

        offset += snprintf_s(buf + offset, size - offset, size - offset - 1,
            "%s%s %u:%u%%", (i == 0 ? " " : ", "), person_name, (int)item->num, (int)score);
        HI_ASSERT(offset < size);
    }
    offset += snprintf_s(buf + offset, size - offset, size - offset - 1, " }");
    HI_ASSERT(offset < size);
    return offset;
}

static HI_S32 ResnetFaceClassifyCal(uintptr_t model,
    VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *resFrm, HI_CHAR** resJson)
{
    SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S*)model; // reference to SDK sample_comm_nnie.h Line 99
    IVE_IMAGE_S img; // referece to SDK hi_comm_ive.h Line 143
    static HI_CHAR prevOsd[NORM_BUF_SIZE] = ""; // 安全，插件架构约定同时只会有一个线程访问插件
    HI_CHAR osdBuf[NORM_BUF_SIZE] = "";
    /*
        01_szy_face   02_czx_face
        03_zzy_face   04_unknown           
    */
    RecogNumInfo resBuf[RET_NUM_MAX] = {0};
    HI_S32 reslen = 0;
    HI_S32 ret;

    ret = FrmToOrigImg((VIDEO_FRAME_INFO_S*)srcFrm, &img);
    HI_EXP_RET(ret != HI_SUCCESS, ret, "ResnetFaceClassifyCal FAIL, for YUV2RGB FAIL, ret=%#x\n", ret);

    ret = CnnCalU8c1Img(self, &img, resBuf, HI_ARRAY_SIZE(resBuf), &reslen); // 沿用该推理逻辑
    HI_EXP_LOGE(ret < 0, "resnet cal FAIL, ret=%d\n", ret);
    HI_ASSERT(reslen <= sizeof(resBuf) / sizeof(resBuf[0]));

    // 生成resJson和resOsd
    HI_CHAR *jsonBuf = ResnetFaceClassifyToJson(resBuf, reslen);
    *resJson = jsonBuf;
    Resnet_face_classify_send_msg(resBuf, reslen, osdBuf, sizeof(osdBuf));
	
    // 仅当resJson与此前计算发生变化时,才重新打OSD输出文字
    if (strcmp(osdBuf, prevOsd) != 0) {
        HiStrxfrm(prevOsd, osdBuf, sizeof(prevOsd));

        // 叠加图形到resFrm中
        HI_OSD_ATTR_S rgn;
        TxtRgnInit(&rgn, osdBuf, TXT_BEGX, TXT_BEGY, ARGB1555_YELLOW2, FONT_WIDTH, FONT_HEIGHT);
        OsdsSetRgn(g_osdsFace, g_osd0Face, &rgn);
        LOGI("RESNET face classify: %s\n", osdBuf);
    }
    return ret;
}

static const AiPlug G_RESNET_FACE_CLASSIFY_ITF = {
    .Prof = ResnetFaceClassifyProf,
    .Load = ResnetFaceClassifyLoad,
    .Unload = ResnetFaceClassifyUnload,
    .Cal = ResnetFaceClassifyCal,
};

const AiPlug* AiPlugItf(uint32_t* magic)
{
    if (magic) {
        *magic = AI_PLUG_MAGIC;
    }

    return (AiPlug*)&G_RESNET_FACE_CLASSIFY_ITF;
}
