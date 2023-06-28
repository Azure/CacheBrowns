use crate::store::CacheStoreStrategy;
use std::collections::{HashMap, HashSet, VecDeque};
use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};
use std::ptr;
use std::ptr::NonNull;

// Struct used to hold a reference to a key
struct KeyRef<Key> {
    key: *const Key,
}

// impl<Key> Into<Key> for KeyRef<Key> {
//     fn into(self) -> Key {
//         self.key.clone()
//     }
// }

impl<Key: Hash> Hash for KeyRef<Key> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        unsafe { (*self.key).hash(state) }
    }
}

impl<Key: PartialEq> PartialEq for KeyRef<Key> {
    fn eq(&self, other: &KeyRef<Key>) -> bool {
        unsafe { (*self.key).eq(&*other.key) }
    }
}

struct Entry<Key> {
    key: Key,
    previous: *mut Entry<Key>,
    next: *mut Entry<Key>,
}

/// Implements the Least Recently Used replacement strategy on top of an arbitrary data store.
///
/// While it supports non-volatile data, the usage order metadata is held in memory volatile.
/// For example, if you restart the process the same data will load, but the order depends on
/// the order the underlying store iterates keys over. This tradeoff was chosen because committing
/// usage tracking in a non-volatile store means that all read operations are now writes in
/// potentially expensive calls. Given that the point of a cache is to optimize reads + storing
/// the order perfectly over long periods of time isn't a real use case, this is the better default
/// implementation.
pub struct LruReplacementStrategy<Key, Value>
    where
        Key: Eq + Hash,
{
    store: Box<dyn CacheStoreStrategy<Key, Value>>,
    usage_order: HashMap<KeyRef<Key>, NonNull<Entry<Key>>>,
    max_capacity: usize,
    head: *mut Entry<Key>,
    tail: *mut Entry<Key>,
}

impl<Key, Value> LruReplacementStrategy<Key, Value>
    where
        Key: Eq + Hash + Clone,
        Value: Clone,
{
    pub fn new(
        max_capacity: usize,
        store: Box<dyn CacheStoreStrategy<Key, Value>>,
    ) -> Self {
        let mut lru = Self {
            store,
            usage_order: HashMap::new(),
            max_capacity,
            head: ptr::null_mut(),
            tail: ptr::null_mut(),
        };

        /// If the data store is non-volatile, there might already be data present. Iterate through
        /// the values to build out an arbitrary usage order.
        for key in store.get_keys() {
            lru.add_to_usage_order(key);
        }

        lru
    }

    fn mark_as_most_recent(&mut self, key: &Key) {
        let old_head =
    }

    fn add_to_usage_order(&mut self, key: &Key) {

    }
    
    fn remove_from_usage_order(&mut self, key: &Key) {
        
    }
}

impl<Key, Value> CacheStoreStrategy<Key, Value> for LruReplacementStrategy<Key, Value>
    where
        Key: Eq + Hash + Clone,
        Value: Clone,
{
    fn get(&mut self, key: &Key) -> Option<Value> {
        self.mark_as_most_recent(key);
        self.store.get(key)
    }

    // TODO: This shows we might want to bring back replacement interface, but as superset of store
    // store won't have peek.
    fn peek(&self, key: &Key) -> Option<Value> {
        self.store.peek(key)
    }

    fn put(&mut self, key: &Key, value: &Value) {
        self.add_to_usage_order(key);
        self.store.put(key, value);
    }

    fn delete(&mut self, key: &Key) -> bool {
        self.remove_from_usage_order(key);
        self.store.delete(key)
    }

    fn flush(&mut self) {
        self.usage_order.clear();
        self.store.flush()
    }

    fn get_keys(&self) -> Box<dyn Iterator<Item=Key> + '_> {
        // We can optimize by guaranteeing a memory lookup checking the metadata instead of the
        // underlying store, which may or may not be in memory
        Box::new(self.usage_order.keys().map(|key_ref| key_ref.into()))
    }

    fn contains(&self, key: &Key) -> bool {
        // We can optimize by guaranteeing a memory lookup checking the metadata instead of the
        // underlying store, which may or may not be in memory
        self.usage_order.contains_key(key)
    }
}
