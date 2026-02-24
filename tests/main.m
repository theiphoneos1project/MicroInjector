#include "Private.h"

int main(void) {
    NSAutoreleasePool *const pool = [NSAutoreleasePool new];

    NSLog(@"Starting MicroInjector tests");    
    NSLog(@"============================");

    {
        NSLog(@"START HookMessageEx tests");
        HookMessageExTests();
        NSLog(@"END HookMessageEx tests");
    }

    NSLog(@"============================");

    {
        NSLog(@"START HookMemory tests");
        HookMemoryTests();
        NSLog(@"END HookMemory tests");
    }

    NSLog(@"============================");

    {
        NSLog(@"START HookFunction tests");
        HookFunctionTests();
        NSLog(@"END HookFunction tests");
    }

    NSLog(@"============================");

    NSLog(@"============================");    
    NSLog(@"Passed all tests!");    

    [pool drain];

    return EXIT_SUCCESS;
}
