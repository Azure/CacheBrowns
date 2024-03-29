use core::hash::Hash;
use interruptible_polling::{PollingTask, StillActiveChecker};
use std::num::TryFromIntError;
use std::sync::{Arc, RwLock};
use std::time::Duration;

use crate::source_of_record::SourceOfRecord;
use crate::store::CacheStoreStrategy;

use super::{CacheHydrationStrategy, CacheLookupSuccess, StoreResult};

pub struct PollingHydrationStrategy<Key, Value> {
    shared_inner_state: Arc<InnerState<Key, Value>>,
    _polling_thread: PollingTask,
}

struct InnerState<Key, Value> {
    data_source: Box<dyn SourceOfRecord<Key, Value> + Send + Sync>,
    store: RwLock<Box<dyn CacheStoreStrategy<Key, Value> + Send + Sync>>,
}

pub enum PollHydrationError {
    IntervalRangeError(String),
}

impl<Key, Value> PollingHydrationStrategy<Key, Value>
where
    Key: Eq + Hash + Clone + 'static,
    Value: Clone + 'static,
{
    pub fn new(
        data_source: Box<dyn SourceOfRecord<Key, Value> + Send + Sync>,
        store: Box<dyn CacheStoreStrategy<Key, Value> + Send + Sync>,
        polling_interval: Duration,
    ) -> Result<Self, TryFromIntError> {
        let shared_state = Arc::new(InnerState {
            data_source,
            store: RwLock::from(store),
        });

        let hydrator = Self {
            shared_inner_state: shared_state.clone(),
            _polling_thread: PollingTask::new_with_checker(
                polling_interval,
                Box::new(move |checker| {
                    Self::poll(&shared_state, checker);
                }),
            )?,
        };

        Ok(hydrator)
    }

    fn poll(shared_inner_state: &Arc<InnerState<Key, Value>>, checker: &StillActiveChecker) {
        let keys: Vec<Key> = shared_inner_state
            .store
            .read()
            .unwrap()
            .get_keys()
            .collect();

        for key in keys {
            if !checker() {
                break;
            }

            // Fetch separately to release the lock
            let peeked_value = shared_inner_state.store.read().unwrap().peek(&key);

            // If value was deleted since pulling keys, don't issue a superfluous retrieve.
            if let Some(value) = peeked_value {
                let canonical_value = shared_inner_state
                    .data_source
                    .retrieve_with_hint(&key, &value);

                if let Some(v) = canonical_value.as_ref() {
                    let mut store_handle = shared_inner_state.store.write().unwrap();

                    // Respect delete if delete occurred during retrieval
                    if store_handle.contains(&key) {
                        store_handle.put(&key, v.clone());
                    }
                }
            }
        }
    }
}

impl<Key, Value> CacheHydrationStrategy<Key, Value> for PollingHydrationStrategy<Key, Value>
where
    Key: Eq + Hash + Clone + 'static,
    Value: Clone + 'static,
{
    fn get(&mut self, key: &Key) -> Option<CacheLookupSuccess<Value>> {
        let value = self.shared_inner_state.store.write().unwrap().get(key);
        value
            .map(|value| {
                CacheLookupSuccess::new(
                    if self.shared_inner_state.data_source.is_valid(key, &value) {
                        StoreResult::Valid
                    } else {
                        StoreResult::Invalid
                    },
                    false,
                    value,
                )
            })
            .or_else(|| {
                self.shared_inner_state
                    .data_source
                    .retrieve(key)
                    .map(|value| {
                        self.shared_inner_state
                            .store
                            .write()
                            .unwrap()
                            .put(key, value.clone());
                        CacheLookupSuccess::new(StoreResult::NotFound, true, value)
                    })
            })
    }

    fn flush(&mut self) {
        self.shared_inner_state.store.write().unwrap().flush()
    }

    fn stop_tracking(&mut self, key: &Key) {
        self.shared_inner_state.store.write().unwrap().delete(key);
    }
}

#[cfg(test)]
mod tests {
    use std::sync::atomic::{AtomicU64, Ordering};
    use std::{collections::HashMap, sync::Mutex, thread};

    use crate::store::memory::MemoryStore;

    use super::*;
    use uuid::Uuid;

    struct BasicSource<K, V> {
        inner: HashMap<K, V>,
    }

    impl<K, V> BasicSource<K, V>
    where
        K: Eq + PartialEq + Hash,
        V: Eq,
    {
        fn new() -> Self {
            Self {
                inner: HashMap::new(),
            }
        }

        fn insert(&mut self, key: K, value: V) {
            self.inner.insert(key, value);
        }
    }

    impl<K, V> SourceOfRecord<K, V> for BasicSource<K, V>
    where
        K: Eq + PartialEq + Hash,
        V: Clone + Eq,
    {
        fn retrieve(&self, key: &K) -> Option<V> {
            self.inner.get(key).cloned()
        }

        fn retrieve_with_hint(&self, key: &K, _current: &V) -> Option<V> {
            self.inner.get(key).cloned()
        }

        fn is_valid(&self, key: &K, value: &V) -> bool {
            self.retrieve(key) == Some(value.clone())
        }
    }

    struct ForgetfulSource {
        inner: HashMap<Uuid, Mutex<Option<u64>>>,
    }

    impl ForgetfulSource {
        fn new() -> Self {
            Self {
                inner: HashMap::new(),
            }
        }

