use std::collections::{HashMap, HashSet};
use crate::store::{CacheStoreStrategy, KeyTrackingStore};

#[derive(Default)]
pub struct MemoryCacheStore<Key, Value> {
    data: HashMap<Key, Value>
}

impl<Key, Value> MemoryCacheStore<Key, Value> {
    pub fn new() -> Self {
        MemoryCacheStore { data: HashMap::new() }
    }
}

// TODO: Should everything be by ref return? Does that have concurrency issues?
impl<Key: Eq + Clone + std::hash::Hash, Value: Clone> CacheStoreStrategy<Key, Value> for MemoryCacheStore<Key, Value> {
    fn get(&self, key: &Key) -> Option<Value> {
        self.data.get(key).map(Value::to_owned)
    }

    fn put(&mut self, key: &Key, value: Value) {
        self.data.insert(key.clone(), value);
    }

    fn delete(&mut self, key: &Key) -> bool {
        self.data.remove(key).is_some()
    }

    fn flush(&mut self) {
        self.data.clear();
    }
}

impl<Key: Eq + Clone + std::hash::Hash, Value: Clone> KeyTrackingStore<Key, Value> for MemoryCacheStore<Key, Value> {
    fn get_keys(&self) -> HashSet<Key> {
        self.data.keys().map(|x| x.clone()).collect()
    }

    fn contains(&self, key: &Key) -> bool {
        self.data.contains_key(key)
    }
}
