#ifndef _RDA5890_IOCTL_H_
#define _RDA5890_IOCTL_H_

#include "linux/sockios.h"

#define IOCTL_RDA5890_BASE                SIOCDEVPRIVATE
#define IOCTL_RDA5890_GET_MAGIC           IOCTL_RDA5890_BASE
#define IOCTL_RDA5890_GET_DRIVER_VER      (IOCTL_RDA5890_BASE + 1)
#define IOCTL_RDA5890_MAC_GET_FW_VER      (IOCTL_RDA5890_BASE + 2)
#define IOCTL_RDA5890_MAC_WID             (IOCTL_RDA5890_BASE + 3)
#define IOCTL_RDA5890_SET_WAPI_ASSOC_IE   (IOCTL_RDA5890_BASE + 4)
#define IOCTL_RDA5890_GET_WAPI_ASSOC_IE   (IOCTL_RDA5890_BASE + 5)

#define RDA5890_MAGIC 0x5890face

#endif /* _RDA5890_IOCTL_H_  */
 
