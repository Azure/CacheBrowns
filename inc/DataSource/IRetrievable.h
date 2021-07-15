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

        //        virtual std::vector<std::pair<Key, std::tuple<bool, Value>>> RetrieveMany(
        //                KeyIterator begin,
        //                KeyIterator end,
        //                size_t size);
        //
        //        template<class Container>
        //        std::vector<std::pair<Key, std::tuple<bool, Value>>> RetrieveMany(const Container& container);
    };

    //    template<typename Key, typename Value>
    //    std::vector<std::pair<Key, std::tuple<bool, Value>>> IRetrievable<Key, Value>::RetrieveMany(
    //            IRetrievable::KeyIterator begin,
    //            IRetrievable::KeyIterator end,
    //            size_t size)
    //    {
    //        std::vector<std::pair<Key, std::tuple<bool, Value>>> results(size);
    //
    //        for (KeyIterator itr = begin; itr != end; itr++)
    //        {
    //            results.emplace_back(*itr);
    //        }
    //
    //        return results;
    //    }
    //
    //    template<typename Key, typename Value>
    //    template<class Container>
    //    std::vector<std::pair<Key, std::tuple<bool, Value>>> IRetrievable<Key, Value>::RetrieveMany(
    //            const Container& container)
    //    {
    //        return Retrieve(container.cbegin(), container.cend(), container.size());
    //    }

}// namespace Microsoft::Azure::CacheBrowns::DataSource

#endif//CACHEBROWNS_IRETRIEVABLE_H
