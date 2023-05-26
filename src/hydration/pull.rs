use crate::hydration::{CacheHydrationStrategy, CacheLookupSuccess};
use crate::source_of_record::SourceOfRecord;
use crate::store::CacheStoreStrategy;

struct PullCacheHydrator<Key, Value> {
    store: Box<dyn CacheStoreStrategy<Key, Value>>,
    data_source: Box<dyn SourceOfRecord<Key, Value>>,
}

impl<Key, Value: Clone> PullCacheHydrator<Key, Value> {
    pub fn new(
        store: Box<dyn CacheStoreStrategy<Key, Value>>,
        data_source: Box<dyn SourceOfRecord<Key, Value>>,
    ) -> Self {
        PullCacheHydrator { store, data_source }
    }

    fn try_hydrate(&mut self, key: &Key) -> Option<Value> {
        let value = self.data_source.retrieve(key);
        self.handle_retrieve_result(key, &value);

        value
    }

    fn try_hydrate_with_hint(&mut self, key: &Key, old_value: &Value) -> (bool, Value) {
        let value = self.data_source.retrieve_with_hint(key, &old_value);
        self.handle_retrieve_result(key, &value);

        return match value {
            None => (false, old_value.clone()),
            Some(value) => (true, value),
        };
    }

    fn handle_retrieve_result(&mut self, key: &Key, value: &Option<Value>) {
        if value.is_some() {
            // TODO: Is this right?
            self.store.put(key, value.clone().unwrap().clone());
        }
    }
}

impl<Key, Value: Clone> CacheHydrationStrategy<Key, Value> for PullCacheHydrator<Key, Value> {
    fn get(&mut self, key: &Key) -> Option<CacheLookupSuccess<Value>> {
        let mut valid = false;
        let mut was_hydrated = false;
        let mut datum: Option<Value> = self.store.get(key);
        let found = datum.is_some();

        match &datum {
            None => {
                datum = self.try_hydrate(key);
                was_hydrated = datum.is_some();

                if !was_hydrated {
                    return None;
                }
            }
            Some(value) => {
                valid = self.data_source.is_valid(key, &value);

                if !valid {
                    // TODO: is there no std::tie() equivalent???
                    let (hydrated, val) = self.try_hydrate_with_hint(key, value);
                    was_hydrated = hydrated;
                    datum = Some(val);
                }
            }
        }

        // Note, datum is guaranteed not to be None here
        Some(CacheLookupSuccess::new(
            found,
            valid,
            was_hydrated,
            datum.unwrap(),
        ))
    }

    fn flush(&mut self) {
        self.store.flush();
    }

    fn stop_tracking(&mut self, key: &Key) {
        self.store.delete(key);
    }
}
