// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>

#include "dllexport.h"
#include "jitinterface.h"

typedef struct _GUID {
    unsigned int Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;

class CORJIT_FLAGS
{
public:
    CORJIT_FLAGS(const CORJIT_FLAGS& other)
    {
        corJitFlags = other.corJitFlags;
    }
private:
    uint64_t corJitFlags;
};

static const GUID JITEEVersionIdentifier = { /* 1ce51eeb-dfd0-4450-ba2c-ea0d2d863df5 */
    0x1ce51eeb,
    0xdfd0,
    0x4450,
    {0xba, 0x2c, 0xea, 0x0d, 0x2d, 0x86, 0x3d, 0xf5}
};

class Jit
{
public:
    virtual int STDMETHODCALLTYPE compileMethod(
        void* compHnd,
        void* methodInfo,
        unsigned flags,
        void* entryAddress,
        void* nativeSizeOfCode) = 0;

    virtual void clearCache() = 0;
    virtual unsigned isCacheCleanupRequired() = 0;
    virtual void ProcessShutdownWork(void* info) = 0;

    // The EE asks the JIT for a "version identifier". This represents the version of the JIT/EE interface.
    // If the JIT doesn't implement the same JIT/EE interface expected by the EE (because the JIT doesn't
    // return the version identifier that the EE expects), then the EE fails to load the JIT.
    // 
    virtual void getVersionIdentifier(GUID* versionIdentifier) = 0;

    // When the EE loads the System.Numerics.Vectors assembly, it asks the JIT what length (in bytes) of
    // SIMD vector it supports as an intrinsic type.  Zero means that the JIT does not support SIMD
    // intrinsics, so the EE should use the default size (i.e. the size of the IL implementation).
    virtual unsigned getMaxIntrinsicSIMDVectorLength(CORJIT_FLAGS cpuCompileFlags) = 0;
};

DLL_EXPORT int JitCompileMethod(
    CorInfoException **ppException,
    Jit * pJit, 
    void * thisHandle, 
    void ** callbacks,
    void* methodInfo,
    unsigned flags,
    void* entryAddress,
    void* nativeSizeOfCode)
{
    *ppException = nullptr;

    GUID versionId;
    pJit->getVersionIdentifier(&versionId);
    if (memcmp(&versionId, &JITEEVersionIdentifier, sizeof(GUID)) != 0)
    {
        // JIT and the compiler disagree on how the interface looks like.
        // Either get a matching version of the JIT from the CoreCLR repo or update the interface
        // on the CoreRT side. Under no circumstances should you comment this line out.
        return 1;
    }

    try
    {
        JitInterfaceWrapper jitInterfaceWrapper(thisHandle, callbacks);
        return pJit->compileMethod(&jitInterfaceWrapper, methodInfo, flags, entryAddress, nativeSizeOfCode);
    }
    catch (CorInfoException *pException)
    {
        *ppException = pException;
    }

    return 1;
}

DLL_EXPORT unsigned GetMaxIntrinsicSIMDVectorLength(
    Jit * pJit,
    CORJIT_FLAGS * flags)
{
    return pJit->getMaxIntrinsicSIMDVectorLength(*flags);
}
