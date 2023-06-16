use core::hash::Hash;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, RwLock};
use std::thread::{self};
use std::time::{Duration, Instant};

use crate::source_of_record::SourceOfRecord;
use crate::store::CacheStoreStrategy;

use super::{CacheHydrationStrategy, CacheLookupSuccess, StoreResult};

pub struct PollingHydrationStrategy<Key, Value> {
    shared_inner_state: Arc<InnerState<Key, Value>>,
}

struct InnerState<Key, Value> {
    data_source: Box<dyn SourceOfRecord<Key, Value> + Send + Sync>,
    store: RwLock<Box<dyn CacheStoreStrategy<Key, Value> + Send + Sync>>,
    polling_interval: AtomicU64,
    alive: AtomicBool,
}

impl<Key, Value> Drop for PollingHydrationStrategy<Key, Value> {
    fn drop(&mut self) {
        // Relaxed is fine because we just care that the signal is eventually
        // observed.
        self.shared_inner_state
            .alive
            .store(false, Ordering::Relaxed);
    }
}

pub enum PollHydrationError {
    IntervalRangeError(String),
}

impl<Key, Value> PollingHydrationStrategy<Key, Value>
where
    Key: Eq + Hash + Clone + 'static,
    Value: Clone + 'static,
{
    fn new(
        data_source: Box<dyn SourceOfRecord<Key, Value> + Send + Sync>,
        store: Box<dyn CacheStoreStrategy<Key, Value> + Send + Sync>,
        polling_interval: u64,
    ) -> Self {
        let hydrator = Self {
            shared_inner_state: Arc::new(InnerState {
                data_source,
                store: RwLock::from(store),
                polling_interval: AtomicU64::new(polling_interval),
                alive: AtomicBool::new(true),
            }),
        };

        hydrator.start_thread();

        hydrator
    }

    fn start_thread(&self) {
        let shared_inner_state = Arc::clone(&self.shared_inner_state);

        // TODO: Refactor this with InterruptablePolling crate
        thread::spawn(move || {
            // Check whether the associated struct is alive for clean exit
            while shared_inner_state.alive.load(Ordering::Relaxed) {
                let start = Instant::now();

                let keys = shared_inner_state.store.read().unwrap().get_keys();
                for key in keys {
                    if !shared_inner_state.alive.load(Ordering::Relaxed) {
                        break;
                    }

                    let value = shared_inner_state.store.read().unwrap().peek(&key);

                    // Don't update the value if it
                    // 1. exists and
                    // 2. is valid.
                    if match value.as_ref() {
                        None => false,
                        Some(v) => shared_inner_state.data_source.is_valid(&key, v),
                    } {
                        continue;
                    }

                    let canonical_value = value.map_or_else(
                        || shared_inner_state.data_source.retrieve(&key),
                        |v| shared_inner_state.data_source.retrieve_with_hint(&key, &v),
                    );

                    if let Some(v) = canonical_value.as_ref() {
                        let mut store_handle = shared_inner_state.store.write().unwrap();
                        if store_handle.contains(&key) {
                            store_handle.put(&key, v.clone());
                        }
                    }
                }

                let elapsed = Instant::now().duration_since(start);

                thread::sleep(
                    Duration::new(
                        // Races with the interval changing are incredibly
                        // unimportant so let's give the least strict
                        // ordering necessary.
                        shared_inner_state.polling_interval.load(Ordering::Relaxed),
                        0,
                    )
                    .saturating_sub(elapsed),
                );
            }
        });
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
    use std::{collections::HashMap, sync::Mutex};

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
            self.inner
                .get(key)
                .map(|x| {
                    let mut v = x.lock().unwrap();
                    let previous = *v;
                    *v = None;
                    previous
                })
                .flatten()
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

        let mut cache =
            PollingHydrationStrategy::new(Box::new(source), Box::new(MemoryStore::new()), 1);

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

        let mut cache =
            PollingHydrationStrategy::new(Box::new(source), Box::new(MemoryStore::new()), 1);

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

        let mut cache =
            PollingHydrationStrategy::new(Box::new(source), Box::new(MemoryStore::new()), 1);

        assert_eq!(cache.get(&key_1), Some(CacheLookupSuccess::Miss(0)));

        assert_eq!(cache.get(&key_1), Some(CacheLookupSuccess::Stale(0)));
    }

    #[test]
    fn poll_hydration_update_after_poll() {
        let source = IncrementingSource::new();
        let mut cache =
            PollingHydrationStrategy::new(Box::new(source), Box::new(MemoryStore::new()), 2);
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

        let mut cache =
            PollingHydrationStrategy::new(Box::new(source), Box::new(MemoryStore::new()), 1);

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
