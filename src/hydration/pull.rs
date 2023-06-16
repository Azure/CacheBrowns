use crate::hydration::{CacheHydrationStrategy, CacheLookupSuccess, StoreResult};
use crate::source_of_record::SourceOfRecord;
use crate::store::CacheStoreStrategy;

pub struct PullCacheHydrator<Key, Value> {
    store: Box<dyn CacheStoreStrategy<Key, Value>>,
    data_source: Box<dyn SourceOfRecord<Key, Value>>,
}

impl<Key, Value: Clone> CacheHydrationStrategy<Key, Value> for PullCacheHydrator<Key, Value> {
    fn get(&mut self, key: &Key) -> Option<CacheLookupSuccess<Value>> {
        match self.store.get(key) {
            Some(value) => {
                if self.data_source.is_valid(key, &value) {
                    Some(CacheLookupSuccess::new(StoreResult::Valid, false, value))
                } else {
                    match self.data_source.retrieve_with_hint(key, &value) {
                        Some(new_value) => {
                            self.store.put(key, new_value.clone());
                            Some(CacheLookupSuccess::new(
                                StoreResult::Invalid,
                                true,
                                new_value,
                            ))
                        }
                        None => Some(CacheLookupSuccess::new(StoreResult::Invalid, false, value)),
                    }
                }
            }
            None => match self.data_source.retrieve(key) {
                Some(value) => {
                    self.store.put(key, value.clone());
                    Some(CacheLookupSuccess::new(StoreResult::NotFound, true, value))
                }
                None => None,
            },
        }
    }

    fn flush(&mut self) {
        self.store.flush();
    }

    fn stop_tracking(&mut self, key: &Key) {
        self.store.delete(key);
    }
}
