> [!IMPORTANT]
> This Is Not Fully Complete, I Will Add More Feautures In The Next Week.

> [!CAUTION]
> This Is Detected In Some Games That Have Anti-Cheats!

## How do you set up this thing? well:
 * 1 Make a .h(header) file in your project.
 * 2 Add the code in the file.
 * 3 Make sure to include your file with "#include "header_file_name.h"".
 * 4 You are done, now read how to use it.
## How do you use this now?:
 * 1 Use "write" to write process memory, example:
 > Write<uintptr_t>(address, WriteValue);
  --------------------------------------------------------------------
 * 2 Use "read" to read process memory, example:
 > int value = read<uintptr_t>(readAddress);
> - You can use : float, double, int etc... 
  --------------------------------------------------------------------
 * 3 Use "GetHandle" to get a handle to the process, example:
 > HANDLE handle = GetHandle("ProcessName.exe");
  --------------------------------------------------------------------
 * 4 GetModuleAddress();
 > GetModuleAddress(processName);
 --------------------------------------------------------------------

 --------------------------------------------------------------------
  -  [x] Add Reading.
-  [x] Add Writing.
-  [X] Add GetHandle.
- [x] Add GetModuleAddress.
- [x] Add A Simple Bypass.