        fn insert(&mut self, key: Uuid, value: u64) {
            self.inner.insert(key, Mutex::new(Some(value)));
        }
    }

    impl SourceOfRecord<Uuid, u64> for ForgetfulSource {
        fn retrieve(&self, key: &Uuid) -> Option<u64> {
            // Immediately remove the value once we retrieve it once.
            // We need a Mutex for this because this takes &self.
            self.inner.get(key).and_then(|x| {
                let mut v = x.lock().unwrap();
                let previous = *v;
                *v = None;
                previous
            })
        }

        fn retrieve_with_hint(&self, key: &Uuid, _value: &u64) -> Option<u64> {
            self.retrieve(key)
        }

        fn is_valid(&self, key: &Uuid, value: &u64) -> bool {
            self.inner
                .get(key)
                .map_or(false, |v| *v.lock().unwrap() == Some(*value))
        }
    }

    struct IncrementingSource {
        value: AtomicU64,
    }

    impl IncrementingSource {
        fn new() -> Self {
            Self {
                value: AtomicU64::new(0),
            }
        }
    }

    impl SourceOfRecord<String, u64> for IncrementingSource {
        fn retrieve(&self, _key: &String) -> Option<u64> {
            Some(self.value.fetch_add(1, Ordering::Relaxed))
        }

        fn retrieve_with_hint(&self, key: &String, _v: &u64) -> Option<u64> {
            self.retrieve(key)
        }

        fn is_valid(&self, _key: &String, _value: &u64) -> bool {
            false
        }
    }

    #[test]
    fn poll_hydration_valid_key() {
        let mut source = BasicSource::new();
        let key_1 = Uuid::new_v4();
        let key_2 = Uuid::new_v4();
        source.insert(key_1, String::from("AAAA"));
        source.insert(key_2, String::from("AAAAAAAA"));

        let mut cache = PollingHydrationStrategy::new(
            Box::new(source),
            Box::new(MemoryStore::new()),
            Duration::from_secs(1),
        )
        .unwrap();

        assert_eq!(
            cache.get(&key_1),
            Some(CacheLookupSuccess::Miss(String::from("AAAA")))
        );

        assert_eq!(
            cache.get(&key_1),
            Some(CacheLookupSuccess::Hit(String::from("AAAA")))
        );
    }

    #[test]
    fn poll_hydration_no_key() {
        let mut source = BasicSource::new();
        let key_1 = Uuid::new_v4();
        let key_2 = Uuid::new_v4();
        let key_3 = Uuid::new_v4();
        source.insert(key_1, String::from("AAAA"));
        source.insert(key_2, String::from("AAAAAAAA"));

        let mut cache = PollingHydrationStrategy::new(
            Box::new(source),
            Box::new(MemoryStore::new()),
            Duration::from_secs(1),
        )
        .unwrap();

        assert_eq!(cache.get(&key_3), None);
        assert_eq!(cache.get(&key_3), None);
    }

    #[test]
    fn poll_hydration_lost_key() {
        let mut source = ForgetfulSource::new();
        let key_1 = Uuid::new_v4();
        let key_2 = Uuid::new_v4();
        source.insert(key_1, 0);
        source.insert(key_2, 1);

        let mut cache = PollingHydrationStrategy::new(
            Box::new(source),
            Box::new(MemoryStore::new()),
            Duration::from_secs(1),
        )
        .unwrap();

        assert_eq!(cache.get(&key_1), Some(CacheLookupSuccess::Miss(0)));

        assert_eq!(cache.get(&key_1), Some(CacheLookupSuccess::Stale(0)));
    }

    #[test]
    fn poll_hydration_update_after_poll() {
        let source = IncrementingSource::new();
        let mut cache = PollingHydrationStrategy::new(
            Box::new(source),
            Box::new(MemoryStore::new()),
            Duration::from_secs(2),
        )
        .unwrap();
        let key = String::from("asdf");

        // First lookup is a miss
        assert_eq!(cache.get(&key), Some(CacheLookupSuccess::Miss(0)),);

        // Align ourselves to be neatly between polls
        thread::sleep(Duration::from_secs(1));

        // Value now updated to 1
        assert_eq!(cache.get(&key), Some(CacheLookupSuccess::Stale(1)));

        assert_eq!(cache.get(&key), Some(CacheLookupSuccess::Stale(1)),);

        thread::sleep(Duration::from_secs(2));

        // Value now updated to 2
        assert_eq!(cache.get(&key), Some(CacheLookupSuccess::Stale(2)),);
    }

    #[test]
    fn poll_hydration_stop_tracking_miss() {
        let mut source = BasicSource::new();
        let key_1 = Uuid::new_v4();
        let key_2 = Uuid::new_v4();
        source.insert(key_1, String::from("AAAA"));
        source.insert(key_2, String::from("AAAAAAAA"));

        let mut cache = PollingHydrationStrategy::new(
            Box::new(source),
            Box::new(MemoryStore::new()),
            Duration::from_secs(1),
        )
        .unwrap();

        assert_eq!(
            cache.get(&key_1),
            Some(CacheLookupSuccess::Miss(String::from("AAAA")))
        );

        assert_eq!(
            cache.get(&key_1),
            Some(CacheLookupSuccess::Hit(String::from("AAAA")))
        );

        cache.stop_tracking(&key_1);

        assert_eq!(
            cache.get(&key_1),
            Some(CacheLookupSuccess::Miss(String::from("AAAA")))
        );
    }
}
