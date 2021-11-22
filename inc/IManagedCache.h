#ifndef UNTITLED_LIBRARY_H
#define UNTITLED_LIBRARY_H

#include "Hydration/ICacheHydrationStrategy.h"

namespace Microsoft::Azure::CacheBrowns
{
    using namespace Hydration;

    template<typename Key, typename Value>
    class IManagedCache
    {
    public:
        virtual ~IManagedCache() = default;

        virtual std::tuple<CacheLookupResult, Value> Get(const Key& key) = 0;

        virtual void Flush() = 0;
    };
}// namespace Microsoft::Azure::CacheBrowns

#endif//UNTITLED_LIBRARY_H
