/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#include <Foundation/Foundation.h>

#include "ExecuteCommand.h"

bool executeCommand(const char* cmdPath, const char* cmdArgs[], int argCount) {
    NSString* cmdPathS = [[NSString alloc] initWithCString: cmdPath encoding: NSASCIIStringEncoding];
    NSURL* cmdUrl = [NSURL fileURLWithPath: cmdPathS];

    NSMutableArray* cmdArgsMA = [NSMutableArray arrayWithCapacity: argCount];
    for (int i = 0; i < argCount; ++i) {
        [cmdArgsMA addObject: [[NSString alloc] initWithCString: cmdArgs[i] encoding: NSASCIIStringEncoding]];
    }

    NSTask* task = [NSTask launchedTaskWithExecutableURL: cmdUrl arguments: [cmdArgsMA copy] error: nil terminationHandler: nil];
    [task waitUntilExit];

    return [task terminationStatus] == EXIT_SUCCESS;
}