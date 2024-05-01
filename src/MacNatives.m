#if PLATFORM_MAC
#include "MacNatives.h"

// OutFolderName is a std::string* that will be filled with the path to the selected folder
bool MacOSSelectFolder(const char* title, const char* defaultPath, std::string* OutFolderName)
{
bool bSuccess = false;
	{
		bSuccess = MainThreadReturn(^{
			SCOPED_AUTORELEASE_POOL;

			NSOpenPanel* Panel = [NSOpenPanel openPanel];
			[Panel setCanChooseFiles: false];
			[Panel setCanChooseDirectories: true];
			[Panel setAllowsMultipleSelection: false];
			[Panel setCanCreateDirectories: true];

			[Panel setTitle:[NSString stringWithUTF8String:title]];

			CFStringRef DefaultPathCFString = CFStringCreateWithCString(kCFAllocatorDefault, defaultPath, kCFStringEncodingUTF8);
			NSURL* DefaultPathURL = [NSURL fileURLWithPath: (NSString*)DefaultPathCFString];
			[Panel setDirectoryURL: DefaultPathURL];
			CFRelease(DefaultPathCFString);

			bool bResult = false;

			NSInteger Result = [Panel runModal];

			if (Result == NSModalResponseOK)
			{
				NSURL *FolderURL = [[Panel URLs] objectAtIndex: 0];
				TCHAR FolderPath[MAC_MAX_PATH];
				// [FolderURL path] -> FolderPath
				std::string FolderPathString = std::string([[FolderURL path] UTF8String]);
				
				*OutFolderName = FolderPathString;

				bResult = true;
			}

			[Panel close];

			return bResult;
		});
	}
	
	return bSuccess;

}

#endif
