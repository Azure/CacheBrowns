use crate::hydration::{CacheHydrationStrategy, CacheLookupSuccess};
use crate::managed_cache::CacheLookupFailure::NotFound;

pub enum CacheLookupFailure {
    NotFound,
    NotValid,
}

pub enum InvalidCacheEntryBehavior {
    /// No valid value could be fetched
    ReturnNotValid,
    /// Value isn't in a Valid state, but was returned anyway as a best effort
    ReturnStale,
}

pub struct ManagedCache<Key, Value> {
    hydrator: Box<dyn CacheHydrationStrategy<Key, Value>>,
    when_invalid: InvalidCacheEntryBehavior,
}

impl<Key, Value> ManagedCache<Key, Value> {
    pub fn new(
        hydrator: Box<dyn CacheHydrationStrategy<Key, Value>>,
        when_invalid: InvalidCacheEntryBehavior,
    ) -> Self {
        Self {
            hydrator,
            when_invalid,
        }
    }

    pub fn get(&mut self, key: &Key) -> Result<CacheLookupSuccess<Value>, CacheLookupFailure> {
        match self.hydrator.get(key) {
            None => Err(NotFound),
            Some(lookup_result) => {
                if let CacheLookupSuccess::Stale(_) = &lookup_result {
                    if let InvalidCacheEntryBehavior::ReturnStale = &self.when_invalid {
                        return Ok(lookup_result);
                    }

                    Err(CacheLookupFailure::NotValid)
                } else {
                    Ok(lookup_result)
                }
            }
        }
    }

    pub fn flush(&mut self) {
        self.hydrator.flush();
    }

    /// Indicates that a given key is no longer relevant and can be purged. For example, a client
    /// session ends or a VM is deallocated.
    ///
    /// IF YOU ARE USING THIS TO TRY TO LOOPHOLE A CACHE INVALIDATION, YOU ARE GOING TO BREAK THINGS
    pub fn stop_tracking(&mut self, key: &Key) {
        self.hydrator.stop_tracking(key);
    }
}
