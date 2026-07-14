//
// Thanks to EthanArbuckle for the huge amount of help!
//

#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

#define kHIDUsage_Csmr_Power            ((uint16_t)0x30)
#define kHIDUsage_Csmr_Menu             ((uint16_t)0x40)
#define kHIDUsage_Csmr_VolumeIncrement  ((uint16_t)0xE9)
#define kHIDUsage_Csmr_VolumeDecrement  ((uint16_t)0xEA)

bool IsButtonPressed(uint16_t usage);

#endif // BUTTONS_H
