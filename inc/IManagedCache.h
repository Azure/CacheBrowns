#ifndef UNTITLED_LIBRARY_H
#define UNTITLED_LIBRARY_H

namespace Microsoft::Azure::CacheBrowns
{
    template<typename Key, typename Value>
    class IManagedCache
    {
    public:

        virtual std::tuple<CacheLookupResult, Value> Get(const Key& key) = 0;

        virtual void Flush() = 0;
    };
}

#endif //UNTITLED_LIBRARY_H
