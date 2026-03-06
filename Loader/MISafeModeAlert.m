#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import "MISafeModeAlert.h"
#import <fcntl.h>
#import <spawn.h>

@interface SBAlertItem : NSObject
- (UIAlertSheet *)alertSheet;
@end

@interface SBAlertItemsController : NSObject
+ (instancetype)sharedInstance;
- (void)activateAlertItem:(SBAlertItem *)alertItem;
- (void)deactivateAlertItem:(SBAlertItem *)alertItem;
- (void)deactivateAlertItem:(SBAlertItem *)alertItem reason:(int)reason;
@end

@interface SpringBoard : NSObject
- (void)relaunchSpringBoard;
@end

@interface SBApplication : NSObject
+ (SpringBoard *)springBoard;
@end

static Class MISafeModeAlertItem_cls = nil;

static void MISafeModeAlertItem_alertSheet_buttonClicked_(SBAlertItem *self, __unused SEL _cmd, __unused id alertSheet, __unused int button) {
    switch (button) {
        case 1: {
            remove(SAFE_MODE_MARKER_PATH);
            [[objc_getClass("SBApplication") springBoard] relaunchSpringBoard];
            
            break;
        }
        case 2: {
            SBAlertItemsController *const itemsController = [objc_getClass("SBAlertItemsController") sharedInstance];
            if ([itemsController respondsToSelector:@selector(deactivateAlertItem:reason:)]) {
                [itemsController deactivateAlertItem:self reason:2];
            } else if ([itemsController respondsToSelector:@selector(deactivateAlertItem:)]) {
                [itemsController deactivateAlertItem:self];
            } else {
                [alertSheet dismiss];
            }

            break;
        }
    }
}

static void MISafeModeAlertItem_configure_requirePasscodeForActions_(SBAlertItem *self, __unused SEL _cmd, __unused BOOL configure, __unused BOOL require) {
    UIAlertSheet *alertSheet = [self alertSheet];
    [alertSheet setTitle:@"Safe Mode"];
    [alertSheet setBodyText:@"You've entered Safe Mode. SpringBoard tweaks will not be injected until you exit Safe Mode.\n\nYou can select Dismiss to safely remove any broken tweaks.\n\nTap the status bar to show this alert again."];
    [alertSheet addButtonWithTitle:@"Exit Safe Mode"];
    [alertSheet addButtonWithTitle:@"Dismiss"];
    [alertSheet setDelegate:self];
}

static void MISafeModeAlertItem_performUnlockAction(SBAlertItem *self, __unused SEL _cmd) {
    [[objc_getClass("SBAlertItemsController") sharedInstance] activateAlertItem:self];
}

void PresentSafeModeAlert(void) {
    if (MISafeModeAlertItem_cls == nil) {
        MISafeModeAlertItem_cls = objc_lookUpClass("MISafeModeAlertItem");
    }

    if (MISafeModeAlertItem_cls == nil) {
        MISafeModeAlertItem_cls = objc_allocateClassPair(objc_getClass("SBAlertItem"), "MISafeModeAlertItem", 0);
        
        if (MISafeModeAlertItem_cls == nil) {
            return;
        }

        class_addMethod(MISafeModeAlertItem_cls, @selector(alertSheet:buttonClicked:), (IMP)&MISafeModeAlertItem_alertSheet_buttonClicked_, "v@:@i");
        class_addMethod(MISafeModeAlertItem_cls, @selector(configure:requirePasscodeForActions:), (IMP)&MISafeModeAlertItem_configure_requirePasscodeForActions_, "v@:cc");
        class_addMethod(MISafeModeAlertItem_cls, @selector(performUnlockAction), (IMP)&MISafeModeAlertItem_performUnlockAction, "v@:");
        objc_registerClassPair(MISafeModeAlertItem_cls);
    }

    const Class SBAlertItemsController_cls = objc_getClass("SBAlertItemsController");
    if (SBAlertItemsController_cls != nil) {
        [[SBAlertItemsController_cls sharedInstance] activateAlertItem:[[MISafeModeAlertItem_cls new] autorelease]];
    }
}
