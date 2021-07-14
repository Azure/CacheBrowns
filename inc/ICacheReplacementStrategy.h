#ifndef CACHEBROWNS_ICACHEREPLACEMENTSTRATEGY_H
#define CACHEBROWNS_ICACHEREPLACEMENTSTRATEGY_H

namespace Microsoft::Azure::CacheBrowns
{
    template<typename Key, typename Value>
    class ICacheReplacementStrategy
    {
    public:
        virtual std::tuple<CacheLookupResult, Value> Get(const Key& key) = 0;

        virtual void Invalidate(const Key& key) = 0;

        virtual void Delete(const Key& key) = 0;

        virtual void Flush() = 0;
    };
}

#endif //CACHEBROWNS_ICACHEREPLACEMENTSTRATEGY_H