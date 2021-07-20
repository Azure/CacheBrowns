#ifndef CACHEBROWNS_ICACHEHYDRATIONSTRATEGY_H
#define CACHEBROWNS_ICACHEHYDRATIONSTRATEGY_H

namespace Microsoft::Azure::CacheBrowns::Hydration
{
    /// <summary>
    /// Grants insight into what happened during cache retrieval for performance tracking and debugging purposes.
    /// </summary>
    /// <remarks>
    /// DO NOT USE THIS TO INFLUENCE APPLICATION BEHAVIOR. Given that these are programmable caches, the purpose of
    /// this library is to enable you to declaratively specify cache control. Building further control on top of this
    /// breaks the abstraction and is likely to produce bugs. Custom behaviors should instead be achieved by injecting
    /// your own strategies.
    /// </remarks>
    enum CacheLookupResult
    {
        /// No value could be fetched
        NotFound = -1,

        /// No valid value could be fetched (returned by ReturnNotValid implementations when stale data was present but not used)
        NotValid = -2,

        /// The value was found, but the cache had to be (re)hydrated
        Miss = 0,

        /// Special case of miss; a stale value was present for fallback but rehydrate effort succeeded
        Refresh = 1,

        /// Value isn't in a Valid state, but was returned anyway as a best effort
        Stale = 2,

        /// Valid entry found
        Hit = 3
    };

    enum InvalidCacheEntryBehavior
    {
        ReturnNotValid,
        ReturnStale
    };

    namespace impl
    {
        inline CacheLookupResult FindCacheLookupResultWithSemantics(
                bool storeHit,
                bool validEntry,
                bool hydrationSucceeded,
                CacheLookupResult whenInvalid)
        {
            if (storeHit)
            {
                if (validEntry)
                {
                    return Hit;
                }
                else
                {
                    if (hydrationSucceeded)
                    {
                        return Refresh;
                    }
                    else
                    {
                        return whenInvalid;
                    }
                }
            }

            if (hydrationSucceeded)
            {
                return Miss;
            }
            else
            {
                return NotFound;
            }
        }
    };// namespace impl

    template<InvalidCacheEntryBehavior whenInvalid>
    CacheLookupResult FindCacheLookupResultWithSemantics(bool storeHit, bool validEntry, bool hydrationSucceeded);

    template<>
    CacheLookupResult FindCacheLookupResultWithSemantics<InvalidCacheEntryBehavior::ReturnNotValid>(
            bool storeHit,
            bool validEntry,
            bool hydrationSucceeded)
    {
        return impl::FindCacheLookupResultWithSemantics(storeHit, validEntry, hydrationSucceeded, NotValid);
    }

    template<>
    CacheLookupResult FindCacheLookupResultWithSemantics<InvalidCacheEntryBehavior::ReturnStale>(
            bool storeHit,
            bool validEntry,
            bool hydrationSucceeded)
    {
        return impl::FindCacheLookupResultWithSemantics(storeHit, validEntry, hydrationSucceeded, Stale);
    }

    template<InvalidCacheEntryBehavior whenInvalid, typename Value>
    std::tuple<CacheLookupResult, Value> CreateCacheLookupResult(
            bool storeHit,
            bool validEntry,
            bool hydrationSucceeded,
            Value value)
    {
        auto result = FindCacheLookupResultWithSemantics<whenInvalid>(storeHit, validEntry, hydrationSucceeded);

        if (result == NotFound || result == NotValid)
        {
            return {result, Value{}};
        }

        return {result, value};
    }

    template<
            typename Key,
            typename Value,
            InvalidCacheEntryBehavior whenInvalid = InvalidCacheEntryBehavior::ReturnNotValid>
    class ICacheHydrationStrategy
    {
    public:
        virtual std::tuple<CacheLookupResult, Value> Get(const Key& key) = 0;

        virtual void Invalidate(const Key& key) = 0;

        virtual void Delete(const Key& key) = 0;

        virtual void Flush() = 0;
    };
}// namespace Microsoft::Azure::CacheBrowns::Hydration

#endif//CACHEBROWNS_ICACHEHYDRATIONSTRATEGY_H
