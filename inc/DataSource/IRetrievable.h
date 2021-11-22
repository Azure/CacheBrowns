// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_IRETRIEVABLE_H
#define CACHEBROWNS_IRETRIEVABLE_H

#include <vector>

namespace Microsoft::Azure::CacheBrowns::DataSource
{
    template<typename Key, typename Value>
    class IRetrievable
    {
    public:
        using KeyIterator = std::iterator<std::forward_iterator_tag, Key>;

        virtual ~IRetrievable() = default;

        virtual std::tuple<bool, Value> Retrieve(const Key& key) = 0;

        /// Accepts current value if one exists in case it can be used for optimized load
        virtual bool Retrieve(const Key& key, Value& value);

        //        virtual std::vector<std::pair<Key, std::tuple<bool, Value>>> RetrieveMany(
        //                KeyIterator begin,
        //                KeyIterator end,
        //                size_t size);
        //
        //        template<class Container>
        //        std::vector<std::pair<Key, std::tuple<bool, Value>>> RetrieveMany(const Container& container);
    };

    template<typename Key, typename Value>
    bool IRetrievable<Key, Value>::Retrieve(const Key& key, Value& value)
    {
        bool found;
        std::tie(found, value) = Retrieve(key);

        return found;
    }

}// namespace Microsoft::Azure::CacheBrowns::DataSource

#endif//CACHEBROWNS_IRETRIEVABLE_H
