//
//  kern_dyld.hpp
//  iBridged
//
//  Created by Zormeister on 8/7/2025.
//

#ifndef kern_dyld_hpp
#define kern_dyld_hpp

#include <Headers/kern_user.hpp>
#include "kern_start.hpp"

#define MODULE_DYLD "DYLD"

class DYLD {
public:

    static void init(KernelPatcher &Patcher);
    
    using _cs_validate_page_t = void (*)(vnode *vp, memory_object_t pager, memory_object_offset_t page_offset,
                                      const void *data, int *validated_p, int *tainted_p, int *nx_p);
};

#endif /* kern_dyld_hpp */
