// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_IPRUNABLE_H
#define CACHEBROWNS_IPRUNABLE_H

namespace Microsoft::Azure::CacheBrowns::Store
{
    template<typename Key>
    class IPrunable
    {
    public:
        virtual bool Delete(const Key& key) = 0;

        virtual void Flush() = 0;
    };
}// namespace Microsoft::Azure::CacheBrowns::Store

#endif//CACHEBROWNS_IPRUNABLE_H
