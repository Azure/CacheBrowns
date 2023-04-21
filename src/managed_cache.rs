use crate::hydration::{CacheHydrationStrategy, CacheLookupSuccess};
use crate::managed_cache::CacheLookupFailure::NotFound;

enum CacheLookupFailure {
    NotFound,
    NotValid
}

enum InvalidCacheEntryBehavior {
    /// No valid value could be fetched
    ReturnNotValid,
    /// Value isn't in a Valid state, but was returned anyway as a best effort
    ReturnStale
}

struct ManagedCache<Key, Value> {
    hydrator: Box<dyn CacheHydrationStrategy<Key, Value>>,
    when_invalid: InvalidCacheEntryBehavior
}

impl<Key, Value> ManagedCache<Key, Value> {
    pub fn get(&mut self, key: &Key) -> Result<CacheLookupSuccess<Value>, CacheLookupFailure> {
        match self.hydrator.get(key) {
            None => Err(NotFound),
            Some(lookup_result) => {
                if let CacheLookupSuccess::Stale(value) = &lookup_result {
                    if let InvalidCacheEntryBehavior::ReturnStale = &self.when_invalid {
                        return Ok(lookup_result);
                    }

                    Err(CacheLookupFailure::NotValid)
                }
                else {
                    Ok(lookup_result)
                }
            }
        }
    }

    pub fn flush(&mut self) {
        self.hydrator.flush();
    }
}