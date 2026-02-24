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

void HookMessageExTests(void);
void HookMemoryTests(void);
void HookFunctionTests(void);
void GetImageByNameTests(void);
