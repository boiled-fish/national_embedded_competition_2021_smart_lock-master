#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

/* Link Header Files */
#include <link_service.h>
// #include <link_service.h>
#include <link_platform.h>
#include <histreaming.h>
#include <hi_io.h>
#include <hi_early_debug.h>
#include <hi_gpio.h>

hi_bool phone_status;

/**
	@berf Control the gpio9 pin level, so as to control the lock on or off
	@param hi_gpio_value v: Input the value of the level on the gpio9 pin, 0 means low level, 1 means high level
*/

/**
	@berf The app gets the eigenvalue and value
	@param struct LinkService* ar:histreaming LinkService structural morphology
	@param const char* property: App get eigenvalue
	@param char* value: App gets value
	@param int len: value length
*/
static int GetStatusValue(struct LinkService* ar, const char* property, char* value, int len)
{
    (void)(ar);

    printf("Receive property: %s(value=%s[%d])\n", property, value, len);

    if (strcmp(property, "Status") == 0) {
        strcpy(value, "Opend");
    }

    /*
     * if Ok return 0,
     * Otherwise, any error, return StatusFailure
     */
    return 0;
}
/**
	@berf 3861 the device side receives the message sent by the app side
	@param struct LinkService* ar: histreaming LinkService structural morphology
	@param const char* property: App get eigenvalue
	@param char* value:App gets value
	@param int len: value length
*/
static int ModifyStatus(struct LinkService* ar, const char* property, char* value, int len)
{
    (void)(ar);

    if (property == NULL || value == NULL) {
        return -1;
    }

    static char last_value[4] = {"xxx"};

    /* modify status property*/
    if (strcmp(property, "status") == 0) {
        if (strcmp(value, last_value) != 0) {
            phone_status = HI_TRUE;
        }
    }

    printf("Receive property: %s(value=%s[%d])\n", property, value,len);

    /*
     * if Ok return 0,
     * Otherwise, any error, return StatusFailure
     */
    return 0;
}

/*
 * It is a Wifi IoT device
 */
static const char* g_wifista_type = "Light";
/**
	@berf Get 3861 device type from App
	@param struct LinkService* ar: histreaming LinkService structural morphology
	@return Return 3861 device type
*/
static const char* GetDeviceType(struct LinkService* ar)
{
    (void)(ar);

    return g_wifista_type;
}

static void *g_link_platform = NULL;

void* histreaming_open(void)
{
    hi_u32 ret = hi_gpio_init();
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ===== gpio -> hi_gpio_init ret:%d\r\n", ret);
        return NULL;
    }
    printf("----- gpio init success-----\r\n");

    LinkService* wifiIot = 0;
    LinkPlatform* link = 0;

    wifiIot = (LinkService*)malloc(sizeof(LinkService));
    if (!wifiIot){
        printf("malloc wifiIot failure\n");
        return NULL;
    }

    wifiIot->get    = GetStatusValue;
    wifiIot->modify = ModifyStatus;
    wifiIot->type = GetDeviceType;

    link = LinkPlatformGet();
    if (!link) {
        printf("get link failure\n");
        return NULL;
    }

    if (link->addLinkService(link, wifiIot, 1) != 0) {
        histreaming_close(link);
        return NULL;
    }

    if (link->open(link) != 0) {
        histreaming_close(link);
        return NULL;
    }

    /* cache link ptr*/
    g_link_platform = (void*)(link);

    return (void*)link;
}

void histreaming_close(void *link)
{
    LinkPlatform *linkPlatform = (LinkPlatform*)(link);
    if (!linkPlatform) {
        return;
    }

    linkPlatform->close(linkPlatform);

    if (linkPlatform != NULL) {
        LinkPlatformFree(linkPlatform);
    }
}

