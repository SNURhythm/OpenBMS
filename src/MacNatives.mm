

#include "MacNatives.h"
#include <Foundation/Foundation.h>
void setSmoothScrolling(bool smoothScrolling) {
  [[NSUserDefaults standardUserDefaults]
      setBool:smoothScrolling ? YES : NO
       forKey:@"AppleMomentumScrollSupported"];
}
