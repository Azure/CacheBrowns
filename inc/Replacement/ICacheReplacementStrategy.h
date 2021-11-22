#ifndef CACHEBROWNS_ICACHEREPLACEMENTSTRATEGY_H
#define CACHEBROWNS_ICACHEREPLACEMENTSTRATEGY_H

namespace Microsoft::Azure::CacheBrowns::Replacement
{
    using namespace Hydration;

    template<typename Key, typename Value>
    class ICacheReplacementStrategy
    {
    public:
        virtual ~ICacheReplacementStrategy() = default;

        virtual std::tuple<CacheLookupResult, Value> Get(const Key& key) = 0;

        virtual void Invalidate(const Key& key) = 0;

        virtual void Delete(const Key& key) = 0;

        virtual void Flush() = 0;
    };
}// namespace Microsoft::Azure::CacheBrowns::Replacement

#endif//CACHEBROWNS_ICACHEREPLACEMENTSTRATEGY_H
