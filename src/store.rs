pub mod discrete_files;
pub mod memory;

use std::collections::HashSet;

pub trait CacheStoreStrategy<Key, Value> {
    fn get(&self, key: &Key) -> Option<Value>;

    fn put(&mut self, key: &Key, value: Value);

    // not sure if the bool is necessary, forget why it's there off hand
    fn delete(&mut self, key: &Key) -> bool;

    fn flush(&mut self);
}

pub trait KeyTrackingStore<Key, Value>: CacheStoreStrategy<Key, Value> {
    // todo make this a generic iterator
    fn get_keys(&self) -> HashSet<Key>;

    fn contains(&self, key: &Key) -> bool;
}

pub struct KeyTrackedStore<Key, Value> {
    store: Box<dyn CacheStoreStrategy<Key, Value>>,
    keys: HashSet<Key>
}

impl<Key, Value> KeyTrackedStore<Key, Value> {
    pub fn new(store: Box<dyn CacheStoreStrategy<Key, Value>>) -> Self {
        Self { store, keys: Default::default() }
    }
}

impl<Key: Eq + Clone + std::hash::Hash, Value> CacheStoreStrategy<Key, Value> for KeyTrackedStore<Key, Value> {
    fn get(&self, key: &Key) -> Option<Value> {
        self.store.get(key)
    }

    fn put(&mut self, key: &Key, value: Value) {
        self.keys.insert(key.clone());
        self.store.put(key, value);
    }

    fn delete(&mut self, key: &Key) -> bool {
        self.keys.remove(key);
        self.store.delete(key)
    }

    fn flush(&mut self) {
        self.keys.clear();
        self.store.flush();
    }
}

impl<Key: Eq + Clone + std::hash::Hash, Value> KeyTrackingStore<Key, Value> for KeyTrackedStore<Key, Value> {
    fn get_keys(&self) -> HashSet<Key> {
        self.keys.clone()
    }

    fn contains(&self, key: &Key) -> bool {
        self.keys.contains(key)
    }
}
