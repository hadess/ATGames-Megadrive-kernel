//===========================
#include "customer.h"
//===========================
#define uchar unsigned char
#define uint  unsigned int

#define ACK         (10)		//应答位为低电平
#define NACK         (20)		//非应答位为高电平
#define FAIL_ACK	(30)
#define PASS         (1)
#define FAIL         (0)
#define FALSE        (0)
#define SUCCESS      (1)
#define TRUE 	     (1)
// Device Configuration Register
#define DCR_ADDR      (0x18)
#define DCR_SME       (0x80)
#define DCR_UCR       (0x40)
#define DCR_UAT       (0x20)
#define DCR_ETA       (0x10)
#define DCR_CS        (0x0F)

// Cryptograms
#define CM_Ci         (0x50)
#define CM_Sk         (0x58)
#define CM_G          (0x90)

// Fuses
#define CM_FAB        (0x06)
#define CM_CMA        (0x04)
#define CM_PER        (0x00)

// Password
#define CM_PSW        (0xB0)
#define CM_PWREAD     (1)
#define CM_PWWRITE    (0)

//============================
// Macros for all of the registers
#define RA       (ucGpaRegisters[0])
#define RB       (ucGpaRegisters[1])
#define RC       (ucGpaRegisters[2])
#define RD       (ucGpaRegisters[3])
#define RE       (ucGpaRegisters[4])
#define RF       (ucGpaRegisters[5])
#define RG       (ucGpaRegisters[6])
#define TA       (ucGpaRegisters[7])
#define TB       (ucGpaRegisters[8])
#define TC       (ucGpaRegisters[9])
#define TD       (ucGpaRegisters[10])
#define TE       (ucGpaRegisters[11])
#define SA       (ucGpaRegisters[12])
#define SB       (ucGpaRegisters[13])
#define SC       (ucGpaRegisters[14])
#define SD       (ucGpaRegisters[15])
#define SE       (ucGpaRegisters[16])
#define SF       (ucGpaRegisters[17])
#define SG       (ucGpaRegisters[18])
#define Gpa_byte (ucGpaRegisters[19])
#define Gpa_Regs (20)

#define RDA       (ucRandRegisters[0])
#define RDB       (ucRandRegisters[1])
#define RDC       (ucRandRegisters[2])
#define RDD       (ucRandRegisters[3])
#define RDE       (ucRandRegisters[4])
#define RDF       (ucRandRegisters[5])
#define RDG       (ucRandRegisters[6])
#define Rand_byte (ucRandRegisters[7])
#define Rand_Regs (8)

// Defines for constants used
#define CM_MOD_R (0x1F)
#define CM_MOD_T (0x1F)
#define CM_MOD_S (0x7F)

// Macros for common operations
#define cm_Mod(x,y,m) ((x+y)>m?(x+y-m):(x+y))
#define cm_RotT(x)    (((x<<1)&0x1e)|((x>>4)&0x01))
#define cm_RotR(x)    (((x<<1)&0x1e)|((x>>4)&0x01))
#define cm_RotS(x)    (((x<<1)&0x7e)|((x>>6)&0x01))
// Function Prototypes
extern uchar cm_VerifyPassword(uchar ucPassword[3], uchar ucSet, uchar ucRW);
extern uchar cm_ResetCrypto(void);
extern uchar cm_SendChecksum(void);
extern uchar cm_AuthenEncrypt(uchar ucKeySet, uchar Buffer[16],uchar ucEncrypt);
extern uchar cm_ActiveSecurity(uchar ucKeySet, uchar ucKey[16],uchar ucEncrypt);
extern uchar cm_RstDevice(void);
extern uchar cm_ReadUserZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount);
extern uchar cm_WriteUserZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount);
extern uchar cm_SetUserZone(uchar ucZoneNumber);
extern uchar cm_WriteConfigZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount);
extern uchar cm_ReadConfigZone(uchar ucAddr, uchar ucBuffer[16], uchar ucCount);
