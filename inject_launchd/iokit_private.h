#ifndef IOKIT_PRIVATE_H
#define IOKIT_PRIVATE_H

#include <CoreFoundation/CoreFoundation.h>

#define kHIDPage_Consumer 0x0C
#define kIOHIDEventTypeKeyboard 3
#define kIOHIDEventFieldKeyboardDown 0x30002

typedef uint32_t IOHIDEventType;

typedef uint32_t IOHIDEventField;

typedef uint32_t IOOptionBits;
typedef IOOptionBits IOHIDEventOptionBits;

typedef struct __IOHIDEvent *IOHIDEventRef;
typedef struct __IOHIDEventSystem *IOHIDEventSystemRef;

extern IOHIDEventSystemRef IOHIDEventSystemCreate(
    CFAllocatorRef allocator
);

extern Boolean IOHIDEventSystemOpen(
    IOHIDEventSystemRef eventSystem,
    void *callback,
    void *target,
    void *refcon,
    void *unused
);

extern IOHIDEventRef IOHIDEventCreateKeyboardEvent(
    CFAllocatorRef allocator, 
    uint64_t timestamp, 
    uint32_t usagePage, 
    uint32_t usage, 
    Boolean down, 
    IOHIDEventOptionBits flags
);

extern IOHIDEventRef IOHIDEventSystemCopyEvent(
    IOHIDEventSystemRef eventSystem,
    IOHIDEventType eventType,
    IOHIDEventRef event,
    IOOptionBits options
);

extern int IOHIDEventGetIntegerValue(
    IOHIDEventRef event,
    IOHIDEventField field
);

#endif // IOKIT_PRIVATE_H
