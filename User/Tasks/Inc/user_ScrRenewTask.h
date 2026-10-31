#ifndef __USER_SCRRENEWTASK_H__
#define __USER_SCRRENEWTASK_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "PageManager.h"
extern volatile uint8_t Key_RequestPageBack;
void ScrRenewTask(void *argument);
#ifdef __cplusplus
}
#endif
#endif