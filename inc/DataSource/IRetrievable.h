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
        typedef std::iterator<std::forward_iterator_tag, Key> KeyIterator;

        virtual std::tuple<bool, Value> Retrieve(const Key& key) = 0;

        /// Accepts current value if one exists in case it can be used for optimized load
        virtual std::tuple<bool, Value> Retrieve(const Key& key, const Value& value);

        // TODO: Blocked by lack of iterator support
        //
        //        virtual std::vector<std::pair<Key, std::tuple<bool, Value>>> RetrieveMany(
        //                KeyIterator begin,
        //                KeyIterator end,
        //                size_t size);
        //
        //        template<class Container>
        //        std::vector<std::pair<Key, std::tuple<bool, Value>>> RetrieveMany(const Container& container);
    };

    /// The default case is no special optimization, so provide dummy implementation
    template<typename Key, typename Value>
    std::tuple<bool, Value> IRetrievable<Key, Value>::Retrieve(const Key& key, const Value&)
    {
        return Retrieve(key);
    }

}// namespace Microsoft::Azure::CacheBrowns::DataSource

#endif//CACHEBROWNS_IRETRIEVABLE_H
