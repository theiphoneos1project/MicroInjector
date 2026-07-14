//
// Thanks to EthanArbuckle for the huge amount of help!
//

#include <mach/mach_time.h>

#include "buttons.h"
#include "iokit_private.h"

static IOHIDEventSystemRef g_EventSystem = NULL;

static bool IsSpecificButtonPressed(uint32_t usagePage, uint32_t usage) {
    uint64_t timestamp = mach_absolute_time();

    const IOHIDEventRef dummyEvent = IOHIDEventCreateKeyboardEvent(kCFAllocatorDefault, timestamp, usagePage, usage, FALSE, 0); 
    if (dummyEvent == NULL) {
        return false;
    }

    const IOHIDEventRef event = IOHIDEventSystemCopyEvent(g_EventSystem, kIOHIDEventTypeKeyboard, dummyEvent, 0);
    CFRelease(dummyEvent);
    if (event == NULL) {
        return false;
    }

    const int isDown = IOHIDEventGetIntegerValue(event, kIOHIDEventFieldKeyboardDown);
    CFRelease(event);
    return isDown != 0; 
}

static bool SetupEventSystem(void) {
    if (g_EventSystem != NULL) {
        return true;
    }
    
    g_EventSystem = IOHIDEventSystemCreate(kCFAllocatorDefault);
    if (g_EventSystem == NULL) {
        fprintf(stderr, "[-] Failed to create event system\n");
        return false;
    }

    const int openResult = IOHIDEventSystemOpen(g_EventSystem, NULL, NULL, NULL, NULL);
    if (openResult == 0) {
        fprintf(stderr, "[-] Failed to open event system\n");
        CFRelease(g_EventSystem);
        g_EventSystem = NULL;
        return false;
    }
        
    return true;
}

bool IsButtonPressed(uint16_t usage) {
    const bool setup = SetupEventSystem();
    if (!setup) {
        return false;
    }

    return IsSpecificButtonPressed(kHIDPage_Consumer, usage);
}
