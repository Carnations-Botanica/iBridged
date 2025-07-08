//
//  kern_dyld.cpp
//  iBridged
//
//  Created by Zormeister on 8/7/2025.
//

#include "kern_dyld.hpp"
#include "kern_start.hpp"

static const UInt8 kBridgeOSInstallBridgeCheckFind15[] = {
    0x48, 0x8d, 0x5d, 0xc0,             // lea     rbx, [rbp-0x40]
    0x48, 0x89, 0xdf,                   // mov     rdi, rbx
    0xe8, 0x00, 0x00, 0x00, 0x00,       // call    _multiverse_platform_get_type
    0x8b, 0x0b,                         // mov     ecx, dword [rbx]
    0x81, 0xf1, 0x00, 0x00, 0x02, 0x00, // xor     ecx, 0x20000
    0x09, 0xc1,                         // or      ecx, eax
    0x41, 0x0f, 0x94, 0xc6              // sete    r14b
};

static const UInt8 kBridgeOSInstallBridgeCheckMask15[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const UInt8 kBridgeOSInstallBridgeCheckReplace15[] = {
    0x48, 0x8d, 0x5d, 0xc0,             // lea     rbx, [rbp-0x40]
    0x48, 0x89, 0xdf,                   // mov     rdi, rbx
    0xe8, 0x00, 0x00, 0x00, 0x00,       // call    _multiverse_platform_get_type
    0x8b, 0x0b,                         // mov     ecx, dword [rbx]
    0x81, 0xf1, 0x00, 0x00, 0x04, 0x00, // xor     ecx, 0x40000
    0x09, 0xc1,                         // or      ecx, eax
    0x41, 0x0f, 0x94, 0xc6              // sete    r14b
};

static DYLD::_cs_validate_range_t _original_cs_validate_page = nullptr;

void ibgd_cs_validate_page(vnode *vp, memory_object_t pager, memory_object_offset_t page_offset,
                           const void *data, int *validated_p, int *tainted_p, int *nx_p) {
    char path[PATH_MAX];
    int pathlen = PATH_MAX;

    _original_cs_validate_page(vp, pager, page_offset, data, validated_p, tainted_p, nx_p);
    
    if (vn_getpath(vp, path, &pathlen)) { return; }

    /*
     * This should hit:
     *  - The dyld cache
     *  - InstallAssistant's BridgeOSInstall variant
     *  - The system copy of BridgeOSInstall
     */
    if (strstr(path, "dyld_shared_cache_x86_64") || strstr(path, "BridgeOSInstall")) {
        /* I have zero idea if this matches the Sonoma binaries. */
        if (IBGD::darwinMajor >= KernelVersion::Sequoia) {
            if (UNLIKELY(KernelPatcher::findAndReplaceWithMask(const_cast<void *>(data), PAGE_SIZE,
                                                                kBridgeOSInstallBridgeCheckFind15, kBridgeOSInstallBridgeCheckMask15,
                                                                kBridgeOSInstallBridgeCheckReplace15, kBridgeOSInstallBridgeCheckMask15, 1, 0))) {
                DBGLOG(MODULE_DYLD, "Performed the bridgeOS Install patch on %s", path);
                return;
            }
        }
    }
}

void DYLD::init(KernelPatcher &Patcher) {
    DBGLOG(MODULE_DYLD, "DYLD::init(Patcher) called. Dyld patcher module is starting.");
    
    KernelPatcher::RouteRequest req = {"_cs_validate_page", ibgd_cs_validate_page, _original_cs_validate_page};
    
    if (!Patcher.routeMultipleLong(KernelPatcher::KernelID, &req, 1)) {
        DBGLOG(MODULE_DYLD, "Failed to route _cs_validate_page");
    }
}
