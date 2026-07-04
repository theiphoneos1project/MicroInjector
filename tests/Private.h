//
// Copyright (c) 2026 Nightwind
//

#import <Foundation/Foundation.h>

typedef struct {
    BOOL boolField;
    id idField;
    int intField;
} MicroInjectorTestReturnStruct_t;

@interface PSBundleController : NSObject
- (void)load;
- (void)unload;
@end

@interface PSListController : NSObject
+ (BOOL)displaysButtonBar;
- (void)reloadSpecifier:(id)specifier animated:(BOOL)animated;
@end

@interface PSViewController : NSObject
+ (BOOL)isOverlay;
- (BOOL)popController;

- (MicroInjectorTestReturnStruct_t)structTest;
@end

@interface PSSpecifier : NSObject
+ (instancetype)emptyGroupSpecifier;
- (id)identifier;
- (void)setProperty:(id)property forKey:(NSString *)key;
@end

@interface PhotosNavigationItem : NSObject
- (int)barStyle;
- (void)setBarStyle:(int)barStyle;
@end

@interface UIApplication : NSObject
+ (NSBundle *)pickerBundle;
+ (void)setPickerBundle:(NSBundle *)pickerBundle;
@end

@interface BackgroundView : NSObject
- (void)popNavigationItem;
@end

@interface PLImageScroller : NSObject
- (BOOL)canHandleSwipes;
- (BOOL)mi_canHandleSwipes;
@end

@interface CPActivityMonitor : NSObject
+ (id)currentMonitor;
- (BOOL)shouldCancel;
@end

@interface CPURLMatch : NSObject
- (NSRange)range;
- (NSString *)description;
@end

void HookMessageExTests(void);
void HookMemoryTests(void);
void HookFunctionTests(void);
void GetImageByNameTests(void);
void FindSymbolTests(void);
void MSHookMessageTests(void);
void MSHookClassPairTests(void);
