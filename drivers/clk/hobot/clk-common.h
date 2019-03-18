#ifndef __CLK_COMMON_H__
#define __CLK_COMMON_H__

//#define DEBUG_ON

#ifdef DEBUG_ON
#define CLK_DEBUG(fmt,args...) printf("[%s_%d]"fmt, __FUNCTION__, __LINE__, ##args)
#else
#define CLK_DEBUG(fmt,args...)
#endif

#endif
