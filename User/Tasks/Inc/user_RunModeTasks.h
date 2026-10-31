#ifndef __USER_RUNMODETASKS_H__
#define __USER_RUNMODETASKS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "user_TasksInit.h"

void IdleEnterTask(void *argument);
void StopEnterTask(void *argument);

extern uint32_t IdleTimerCount;
/* STOP 唤醒后的宽限期(ms)：此期间 IdleEnterTask 不发送 Stop 消息，
   避免唤醒后空闲计时误判导致刚点亮的屏幕立即再次熄屏 */
extern volatile uint32_t WakeupGraceMs;

#ifdef __cplusplus
}
#endif

#endif