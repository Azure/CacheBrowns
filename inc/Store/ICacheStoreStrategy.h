// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_ICACHESTORESTRATEGY_H
#define CACHEBROWNS_ICACHESTORESTRATEGY_H

#include "IHydratable.h"
#include "IPrunable.h"

namespace Microsoft::Azure::CacheBrowns::Store
{
    template<typename Key, typename Value>
    class ICacheStoreStrategy : public IHydratable<Key, Value>, IPrunable<Key>
    {
        // TODO: This has no clear purpose until the lambda factory init pattern is implemented.
    };
}// namespace Microsoft::Azure::CacheBrowns::Store

#endif//CACHEBROWNS_ICACHESTORESTRATEGY_H
