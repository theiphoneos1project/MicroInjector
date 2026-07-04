//
// Copyright (c) 2026 Nightwind
//

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

    {
        NSLog(@"START GetImageByName tests");
        GetImageByNameTests();
        NSLog(@"END GetImageByName tests");
    }

    NSLog(@"============================");

    {
        NSLog(@"START FindSymbol tests");
        FindSymbolTests();
        NSLog(@"END FindSymbol tests");
    }

    NSLog(@"============================");
    NSLog(@"Substrate legacy API shims");
    NSLog(@"============================");

    {
        NSLog(@"START MSHookMessage tests");
        MSHookMessageTests();
        NSLog(@"END MSHookMessage tests");
    }

    NSLog(@"============================");

    {
        NSLog(@"START MSHookClassPair tests");
        MSHookClassPairTests();
        NSLog(@"END MSHookClassPair tests");
    }

    NSLog(@"============================");    
    NSLog(@"Passed all tests!");    

    [pool drain];

    return EXIT_SUCCESS;
}
