#if defined(OS_MACOS)

    #include <CoreFoundation/CFBundle.h>
    #include <ApplicationServices/ApplicationServices.h>
    #include <Foundation/NSUserDefaults.h>
    #include <AppKit/NSScreen.h>
    #include <CoreFoundation/CoreFoundation.h>
    #include <CoreText/CoreText.h>

    #include <string.h>
    #include <stdlib.h>
    #include <stdint.h>

    #import <Cocoa/Cocoa.h>
    #import <Foundation/Foundation.h>

    #include <SDL3/SDL.h>

    static NSWindow* getCocoaWindow(SDL_Window *window) {
        return (NSWindow*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    }


    void errorMessageMacos(const char *cMessage) {
        CFStringRef strMessage = CFStringCreateWithCString(NULL, cMessage, kCFStringEncodingUTF8);
        CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, strMessage, NULL, NULL, NULL, NULL, NULL);
    }

    void openFile(const char *path);
    void registerFont(const char *fontName, const char *fontPath);

    void openWebpageMacos(const char *url) {
        CFURLRef urlRef = CFURLCreateWithBytes(NULL, (uint8_t*)(url), strlen(url), kCFStringEncodingASCII, NULL);
        LSOpenCFURLRef(urlRef, NULL);
        CFRelease(urlRef);
    }

    bool isMacosSystemDarkModeEnabled(void) {
        NSString * appleInterfaceStyle = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];

        if (appleInterfaceStyle && [appleInterfaceStyle length] > 0) {
            return [[appleInterfaceStyle lowercaseString] containsString:@"dark"];
        } else {
            return false;
        }
    }

    float getBackingScaleFactor(void) {
        return [[NSScreen mainScreen] backingScaleFactor];
    }

    void setupMacosWindowStyle(SDL_Window *window, bool borderlessWindowMode) {
        NSWindow* cocoaWindow = getCocoaWindow(window);

        cocoaWindow.titleVisibility = NSWindowTitleHidden;

        if (borderlessWindowMode) {
            cocoaWindow.titlebarAppearsTransparent = YES;
            cocoaWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;

            [cocoaWindow setOpaque:NO];
            [cocoaWindow setHasShadow:YES];
            [cocoaWindow setBackgroundColor:[NSColor colorWithWhite: 0 alpha: 0.001f]];
        }
    }

    bool isMacosFullScreenModeEnabled(SDL_Window *window) {
        NSWindow* cocoaWindow = getCocoaWindow(window);
        return (cocoaWindow.styleMask & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
    }

    void enumerateFontsMacos(void) {
        CFArrayRef fontDescriptors = CTFontManagerCopyAvailableFontFamilyNames();
        CFIndex count = CFArrayGetCount(fontDescriptors);

        for (CFIndex i = 0; i < count; i++) {
            CFStringRef fontName = (CFStringRef)CFArrayGetValueAtIndex(fontDescriptors, i);

            // Get font path
            CFDictionaryRef attributes = (__bridge CFDictionaryRef)@{ (__bridge NSString *)kCTFontNameAttribute : (__bridge NSString *)fontName };
            CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithAttributes(attributes);
            CFURLRef fontURL = CTFontDescriptorCopyAttribute(descriptor, kCTFontURLAttribute);
            CFStringRef fontPath = CFURLCopyFileSystemPath(fontURL, kCFURLPOSIXPathStyle);

            registerFont([(__bridge NSString *)fontName UTF8String], [(__bridge NSString *)fontPath UTF8String]);

            CFRelease(descriptor);
            CFRelease(fontURL);
        }

        CFRelease(fontDescriptors);
    }

    void macosHandleTitlebarDoubleClickGesture(SDL_Window *window) {
        NSWindow* cocoaWindow = getCocoaWindow(window);

        // Consult user preferences: "System Settings -> Desktop & Dock -> Double-click a window's title bar to"
        NSString* action = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleActionOnDoubleClick"];
        
        if (action == nil || [action isEqualToString:@"None"]) {
            // Nothing to do
        } else if ([action isEqualToString:@"Minimize"]) {
            if ([cocoaWindow isMiniaturizable]) {
                [cocoaWindow miniaturize:nil];
            }
        } else if ([action isEqualToString:@"Maximize"]) {
            // `[NSWindow zoom:_ sender]` takes over pumping the main runloop for the duration of the resize,
            // and would interfere with our renderer's frame logic. Schedule it for the next frame
            
            CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^{
                if ([cocoaWindow isZoomable]) {
                    [cocoaWindow zoom:nil];
                }
            });
        }
    }

    bool macosIsWindowBeingResizedByUser(SDL_Window *window) {
        NSWindow* cocoaWindow = getCocoaWindow(window);
        
        return cocoaWindow.inLiveResize;
    }

    void macosMarkContentEdited(SDL_Window *window, bool edited) {
        NSWindow* cocoaWindow = getCocoaWindow(window);

        [cocoaWindow setDocumentEdited:edited];
    }

    typedef void (*MenuAction)(void *);

    @interface MenuActionHandler : NSObject
        @property (nonatomic, assign) MenuAction action;
        @property (nonatomic, assign) void *userdata;
        - (instancetype)initWithAction:(MenuAction)action andUserdata:(void *)userdata;
        - (void)triggerAction:(id)sender;
    @end

    @implementation MenuActionHandler

    - (instancetype)initWithAction:(MenuAction)action andUserdata:(void *)userdata {
        self = [super init];
        if (self) {
            _action = action;
            _userdata = userdata;
        }
        return self;
    }

    - (void)triggerAction:(id)sender {
        if (self.action) {
            self.action(self.userdata);
        }
    }

    @end

    void macosAddMainMenuEntry(const char **path, unsigned long pathSize, MenuAction action, void *userdata) {
        if (pathSize < 2) {
            return;
        }

        NSMenu *menu = [NSApp mainMenu];

        for (unsigned long i = 0; i < pathSize - 1; i++) {
            NSString *menuTitle = [NSString stringWithUTF8String:path[i]];

            NSMenuItem *existingItem = [menu itemWithTitle:menuTitle];
            if (existingItem != nil && [existingItem hasSubmenu]) {
                menu = [existingItem submenu];
            } else {
                NSMenu *subMenu = [[NSMenu alloc] initWithTitle:menuTitle];
                NSMenuItem *menuEntry = [[NSMenuItem alloc] initWithTitle:menuTitle action:nil keyEquivalent:@""];
                [menuEntry setSubmenu:subMenu];

                int index = [menu numberOfItems] - 1;
                if (index < 0) index = 0;

                [menu insertItem:menuEntry atIndex: index];
                menu = subMenu;
            }
        }

        NSString *menuItemTitle = [NSString stringWithUTF8String:path[pathSize - 1]];
        NSMenuItem *actionItem = [[NSMenuItem alloc] initWithTitle:menuItemTitle action:nil keyEquivalent:@""];

        MenuActionHandler *handler = [[MenuActionHandler alloc] initWithAction:action andUserdata:userdata];

        [actionItem setTarget:handler];
        [actionItem setAction:@selector(triggerAction:)];

        [menu addItem:actionItem];
    }

    @interface HexDocument : NSDocument

    @end

    @implementation HexDocument

    - (BOOL) readFromURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError **)outError {
        NSString* urlString = [url absoluteString];
        const char* utf8String = [urlString UTF8String];

        const char *prefix = "file://";
        size_t prefixLength = strlen(prefix);
        if (strncmp(utf8String, prefix, prefixLength) == 0)
            utf8String += prefixLength;

        openFile(utf8String);

        return YES;
    }

    @end

#endif
